#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <string>
#include "socket_tools.h"


class Client {
protected:
    bool connected = false;

    addrinfo resAddrInfo;
    int sfd;

public:
    Client() {
        reset();
    }

    bool is_connected() const;

    bool connect(const std::string& server_name, int port);
    void start_listening();
    void start_reading();

    void reset();
};


bool Client::is_connected() const {
    return connected;
}

bool Client::connect(const std::string& server_name, int port) {
    // TODO
    const std::string port_str = std::to_string(port);
    const char* port_cstr = port_str.data();

    sfd = create_dgram_socket(server_name.data(), port_cstr, &resAddrInfo);

    if (sfd == -1) {
        std::cout << "Client::connect - Cannot create a socket" << std::endl;
        return false;
    }

    connected = true;
    return true;
}

void Client::start_listening() {
    // TODO
}

void Client::start_reading() {
    // TODO
    while (true)
    {
        std::string input;
        printf(">");
        std::getline(std::cin, input);
        ssize_t res = sendto(sfd, input.c_str(), input.size(), 0, resAddrInfo.ai_addr, resAddrInfo.ai_addrlen);
        if (res == -1)
          std::cout << strerror(errno) << std::endl;
    }
}

void Client::reset() {
    connected = false;
    resAddrInfo = {};
    sfd = -1;
}


int main(int argc, const char **argv)
{
    Client client;

    client.connect("localhost", 2025);
    if (!client.is_connected()) {
        std::cout << "main - Couldn't connect to the server" << std::endl;
        return 1;
    }

    client.start_listening();
    client.start_reading();

    while (client.is_connected()) {}
    return 0;
}
