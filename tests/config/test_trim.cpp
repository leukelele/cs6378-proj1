#include <iostream>
#include <string>
#include <cassert>  // for assert()
#include "config.hpp"  // gives us access to trim()

// Helper to print test results
using std::string;

void run_trim_test(const std::string &input, const std::string &expected) {
    string output = testable_trim(input);
    if (output != expected) {
    std::cerr << "Test failed:\n"
              << "  input:    [" << input << "]\n"
              << "  expected: [" << expected << "]\n"
              << "  got:      [" << output << "]\n";
    std::exit(1);
  }
}

int main() {
    // basic trimming
    run_trim_test("   hello   ", "hello");
    run_trim_test("\t  world\n", "world");

    // no trimming needed
    run_trim_test("text", "text");

    // only whitespace
    run_trim_test("   \t \n  ", "");

    // mixed spaces
    run_trim_test("  \r\ntrim me\t ", "trim me");

    std::cout << "All trim() tests passed!\n";
    return 0;
}
