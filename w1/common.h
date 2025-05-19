#pragma once

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <string>


static const char* SERVER_ADDR = "localhost";
static const int SERVER_PORT = 2025;
static const int CLIENT_SLEEP_BETWEEN_RECEIVE = 0;
static const int CLIENT_RECEIVE_TIMEOUT_US = 1'000'000;
static const int SERVER_SLEEP_BETWEEN_RECEIVE = 0;
static const int SERVER_RECEIVE_TIMEOUT_US = 100'000;
static const int MESSAGE_BUFFER_SIZE = 1024;


struct ReadResult {
    std::string msg;
    sockaddr_in responder_sockaddr;
    bool is_error = false;
    bool is_empty = true;
};


class CommonCS {
protected:
    ssize_t _send(const std::string& msg, int sfd, sockaddr_in client_sockaddr) const;
    ReadResult _read(int sfd) const;
};

