#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "node.hpp"
#include <vector>

using namespace std;

struct Config {
    int n;
    int minPerActive, maxPerActive;
    int minSendDelay_ms;
    int snapshotDelay_ms;
    int maxNumber;
    vector<NodeInfo> nodes;
    vector<vector<int>> neighbors;
    string config_name;
};

bool parse_config(const string &path, Config &cfg);
void print_config(const Config &cfg);

#ifdef ENABLE_TESTS
string              testable_trim               (const string &s);
string              testable_strip_comments     (const string &line);
bool                testable_is_valid_line      (const string &line);
string              testable_get_filename_no_ext(const string &path);
bool                testable_open_file          (const string &path, 
                                                   ifstream &in);
vector<string>      testable_clean_valid_lines  (istream &in);
bool                testable_read_valid_lines   (const string &path,
                                                   vector<string> &validLines,
                                                   string &configName);
bool                testable_parse_globals      (const string &line,
                                                   Config &cfg);
bool                testable_parse_nodes        (const vector<string> &lines,
                                                   Config &cfg);
bool                testable_parse_neighbors    (const vector<string> &lines,
                                                   Config &cfg);
#endif // ENABLE_TESTS
#endif // CONFIG_HPP
