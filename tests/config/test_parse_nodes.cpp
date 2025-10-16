#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>  // for std::exit
#include "config.hpp"

using std::string;
using std::vector;

// Helper to print test results for parse_nodes
void run_parse_nodes_test(const vector<string> &lines,
                          int n,
                          bool expectedReturn,
                          const vector<NodeInfo> &expectedNodes = {}) {
    Config cfg;
    cfg.n = n;  // number of nodes expected
    bool result = testable_parse_nodes(lines, cfg);

    if (result != expectedReturn) {
        std::cerr << "Test failed (return value mismatch):\n"
                  << "  expected: " << (expectedReturn ? "true" : "false") << "\n"
                  << "  got:      " << (result ? "true" : "false") << "\n";
        std::exit(1);
    }

    if (expectedReturn) {
        if (cfg.nodes.size() != expectedNodes.size()) {
            std::cerr << "Test failed: node vector size mismatch\n";
            std::exit(1);
        }
        for (size_t i = 0; i < expectedNodes.size(); ++i) {
            if (cfg.nodes[i].id != expectedNodes[i].id ||
                cfg.nodes[i].port != expectedNodes[i].port ||
                cfg.nodes[i].host != expectedNodes[i].host) {
                std::cerr << "Test failed: node " << i << " mismatch\n";
                std::exit(1);
            }
        }
    }
}

int main() {
    // --- Test 1: valid nodes ---
    vector<string> validLines = {
        "0 localhost 5000",
        "1 127.0.0.1 5001",
        "2 example.com 5002"
    };
    vector<NodeInfo> expectedNodes = {
        {0, 5000, "localhost"},
        {1, 5001, "127.0.0.1"},
        {2, 5002, "example.com"}
    };
    run_parse_nodes_test(validLines, 3, true, expectedNodes);

    // --- Test 2: missing fields ---
    vector<string> missingFields = {
        "0 localhost 5000",
        "1 127.0.0.1"  // missing port
    };
    run_parse_nodes_test(missingFields, 2, false);

    // --- Test 3: non-numeric port ---
    vector<string> badPort = {
        "0 localhost 5000",
        "1 127.0.0.1 X"
    };
    run_parse_nodes_test(badPort, 2, false);

    std::cout << "All parse_nodes() tests passed!\n";
    return 0;
}
