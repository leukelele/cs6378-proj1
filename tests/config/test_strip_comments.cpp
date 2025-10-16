#include <iostream>
#include <string>
#include <cstdlib>  // for std::exit
#include "config.hpp"  // gives us access to strip_comments()

using std::string;

// Helper to print test results for strip_comments
void run_strip_comments_test(const string &input, const string &expected) {
    string output = testable_strip_comments(input);
    if (output != expected) {
        std::cerr << "Test failed:\n"
                  << "  input:    [" << input << "]\n"
                  << "  expected: [" << expected << "]\n"
                  << "  got:      [" << output << "]\n";
        std::exit(1);
    }
}

int main() {
    // comment after some text
    run_strip_comments_test("hello # world", "hello");

    // comment at beginning
    run_strip_comments_test("# just a comment", "");

    // no comment at all
    run_strip_comments_test("no comment here", "no comment here");

    // comment at the end of a trimmed line
    run_strip_comments_test("  text before  # comment after", "text before");

    // multiple '#' characters — only first one counts
    run_strip_comments_test("abc # def # ghi", "abc");

    // empty input
    run_strip_comments_test("", "");

    std::cout << "All strip_comments() tests passed!\n";
    return 0;
}
