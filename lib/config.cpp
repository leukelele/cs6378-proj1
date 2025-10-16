#include "config.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>

// **************************************************************************
// helper funcs are only visible within this translation unit; the funcs 
// handle basic string cleaning, file reading, and config parsing.
// **************************************************************************
namespace {

    /**
     * @brief trim leading and trailing whitespace from a string
     * @param s input string.
     * @return a copy of the string with whitespace removed from both ends
     */
    string trim(const string &s) {
        size_t a = s.find_first_not_of(" \t\r\n");
        /**
         * in C++, npos is a special constant defined in the std::string class
         * that represents “not found” or an invalid position.
         */
        if (a == string::npos) return "";
        size_t b = s.find_last_not_of(" \t\r\n");
        return s.substr(a, b - a + 1);
    } // trim()

    /**
     * @brief remove everything after the first '#' character (for comments)
     * @param line input string.
     * @return the line trimmed without trailing comments
     */
    string strip_comments(const string &line) {
        size_t hash = line.find('#');
        if (hash != string::npos)
            return trim(line.substr(0, hash));
        return line;
    } // strip_comments()

    /**
     * @brief determine if a line is considered a valid config line; a valid
     * line starts with a digit
     * @param line Input string.
     * @return True if valid, false otherwise.
     */
    bool is_valid_line(const string &line) {
        return !line.empty() && isdigit(static_cast<unsigned char>(line[0]));
    } // is_valid_line()

    /**
     * @brief extract the filename without its extension from a path
     * @param path file path
     * @return filename without extension
     */
    string get_filename_no_ext(const string &path) {
        size_t lastdot = path.find_last_of('.');
        if (lastdot == string::npos) return path;
        return path.substr(0, lastdot);
    } // get_filename_no_ext()

    /**
     * @brief attempt to open a file for reading
     * @param path path to file
     * @param in input file stream
     * @return true if file was opened successfully, false otherwise.
     */
    bool open_file(const string &path, ifstream &in) {
        in.open(path);
        if (!in.is_open()) {
            cerr << "Cannot open config file: " << path << "\n";
            return false;
        }
        return true;
    } // open_file()

    /**
     * @brief read all lines from a file stream, clean them, and filter valid
     * config lines
     * @param in open file stream.
     * @return vector of valid, trimmed lines.
     */
    vector<string> clean_valid_lines(istream &in) {
        vector<string> lines;
        string line;
        while (getline(in, line)) {
            line = trim(line);
            if (line.empty()) continue;
            line = strip_comments(line);
            if (is_valid_line(line))
                lines.push_back(line);
        }
        return lines;
    } // clean_valid_lines()

    /**
     * @brief top-level helper, reads valid lines from a file and extract
     * config name
     * @param path path to config file
     * @param validLines output vector with valid lines
     * @param configName output config name
     * @return true if file opened and contained valid lines, false otherwise
     */
    bool read_valid_lines(const string &path, vector<string> &validLines,
            string &configName) {
        ifstream in;
        if (!open_file(path, in)) return false;
        configName = get_filename_no_ext(path);
        validLines = clean_valid_lines(in);
        in.close();
        return !validLines.empty();
    } // read_valid_lines()

    /**
     * @brief parse the first line of the config file
     * @param line the first config line
     * @param cfg config structure to fill
     * @return true if parsing succeeded, false otherwise
     */
    bool parse_globals(const string &line, Config &cfg) {
        istringstream iss(line);
        if (!(iss >> cfg.n >> cfg.minPerActive >> cfg.maxPerActive 
                  >> cfg.minSendDelay_ms >> cfg.snapshotDelay_ms 
                  >> cfg.maxNumber)) {
            cerr << "Invalid first config line\n";
            return false;
        }
        return true;
    } // parse_globals()

    /**
     * @brief parse the node definition lines and populate 'cfg.nodes'
     * @param lines vector of node definition lines.
     * @param cfg config structure to fill.
     * @return true if all node lines parsed successfully, false otherwise.
     */
    bool parse_nodes(const vector<string> &lines, Config &cfg) {
        cfg.nodes.resize(cfg.n);
        for (int i = 0; i < cfg.n; ++i) {
            istringstream iss(lines[i]);
            int id; string host; int port;
            if (!(iss >> id >> host >> port)) {
                cerr << "Invalid node line: " << lines[i] << "\n";
                return false;
            }
            cfg.nodes[id] = {id, port, host};
        }
        return true;
    } // parse_nodes

    /**
     * @brief parse neighbor definitions for each node
     * @param lines Vector of neighbor lines
     * @param cfg Config structure to fill
     * @return always true (invalid neighbors are skipped silently)
     */
    bool parse_neighbors(const vector<string> &lines, Config &cfg) {
        cfg.neighbors.assign(cfg.n, vector<int>());
        for (int k = 0; k < cfg.n; ++k) {
            istringstream iss(lines[k]);
            int nb;
            while (iss >> nb) {
                if (nb >= 0 && nb < cfg.n && nb != k)
                    cfg.neighbors[k].push_back(nb);
            }
        }
        return true;
    } // parse_neighbors()

} // end anonymous namespace

//   the folloing exposes internals for testing (if enabled)
#ifdef ENABLE_TESTS
string testable_trim(const string &s) { return trim(s); }

