#pragma once

#include <string>


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
static const char* LOBBY_ADDR = "localhost";
static const int LOBBY_PORT = 10887;


// non-client
static const char* SERVER_ADDR = "localhost";
static const int SERVER_PORT = 10888;
static const int MAX_PLAYERS = 64;
static const int LOBBY_SERVICE_TIMEOUT_MS = 100;
