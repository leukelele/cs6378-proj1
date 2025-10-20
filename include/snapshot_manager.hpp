#ifndef SNAPSHOT_MANAGER_HPP
#define SNAPSHOT_MANAGER_HPP

#include <vector>
#include <string>
#include <mutex>

class SnapshotManager {
public:
    SnapshotManager(int node_id, const std::string &config_name, int num_nodes);

    void record_snapshot(const std::vector<int> &vector_clock);
    void write_snapshots_to_file();

private:
    int node_id_;
    std::string config_name_;
    int num_nodes_;
    std::vector<std::vector<int>> snapshots_;
    std::mutex snapshot_mutex_;
};

#endif // SNAPSHOT_MANAGER_HPP
