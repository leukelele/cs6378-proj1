/****************************************************************************
 * file: config.hpp
 * author: luke le
 * description:
 *     declares data structures and functions for reading and storing
 *     configuration parameters
 ****************************************************************************/
#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "node.hpp"
#include <vector>

struct Config {
    int n;                          // number of nodes
    int minPerActive, maxPerActive;
    int minSendDelay_ms;
    int snapshotDelay_ms;           // amount of time to wait between 
                                    // initiating snapshots
    int maxNumber;
    std::vector<NodeInfo> nodes;
    std::vector<std::vector<int>> neighbors;
    std::string config_name;
};

bool parse_config(const std::string &path, Config &cfg);
void print_config(const Config &cfg);

#ifdef ENABLE_TESTS
std::string testable_trim                (const std::string &s);
std::string testable_strip_comments      (const std::string &line);
bool        testable_is_valid_line       (const std::string &line);
std::string testable_get_filename_no_ext (const std::string &path);
bool        testable_open_file           (const std::string &path, 
                                                std::ifstream &in);
std::vector<std::string>testable_clean_valid_lines  (std::istream &in);
bool        testable_read_valid_lines    (const std::string &path,
                                                std::vector<std::string> &vL,
                                                std::string &configName);
bool        testable_parse_globals      (const std::string &line, Config &cfg);

bool        testable_parse_nodes         (const std::vector<std::string> &l, 
                                                Config &cfg);
bool        testable_parse_neighbors    (const std::vector<std::string> &l,
                                               Config &cfg);
#endif // ENABLE_TESTS

#endif // CONFIG_HPP