string testable_strip_comments(const string &line)
{ return strip_comments(line); }

bool testable_is_valid_line(const string &line)
{ return is_valid_line(line); }

string testable_get_filename_no_ext(const string &path)
{ return get_filename_no_ext(path); }

bool testable_open_file(const string &path, ifstream &in)
{ return open_file(path, in); }

vector<string> testable_clean_valid_lines(istream &in)
{ return clean_valid_lines(in); }

bool testable_read_valid_lines(const string &path, vector<string> &validLines,
        string &configName)
{ return read_valid_lines(path, validLines, configName); }

bool testable_parse_globals(const string &line, Config &cfg)
{ return parse_globals(line, cfg); }

bool testable_parse_nodes(const vector<string> &lines, Config &cfg)
{ return parse_nodes(lines, cfg); }

bool testable_parse_neighbors(const vector<string> &lines, Config &cfg)
{ return parse_neighbors(lines, cfg); }
#endif // ENABLE_TESTS

/**
 * @brief top-level function to parse a config file and fill a 'Config' struct
 * 
 * reads a config file, extracts valid lines, and populates the fields of
 * the provided Config struct (`cfg`). This includes:
 *   - Global settings (first line)
 *   - Node definitions
 *   - Neighbor definitions
 *
 * @param path path to the config file
 * @param cfg config struct to fill with parsed values
 * @return true if parsing succeeded, false otherwise
 *
 * the function performs the following steps:
 * 1. read all valid, non-empty, non-comment lines from the file using
 *    `read_valid_lines()`. If no valid lines are found, returns false.
 * 2. parse the first line (global settings) with `parse_globals()`.
 *    If parsing fails, return false.
 * 3. check that the number of valid lines is sufficient for `n` nodes:
 *    - Expected lines: 1 (globals) + n (nodes) + n (neighbors) = 2*n + 1
 *    - If there are fewer lines, return false.
 * 4. slice the valid lines into:
 *    - Node lines (`validLines[1]` to `validLines[n]`)
 *    - Neighbor lines (`validLines[n+1]` to `validLines[2*n]`)
 * 5. parse the node definitions using `parse_nodes()`.
 *    If parsing fails, return false.
 * 6. parse the neighbor definitions using `parse_neighbors()`.
 *    Always returns true, but still checked for consistency.
 * 7. ff all steps succeed, return true.
 */
bool parse_config(const string &path, Config &cfg) {
    vector<string> validLines;

    // Step 1: read valid, trimmed lines from file
    if (!read_valid_lines(path, validLines, cfg.config_name)) {
        cerr << "No valid lines found in config\n";
        return false;
    }

    // Step 2: parse global settings
    if (!parse_globals(validLines[0], cfg)) return false;

    // Step 3: check number of valid lines
    size_t expected = static_cast<size_t>(2 * cfg.n + 1);
    if (validLines.size() < expected) {
        cerr << "Config has fewer than expected valid lines. expected >= "
             << expected << " got " << validLines.size() << "\n";
        return false;
    }

    // Step 4: slice lines into nodes and neighbors
    auto nodeBegin = validLines.begin() + 1;
    auto nodeEnd = nodeBegin + cfg.n;
    auto neighborBegin = nodeEnd;
    auto neighborEnd = neighborBegin + cfg.n;

    // Step 5: parse node definitions
    if (!parse_nodes({nodeBegin, nodeEnd}, cfg)) return false;

    // Step 6: parse neighbor definitions
    if (!parse_neighbors({neighborBegin, neighborEnd}, cfg)) return false;

    // Step 7: all successful
    return true;
} // parse_config()

/**
 * @brief prints the parsed contents stored in 'Config'
 *
 * @param cfg config struct for printing
 */
void print_config(const Config &cfg) {
    cout << "[!] Config parsed successfully!\n";
    cout << "[*] Config file: " << cfg.config_name << "\n\n";

    // print global variables
    cout << "=== Global Parameters ===\n";
    cout << "Number of nodes (n):       " << cfg.n << "\n";
    cout << "minPerActive:              " << cfg.minPerActive << "\n";
    cout << "maxPerActive:              " << cfg.maxPerActive << "\n";
    cout << "minSendDelay (ms):         " << cfg.minSendDelay_ms << "\n";
    cout << "snapshotDelay (ms):        " << cfg.snapshotDelay_ms << "\n";
    cout << "maxNumber:                 " << cfg.maxNumber << "\n\n";

    // print node information
    cout << "=== Nodes ===\n";
    for (const auto &node : cfg.nodes) {
        cout << "Node ID: " << node.id
                  << " | Host: " << node.host
                  << " | Port: " << node.port << "\n";
    }

    // print neighbors
    cout << "\n=== Neighbors ===\n";
    for (size_t i = 0; i < cfg.neighbors.size(); ++i) {
        cout << "Node " << i << " neighbors: ";
        if (cfg.neighbors[i].empty()) {
            cout << "(none)";
        } else {
            for (size_t j = 0; j < cfg.neighbors[i].size(); ++j) {
                cout << cfg.neighbors[i][j];
                if (j + 1 < cfg.neighbors[i].size())
                    cout << ", ";
            }
        }
        cout << "\n";
    }
} // print_config()
