#include "config.hpp"
#include <iostream>
#include <string>

int main() {

    Config cfg;
    const std::string path = CONFIG_FILE_PATH;

    if (!parse_config(path, cfg)) {
        std::cerr << "Failed to parse config file: " << path << "\n";
        return 1;
    }
    print_config(cfg);

    return 0;
}
