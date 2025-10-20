#include "snapshot_manager.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <sys/stat.h>

// --- new helper: strip directory, keep basename only ---
static std::string basename_no_dirs(const std::string& p) {
    size_t slash = p.find_last_of("/\\");
    return (slash == std::string::npos) ? p : p.substr(slash + 1);
}

SnapshotManager::SnapshotManager(int node_id, const std::string &config_name,
    int num_nodes)
    : node_id_(node_id), config_name_(config_name), num_nodes_(num_nodes) {}

void SnapshotManager::record_snapshot(const std::vector<int> &vector_clock) {
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    snapshots_.push_back(vector_clock);
    mkdir("logs", 0777); // creates logs/ if not present
    write_snapshots_to_file();
}

void SnapshotManager::write_snapshots_to_file() {
    std::ostringstream filename;

    // sanitize config name so we never embed a path in the output filename
    const std::string cfg_base = basename_no_dirs(config_name_);

    filename << "logs/" << cfg_base << "-" << node_id_ << ".out";
    std::ofstream out(filename.str());

    if (!out.is_open()) {
        std::cerr << "[!] Failed to open snapshot output file: "
                  << filename.str() << "\n";
        return;
    }

    for (const auto &vc : snapshots_) {
        for (size_t i = 0; i < vc.size(); ++i) {
            out << vc[i];
            if (i + 1 < vc.size()) out << " ";
        }
        out << "\n";
    }

    out.close();
}
