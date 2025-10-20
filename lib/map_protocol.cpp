// lib/map_protocol.cpp
#include "map_protocol.hpp"

#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <string>
#include <condition_variable>
#include <thread>

using namespace std;

// -------------------- static helpers (no lambdas) --------------------
std::string MapProtocol::make_hello(int id) {
    return std::string("HELLO|") + std::to_string(id);
}

bool MapProtocol::parse_hello(const std::string& s, int& out_id) {
    const std::string pfx = "HELLO|";
    if (s.size() < pfx.size()) return false;
    if (s.compare(0, pfx.size(), pfx) != 0) return false;
    try {
        out_id = std::stoi(s.substr(pfx.size()));
    } catch (...) {
        return false;
    }
    return true;
}

// -------------------- ctor --------------------
MapProtocol::MapProtocol(const Config& cfg, int node_id)
    : cfg_(cfg),
      id_(node_id),
      vc_(cfg.n, 0),
      stop_(false),
      rng_(static_cast<unsigned>(
               std::chrono::steady_clock::now().time_since_epoch().count()) ^
           static_cast<unsigned>(node_id * 0x9e3779b1u)),
      snapshot_mgr_(node_id, cfg.config_name, cfg.n)
{
    // Force line-buffered behavior so each '\n' flushes (useful under nohup)
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);
}

// -------------------- small utility --------------------
bool MapProtocol::is_neighbor(int peer_id) const {
    const std::vector<int>& nbs = cfg_.neighbors[id_];
    return std::find(nbs.begin(), nbs.end(), peer_id) != nbs.end();
}

// -------------------- acceptor thread function --------------------
void MapProtocol::acceptor_loop(std::atomic<bool>* accepting,
                                int expected_links)
{
    using namespace std::chrono;
    while (!stop_.load() && accepting->load()) {
        SCTPSocket peer;
        if (!listen_sock_.accept(peer)) {
            std::this_thread::sleep_for(milliseconds(10));
            continue;
        }

        // Expect peer HELLO first
        std::string hello;
        if (!peer.receive(hello)) {
            peer.close();
            continue;
        }

        int peer_id = -1;
        if (!parse_hello(hello, peer_id) || !is_neighbor(peer_id)) {
            // malformed or not an expected neighbor
            peer.close();
            continue;
        }

        // Reply with our HELLO
        (void)peer.send(make_hello(id_));

        // Store link if not present
        {
            std::lock_guard<std::mutex> lk(m_);
            if (links_.find(peer_id) == links_.end()) {
                links_[peer_id] = std::move(peer);
                std::cout << "[+] " << id_ << " accepted from " << peer_id << "\n";
            } else {
                // duplicate (race with outbound) → close
                peer.close();
            }
        }

        // Early out if we already have all links
        if (static_cast<int>(links_.size()) >= expected_links) break;
    }
}

