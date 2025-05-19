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

#include "common.h"
#include "socket_tools.h"


class Server : public CommonCS {
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
    return _send(msg, sfd, client_sockaddr);
}

ReadResult Server::read() const {
    return _read(sfd);
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

