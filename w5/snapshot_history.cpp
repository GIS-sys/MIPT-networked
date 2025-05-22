#include "snapshot_history.h"

#include <algorithm>
#include "utils.h"

// TODO
//   ? CLIENT_INTERPOLATION_DELAY_US
//   ? clear old
//   ? track time
//   ? track sim frame ids
//   interpolate?
//   simulate
//   delta from history ???

void SnapshotHistory::init(const Entity& e) {
    snapshots.push_back(Snapshot(e, 0, 0));
}

void SnapshotHistory::add(const Entity& e, int current_sim_frame_id, uint32_t curTime) {
    snapshots.push_back(Snapshot(e, current_sim_frame_id, curTime));
}

void SnapshotHistory::update_controls(float thr, float steer) {
    thr = thr;
    steer = steer;
}

Entity SnapshotHistory::get_current_entity() const {
    return current;
}

void SnapshotHistory::_sort() {
    std::sort(snapshots.begin(), snapshots.end(), [](const auto& x, const auto& y) { return x.sim_id - y.sim_id; });
}

void SnapshotHistory::_clear_old(uint32_t current_time) {
    while (snapshots.size() >= 2 && snapshots.front().curTime < current_time)
        snapshots.pop_front();
}

void SnapshotHistory::_predict(uint32_t current_time) {
    // TODO (simulate)
    current = snapshots.back().e;
}

void SnapshotHistory::set_time(uint32_t time) {
    uint32_t current_time = time - CLIENT_INTERPOLATION_DELAY_US;
    _sort();
    _clear_old(current_time);
    _predict(current_time);
}
