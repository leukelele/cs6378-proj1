#include "config.hpp"
#include "map_protocol.hpp"

#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    // arg verification
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <node_id>\n";
        return 1;
    }
    int node_id = -1;
    try {
        node_id = std::stoi(argv[1]);
    } catch (...) {
        std::cerr << "[!] invalid node_id: " << argv[1] << "\n";
        return 1;
    }

    // config parsing
    Config cfg;
    std::string path = CONFIG_FILE_PATH;      // set by CMake
    if (!parse_config(path, cfg)) {
        std::cerr << "[!] something went wrong with the config file.\n";
        return 1;
    }

    // map protocol
    MapProtocol node(cfg, node_id);
    return node.run();
}
