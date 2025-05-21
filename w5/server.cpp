#include <enet/enet.h>
#include <iostream>
#include "entity.h"
#include "protocol.h"
#include "mathUtils.h"
#include <stdlib.h>
#include <vector>
#include <map>

#include "utils.h"

static std::vector<Entity> entities;
static std::map<uint16_t, ENetPeer*> controlledMap;

uint32_t curTime = 0;

void on_join(ENetPacket *packet, ENetPeer *peer, ENetHost *host)
{
  // send all entities
  for (const Entity &ent : entities)
    send_new_entity(peer, ent);

  // find max eid
  uint16_t maxEid = entities.empty() ? invalid_entity : entities[0].eid;
  for (const Entity &e : entities)
    maxEid = std::max(maxEid, e.eid);
  uint16_t newEid = maxEid + 1;
  uint32_t color = COLOR_A +
                   COLOR_R * (rand() % 4 + 1) +
                   COLOR_G * (rand() % 4 + 1) +
                   COLOR_B * (rand() % 4 + 1);
  float x = (rand() % 4) * 5.f;
  float y = (rand() % 4) * 5.f;
  Entity ent = { .color=color, .x=x, .y=y, .vx=0.f, .vy=0.f, .ori=(rand() * 1.0f / RAND_MAX) * 3.141592654f, .omega=0.f, .thr=0.f, .steer=0.f, .eid=newEid};
  entities.push_back(ent);

  controlledMap[newEid] = peer;


  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i)
    send_new_entity(&host->peers[i], ent);
  // send info about controlled entity
  send_set_controlled_entity(peer, newEid);
}

void on_input(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float thr = 0.f; float steer = 0.f;
  deserialize_entity_input(packet, eid, thr, steer);
  for (Entity &e : entities)
    if (e.eid == eid)
    {
      e.thr = thr;
      e.steer = steer;
      break;
    }
}

static void update_net(ENetHost* server)
{
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
        case E_CLIENT_TO_SERVER_INPUT:
          on_input(event.packet);
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
}

static std::pair<int, float> simulate_world(ENetHost* server, int current_sim_frame_id, float dt)
{
    while (dt >= SIMULATION_DT_S) {
        ++current_sim_frame_id;
        dt -= SIMULATION_DT_S;
        for (Entity &e : entities)
        {
            // simulate
            simulate_entity(e, SIMULATION_DT_S);
        }
    }
    // Only after we've simulated all the missing frames - send the current frame
    for (Entity &e : entities)
    {
        // send
        for (size_t i = 0; i < server->peerCount; ++i)
        {
            ENetPeer *peer = &server->peers[i];
            send_snapshot(peer, e, current_sim_frame_id, curTime);
        }
    }
    return { current_sim_frame_id, dt };
}

static void update_time(ENetHost* server, uint32_t curTime)
{
  // We can send it less often too
  for (size_t i = 0; i < server->peerCount; ++i)
    send_time_msec(&server->peers[i], curTime);
}

int main(int argc, const char **argv)
{
  std::srand(0);
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;
  address.host = ENET_HOST_ANY;
  address.port = SERVER_PORT;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  uint32_t lastTime = 0;
  float unused_dt = 0; // in case we dont even need to simulate game yet
  int sim_frame_id = 0;
  enet_time_set(0);
  while (true)
  {
    curTime = enet_time_get();
    float dt = (curTime - lastTime) * 0.001f + unused_dt;
    lastTime = curTime;

    update_net(server);
    auto [dsim_frame_id, ddt] = simulate_world(server, sim_frame_id, dt);
    update_time(server, curTime);

    sim_frame_id = dsim_frame_id;
    unused_dt = ddt;

    printf("%d\n", curTime);

    usleep(SERVER_DELAY_PING_SIMULATION_US);
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}


