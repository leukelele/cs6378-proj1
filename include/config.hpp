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

/**
 * @brief holds all configuration parameters parsed from the input file.
 *
 * This structure encapsulates both the global parameters that define
 * system-wide behavior and the per-node information required to establish
 * network topology and communication links.
 *
 * The configuration file is expected to contain:
 *   - One global parameter line with six integer values:
 *       1. n                 — total number of nodes in the system
 *       2. minPerActive      — minimum number of messages an active node
 *                              sends before turning passive
 *       3. maxPerActive      — maximum number of messages an active node
 *                              sends before turning passive
 *       4. minSendDelay_ms   — minimum delay (in milliseconds) between
 *                              consecutive sends while active
 *       5. snapshotDelay_ms  — delay (in milliseconds) between initiating
 *                              successive global snapshots
 *       6. maxNumber         — maximum total number of messages a node may
 *                              send before remaining passive
 *
 *   - n node definition lines, each containing:
 *       <nodeID> <hostName> <port>
 *
 *   - n neighbor definition lines, each listing the IDs of nodes that are
 *     neighbors of node k (0 <= k < n)
 *
 * @param n                 number of nodes in the distributed system.
 * @param minPerActive      minimum number of messages a node sends before
 *                          becoming passive.
 * @param maxPerActive      maximum number of messages a node sends before
 *                          becoming passive.
 * @param minSendDelay_ms   minimum delay between consecutive sends (in ms).
 * @param snapshotDelay_ms  time to wait between initiating snapshots (in ms).
 * @param maxNumber         maximum number of total messages a node can send.
 * @param nodes             vector of NodeInfo structures describing each node
 *                          (ID, hostname, port).
 * @param neighbors         adjacency list representing neighbors of each node.
 * @param config_name       base name of the configuration file (without
 *                          extension).
 */
struct Config {
    int n;
    int minPerActive, maxPerActive;
    int minSendDelay_ms;
    int snapshotDelay_ms;
    int maxNumber;
    std::vector<NodeInfo> nodes;
    std::vector<std::vector<int>> neighbors;
    std::string config_name;
};

/**
 * @brief parses a configuration file and populate a Config structure.
 *
 * Reads the specified configuration file, extracts all valid lines, and fills
 * the provided Config object with global parameters, node definitions, and
 * neighbor relationships. Lines that are empty, contain only comments, or
 * do not begin with a digit are ignored. The expected configuration format
 * is desribed in the Config documentation.
 *
 * Any text after a '#' character on a valid line is treated as a comment and
 * ignored.
 *
 * @param path to the configuration file to be parsed.
 * @param cfg reference to a Config structure that will be filled with parsed
 *        data.
 * @return true if parsing succeeded and all required sections were read;
 *         false if the file could not be opened, was malformed, or missing
 *         entries.
 *
 * @note The function does not assume any particular ordering of lines
 *       beyond the defined format, and it safely handles extra blank lines
 *       or interleaved comments.
 */
bool parse_config(const std::string &path, Config &cfg);

/**
 * @brief print the contents of a Config structure in a human-readable format.
 *
 * Displays all configuration parameters parsed from the input file, including
 * global parameters, node definitions, and neighbor relationships.
 *
 * @param cfg reference to the Config structure whose contents are to be
 *        printed.
 *
 * @note this function prints to standard output (stdout) using std::cout.
 *       It does not perform any file I/O or formatting suitable for logging.
 */
void print_config(const Config &cfg);




// **************************************************************************
// for tests
// **************************************************************************
#ifdef ENABLE_TESTS
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
bool        testable_parse_globals      (const std::string &line, Config &cfg);

bool        testable_parse_nodes         (const std::vector<std::string> &l, 
                                                Config &cfg);
bool        testable_parse_neighbors    (const std::vector<std::string> &l,
                                               Config &cfg);
bool        testable_check_bidirectional_neighbors (const Config &cfg);
#endif // ENABLE_TESTS

#endif // CONFIG_HPP
