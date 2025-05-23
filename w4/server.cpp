#include <enet/enet.h>
#include <iostream>
#include "entity.h"
#include "protocol.h"
#include <stdlib.h>
#include <vector>
#include <math.h>
#include <map>

static std::vector<Entity> entities;
static std::map<uint16_t, ENetPeer*> controlledMap;
static std::map<uint16_t, float> controlledMapPoints;

float random_coor() {
    return (rand() % 200 - 100) * 4.f;
}

static uint16_t create_random_entity()
{
  uint16_t newEid = entities.size();
  uint32_t color = 0xff000000 +
                   0x00440000 * (1 + rand() % 4) +
                   0x00004400 * (1 + rand() % 4) +
                   0x00000044 * (1 + rand() % 4);
  float x = random_coor();
  float y = random_coor();
  float size = 10.0f * (1 + 0.001f * (rand() % 1000 - 499.5f));
  Entity ent = {color, x, y, newEid, false, 0.f, 0.f, size};
  entities.push_back(ent);
  return newEid;
}

void on_join(ENetPacket *packet, ENetPeer *peer, ENetHost *host)
{
  // send all entities
  for (const Entity &ent : entities)
    send_new_entity(peer, ent);
  // send all points
  for (const auto& [eid, point] : controlledMapPoints) {
    send_point(peer, eid, point);
  }

  // find max eid
  uint16_t newEid = create_random_entity();
  const Entity& ent = entities[newEid];

  controlledMap[newEid] = peer;
  controlledMapPoints[newEid] = 0;

  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i) {
    send_new_entity(&host->peers[i], ent);
    send_point(&host->peers[i], newEid, controlledMapPoints[newEid]);
  }
  // send info about controlled entity
  send_set_controlled_entity(peer, newEid);
}

void on_state(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f; float size = 0.f;
  deserialize_entity_state(packet, eid, x, y, size);
  for (Entity &e : entities)
    if (e.eid == eid)
    {
      e.x = x;
      e.y = y;
      e.size = size;
      break;
    }
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10131;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  constexpr int numAi = 10;

  for (int i = 0; i < numAi; ++i)
  {
    uint16_t eid = create_random_entity();
    entities[eid].serverControlled = true;
    controlledMap[eid] = nullptr;
    controlledMap[eid] = 0;
  }

  uint32_t lastTime = enet_time_get();
  while (true)
  {
    uint32_t curTime = enet_time_get();
    float dt = (curTime - lastTime) * 0.001f;
    lastTime = curTime;
    ENetEvent event;
    while (enet_host_service(server, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
          case E_CLIENT_TO_SERVER_JOIN:
            on_join(event.packet, event.peer, server);
            break;
          case E_CLIENT_TO_SERVER_STATE:
            on_state(event.packet);
            break;
          default:
            break;
        };
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
    for (Entity &e : entities)
    {
      if (e.serverControlled)
      {
        const float diffX = e.targetX - e.x;
        const float diffY = e.targetY - e.y;
        const float dirX = diffX > 0.f ? 1.f : -1.f;
        const float dirY = diffY > 0.f ? 1.f : -1.f;
        constexpr float spd = 50.f;
        e.x += dirX * spd * dt;
        e.y += dirY * spd * dt;
        if (fabsf(diffX) < 10.f && fabsf(diffY) < 10.f)
        {
          e.targetX = random_coor();
          e.targetY = random_coor();
        }
      }
    }
    for (Entity &e1 : entities)
    {
      for (Entity &e2 : entities)
      {
        if (e1.eid == e2.eid) continue;
        const float diffX = std::abs(e1.x - e2.x);
        const float diffY = std::abs(e1.y - e2.y);
        bool touch = (diffX * 2 < e1.size + e2.size) && (diffY * 2 < e1.size + e2.size);
        if (touch) {
          Entity* bigger; Entity* smaller;
          if (e1.size > e2.size) {
              bigger = &e1;
              smaller = &e2;
          } else {
              bigger = &e2;
              smaller = &e1;
          }
          std::cout << bigger->eid << " ate " << smaller->eid << std::endl;
          float dsize = smaller->size / 2;
          bigger->size = std::sqrt(bigger->size * bigger->size + dsize * dsize);
          smaller->size = std::sqrt(smaller->size * smaller->size - dsize * dsize);
          if (!bigger->serverControlled) {
            float dpoints = std::sqrt(dsize);
            controlledMapPoints[bigger->eid] += dpoints;
            std::cout << bigger->eid << " got " << dpoints << " points, total: " << controlledMapPoints[bigger->eid] << std::endl;
            for (size_t i = 0; i < server->peerCount; ++i)
            {
              ENetPeer *peer = &server->peers[i];
              send_point(peer, bigger->eid, controlledMapPoints[bigger->eid]);
            }
          }
          smaller->x = random_coor();
          smaller->y = random_coor();
        }
      }
    }
    for (const Entity &e : entities)
    {
      for (size_t i = 0; i < server->peerCount; ++i)
      {
        ENetPeer *peer = &server->peers[i];
        //if (controlledMap[e.eid] != peer)
          send_snapshot(peer, e.eid, e.x, e.y, e.size);
      }
    }
    //usleep(400000);
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}


