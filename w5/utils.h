#pragma once


static const char* CLIENT_NAME = "w5 networked MIPT";
static const int CLIENT_WIDTH = 600;
static const int CLIENT_HEIGHT = 600;
static const int CLIENT_FPS = 60;
static const float CLIENT_INTERPOLATION_DELAY_US = 200000;

static const int SERVER_DELAY_PING_SIMULATION_US = 100000;
static const int SERVER_PORT = 10131;
static const char* SERVER_HOST = "localhost";

static const float SIMULATION_DT_S = 1.0 / 32.0;
