#ifndef MAP_PROTOCOL_HPP
#define MAP_PROTOCOL_HPP

#include "sctp_wrapper.hpp"
#include "message.hpp"
#include "config.hpp"
#include "node.hpp"

#include <unordered_map>
#include <atomic>
#include <string>
#include <thread>
#include <vector>
#include <mutex>

class MapProtocol {
public:
    MapProtocol(int node_id, const Config &cfg);
    ~MapProtocol();

    bool initialize();
    void start();

private:
    // Node identity and configuration
    int id_;
    NodeInfo self_;
    std::vector<int> neighbors_;
    Config config_;

    // State
    std::atomic<bool> is_active_;
    std::atomic<int> total_sent_;
    std::vector<int> vector_clock_;
    std::mutex state_mutex_;

    // Networking
    SCTPSocket listener_;
    std::unordered_map<int, SCTPSocket> outgoing_sockets_;
    std::unordered_map<int, SCTPSocket> incoming_sockets_;

    // Threads
    std::thread listener_thread_;
    std::thread protocol_thread_;

    // Initialization
    bool setup_listener();
    bool connect_to_neighbors();
    void accept_connections();

    // Protocol logic
    void run_protocol();
    void listen_for_messages();
    void send_to_neighbor(int neighbor_id, const std::string &payload);
    void process_message(const Message &msg);

    // State transitions
    void activate();
    void deactivate();
    bool can_become_active() const;

    // Utility
    int choose_random_neighbor();
    int choose_random_message_count();
    void sleep_ms(int ms);
};

#endif // MAP_PROTOCOL_HPP
