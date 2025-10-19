#include "map_protocol.hpp"

#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include <chrono>
#include <random>
#include <thread>

// Constructor
MapProtocol::MapProtocol(int node_id, const Config &cfg)
    : id_(node_id),
      config_(cfg),
      is_active_(false),
      total_sent_(0),
      vector_clock_(cfg.n, 0)
{
    self_ = cfg.nodes[id_];
    neighbors_ = cfg.neighbors[id_];
}

// Destructor
MapProtocol::~MapProtocol() {
    listener_.close();
    for (auto &pair : outgoing_sockets_) {
        pair.second.close();
    }
    for (auto &pair : incoming_sockets_) {
        pair.second.close();
    }
}

// Entry point
void MapProtocol::start() {
    listener_thread_ = std::thread(&MapProtocol::listen_for_messages, this);
    protocol_thread_ = std::thread(&MapProtocol::run_protocol, this);

    listener_thread_.join();
    protocol_thread_.join();
}

// Initialization
bool MapProtocol::initialize() {
    if (!setup_listener()) return false;
    if (!connect_to_neighbors()) return false;
    accept_connections();
    return true;
}

bool MapProtocol::setup_listener() {
    if (!listener_.create()) return false;
    if (!listener_.bind(self_.port)) return false;
    if (!listener_.listen()) return false;
    return true;
}

bool MapProtocol::connect_to_neighbors() {
    for (int neighbor_id : neighbors_) {
        const NodeInfo &info = config_.nodes[neighbor_id];
        SCTPSocket sock;
        if (!sock.create()) return false;
        if (!sock.connect(info.host, info.port)) return false;
        outgoing_sockets_[neighbor_id] = std::move(sock);
    }
    return true;
}

void MapProtocol::accept_connections() {
    for (size_t i = 0; i < neighbors_.size(); ++i) {
        SCTPSocket sock;
        if (!listener_.accept(sock)) {
            std::cerr << "[!] Failed to accept connection on node " << id_ << "\n";
            continue;
        }
        sockaddr_in addr = sock.get_peer_addr();
        std::string ip = inet_ntoa(addr.sin_addr);
        int port = ntohs(addr.sin_port);

        // Match incoming connection to a known neighbor
        for (int neighbor_id : neighbors_) {
            const NodeInfo &info = config_.nodes[neighbor_id];
            if (info.port == port) {
                incoming_sockets_[neighbor_id] = std::move(sock);
                break;
            }
        }
    }
}

// Protocol logic
void MapProtocol::run_protocol() {
    if (id_ == 0) activate(); // Node 0 starts active

    while (true) {
        if (!is_active_) {
            sleep_ms(100); // Passive wait
            continue;
        }

        int messages_to_send = choose_random_message_count();
        for (int i = 0; i < messages_to_send; ++i) {
            int target = choose_random_neighbor();
            send_to_neighbor(target, "Hello from node " + std::to_string(id_));
            total_sent_++;
            sleep_ms(config_.minSendDelay_ms);
        }

        deactivate();
    }
}

void MapProtocol::listen_for_messages() {
    while (true) {
        for (auto &pair : incoming_sockets_) {
            std::string raw;
            if (pair.second.receive(raw)) {
                Message msg;
                msg.sender_id = pair.first;
                msg.payload = raw;
                msg.vector_clock = vector_clock_; // Placeholder
                process_message(msg);
            }
        }
        sleep_ms(50);
    }
}

void MapProtocol::send_to_neighbor(int neighbor_id, const std::string &payload) {
    Message msg;
    msg.sender_id = id_;
    msg.payload = payload;
    msg.vector_clock = vector_clock_; // Placeholder

    std::ostringstream oss;
    oss << msg.sender_id << "|" << msg.payload; // Simple serialization
    outgoing_sockets_[neighbor_id].send(oss.str());
}

void MapProtocol::process_message(const Message &msg) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (!is_active_ && can_become_active()) {
        activate();
    }
    // Log or process message if needed
    std::cout << "[Node " << id_ << "] Received from " << msg.sender_id
              << ": " << msg.payload << "\n";
}

// State transitions
void MapProtocol::activate() {
    is_active_ = true;
    std::cout << "[Node " << id_ << "] Activated\n";
}

void MapProtocol::deactivate() {
    is_active_ = false;
    std::cout << "[Node " << id_ << "] Became passive\n";
}

bool MapProtocol::can_become_active() const {
    return total_sent_ < config_.maxNumber;
}

// Utility
int MapProtocol::choose_random_neighbor() {
    static std::mt19937 gen(id_ + std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<> dist(0, neighbors_.size() - 1);
    return neighbors_[dist(gen)];
}

int MapProtocol::choose_random_message_count() {
    static std::mt19937 gen(id_ + std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<> dist(config_.minPerActive, config_.maxPerActive);
    return dist(gen);
}

void MapProtocol::sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
