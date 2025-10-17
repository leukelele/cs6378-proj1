#include <iostream>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cassert>
#include <thread>

// Forward declarations of the functions under test
int create_listen_socket(int port, int backlog);
int connect_to_peer(const std::string &host, int port);

// Helper: run a simple server in another thread so we can connect to it
void run_server_thread(int port) {
    int fd = create_listen_socket(port, 5);
    if (fd < 0) {
        std::cerr << "[Server] Failed to create listen socket\n";
        return;
    }

    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd >= 0) {
        // Just hold connection briefly
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        close(client_fd);
    }

    close(fd);
}

// Helper for testing
void run_connect_to_peer_test(const std::string &host, int port, bool expectedSuccess) {
    int fd = connect_to_peer(host, port);
    bool success = (fd >= 0);

    if (success != expectedSuccess) {
        std::cerr << "Test failed:\n"
                  << "  host: " << host << "\n"
                  << "  port: " << port << "\n"
                  << "  expected success: " << (expectedSuccess ? "true" : "false") << "\n"
                  << "  got: " << (success ? "true" : "false") << "\n";
        std::exit(1);
    }

    if (success) {
        std::cout << "Successfully connected to " << host << ":" << port << "\n";
        close(fd);
    }
}

int main() {
    // --- Test 1: valid connection ---
    // Spin up server thread first
    int testPort = 6000;
    std::thread serverThread(run_server_thread, testPort);
    // Give server time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    run_connect_to_peer_test("127.0.0.1", testPort, true);

    serverThread.join();

    // --- Test 2: invalid port ---
    run_connect_to_peer_test("127.0.0.1", -1, false);

    // --- Test 3: no server listening on this port ---
    run_connect_to_peer_test("127.0.0.1", 6010, false);

    // --- Test 4: invalid hostname ---
    run_connect_to_peer_test("invalid.host.name", 6001, false);

    std::cout << "All connect_to_peer() tests passed!\n";
    return 0;
}
