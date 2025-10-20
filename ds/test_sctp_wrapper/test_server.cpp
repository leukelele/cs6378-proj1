#include "sctp_wrapper.hpp"
#include <iostream>

int main() {
    SCTPSocket server;
    if (!server.create()) return 1;
    if (!server.bind(4001)) return 1; // Port for dc02
    if (!server.listen()) return 1;

    std::cout << "[dc02] Server listening on port 4001...\n";

    SCTPSocket client;
    if (!server.accept(client)) return 1;

    std::string msg;
    if (client.receive(msg)) {
        std::cout << "[dc02] Received: " << msg << "\n";
    }

    return 0;
}
