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
#include <thread>
#include <unistd.h>

#include "socket_tools.h"


class Server {
protected:
    int port = -1;
    int sfd = -1;

    bool working = false;

public:
    Server(int port) : port(port) {}

    bool is_working() const {
        return working;
    }

    void close() {
        if (sfd != -1) ::close(sfd);
        working = false;
    }

    bool start() {
        // Convert port to string
        std::string port_str = std::to_string(port);
        const char* port_cstr = port_str.c_str();

        // Start listening
        sfd = create_dgram_socket(nullptr, port_cstr, nullptr);

        // Process edge case
        if (sfd == -1) {
            std::cout << "Server::start - Cannot create socket" << std::endl;
            return false;
        }

        // Set myself as working
        std::cout << "Listening on port " << port << std::endl;
        working = true;
        return true;
    }
    ssize_t send(const std::string& msg, sockaddr_in client_sockaddr) const;
    ReadResult read() const;
};

ssize_t Server::send(const std::string& msg, sockaddr_in client_sockaddr) const {
    std::cout << "(sending message: " << msg << ")" << std::endl;
    ssize_t res = sendto(sfd, msg.c_str(), msg.size(), 0, (sockaddr*)&client_sockaddr, sizeof(client_sockaddr));
    if (res == -1) std::cout << "(error while sending message: " << strerror(errno) << ")" << std::endl;
    return res;
}

ReadResult Server::read() const {
    // fd_set containing only sfd
    fd_set readFds;
    FD_ZERO(&readFds);
    FD_SET(sfd, &readFds);

    // Timeout
    timeval tv = {0, SERVER_RECEIVE_TIMEOUT_US};

    // See if there are updates for any fd <= sfd
    int ready = select(sfd + 1, &readFds, nullptr, nullptr, &tv);

    // Process edge cases
    if (ready < 0) {
        // perror("select error");
        return ReadResult{ .is_error = true, .is_empty = true };
    }
    if (ready == 0) {
        // std::cout << "no response from server?" << std::endl;
        return ReadResult{ .is_error = false, .is_empty = true };
    }

    // Read message
    char buffer[MESSAGE_BUFFER_SIZE];
    sockaddr_in responderAddr;
    socklen_t addrLen = sizeof(responderAddr);
    ssize_t numBytes = recvfrom(sfd, buffer, sizeof(buffer), 0, (sockaddr*)&responderAddr, &addrLen);

    if (numBytes > 0) {
        buffer[numBytes] = '\0';
    }
    char* responder_addr = inet_ntoa(responderAddr.sin_addr);
    int responder_port = ntohs(responderAddr.sin_port);
    std::cout << "(got message from client " << responder_addr << ":" << responder_port << ": " << buffer << ")" << std::endl;
    return ReadResult{ .msg = buffer, .responder_sockaddr = responderAddr, .is_error = false, .is_empty = false };
}


int main() {
    Server server(SERVER_PORT);

    server.start();
    if (!server.is_working()) {
        std::cout << "main - Couldn't connect to the server" << std::endl;
        return 1;
    }

    // Start thread for reading user input and sending it to the server
    std::thread thread_server_listen([&]() {
        while (true) {
            sleep(SERVER_SLEEP_BETWEEN_RECEIVE);
            ReadResult result = server.read();
            if (result.is_empty) continue;
            server.send("pong", result.responder_sockaddr);
        }
    });

    // Wait till the end
    while (server.is_working()) {}
    thread_server_listen.join();
    server.close();

    return 0;
}

