#include <cstring>
#include <enet/enet.h>
#include <iostream>
#include <map>
#include <random>
#include <string>

#include "raylib.h"


static const int HEIGHT = 600;
static const int WIDTH = 800;
static const char* NAME = "w2 MIPT networked";
static const int FPS = 60;
static const float PLAYER_SPEED = 100;
static const float PLAYER_SIZE = 5;


struct Vector2D {
    float x;
    float y;

    Vector2D() : x(0), y(0) {}
    Vector2D(float x, float y) : x(x), y(y) {}
    Vector2D(std::pair<float, float> pos) : Vector2D(pos.first, pos.second) {}

    Vector2D operator*(float k) const { return { x * k, y * k }; }
    Vector2D operator/(float k) const { return *this * (1 / k); }
    Vector2D operator+(const Vector2D& oth) const { return { x + oth.x, y + oth.y }; }

    float norm() const { return x * x + y * y; }
    float length() const { return std::sqrt(norm()); }
    Vector2D normalize() const {
        if (x == 0 && y == 0) return Vector2D();
        return *this / length();
    }

    operator Vector2() const { return { x, y }; }
};

Vector2D operator*(float k, const Vector2D& vec) { return vec * k; }
Vector2D operator/(float k, const Vector2D& vec) { return vec / k; }


struct Player {
    // Constant data
    std::string name;
    int id = -1;
    // Pass between servers dynamically
    Vector2D pos;
    int ping = -1;

    Player(const std::string& name, int id = -1, Vector2D pos = {}, int ping = -1)
        : name(name), id(id), pos(pos), ping(ping) {}
};


class Game {
private:
    std::map<int, Player> players;
    Player me;

public:
    Game(int width, int height, const char* name, int fps, const std::string& player_name) : me(player_name) {
        // Init GUI
        InitWindow(width, height, name);

        // Resize if user has small monitor
        const int scrWidth = GetMonitorWidth(0);
        const int scrHeight = GetMonitorHeight(0);
        if (scrWidth < width || scrHeight < height) {
            width = std::min(scrWidth, width);
            height = std::min(scrHeight, height);
            SetWindowSize(width, height);
        }

        // Set FPS
        SetTargetFPS(fps);

        // Initialize player position
        me.pos.x = rand() % width;
        me.pos.y = rand() % height;
    }

    bool is_connected_lobby() const { return false; }
    bool is_connected_server() const { return false; }

    ~Game() {
        CloseWindow();
    }

    void update(float dt) {
        // Handle input
        bool left = IsKeyDown(KEY_LEFT);
        bool right = IsKeyDown(KEY_RIGHT);
        bool up = IsKeyDown(KEY_UP);
        bool down = IsKeyDown(KEY_DOWN);

        // Calculate speed based on pressed buttons
        Vector2D speed;
        if (left) speed.x -= 1;
        if (right) speed.x += 1;
        if (up) speed.y -= 1;
        if (down) speed.y += 1;
        speed = speed.normalize() * PLAYER_SPEED;
        me.pos = me.pos + speed * dt;
    }

    void render_player(const Player& player, int x, int y) const {
        DrawText(("ID: " + std::to_string(me.id) + " " + me.name + " ping: " + std::to_string(me.ping)).c_str(), x, y, 20, WHITE);
        DrawCircleV(player.pos, PLAYER_SIZE, WHITE);
    }

    void render() const {
        BeginDrawing();
        {
            // Clear
            ClearBackground(BLACK);

            // Current status
            std::string status = "ERROR";
            if (is_connected_lobby()) {
                status = "lobby";
            }
            if (is_connected_server()) {
                status = "in-game";
            }
            DrawText(("Current status: " + status).c_str(), 20, 20, 20, WHITE);

            // My info
            render_player(me, 20, 60);

            // List all players
            DrawText("List of players:", 20, 100, 20, WHITE);
            int i = 0;
            for (const auto& player_it : players) {
                render_player(player_it.second, 40, 140 + 20 * i);
                ++i;
            }
        }
        EndDrawing();
    }

