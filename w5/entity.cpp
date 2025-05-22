#include "entity.h"
#include "mathUtils.h"

constexpr float worldSize = 30.f;

float tile_val(float val, float border)
{
  if (val < -border)
    return val + 2.f * border;
  else if (val > border)
    return val - 2.f * border;
  return val;
}

void simulate_entity(Entity &e, float dt)
{
  bool isBraking = sign(e.thr) < 0.f;
  float accel = isBraking ? 6.f : 1.5f;
  float va = clamp(e.thr, -0.3, 1.f) * accel;
  e.vx += cosf(e.ori) * va * dt;
  e.vy += sinf(e.ori) * va * dt;
  e.omega += e.steer * dt * 0.3f;
  e.ori += e.omega * dt;
  e.x += e.vx * dt;
  e.y += e.vy * dt;

  e.x = tile_val(e.x, worldSize);
  e.y = tile_val(e.y, worldSize);
}

Entity interpolate_entity(const Entity &e1, const Entity &e2, float dt1, float dt2)
{
    Entity e;
    e.x = (e1.x * dt1 + e2.x * dt2) / (dt1 + dt2);
    e.y = (e1.y * dt1 + e2.y * dt2) / (dt1 + dt2);
    e.vx = (e1.vx * dt1 + e2.vx * dt2) / (dt1 + dt2);
    e.vy = (e1.vy * dt1 + e2.vy * dt2) / (dt1 + dt2);
    e.ori = (e1.ori * dt1 + e2.ori * dt2) / (dt1 + dt2);
    e.omega = (e1.omega * dt1 + e2.omega * dt2) / (dt1 + dt2);
    return e;
}

