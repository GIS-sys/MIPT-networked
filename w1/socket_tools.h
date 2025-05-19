#pragma once

#include <string>


static const char* SERVER_ADDR = "localhost";
static const int SERVER_PORT = 2025;
static const int CLIENT_SLEEP_BETWEEN_RECEIVE = 1;
static const int CLIENT_RECEIVE_TIMEOUT = 1;
static const int MESSAGE_BUFFER_SIZE = 1024;


struct addrinfo;


struct ReadResult {
    std::string msg;
    bool is_error = false;
    bool is_empty = true;
};


int create_dgram_socket(const char *address, const char *port, addrinfo *res_addr);

