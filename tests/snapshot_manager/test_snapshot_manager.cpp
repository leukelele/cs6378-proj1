#include <iostream>
#include <vector>
#include <cassert>
#include "snapshot_manager.hpp"

int main() {
    SnapshotManager sm(0, "testconfig", 3);

    std::vector<int> vc1 = {1, 0, 0};
    std::vector<int> vc2 = {2, 1, 0};

    sm.record_snapshot(vc1);
    sm.record_snapshot(vc2);

    std::cout << "SnapshotManager test passed.\n";
    return 0;
}
