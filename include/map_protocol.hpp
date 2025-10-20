// include/map_protocol.hpp
/****************************************************************************
 * file: map_protocol.hpp
 * author: luke le
 * description:
 * Minimal MAP protocol + vector clocks (Part 1 & 3) using persistent SCTP links.
 ****************************************************************************/
#ifndef MAP_PROTOCOL_HPP
#define MAP_PROTOCOL_HPP

#include "config.hpp"
#include "sctp_wrapper.hpp"
#include "message.hpp"
#include "snapshot_manager.hpp"

#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <random>

class MapProtocol {
public:
    MapProtocol(const Config& cfg, int node_id);

    // Blocking run; returns when process is terminated externally (Ctrl-C) or
    // when we finish sending maxNumber bursts and no further activation occurs.
    void run();

private:
    void establish_connections();
    void receiver_loop(int neighbor_id); // one thread per neighbor
    void driver_loop();                  // sends bursts when active
    void snapshot_timer_loop();          // temporary: periodic VC recording

    void on_receive_app(int from, const std::vector<int>& msg_vc,
                        const std::string& payload);

    // helpers
    std::string pick_random_payload();
    int pick_random_neighbor();

private:
    const Config cfg_;
    const int id_;
    std::vector<int> vc_;

    // neighbor_id -> SCTPSocket
    std::map<int, SCTPSocket> links_;

    // listening socket
    SCTPSocket listen_sock_;

    // concurrency
    std::mutex m_;
    std::condition_variable cv_;
    std::atomic<bool> is_active_;
    int sent_total_; // guarded by m_ (small increments)
    std::atomic<bool> stop_;

    // RNG
    std::mt19937 rng_;

    // snapshot
    SnapshotManager snapshot_mgr_;
    std::vector<std::thread> recv_threads_;
    std::thread driver_thread_;
    std::thread snapshot_thread_;
};

#endif // MAP_PROTOCOL_HPP
