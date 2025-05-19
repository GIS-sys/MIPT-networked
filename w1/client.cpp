#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <random>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "socket_tools.h"


class Client {
protected:
    sockaddr_in serverSockAddr;
    int sfd = -1;

    bool connected = false;
    std::string id;

public:
    Client() {
        id = std::to_string(std::rand());
        reset();
    }

    bool is_connected() const {
        return connected;
    }

    bool connect(const std::string& server_name, int port);
    void start_listening();
    void start_reading();

    void reset() {
        connected = false;
        serverSockAddr = {};
        if (sfd != -1) close(sfd);
        sfd = -1;
    }
};

bool Client::connect(const std::string& server_name, int port) {
    // Convert port to string
    std::string port_str = std::to_string(port);
    const char* port_cstr = port_str.c_str();

    // Server parameters
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    // Server resolved
    addrinfo* res;
    if (getaddrinfo(server_name.c_str(), port_cstr, &hints, &res) != 0) {
        std::cout << "Client::connect - Failed to resolve server address" << std::endl;
        return false;
    }

    // Save server address
    memcpy(&serverSockAddr, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);

    // Create client socket
    sfd = create_dgram_socket(nullptr, "0", nullptr); // Bind to any available port
    if (sfd == -1) {
        std::cout << "Client::connect - Failed to create client socket" << std::endl;
        return false;
    }

    // Set myself as connected
    connected = true;
    return true;
}

void Client::start_listening() {
    // TODO
}

void Client::start_reading() {
    std::string pingMsgStr = "ping";
    pingMsgStr += id;
    const char *pingMsg = pingMsgStr.c_str();
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
            std::cout << pingMsgStr << std::endl;
            sendto(sfd, pingMsg, strlen(pingMsg), 0, (sockaddr*)&serverSockAddr, sizeof(serverSockAddr));
        } else {
            char buffer[1024];
            sockaddr_in responderAddr;
            socklen_t addrLen = sizeof(responderAddr);
            ssize_t numBytes = recvfrom(sfd, buffer, sizeof(buffer), 0, (sockaddr*)&responderAddr, &addrLen);

            if (numBytes > 0) {
                buffer[numBytes] = '\0';
                printf("Received '%s' from server. Sending 'ping' again...\n", buffer);
                std::cout << pingMsgStr << std::endl;
                sleep(1);
                sendto(sfd, pingMsg, strlen(pingMsg), 0, (sockaddr*)&responderAddr, addrLen);
            }
        }
    }
}



int main() {
    std::srand(std::time({}));
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
