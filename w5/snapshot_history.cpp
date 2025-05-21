#include "snapshot_history.h"
#include "utils.h"

// TODO
//   CLIENT_INTERPOLATION_DELAY_US
//   clear old
//   track time?
//   track sim frame ids
//   interpolate?
//   simulate
//   delta from history ???

void SnapshotHistory::init(const Entity& e) {
    // TODO
    snapshots.push_back(e);
}

void SnapshotHistory::add(const Entity& e, int current_sim_frame_id, uint32_t curTime) {
    // TODO
    snapshots.push_back(e);
}

void SnapshotHistory::update_controls(float thr, float steer) {
    // TODO
    snapshots.back().thr = thr;
    snapshots.back().steer = steer;
}

Entity SnapshotHistory::get_current_entity() const {
    // TODO
    return snapshots.back();
}

void SnapshotHistory::set_time(uint32_t time) {
    // TODO
}
