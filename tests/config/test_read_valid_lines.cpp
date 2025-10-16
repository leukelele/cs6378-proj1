#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstdio>   // for std::remove
#include "config.hpp"

using std::string;
using std::vector;
using std::ofstream;

void create_file(const string &path, const string &content) {
    ofstream out(path);
    if (!out) {
        std::cerr << "Failed to create test file: " << path << "\n";
        std::exit(1);
    }
    out << content;
    out.close();
}

void delete_file(const string &path) {
    std::remove(path.c_str());
}

void run_read_valid_lines_test(const string &path,
                               const string &fileContent,
                               const vector<string> &expectedLines,
                               const string &expectedName,
                               bool expectedReturn) {
    create_file(path, fileContent);

    vector<string> lines;
    string name;
    bool result = testable_read_valid_lines(path, lines, name);

    if (result != expectedReturn) {
        std::cerr << "Return value mismatch for path: " << path
                  << "\nExpected: " << expectedReturn
                  << ", Got: " << result << "\n";
        delete_file(path);
        std::exit(1);
    }

    if (lines != expectedLines) {
        std::cerr << "Lines mismatch for path: " << path << "\n";
        std::cerr << "Expected:\n";
        for (auto &l : expectedLines) std::cerr << "[" << l << "]\n";
        std::cerr << "Got:\n";
        for (auto &l : lines) std::cerr << "[" << l << "]\n";
        delete_file(path);
        std::exit(1);
    }

    if (name != expectedName) {
        std::cerr << "Config name mismatch for path: " << path
                  << "\nExpected: [" << expectedName << "]"
                  << "\nGot:      [" << name << "]\n";
        delete_file(path);
        std::exit(1);
    }

    delete_file(path);
}

int main() {
    // Test 1: valid file with some valid lines
    run_read_valid_lines_test("test_config1.txt",
        "1 first line\n# comment\n2 second line\nabc invalid",
        {"1 first line", "2 second line"},
        "test_config1",
        true
    );

    // Test 2: file exists but no valid lines
    run_read_valid_lines_test("test_config_empty.txt",
        "# comment only\nabc invalid",
        {},
        "test_config_empty",
        false
    );

    // Test 3: non-existent file
    vector<string> lines;
    string name;
    bool result = testable_read_valid_lines("file_does_not_exist.txt", lines, name);
    if (result != false || !lines.empty() || !name.empty()) {
        std::cerr << "Failed handling non-existent file\n";
        std::exit(1);
    }

    std::cout << "All read_valid_lines() tests passed!\n";
    return 0;
}