    void run() {
        while (!WindowShouldClose()) {
            const float dt = GetFrameTime();

            update(dt);
            render();
        }
    }
};


/*void send_fragmented_packet(ENetPeer *peer)
{
  const char *baseMsg = "Stay awhile and listen. ";
  const size_t msgLen = strlen(baseMsg);

  const size_t sendSize = 2500;
  char *hugeMessage = new char[sendSize];
  for (size_t i = 0; i < sendSize; ++i)
    hugeMessage[i] = baseMsg[i % msgLen];
  hugeMessage[sendSize-1] = '\0';

  ENetPacket *packet = enet_packet_create(hugeMessage, sendSize, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);

  delete[] hugeMessage;
}

void send_micro_packet(ENetPeer *peer)
{
  const char *msg = "dv/dt";
  ENetPacket *packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_UNSEQUENCED);
  enet_peer_send(peer, 1, packet);
}*/

int main(int argc, const char **argv)
{
    std::string player_name;
    if (argc == 2) {
        player_name = argv[1];
    } else {
        std::cout << "You can pass argument - your name - to this script. Since you didn't do that, name will be chosen randomly for you" << std::endl;
        player_name = "Player" + std::to_string(rand());
    }
    std::cout << "Starting game as " << player_name << std::endl;
    Game game(WIDTH, HEIGHT, NAME, FPS, player_name);
    game.run();

  /*if (enet_initialize() != 0)
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
  enet_address_set_host(&address, "localhost");
  address.port = 10887;

  ENetPeer *lobbyPeer = enet_host_connect(client, &address, 2, 0);
  if (!lobbyPeer)
  {
    printf("Cannot connect to lobby");
    return 1;
  }

  uint32_t timeStart = enet_time_get();
  uint32_t lastFragmentedSendTime = timeStart;
  uint32_t lastMicroSendTime = timeStart;
  bool connected = false;
  float posx = GetRandomValue(100, 1000);
  float posy = GetRandomValue(100, 500);
  float velx = 0.f;
  float vely = 0.f;
  while (!WindowShouldClose())
  {
    const float dt = GetFrameTime();
    ENetEvent event;
    while (enet_host_service(client, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        connected = true;
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        printf("Packet received '%s'\n", event.packet->data);
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
    if (connected)
    {
      uint32_t curTime = enet_time_get();
      if (curTime - lastFragmentedSendTime > 1000)
      {
        lastFragmentedSendTime = curTime;
        send_fragmented_packet(lobbyPeer);
      }
      if (curTime - lastMicroSendTime > 100)
      {
        lastMicroSendTime = curTime;
        send_micro_packet(lobbyPeer);
      }
    }
    bool left = IsKeyDown(KEY_LEFT);
    bool right = IsKeyDown(KEY_RIGHT);
    bool up = IsKeyDown(KEY_UP);
    bool down = IsKeyDown(KEY_DOWN);
    constexpr float accel = 30.f;
    velx += ((left ? -1.f : 0.f) + (right ? 1.f : 0.f)) * dt * accel;
    vely += ((up ? -1.f : 0.f) + (down ? 1.f : 0.f)) * dt * accel;
    posx += velx * dt;
    posy += vely * dt;
    velx *= 0.99f;
    vely *= 0.99f;

    BeginDrawing();
      ClearBackground(BLACK);
      DrawText(TextFormat("Current status: %s", "unknown"), 20, 20, 20, WHITE);
      DrawText(TextFormat("My position: (%d, %d)", (int)posx, (int)posy), 20, 40, 20, WHITE);
      DrawText("List of players:", 20, 60, 20, WHITE);
      DrawCircleV(Vector2{posx, posy}, 10.f, WHITE);
    EndDrawing();
  }*/
    return 0;
}
