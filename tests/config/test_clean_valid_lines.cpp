#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdlib>
#include "config.hpp"

using std::string;
using std::vector;
using std::istringstream;

void run_clean_valid_lines_test(const string &input, const vector<string> &expected) {
    istringstream in(input);
    vector<string> result = testable_clean_valid_lines(in);
    if (result != expected) {
        std::cerr << "Test failed:\n";
        std::cerr << "Input:\n[" << input << "]\n";
        std::cerr << "Expected:\n";
        for (auto &s : expected) std::cerr << "[" << s << "]\n";
        std::cerr << "Got:\n";
        for (auto &s : result) std::cerr << "[" << s << "]\n";
        std::exit(1);
    }
}

int main() {
    // Lines with digits → valid
    run_clean_valid_lines_test(
        "1 first line\n2 second line\n3 third line",
        {"1 first line", "2 second line", "3 third line"}
    );

    // Lines with comments
    run_clean_valid_lines_test(
        "1 first line # comment\n# full line comment\n2 second line",
        {"1 first line", "2 second line"}
    );

    // Lines with leading/trailing whitespace
    run_clean_valid_lines_test(
        "  1 spaced line  \n   2 another line   ",
        {"1 spaced line", "2 another line"}
    );

    // Lines that are empty or non-valid
    run_clean_valid_lines_test(
        "\n   \nabc not valid\n1 valid line",
        {"1 valid line"}
    );

    // No valid lines
    run_clean_valid_lines_test(
        "abc\n#comment\n   ",
        {}
    );

    std::cout << "All clean_valid_lines() tests passed!\n";
    return 0;
}