// -------------------- connection setup (no lambdas) --------------------
void MapProtocol::establish_connections() {
    using namespace std::chrono;

    const int expected_links = static_cast<int>(cfg_.neighbors[id_].size());

    // 1) Bind + listen (retries to handle races/TIME_WAIT)
    bool bound_ok = false;
    {
        const int kMaxBindRetries = 50;        // ~10s at 200ms per attempt
        int attempt = 0;
        for (;;) {
            if (listen_sock_.create() && listen_sock_.bind(cfg_.nodes[id_].port)) {
                bound_ok = true;
                break; // ok
            }
            listen_sock_.close();
            ++attempt;
            if (attempt >= kMaxBindRetries) {
                std::cerr << "[!] Node " << id_ << " failed to bind after "
                          << kMaxBindRetries << " attempts on port "
                          << cfg_.nodes[id_].port << "\n";
                break;
            }
            std::this_thread::sleep_for(milliseconds(200));
        }
        if (bound_ok) {
            if (!listen_sock_.listen(16)) {
                std::cerr << "[!] Failed to listen on SCTP socket\n";
            } else {
                // Startup line per node so stdout-<id>.log always shows a first event
                std::cout << "[*] " << id_ << " listening on port "
                          << cfg_.nodes[id_].port << " with "
                          << expected_links << " neighbors\n";
            }
        }
    }

    // 2) Start acceptor thread (only useful if we're listening)
    std::atomic<bool> accepting(true);
    std::thread acceptor_thread;
    if (bound_ok && expected_links > 0) {
        acceptor_thread = std::thread(&MapProtocol::acceptor_loop, this,
                                      &accepting, expected_links);
    }

    // 3) Outgoing connects with HELLO handshake
    for (size_t idx = 0; idx < cfg_.neighbors[id_].size(); ++idx) {
        int nb = cfg_.neighbors[id_][idx];

        // If acceptor already created it, skip
        {
            std::lock_guard<std::mutex> lk(m_);
            if (links_.find(nb) != links_.end()) {
                continue;
            }
        }

        const NodeInfo& info = cfg_.nodes[nb];
        const steady_clock::time_point deadline =
            steady_clock::now() + seconds(40); // allow peers to come up

        SCTPSocket s;
        if (!s.create()) {
            std::cerr << "[!] " << id_ << " failed to create socket for neighbor " << nb << "\n";
            continue;
        }

        bool connected = false;
        while (!stop_.load() && steady_clock::now() < deadline) {
            // NEW: if acceptor already formed this link, stop retrying outbound
            {
                std::lock_guard<std::mutex> lk(m_);
                if (links_.find(nb) != links_.end()) {
                    connected = true;
                    break;
                }
            }

            if (s.connect(info.host, info.port)) {
                // Send our HELLO and expect theirs
                (void)s.send(make_hello(id_));
                std::string hello;
                if (s.receive(hello)) {
                    int peer_id = -1;
                    if (parse_hello(hello, peer_id) && peer_id == nb) {
                        std::lock_guard<std::mutex> lk(m_);
                        if (links_.find(nb) == links_.end()) {
                            links_[nb] = std::move(s);
                            std::cout << "[+] " << id_ << " connected to "
                                      << nb << " (" << info.host << ":" << info.port << ")\n";
                        } else {
                            // acceptor won the race; close outbound
                            s.close();
                        }
                        connected = true;
                        break;
                    }
                }
                // Handshake failed → recreate and retry
                s.close();
                s.create();
            }
            std::this_thread::sleep_for(milliseconds(200));
        }

        if (!connected) {
            // Only complain if we still don't have the link
            std::lock_guard<std::mutex> lk(m_);
            if (links_.find(nb) == links_.end()) {
                std::cerr << "[!] " << id_ << " could not connect to neighbor "
                          << nb << " (" << info.host << ":" << info.port
                          << ") within timeout\n";
            }
        }
    }

    // 4) Grace for late accepts, stop accepting, close listener
    if (acceptor_thread.joinable()) {
        const steady_clock::time_point grace_deadline =
            steady_clock::now() + seconds(2);
        while (static_cast<int>(links_.size()) < expected_links &&
               steady_clock::now() < grace_deadline) {
            std::this_thread::sleep_for(milliseconds(50));
        }
        accepting.store(false);
        acceptor_thread.join();
    }
    listen_sock_.close();

    // 5) Summary
    {
        std::lock_guard<std::mutex> lk(m_);
        std::cout << "[*] Node " << id_ << " established "
                  << links_.size() << " / " << expected_links << " links.\n";
    }
}

// -------------------- output --------------------
void MapProtocol::record_initial_snapshot() {
    std::lock_guard<std::mutex> lk(m_);
    snapshot_mgr_.record_snapshot(vc_); // writes logs/<config>-<id>.out
}

// -------------------- run --------------------
void MapProtocol::run() {
    establish_connections();
    record_initial_snapshot();

    // Keep alive so launcher captures stdout/stderr to logs/
    while (!stop_.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}
