#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>  // for std::exit
#include "config.hpp"

using std::string;
using std::vector;

// Helper to print test results for parse_neighbors
void run_parse_neighbors_test(const vector<string> &lines,
                              int n,
                              const vector<vector<int>> &expectedNeighbors) {
    Config cfg;
    cfg.n = n;
    bool result = testable_parse_neighbors(lines, cfg);

    if (!result) {
        std::cerr << "parse_neighbors() should always return true\n";
        std::exit(1);
    }

    if (cfg.neighbors.size() != expectedNeighbors.size()) {
        std::cerr << "Test failed: neighbor vector size mismatch\n";
        std::exit(1);
    }

    for (size_t i = 0; i < expectedNeighbors.size(); ++i) {
        if (cfg.neighbors[i] != expectedNeighbors[i]) {
            std::cerr << "Test failed: neighbors for node " << i << " mismatch\n";
            std::cerr << "Expected: ";
            for (auto x : expectedNeighbors[i]) std::cerr << x << " ";
            std::cerr << "\nGot:      ";
            for (auto x : cfg.neighbors[i]) std::cerr << x << " ";
            std::cerr << "\n";
            std::exit(1);
        }
    }
}

int main() {
    // --- Test 1: normal neighbors ---
    vector<string> lines1 = {
        "1 2",
        "0 2",
        "0 1"
    };
    vector<vector<int>> expected1 = {
        {1, 2},  // node 0
        {0, 2},  // node 1
        {0, 1}   // node 2
    };
    run_parse_neighbors_test(lines1, 3, expected1);

    // --- Test 2: invalid neighbors (self-reference and out of bounds) ---
    vector<string> lines2 = {
        "0 1 3 -1",  // node 0 → 0 ignored, 1 added, 3/-1 ignored
        "1 2 5",     // node 1 → 1 ignored, 2 added, 5 ignored
        "0 2 2"      // node 2 → 0 added, 2 ignored, 2 ignored
    };
    vector<vector<int>> expected2 = {
        {1},    // node 0
        {2},    // node 1
        {0}     // node 2
    };
    run_parse_neighbors_test(lines2, 3, expected2);

    // --- Test 3: empty lines ---
    vector<string> lines3 = {"", "", ""};
    vector<vector<int>> expected3 = {{}, {}, {}};
    run_parse_neighbors_test(lines3, 3, expected3);

    std::cout << "All parse_neighbors() tests passed!\n";
    return 0;
}
