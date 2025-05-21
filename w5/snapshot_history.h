#pragma once

#include <deque>

#include "entity.h"


class SnapshotHistory {
private:
    std::deque<Entity> snapshots;

public:
    SnapshotHistory() {}

    void init(const Entity& e);
    void add(const Entity& e, int current_sim_frame_id, uint32_t curTime);
    void update_controls(float thr, float steer);
    Entity get_current_entity() const;
    void set_time(uint32_t time);
};
