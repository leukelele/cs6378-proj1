#include <iostream>
#include <string>
#include <cstdlib>  // for std::exit
#include "config.hpp"

using std::string;

// Helper to print test results for parse_globals
void run_parse_globals_test(const string &input,
                            bool expectedReturn,
                            const Config &expectedCfg = Config()) {
    Config cfg;
    bool result = testable_parse_globals(input, cfg);
    if (result != expectedReturn) {
        std::cerr << "Test failed (return value mismatch):\n"
                  << "  input:    [" << input << "]\n"
                  << "  expected: " << (expectedReturn ? "true" : "false") << "\n"
                  << "  got:      " << (result ? "true" : "false") << "\n";
        std::exit(1);
    }

    // Only check cfg values if parsing should succeed
    if (expectedReturn) {
        if (cfg.n != expectedCfg.n || cfg.minPerActive != expectedCfg.minPerActive ||
            cfg.maxPerActive != expectedCfg.maxPerActive ||
            cfg.minSendDelay_ms != expectedCfg.minSendDelay_ms ||
            cfg.snapshotDelay_ms != expectedCfg.snapshotDelay_ms ||
            cfg.maxNumber != expectedCfg.maxNumber) {
            std::cerr << "Test failed (Config values mismatch):\n"
                      << "  input: [" << input << "]\n";
            std::exit(1);
        }
    }
}

int main() {
    // --- Test 1: valid input ---
    Config expected1;
    expected1.n = 3;
    expected1.minPerActive = 1;
    expected1.maxPerActive = 5;
    expected1.minSendDelay_ms = 10;
    expected1.snapshotDelay_ms = 1000;
    expected1.maxNumber = 20;
    run_parse_globals_test("3 1 5 10 1000 20", true, expected1);

    // --- Test 2: missing fields ---
    run_parse_globals_test("3 1 5", false);

    // --- Test 3: non-numeric field ---
    run_parse_globals_test("3 1 X 10 1000 20", false);

    // --- Test 4: extra fields (should succeed, extra ignored) ---
    Config expected4;
    expected4.n = 3;
    expected4.minPerActive = 1;
    expected4.maxPerActive = 5;
    expected4.minSendDelay_ms = 10;
    expected4.snapshotDelay_ms = 1000;
    expected4.maxNumber = 20;
    run_parse_globals_test("3 1 5 10 1000 20 999", true, expected4);

    std::cout << "All parse_globals() tests passed!\n";
    return 0;
}
