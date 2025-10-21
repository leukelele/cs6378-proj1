#ifndef TERMINATION_MANAGER_HPP
#define TERMINATION_MANAGER_HPP

#include <atomic>
#include <mutex>

class TerminationManager {
public:
    TerminationManager(int node_id, int num_nodes);

    void mark_passive();
    void mark_active();
    bool is_terminated() const;

private:
    int node_id_;
    int num_nodes_;
    std::atomic<bool> is_passive_;
    std::atomic<bool> terminated_;
    mutable std::mutex m_;
};

#endif // TERMINATION_MANAGER_HPP
