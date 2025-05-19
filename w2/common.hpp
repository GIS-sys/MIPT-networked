#pragma once

#include <enet/enet.h>
#include <iostream>
#include <math.h>
#include <string>
#include <vector>


// client specific
static const char* NAME = "w2 MIPT networked";
static const int HEIGHT = 600;
static const int WIDTH = 800;
static const int FPS = 60;
static const float PLAYER_SPEED = 100;
static const float PLAYER_SIZE = 5;
static const int CLIENT_SERVICE_TIMEOUT_MS = 10;
static const int CLIENT_PING_INTERVAL_MS = 1000;
static const float CLIENT_SEND_DATA_INTERVAL_MS = 10;


// common
static const std::string SYSCMD_START = "START";
static const int CHANNEL_LOBBY_START = 0;
static const int CHANNEL_SERVER_PING = 1;
static const int CHANNEL_SERVER_PLAYERS_LIST = 2;
static const int CHANNEL_SERVER_PLAYER_CRED = 3;
static const int CHANNEL_SERVER_PLAYERS_DATA = 4;
static const int CHANNELS_AMOUNT = 5;
static const std::string LOBBY_ADDR = "localhost";
static const int LOBBY_PORT = 10887;
static const std::string NETWORK_MSG_DIVIDER = "|";
static const bool DEBUG = false;  // for more stdout


// non-client
static const std::string SERVER_ADDR = "localhost";
static const int SERVER_PORT = 10888;
static const int MAX_PLAYERS = 64;
static const int LOBBY_SERVICE_TIMEOUT_MS = 100;
static const int SERVER_SERVICE_TIMEOUT_MS = 10;
static const float SERVER_SEND_DATA_INTERVAL_MS = 10;


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
};

Vector2D operator*(float k, const Vector2D& vec) { return vec * k; }
Vector2D operator/(float k, const Vector2D& vec) { return vec / k; }


struct PlayerUseData {
    bool name = false;
    bool id = false;
    bool pos = false;
    bool ping = false;

    int length() const { return name * 1 + id * 1 + pos * 2 + ping * 1; }
};

struct Player {
    ENetPeer* peer;

    // Constant data
    std::string name;
    int id = -1;
    // Pass between servers dynamically
    Vector2D pos;
    int ping = -1;

    Player(ENetPeer* peer = nullptr, const std::string& name = "", int id = -1, Vector2D pos = {}, int ping = -1)
        : peer(peer), name(name), id(id), pos(pos), ping(ping) {}



    std::vector<std::string> to_string_vector(PlayerUseData use = {}) const {
        std::vector<std::string> res;
        if (use.name) res.push_back(name);
        if (use.id) res.push_back(std::to_string(id));
        if (use.pos) res.push_back(std::to_string(pos.x));
        if (use.pos) res.push_back(std::to_string(pos.y));
        if (use.ping) res.push_back(std::to_string(ping));
        return res;
    }

    static Player from_string_vector(const std::vector<std::string>& strings, PlayerUseData use = {}, int start_i = 0) {
        Player player;
        int i = start_i;
        if (use.name) player.name = strings[i++];
        if (use.id) player.id = std::atoi(strings[i++].c_str());
        if (use.pos) player.pos.x = std::atoi(strings[i++].c_str());
        if (use.pos) player.pos.y = std::atoi(strings[i++].c_str());
        if (use.ping) player.ping = std::atoi(strings[i++].c_str());
        return player;
    }

    void generate_random_credentials() {
        id = rand();
        name = "Player" + std::to_string(id);
    }
};


std::string prepare_for_send(const std::vector<std::string>& strings) {
    std::string msg;
    for (const std::string& string : strings) {
        msg += string + NETWORK_MSG_DIVIDER;
    }
    return msg;
}


 std::vector<std::string> parse_from_receive(std::string msg) {
    std::vector<std::string> strings;
    while (!msg.empty() && msg.find(NETWORK_MSG_DIVIDER) != -1) {
        int i = msg.find(NETWORK_MSG_DIVIDER);
        std::string string = msg.substr(0, i);
        msg = msg.substr(i + 1);
        strings.push_back(string);
    }
    return strings;
}

void send(const std::string& msg, ENetPeer* peer, int channel, bool reliable) {
    if (!peer) return;
    if (DEBUG) std::cout << "Sending data " << msg << std::endl;
    ENetPacket* packet = enet_packet_create(msg.c_str(), msg.size() + 1, reliable ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNSEQUENCED);
    enet_peer_send(peer, channel, packet);
}
