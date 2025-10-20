// include/map_protocol.hpp
/****************************************************************************
 * file: map_protocol.hpp
 * description:
 *   Minimal scaffold: connections + initial snapshot (no lambdas).
 ****************************************************************************/
#ifndef MAP_PROTOCOL_HPP
#define MAP_PROTOCOL_HPP

#include "config.hpp"
#include "sctp_wrapper.hpp"
#include "snapshot_manager.hpp"

#include <map>
#include <vector>
#include <mutex>
#include <atomic>
#include <random>
#include <string>

class MapProtocol {
public:
    MapProtocol(const Config& cfg, int node_id);
    void run(); // blocking

private:
    // --- connection setup ---
    void establish_connections();

    // accept loop run by a thread (no lambdas)
    void acceptor_loop(std::atomic<bool>* accepting,
                       int expected_links);

    // --- utilities ---
    bool is_neighbor(int peer_id) const;
    void record_initial_snapshot();

    // handshake helpers
    static std::string make_hello(int id);
    static bool parse_hello(const std::string& s, int& out_id);

private:
    // immutable config
    const Config cfg_;
    const int id_;

    // vector clock (size n)
    std::vector<int> vc_;

    // neighbor_id -> persistent SCTP link
    std::map<int, SCTPSocket> links_;

    // listening socket (only during setup)
    SCTPSocket listen_sock_;

    // concurrency/state
    std::mutex m_;
    std::atomic<bool> stop_;

    // rng
    std::mt19937 rng_;

    // output manager (writes logs/<config>-<id>.out)
    SnapshotManager snapshot_mgr_;
};

#endif // MAP_PROTOCOL_HPP
