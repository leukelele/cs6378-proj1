// lib/map_protocol.cpp
#include "map_protocol.hpp"

#include <iostream>
#include <chrono>
#include <cstring>
#include <unistd.h>

using namespace std::chrono;

MapProtocol::MapProtocol(const Config& cfg, int node_id)
    : cfg_(cfg),
      id_(node_id),
      vc_(cfg.n, 0),
      is_active_(node_id == 0),      // Node 0 starts active (simple choice)
      sent_total_(0),
      stop_(false),
      rng_(static_cast<unsigned>(steady_clock::now().time_since_epoch().count()) ^ (node_id * 0x9e3779b1u)),
      snapshot_mgr_(node_id, cfg.config_name, cfg.n) {}

void MapProtocol::establish_connections() {
    // Create listening socket
    listen_sock_.create();
    listen_sock_.bind(cfg_.nodes[id_].port);
    listen_sock_.listen(5);

    // Connect to neighbors with higher id; accept from lower id
    // First: async connects (retry until success)
    for (int nb : cfg_.neighbors[id_]) {
        if (nb > id_) {
            SCTPSocket s;
            s.create();
            const auto& info = cfg_.nodes[nb];
            // retry loop until connect succeeds
            while (!stop_.load()) {
                if (s.connect(info.host, info.port)) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
            links_[nb] = std::move(s);
            std::cerr << "[+] " << id_ << " connected to " << nb << " (" << info.host
                      << ":" << info.port << ")\n";
        }
    }

    // Accept from neighbors with lower id
    int accepts_needed = 0;
    for (int nb : cfg_.neighbors[id_]) if (nb < id_) ++accepts_needed;

    while (accepts_needed > 0 && !stop_.load()) {
        SCTPSocket peer;
        if (listen_sock_.accept(peer)) {
            // Determine which neighbor this is (by peer port host match)
            sockaddr_in sin = peer.get_peer_addr();
            // Map by scanning neighbors with lower id
            bool mapped = false;
            for (int nb : cfg_.neighbors[id_]) {
                if (nb < id_) {
                    const auto& info = cfg_.nodes[nb];
                    // We can't rely on port alone (NAT not in dcXX); match by port.
                    if (info.port == ntohs(sin.sin_port)) {
                        links_[nb] = std::move(peer);
                        std::cerr << "[+] " << id_ << " accepted from " << nb
                                  << " (:" << info.port << ")\n";
                        --accepts_needed;
                        mapped = true;
                        break;
                    }
                }
            }
            if (!mapped) {
                // Fallback: store anyway (best effort)
                // Find first unmapped lower-id neighbor
                for (int nb : cfg_.neighbors[id_]) {
                    if (nb < id_ && links_.find(nb) == links_.end()) {
                        links_[nb] = std::move(peer);
                        --accepts_needed;
                        break;
                    }
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    // All connections established; we can close the listening socket if desired.
    listen_sock_.close();
}

void MapProtocol::receiver_loop(int neighbor_id) {
    auto& sock = links_[neighbor_id];
    for (;;) {
        if (stop_.load()) return;
        std::string msg;
        if (!sock.receive(msg)) {
            // Backoff and retry on transient failure
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        int sender = -1;
        std::vector<int> mvc;
        std::string payload;
        if (decode_app_message(msg, sender, mvc, payload)) {
            on_receive_app(sender, mvc, payload);
        } else {
            // Unknown or malformed control; ignore for now
        }
    }
}

void MapProtocol::driver_loop() {
    std::uniform_int_distribution<int> per_active(cfg_.minPerActive, cfg_.maxPerActive);

    while (!stop_.load()) {
        // Wait until activated or stopping
        {
            std::unique_lock<std::mutex> lk(m_);
            cv_.wait(lk, [this]{
                return stop_.load() || is_active_.load();
            });
            if (stop_.load()) break;
        }

        // Compute how many messages to send in this active burst
        int to_send = per_active(rng_);
        for (int i = 0; i < to_send; ++i) {
            int nb = pick_random_neighbor();
            if (nb < 0) break; // no neighbors
            // Vector clock: before send
            {
                std::lock_guard<std::mutex> lk(m_);
                if (sent_total_ >= cfg_.maxNumber) {
                    is_active_.store(false);
                    break;
                }
                vc_[id_] += 1;
                std::string payload = pick_random_payload();
                std::string wire = encode_app_message(id_, vc_, payload);
                links_[nb].send(wire);
                ++sent_total_;
            }

            // Respect minSendDelay between sequential sends in a burst
            if (i + 1 < to_send) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cfg_.minSendDelay_ms));
            }
        }

        // End of burst -> go passive
        is_active_.store(false);

        // If we've exhausted our budget, we simply remain passive forever
        // (Part 4 will introduce proper global termination/halt).
    }
}

void MapProtocol::snapshot_timer_loop() {
    // TEMPORARY: local VC snapshot every snapshotDelay_ms
    // This only serves to produce the required output files now.
    while (!stop_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cfg_.snapshotDelay_ms));
        if (stop_.load()) break;
        std::vector<int> snap;
        {
            std::lock_guard<std::mutex> lk(m_);
            snap = vc_;
        }
        snapshot_mgr_.record_snapshot(snap);
    }
}

void MapProtocol::on_receive_app(int from, const std::vector<int>& msg_vc,
                                 const std::string& /*payload*/)
{
    // Vector clock merge: vc[k] = max(vc[k], msg_vc[k]); then vc[i]++
    {
        std::lock_guard<std::mutex> lk(m_);
        if (msg_vc.size() == vc_.size()) {
            for (size_t k = 0; k < vc_.size(); ++k) {
                vc_[k] = std::max(vc_[k], msg_vc[k]);
            }
        }
        vc_[id_] += 1;

        // Activation rule for MAP:
        if (!is_active_.load() && sent_total_ < cfg_.maxNumber) {
            is_active_.store(true);
            cv_.notify_one();
        }
    }
}

std::string MapProtocol::pick_random_payload() {
    // Keep it tiny—payload is irrelevant for Part 1/3
    return "x";
}

int MapProtocol::pick_random_neighbor() {
    const auto& nbs = cfg_.neighbors[id_];
    if (nbs.empty()) return -1;
    std::uniform_int_distribution<int> dist(0, static_cast<int>(nbs.size()) - 1);
    return nbs[dist(rng_)];
}

void MapProtocol::run() {
    // 1) Create SCTP connections
    establish_connections();

    // 2) Start receiver threads
    for (const auto& kv : links_) {
        int nb = kv.first;
        recv_threads_.emplace_back(&MapProtocol::receiver_loop, this, nb);
    }

    // 3) Start driver + snapshot timer
    driver_thread_ = std::thread(&MapProtocol::driver_loop, this);
    snapshot_thread_ = std::thread(&MapProtocol::snapshot_timer_loop, this);

    // 4) If initially active, wake the driver
    if (is_active_.load()) cv_.notify_one();

    // 5) Block forever (Ctrl-C/cleanup.sh will kill the process)
    driver_thread_.join(); // (In practice unreachable until stop_ becomes true)
    snapshot_thread_.join();
    for (auto& t : recv_threads_) t.join();
}
