#pragma once

#include <string>
#include <arpa/inet.h>


static const char* SERVER_ADDR = "localhost";
static const int SERVER_PORT = 2025;
static const int CLIENT_SLEEP_BETWEEN_RECEIVE = 0;
static const int CLIENT_RECEIVE_TIMEOUT_US = 1'000'000;
static const int SERVER_SLEEP_BETWEEN_RECEIVE = 0;
static const int SERVER_RECEIVE_TIMEOUT_US = 100'000;
static const int MESSAGE_BUFFER_SIZE = 1024;


struct addrinfo;


struct ReadResult {
    std::string msg;
    sockaddr_in responder_sockaddr;
    bool is_error = false;
    bool is_empty = true;
};


int create_dgram_socket(const char *address, const char *port, addrinfo *res_addr);

