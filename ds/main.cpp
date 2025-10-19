#include "config.hpp"
#include "map_protocol.hpp"

#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <node_id>\n";
        return 1;
    }

    int node_id = std::stoi(argv[1]);

    Config cfg;
    const std::string path = CONFIG_FILE_PATH;
    if (!parse_config(path, cfg)) {
        std::cerr << "Failed to parse config file: " << path << "\n";
        return 1;
    }
    print_config(cfg);

    if (node_id < 0 || node_id >= cfg.n) {
        std::cerr << "Invalid node ID: " << node_id << "\n";
        return 1;
    }

    MapProtocol node(node_id, cfg);
    if (!node.initialize()) {
        std::cerr << "[!] Node " << node_id << " failed to initialize.\n";
        return 1;
    }

    node.start();

    return 0;
}
