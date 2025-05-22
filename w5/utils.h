#pragma once

#include <cstdint>


// v
// Block for testing smoothing algorithms
// TODO
// TODO add different server delays, changing like sinus idk
// TODO add flags to disable smoothing or smth to show
// TODO write into readme what is done and how to play with
static bool DISABLE_SIMULATION_AND_INTERPOLATION = false;
static const uint32_t CLIENT_INTERPOLATION_DELAY_MS = 200;
static const int SERVER_DELAY_PING_SIMULATION_US = 100000;
// ^


static const uint32_t COLOR_R = 0xff000000;
static const uint32_t COLOR_G = 0x00ff0000;
static const uint32_t COLOR_B = 0x0000ff00;
static const uint32_t COLOR_A = 0x000000ff;

static const uint32_t COLOR_CLIENT_BG = COLOR_A + (COLOR_R + COLOR_G + COLOR_B) / 3;


static const char* CLIENT_NAME = "w5 networked MIPT";
static const int CLIENT_WIDTH = 600;
static const int CLIENT_HEIGHT = 600;
static const int CLIENT_FPS = 60;

static const int SERVER_PORT = 10131;
static const char* SERVER_HOST = "localhost";

static const float SIMULATION_DT_S = 1.0 / 32.0;
