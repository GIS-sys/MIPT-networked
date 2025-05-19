#pragma once

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
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

    static char* calc_responder_addr(const sockaddr_in& some_sockaddr) { return inet_ntoa(some_sockaddr.sin_addr); };
    static int calc_responder_port(const sockaddr_in& some_sockaddr) { return ntohs(some_sockaddr.sin_port); };

    char* get_responder_addr() const { return calc_responder_addr(responder_sockaddr); };
    int get_responder_port() const { return calc_responder_port(responder_sockaddr); };
};


class CommonCS {
protected:
    ssize_t _send(const std::string& msg, int sfd, sockaddr_in client_sockaddr) const;
    ReadResult _read(int sfd) const;
};

