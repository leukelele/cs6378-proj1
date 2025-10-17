#include <iostream>
#include <cstdlib>
#include <cassert>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <chrono>

// Forward declarations
int connect_to_peer(const std::string &host, int port);
int create_listen_socket(int port, int backlog);

// Helper to run the test
void run_connect_to_peer_test(const std::string &host, int port, bool expectedSuccess) {
    int fd = connect_to_peer(host, port);
    bool success = (fd >= 0);

    if (success != expectedSuccess) {
        std::cerr << "Test failed:\n"
                  << "  host: " << host << "\n"
                  << "  port: " << port << "\n"
                  << "  expected success: "
                  << (expectedSuccess ? "true" : "false") << "\n"
                  << "  got: " << (success ? "true" : "false") << "\n";
        std::exit(1);
    }

    if (success) {
        std::cout << "Connected to peer " << host << ":" << port << "\n";
        close(fd);
    }
}

int main() {
    const int TEST_PORT = 6000;
    const std::string LOCALHOST = "127.0.0.1";

    // --- Test 1: successful connection ---
    // Start a temporary listening server in a background thread
    std::thread serverThread([&]() {
        int listen_fd = create_listen_socket(TEST_PORT, 5);
        assert(listen_fd >= 0);

        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int conn_fd = accept(listen_fd, (sockaddr*)&client_addr, &client_len);
        if (conn_fd >= 0) {
            close(conn_fd);
        }
        close(listen_fd);
    });

    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    run_connect_to_peer_test(LOCALHOST, TEST_PORT, true);
    serverThread.join();

    // --- Test 2: invalid host ---
    run_connect_to_peer_test("invalid.hostname.example", TEST_PORT, false);

    // --- Test 3: invalid port ---
    run_connect_to_peer_test(LOCALHOST, -1, false);

    // --- Test 4: connection to non-listening port ---
    run_connect_to_peer_test(LOCALHOST, 6500, false);

    std::cout << "All connect_to_peer() tests passed!\n";
    return 0;
}
