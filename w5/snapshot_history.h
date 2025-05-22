#pragma once

#include <deque>

#include "entity.h"
#include "utils.h"


struct Snapshot {
    Entity e;
    int sim_id = 0;
    uint32_t curTime = 0;
};

class SnapshotHistory {
private:
    std::deque<Snapshot> snapshots;
    Entity current;
    float thr = 0.0;
    float steer = 0.0;

    void _sort();
    void _clear_old(uint32_t current_time);
    void _predict(uint32_t current_time);

public:
    SnapshotHistory() {}

    void init(const Entity& e);
    void add(const Entity& e, int current_sim_frame_id, uint32_t curTime);
    void update_controls(float thr, float steer);
    Entity get_current_entity() const;
    void set_time(uint32_t time);
};
