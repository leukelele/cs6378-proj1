#include <iostream>
#include <string>
#include <cstdlib>  // for std::exit
#include "config.hpp"  // gives us access to get_filename_no_ext()

using std::string;

// Helper to print test results for get_filename_no_ext
void run_get_filename_no_ext_test(const string &input, const string &expected) {
    string result = testable_get_filename_no_ext(input);
    if (result != expected) {
        std::cerr << "Test failed:\n"
                  << "  input:    [" << input << "]\n"
                  << "  expected: [" << expected << "]\n"
                  << "  got:      [" << result << "]\n";
        std::exit(1);
    }
}

int main() {
    // Basic filename with extension
    run_get_filename_no_ext_test("file.txt", "file");

    // Filename with multiple dots (take only before last one)
    run_get_filename_no_ext_test("archive.tar.gz", "archive.tar");

    // Filename without extension
    run_get_filename_no_ext_test("Makefile", "Makefile");

    // Full path with extension
    run_get_filename_no_ext_test("/home/user/document.pdf", "/home/user/document");

    // Hidden file (e.g., .gitignore) — important edge case
    run_get_filename_no_ext_test(".gitignore", "");

    // Dot at the end of the filename (edge case)
    run_get_filename_no_ext_test("filename.", "filename");

    // Empty string
    run_get_filename_no_ext_test("", "");

    std::cout << "All get_filename_no_ext() tests passed!\n";
    return 0;
}
