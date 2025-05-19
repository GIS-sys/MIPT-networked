#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "socket_tools.h"


class Client {
protected:
    bool connected = false;

    sockaddr_in serverSockAddr;
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
    std::string port_str = std::to_string(port);
    const char* port_cstr = port_str.c_str();

    // Resolve server address
    addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(server_name.c_str(), port_cstr, &hints, &res) != 0) {
        printf("Failed to resolve server address\n");
        return false;
    }

    memcpy(&serverSockAddr, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);

    // Create client socket
    sfd = create_dgram_socket(nullptr, "0", nullptr); // Bind to any available port
    if (sfd == -1) {
        printf("Failed to create client socket\n");
        return false;
    }
    connected = true;
    return true;
}

void Client::start_listening() {
    // TODO
}

void Client::start_reading() {
    const char *pingMsg = "ping";
    printf("Sending 'ping' to server...\n");
    sendto(sfd, pingMsg, strlen(pingMsg), 0, (sockaddr*)&serverSockAddr, sizeof(serverSockAddr));

    while (true) {
        fd_set readFds;
        FD_ZERO(&readFds);
        FD_SET(sfd, &readFds);

        timeval tv = {1, 0}; // 1 second timeout
        int ready = select(sfd + 1, &readFds, nullptr, nullptr, &tv);

        if (ready < 0) {
            perror("select error");
            break;
        } else if (ready == 0) {
            // Timeout: resend ping
            printf("No response, resending 'ping'...\n");
            sendto(sfd, pingMsg, strlen(pingMsg), 0, (sockaddr*)&serverSockAddr, sizeof(serverSockAddr));
        } else {
            char buffer[1024];
            sockaddr_in responderAddr;
            socklen_t addrLen = sizeof(responderAddr);
            ssize_t numBytes = recvfrom(sfd, buffer, sizeof(buffer), 0, (sockaddr*)&responderAddr, &addrLen);

            if (numBytes > 0) {
                buffer[numBytes] = '\0';
                printf("Received '%s' from server. Sending 'ping' again...\n", buffer);
                sleep(1);
                sendto(sfd, pingMsg, strlen(pingMsg), 0, (sockaddr*)&responderAddr, addrLen);
            }
        }
    }
}

void Client::reset() {
    connected = false;
    serverSockAddr = {};
    if (sfd != -1) close(sfd);
    sfd = -1;
}


int main() {
    Client client;

    client.connect("localhost", 2025);
    if (!client.is_connected()) {
        std::cout << "main - Couldn't connect to the server" << std::endl;
        return 1;
    }

    client.start_listening();
    client.start_reading();

    while (client.is_connected()) {}

    client.reset();

    return 0;
}
