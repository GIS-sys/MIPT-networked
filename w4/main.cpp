#include <functional>
#include <algorithm> // min/max
#include "raylib.h"
#include <enet/enet.h>
#include <stdio.h>
#include <iostream>

#include <string>
#include <vector>
#include "entity.h"
#include "protocol.h"


static std::vector<Entity> entities;
static std::unordered_map<uint16_t, size_t> indexMap;
static uint16_t my_entity = invalid_entity;

void on_new_entity_packet(ENetPacket *packet)
{
  Entity newEntity;
  deserialize_new_entity(packet, newEntity);
  auto itf = indexMap.find(newEntity.eid);
  if (itf != indexMap.end())
    return; // don't need to do anything, we already have entity
  indexMap[newEntity.eid] = entities.size();
  entities.push_back(newEntity);
}

void on_set_controlled_entity(ENetPacket *packet)
{
  deserialize_set_controlled_entity(packet, my_entity);
}

template<typename Callable>
static void get_entity(uint16_t eid, Callable c)
{
  auto itf = indexMap.find(eid);
  if (itf != indexMap.end())
    c(entities[itf->second]);
}

void on_snapshot(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f; float size = 0.f;
  deserialize_snapshot(packet, eid, x, y, size);
  get_entity(eid, [&](Entity& e)
  {
    e.x = x;
    e.y = y;
    e.size = size;
  });
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 1, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  enet_address_set_host(&address, "127.0.0.1");
  address.port = 10131;

  ENetPeer *serverPeer = enet_host_connect(client, &address, 2, 0);
  if (!serverPeer)
  {
    printf("Cannot connect to server");
    return 1;
  }

  int width = 1200;
  int height = 1000;
  InitWindow(width, height, "w4 networked MIPT");

  const int scrWidth = GetMonitorWidth(0);
  const int scrHeight = GetMonitorHeight(0);
  if (scrWidth < width || scrHeight < height)
  {
    width = std::min(scrWidth, width);
    height = std::min(scrHeight - 150, height);
    SetWindowSize(width, height);
  }

  Camera2D camera = { {0, 0}, {0, 0}, 0.f, 1.f };
  camera.target = Vector2{ 0.f, 0.f };
  camera.offset = Vector2{ width * 0.5f, height * 0.5f };
  camera.rotation = 0.f;
  camera.zoom = 1.f;

  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

  bool connected = false;
  while (!WindowShouldClose())
  {
    float dt = GetFrameTime();
    ENetEvent event;
    while (enet_host_service(client, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        send_join(serverPeer);
        connected = true;
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
        case E_SERVER_TO_CLIENT_NEW_ENTITY:
          on_new_entity_packet(event.packet);
          printf("new it\n");
          break;
        case E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY:
          on_set_controlled_entity(event.packet);
          printf("got it\n");
          break;
        case E_SERVER_TO_CLIENT_SNAPSHOT:
          on_snapshot(event.packet);
          break;
        };
        break;
      default:
        break;
      };
    }
    if (my_entity != invalid_entity)
    {
      bool left = IsKeyDown(KEY_LEFT);
      bool right = IsKeyDown(KEY_RIGHT);
      bool up = IsKeyDown(KEY_UP);
      bool down = IsKeyDown(KEY_DOWN);
      get_entity(my_entity, [&](Entity& e)
      {
        // Update
        e.x += ((left ? -dt : 0.f) + (right ? +dt : 0.f)) * 100.f;
        e.y += ((up ? -dt : 0.f) + (down ? +dt : 0.f)) * 100.f;

        // Send
        send_entity_state(serverPeer, my_entity, e.x, e.y, e.size);
        camera.target.x = e.x;
        camera.target.y = e.y;
      });
    }


    BeginDrawing();
      ClearBackground(Color{40, 40, 40, 255});
      BeginMode2D(camera);
        for (const Entity &e : entities)
        {
          const Rectangle rect = {e.x - e.size / 2, e.y - e.size / 2, e.size, e.size};
          DrawRectangleRec(rect, GetColor(e.color));
          DrawText(std::to_string(e.eid).c_str(), e.x + e.size / 2, e.y - e.size, 4.0, GetColor(e.color));
          DrawText(std::to_string(e.size).c_str(), e.x + e.size / 2, e.y + e.size / 2, 4.0, GetColor(e.color));
          if (e.eid == my_entity) {
            for (const Entity &e2 : entities)
            {
              if (e.eid == e2.eid) continue;
              const float diffX = std::abs(e.x - e2.x);
              const float diffY = std::abs(e.y - e2.y);
              float almost_touch = std::max(diffX, diffY) * 2 / (e.size + e2.size);
              if (almost_touch < 10) {
                if (e.size > e2.size)
                  DrawLine(e.x, e.y, e2.x, e2.y, GREEN);
                else
                  DrawLine(e.x, e.y, e2.x, e2.y, RED);
              }
            }
          }
        }

      EndMode2D();
    EndDrawing();
  }

  CloseWindow();

  return 0;
}
