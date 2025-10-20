#include <iostream>
#include <vector>
#include <cassert>
#include "map_protocol.hpp"

int main() {
    Config cfg;
    cfg.n = 3;
    cfg.minPerActive = 1;
    cfg.maxPerActive = 2;
    cfg.minSendDelay_ms = 100;
    cfg.snapshotDelay_ms = 1000;
    cfg.maxNumber = 5;
    cfg.config_name = "testconfig";

    NodeInfo node0 = {0, "localhost", 4000};
    NodeInfo node1 = {1, "localhost", 4001};
    NodeInfo node2 = {2, "localhost", 4002};
    cfg.nodes = {node0, node1, node2};
    cfg.neighbors = {{1,2}, {0,2}, {0,1}};

    MapProtocol mp(0, cfg);
    assert(mp.initialize() == true);

    std::cout << "MapProtocol test passed.\n";
    return 0;
}
