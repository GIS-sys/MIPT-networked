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


class Client : public CommonCS {
protected:
    sockaddr_in server_sockaddr;
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

    void reset() {
        connected = false;
        server_sockaddr = {};
        if (sfd != -1) close(sfd);
        sfd = -1;
    }

    bool connect(const std::string& server_name, int port);
    ssize_t send(const std::string& msg) const;
    ReadResult read() const;
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
    memcpy(&server_sockaddr, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);

    // Create client socket
    sfd = create_dgram_socket(nullptr, "0", nullptr); // Bind to any available port
    if (sfd == -1) {
        std::cout << "Client::connect - Failed to create client socket" << std::endl;
        return false;
    }

    // Set myself as connected
    connected = true;

    // Send registration message
    send(SYSMSG_REGISTER + id);

    return true;
}

ssize_t Client::send(const std::string& msg) const {
    return _send(msg, sfd, server_sockaddr);
}

ReadResult Client::read() const {
    return _read(sfd);
}


int main() {
    std::srand(std::time({}));
    Client client;

    client.connect(SERVER_ADDR, SERVER_PORT);
    if (!client.is_connected()) {
        std::cout << "main - Couldn't connect to the server" << std::endl;
        return 1;
    }

    // Start thread for reading user input and sending it to the server
    std::thread thread_user_read([&]() {
        while (true) {
            std::string input;
            std::getline(std::cin, input);
            client.send(input);
        }
    });

    // Start thread for reading user input and sending it to the server
    std::thread thread_client_listen([&]() {
        while (true) {
            sleep(CLIENT_SLEEP_BETWEEN_RECEIVE);
            ReadResult result = client.read();
            if (result.is_empty) continue;
        }
    });

    // Wait till the end
    while (client.is_connected()) {}
    thread_user_read.join();
    thread_client_listen.join();
    client.reset();

    return 0;
}
