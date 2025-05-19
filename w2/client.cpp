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
static const char* LOBBY_ADDR = "localhost";
static const int LOBBY_PORT = 10887;
static const int CLIENT_SERVICE_TIMEOUT_MS = 10;
static const std::string SYSCMD_START = "START";


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


struct NetworkAddress {
    uint32_t host;
    uint16_t port;

    NetworkAddress(uint32_t h = 0, uint16_t p = 0) : host(h), port(p) {}
};


class NetworkClient {
private:
    ENetHost* client_host = nullptr;
    ENetPeer* lobby_peer = nullptr;
    ENetPeer* server_peer = nullptr;

public:
    ENetPeer* get_lobby_peer() const { return lobby_peer; }

    ENetPeer* get_server_peer() const { return server_peer; }

    NetworkClient() {
        if (enet_initialize() != 0) {
            throw std::runtime_error("Cannot init ENet");
        }
        atexit(enet_deinitialize);

        client_host = enet_host_create(nullptr, 2, 3, 0, 0);
        if (!client_host) {
            throw std::runtime_error("Cannot create client host");
        }
    }

    ~NetworkClient() {
        if (client_host) {
            enet_host_destroy(client_host);
        }
    }

    bool connect_to_lobby(const char* address, int port) {
        ENetAddress enetAddr;
        enet_address_set_host(&enetAddr, address);
        enetAddr.port = port;

        lobby_peer = enet_host_connect(client_host, &enetAddr, 1, 0);
        if (!lobby_peer) {
            return false;
        }
        return true;
    }

    bool connect_to_server(const NetworkAddress& address) {
        ENetAddress enetAddr;
        enetAddr.host = address.host;
        enetAddr.port = address.port;

        server_peer = enet_host_connect(client_host, &enetAddr, 3, 0);
        if (!server_peer) {
            return false;
        }
        return true;
    }

    ENetEvent service(int timeout = CLIENT_SERVICE_TIMEOUT_MS) {
        ENetEvent event;
        enet_host_service(client_host, &event, timeout);
        return event;
    }

    void send(const std::string& msg, ENetPeer* peer, bool reliable) {
        if (!peer) return;

        ENetPacket* packet = enet_packet_create(msg.c_str(), msg.size() + 1, reliable ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNSEQUENCED);
        enet_peer_send(peer, 0, packet);
    }
};


class Game {
private:
    std::map<int, Player> players;
    Player me;
    NetworkClient network_client;
    bool _is_connected_lobby = false;
    bool _is_connected_server = false;

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

        // Connect to lobby
        if (!network_client.connect_to_lobby(LOBBY_ADDR, LOBBY_PORT)) {
            throw std::runtime_error("Cannot connect to lobby");
        }
    }

    bool is_connected_lobby() const { return _is_connected_lobby; }
    bool is_connected_server() const { return _is_connected_server; }

    ~Game() {
        CloseWindow();
    }

    void update(float dt) {
        // Handle input
        bool left = IsKeyDown(KEY_LEFT);
        bool right = IsKeyDown(KEY_RIGHT);
        bool up = IsKeyDown(KEY_UP);
        bool down = IsKeyDown(KEY_DOWN);
        bool enter = IsKeyDown(KEY_ENTER);

        // Calculate speed based on pressed buttons
        Vector2D speed;
        if (left) speed.x -= 1;
        if (right) speed.x += 1;
        if (up) speed.y -= 1;
        if (down) speed.y += 1;
        speed = speed.normalize() * PLAYER_SPEED;
        me.pos = me.pos + speed * dt;

        // If enter is pressed, and not yet connected to the game server - send request to lobby
        if (enter && !is_connected_server()) {
            network_client.send(SYSCMD_START, network_client.get_lobby_peer(), true);
        }

        // If connected to the server - try to pass my position
        // TODO

        // Also update ping
        // pinger.update(dt); // TODO
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

            ENetEvent event = network_client.service();
            handleNetworkEvent(event);

            update(dt);
            render();
        }
    }

    void handleNetworkEvent(const ENetEvent& event) {
        switch (event.type) {
            case ENET_EVENT_TYPE_NONE:
                break;

            case ENET_EVENT_TYPE_CONNECT:
                std::cout << "Connected: " << event.peer->address.host << ":" << event.peer->address.port << std::endl;
                if (!_is_connected_lobby) _is_connected_lobby = true;
                else _is_connected_server = true;
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << "Disconnected: " << event.peer->address.host << ":" << event.peer->address.port << std::endl;
                event.peer->data = nullptr;
                CloseWindow();
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                handlePacket(event);
                enet_packet_destroy(event.packet);
                break;

            default:
                std::cout << "WTF is that?" << std::endl;
                break;
        }
    }

    void handlePacket(const ENetEvent& event) {
        std::cout << "Got data from " << event.peer->address.host << ":" << event.peer->address.port << " - " << event.packet->data << std::endl;

        const char* msgData = reinterpret_cast<const char*>(event.packet->data);
        // TODO
    }
};


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

    return 0;
}
