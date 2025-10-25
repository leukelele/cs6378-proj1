#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>   // for std::exit()
#include "config.hpp"  // for Config struct and function prototype

using std::cout;
using std::cerr;
using std::vector;
using std::string;

// helper to run one test case
void run_bidirectional_test(const vector<vector<int>> &neighbors,
                            bool expected,
                            const string &description) {
    Config cfg;
    cfg.neighbors = neighbors;

    bool result = testable_check_bidirectional_neighbors(cfg);

    if (result != expected) {
        cerr << "Test failed: " << description << "\n"
             << "  Expected: " << (expected ? "true" : "false") << "\n"
             << "  Got: " << (result ? "true" : "false") << "\n";
        std::exit(1);
    }
}

// entry point
int main() {
    // 1️⃣ perfectly bidirectional connections
    run_bidirectional_test(
        {
            {1, 2},   // node 0 ↔ 1,2
            {0, 2},   // node 1 ↔ 0,2
            {0, 1}    // node 2 ↔ 0,1
        },
        true,
        "fully bidirectional network"
    );

    // 2️⃣ unidirectional edge (0→1 but 1→0 missing)
    run_bidirectional_test(
        {
            {1},   // node 0 → 1
            {},    // node 1 → (none)
        },
        false,
        "unidirectional edge 0→1"
    );

    // 3️⃣ out-of-range neighbor index (node 0 lists nonexistent node 3)
    run_bidirectional_test(
        {
            {3},  // invalid neighbor
            {}, 
            {},
        },
        false,
        "out-of-range neighbor"
    );

    // 4️⃣ asymmetric partial graph
    run_bidirectional_test(
        {
            {1},       // 0 → 1
            {0, 2},    // 1 ↔ 0 and → 2
            {}         // 2 → none
        },
        false,
        "missing reverse edge 2↔1"
    );

    // 5️⃣ empty neighbor lists (no edges at all)
    run_bidirectional_test(
        {
            {}, {}, {}
        },
        true,
        "no connections (trivially bidirectional)"
    );

    cout << "all check_bidirectional_neighbors() tests passed!\n";
    return 0;
}
