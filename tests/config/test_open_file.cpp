#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>  // for std::exit
#include "config.hpp"  // gives us access to open_file()

using std::string;
using std::ifstream;

// Helper to run tests for open_file()
void run_open_file_test(const string &path, bool expected) {
    ifstream in;
    bool result = testable_open_file(path, in);
    if (result != expected) {
        std::cerr << "Test failed:\n"
                  << "  input path: [" << path << "]\n"
                  << "  expected:   " << (expected ? "true" : "false") << "\n"
                  << "  got:        " << (result ? "true" : "false") << "\n";
        std::exit(1);
    }
    if (in.is_open()) {
        in.close();
    }
}

int main() {
    // ✅ existing file — should succeed
    // assuming config.txt exists in ds/config.txt
    run_open_file_test("../ds/config.txt", true);

    // ❌ non-existent file — should fail
    run_open_file_test("../ds/this_file_does_not_exist.txt", false);

    std::cout << "All open_file() tests passed!\n";
    return 0;
}
