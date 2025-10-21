#include "termination_manager.hpp"

TerminationManager::TerminationManager(int node_id, int num_nodes)
    : node_id_(node_id), num_nodes_(num_nodes),
      is_passive_(false), terminated_(false) {}

void TerminationManager::mark_passive() {
    is_passive_.store(true);
}

void TerminationManager::mark_active() {
    is_passive_.store(false);
}

bool TerminationManager::is_terminated() const {
    return is_passive_.load(); // Simplified for now
}
