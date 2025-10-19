#include "sctp_wrapper.hpp"
#include <iostream>

int main() {
    SCTPSocket client;
    if (!client.create()) return 1;
    if (!client.connect("dc02.utdallas.edu", 4001)) return 1;

    std::string message = "Hello from dc01!";
    if (!client.send(message)) return 1;

    std::cout << "[dc01] Message sent: " << message << "\n";
    return 0;
}
