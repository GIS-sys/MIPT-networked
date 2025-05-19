#pragma once

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


// common
static const std::string SYSCMD_START = "START";
static const int CHANNEL_LOBBY_START = 0;
static const int CHANNEL_SERVER_PING = 1;
static const int CHANNEL_SERVER_PLAYER_LIST = 2;
static const int CHANNEL_SERVER_PLAYER_CRED = 3;
static const int CHANNEL_SERVER_PLAYER_DATA = 4;
static const std::string LOBBY_ADDR = "localhost";
static const int LOBBY_PORT = 10887;

static const std::string NETWORK_MSG_DIVIDER = "|";


// non-client
static const std::string SERVER_ADDR = "localhost";
static const int SERVER_PORT = 10888;
static const int MAX_PLAYERS = 64;
static const int LOBBY_SERVICE_TIMEOUT_MS = 100;
static const int SERVER_SERVICE_TIMEOUT_MS = 10;


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
