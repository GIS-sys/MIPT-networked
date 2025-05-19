#include <cstring>
#include <enet/enet.h>
#include <iostream>
#include <map>
#include <random>
#include <string>

#include "raylib.h"
#include "common.hpp"
Vector2 to_vector2(const Vector2D& vec) {
    return { vec.x, vec.y };
}


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

        client_host = enet_host_create(nullptr, 2, CHANNELS_AMOUNT, 0, 0);
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

        lobby_peer = enet_host_connect(client_host, &enetAddr, CHANNELS_AMOUNT, 0);
        if (!lobby_peer) {
            return false;
        }
        return true;
    }

    bool connect_to_server(const char* address, int port) {
        ENetAddress enetAddr;
        enet_address_set_host(&enetAddr, address);
        enetAddr.port = port;

        server_peer = enet_host_connect(client_host, &enetAddr, CHANNELS_AMOUNT, 0);
        if (!lobby_peer) {
            return false;
        }
        return true;
    }

    ENetEvent service(int timeout = CLIENT_SERVICE_TIMEOUT_MS) {
        ENetEvent event;
        enet_host_service(client_host, &event, timeout);
        return event;
    }
};



class Pinger {
private:
    float since_last_sent = 0;
    float _ping = 0;
public:
    void update(float dt) {
        since_last_sent += dt;
    }

    float ping() const { return _ping; }

    bool need_ping() {
        return since_last_sent * 1000 > CLIENT_PING_INTERVAL_MS;
    }

    void sent() {
        since_last_sent = 0;
    }

    void got() {
        _ping = since_last_sent / 2;
    }
};


class Game {
private:
    std::map<int, Player> players;
    Player me;
    Pinger pinger;
    NetworkClient network_client;
    bool _is_connected_lobby = false;
    bool _is_connected_server = false;

public:
    Game(int width, int height, const char* name, int fps) {
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
        if (!network_client.connect_to_lobby(LOBBY_ADDR.c_str(), LOBBY_PORT)) {
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
            send(SYSCMD_START, network_client.get_lobby_peer(), CHANNEL_LOBBY_START, true);
        }

        // If connected to the server - try to pass my position
        if (is_connected_server()) {
            send(prepare_for_send(me.to_string_vector({ .pos = true, .ping = true })), network_client.get_server_peer(), CHANNEL_SERVER_PLAYERS_DATA, true);
        }

        // Also update ping
        pinger.update(dt);
        if (is_connected_server() && pinger.need_ping()) {
            send("ping", network_client.get_server_peer(), CHANNEL_SERVER_PING, false);
            pinger.sent();
        }
    }

    void render_player(const Player& player, int x, int y) const {
        DrawText(("ID: " + std::to_string(me.id) + " Name: " + me.name + " Ping: " + std::to_string(me.ping)).c_str(), x, y, 20, WHITE);
        DrawCircleV(to_vector2(player.pos), PLAYER_SIZE, WHITE);
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

        // Decide what to do depending on the channel
        if (event.channelID == CHANNEL_LOBBY_START) {
            if (is_connected_server()) return;
            // Parse lobby data to connect to server
            std::string msg(msgData);
            std::vector<std::string> parsed = parse_from_receive(msg);
            std::string server_addr = parsed[0];
            int server_port = std::atoi(parsed[1].c_str());
            std::cout << "Connecting to server: " << server_addr << ":" << server_port << std::endl;
            network_client.connect_to_server(server_addr.c_str(), server_port);
            return;
        }
        if (event.channelID == CHANNEL_SERVER_PLAYER_CRED) {
            // Parse player data
            std::string msg(msgData);
            std::vector<std::string> parsed = parse_from_receive(msg);
            Player player_data = Player::from_string_vector(parsed, { .name = true, .id = true });
            me.id = player_data.id;
            me.name = player_data.name;
            std::cout << "Got new credentials: name=" << me.name << " id=" << me.id << std::endl;
            return;
        }
        if (event.channelID == CHANNEL_SERVER_PING) {
            pinger.got();
            me.ping = pinger.ping();
            return;
        }
        if (event.channelID == CHANNEL_SERVER_PLAYERS_LIST) {
            std::string msg(msgData);
            std::vector<std::string> parsed = parse_from_receive(msg);
            PlayerUseData use{ .name = true, .id = true };
            for (int i = 0; i < parsed.size(); i += use.length()) {
                Player player = Player::from_string_vector(parsed, use);
                players[player.id] = player;
            }
            return;
        }
        if (event.channelID == CHANNEL_SERVER_PLAYERS_DATA) {
            std::string msg(msgData);
            std::vector<std::string> parsed = parse_from_receive(msg);
            PlayerUseData use{ .name = true, .id = true, .pos = true, .ping = true };
            for (int i = 0; i < parsed.size(); i += use.length()) {
                Player player = Player::from_string_vector(parsed, use);
                if (players.find(player.id) != players.end()) {
                    players[player.id].pos = player.pos;
                    players[player.id].ping = player.ping;
                }
            }
            return;
        }
    }
};


int main(int argc, const char **argv)
{
    Game game(WIDTH, HEIGHT, NAME, FPS);
    game.run();

    return 0;
}
