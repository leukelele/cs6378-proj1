#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstdio>   // for std::remove
#include "config.hpp"

using std::string;
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

int main() {
    // --- Test 1: valid config ---
    const string path1 = "test_config_valid.txt";
    string content1 =
        "2 1 5 10 1000 20\n"   // globals: n=2
        "0 localhost 5000\n"    // node 0
        "1 127.0.0.1 5001\n"    // node 1
        "1\n"                   // neighbors for node 0
        "0\n";                  // neighbors for node 1

    create_file(path1, content1);

    Config cfg1;
    bool result1 = parse_config(path1, cfg1);
    if (!result1) {
        std::cerr << "Test 1 failed: valid config should parse\n";
        delete_file(path1);
        std::exit(1);
    }
    if (cfg1.n != 2 || cfg1.minPerActive != 1 || cfg1.nodes.size() != 2 ||
        cfg1.neighbors[0][0] != 1 || cfg1.neighbors[1][0] != 0) {
        std::cerr << "Test 1 failed: cfg fields incorrect\n";
        delete_file(path1);
        std::exit(1);
    }
    delete_file(path1);

    // --- Test 2: missing lines ---
    const string path2 = "test_config_missing.txt";
    string content2 =
        "2 1 5 10 1000 20\n"   // globals: n=2
        "0 localhost 5000\n";   // only one node line

    create_file(path2, content2);

    Config cfg2;
    bool result2 = parse_config(path2, cfg2);
    if (result2) {
        std::cerr << "Test 2 failed: should fail due to missing lines\n";
        delete_file(path2);
        std::exit(1);
    }
    delete_file(path2);

    // --- Test 3: malformed globals ---
    const string path3 = "test_config_badglobals.txt";
    string content3 =
        "2 1 X 10 1000 20\n"   // X is invalid
        "0 localhost 5000\n"
        "1 127.0.0.1 5001\n"
        "1\n0\n";

    create_file(path3, content3);

    Config cfg3;
    bool result3 = parse_config(path3, cfg3);
    if (result3) {
        std::cerr << "Test 3 failed: should fail due to malformed globals\n";
        delete_file(path3);
        std::exit(1);
    }
    delete_file(path3);

    std::cout << "All parse_config() tests passed!\n";
    return 0;
}
