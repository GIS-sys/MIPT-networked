#include "snapshot_history.h"

#include <algorithm>
#include <iostream>
#include "entity.h"
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
    // TODO
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
    std::sort(snapshots.begin(), snapshots.end(), [](const auto& x, const auto& y) { return x.sim_id < y.sim_id; });
}

void SnapshotHistory::_clear_old(uint32_t current_time) {
    // Delete all old ones, except for the most recent
    Snapshot last = snapshots.front();
    snapshots.pop_front();
    while (!snapshots.empty() && snapshots.front().curTime < current_time) {
        last = snapshots.front();
        snapshots.pop_front();
    }
    snapshots.push_front(last);
    std::cout << "After deleting snapshot length is: " << snapshots.size() << std::endl;
}

void SnapshotHistory::_interpolate(uint32_t current_time) {
    std::cout << "Snapshot is interpolating (server has sent more than enough data)" << std::endl;
    float dt1 = (current_time - snapshots[0].curTime) * 0.001;
    float dt2 = (snapshots[1].curTime - current_time) * 0.001;
    current = interpolate_entity(snapshots[0].e, snapshots[1].e, dt1, dt2);
    std::cout << snapshots[0].curTime << " < " << current_time << " > " << snapshots[1].curTime << std::endl;
    std::cout << dt1 << " - " << dt2 << std::endl;
    std::cout << snapshots[0].e.x << " - " << snapshots[1].e.x << " = " << current.x << std::endl;
    std::cout << snapshots[0].e.y << " - " << snapshots[1].e.y << " = " << current.y << std::endl;
    std::cout << snapshots[0].e.ori << " - " << snapshots[1].e.ori << " = " << current.ori << std::endl;
    // TODO
}

void SnapshotHistory::_simulate(uint32_t current_time) {
    std::cout << "Snapshot is simulating (server has not sent new data yet)" << std::endl;
    current = snapshots.back().e;
    float dt = ((int)current_time - (int)snapshots.back().curTime) * 0.001;
    simulate_entity(current, dt);
}

void SnapshotHistory::_predict(uint32_t current_time) {
    // TODO add different server delays, changing like sinus idk
    if (snapshots.size() >= 2) _interpolate(current_time);
    else _simulate(current_time);
}

void SnapshotHistory::set_time(uint32_t time) {
    uint32_t current_time = time - CLIENT_INTERPOLATION_DELAY_MS;
    // current_time += CLIENT_INTERPOLATION_DELAY_MS; // TODO delete
    _sort();
    _clear_old(current_time);
    _predict(current_time);
}

// TODO add flags to disable smoothing or smth to show
// TODO write into readme what is done and how to play with
