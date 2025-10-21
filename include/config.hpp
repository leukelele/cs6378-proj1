/****************************************************************************
 * file: config.hpp
 * author: luke le
 * description:
 *     declares data structures and functions for reading and storing
 *     configuration parameters
 ****************************************************************************/
#ifndef config_hpp
#define config_hpp

#include "node.hpp"
#include <vector>

struct config {
    int n;                          // number of nodes
    int minperactive, maxperactive;
    int minsenddelay_ms;
    int snapshotdelay_ms;           // amount of time to wait between 
                                    // initiating snapshots
    int maxnumber;
    std::vector<nodeinfo> nodes;
    std::vector<std::vector<int>> neighbors;
    std::string config_name;
};

bool parse_config(const std::string &path, config &cfg);
void print_config(const config &cfg);

#ifdef enable_tests
std::string testable_trim                (const std::string &s);
std::string testable_strip_comments      (const std::string &line);
bool        testable_is_valid_line       (const std::string &line);
std::string testable_get_filename_no_ext (const std::string &path);
bool        testable_open_file           (const std::string &path, 
                                                std::ifstream &in);
std::vector<std::string>testable_clean_valid_lines  (std::istream &in);
bool        testable_read_valid_lines    (const std::string &path,
                                                std::vector<std::string> &vl,
                                                std::string &configname);
bool        testable_parse_globals      (const std::string &line, config &cfg);

bool        testable_parse_nodes         (const std::vector<std::string> &l, 
                                                config &cfg);
bool        testable_parse_neighbors    (const std::vector<std::string> &l,
                                               config &cfg);
#endif // enable_tests

#endif // config_hpp
