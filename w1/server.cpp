#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <netdb.h>
#include <netinet/in.h>
#include <random>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <utility>

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


struct ClientState {
    int port;
    std::string addr;
    int id;
    sockaddr_in my_sockaddr;

    std::string to_string() const { return std::to_string(id) + "(" + addr + ":" + std::to_string(port) + ")"; }
};


class StateMachine {
protected:
    Server& server;

    std::map<std::pair<int, std::string>, ClientState> clients;

public:
    StateMachine(Server& server) : server(server) {}

    void process(const ReadResult& result) {
        // ID to find in clients map
        std::pair<int, std::string> client_state_id = {
            result.get_responder_port(),
            result.get_responder_addr()
        };

        // Register or unregister
        if (result.starts(SYSMSG_REGISTER)) {
            std::string msg = result.get_sys_msg(SYSMSG_REGISTER);

            if (clients.find(client_state_id) != clients.end()) {
                server.send("Client with such address and port was already registered!", result.responder_sockaddr);
                return;
            }
            ClientState new_client_state {
                .port = result.get_responder_port(),
                .addr = result.get_responder_addr(),
                .id = std::atoi(msg.c_str()),
                .my_sockaddr = result.responder_sockaddr
            };
            clients[client_state_id] = new_client_state;
            server.send("Registration for " + new_client_state.to_string()  + " succesfull!", result.responder_sockaddr);
            return;
        }
        if (result.starts(SYSMSG_UNREGISTER)) {
            std::string msg = result.get_sys_msg(SYSMSG_UNREGISTER);

            if (clients.find(client_state_id) == clients.end()) {
                server.send("No client with such address and port has been registered!", result.responder_sockaddr);
                return;
            }
            server.send("Client " + clients[client_state_id].to_string()  + " was unregistered", result.responder_sockaddr);
            clients.erase(client_state_id);
            return;
        }

        // Check if registered and find the client
        if (clients.find(client_state_id) == clients.end()) {
            server.send("Please, register first bu using command " + SYSMSG_REGISTER + "{id}", result.responder_sockaddr);
            return;
        }
        ClientState& client = clients[client_state_id];

        // Connect and chat between clients
        if (result.starts(SYSMSG_SEND_TO_OTHERS)) {
            std::string msg = result.get_sys_msg(SYSMSG_SEND_TO_OTHERS);
            for (const auto& client_other : clients) {
                if (client_other.second.id == client.id) continue; // Don't send to yourself
                server.send("Message from " + client.to_string() + ": " + msg, client_other.second.my_sockaddr);
            }
            return;
        }

        // Process math duel
        if (result.starts(SYSMSG_MATHDUEL_INIT)) {
            std::string msg = result.get_sys_msg(SYSMSG_MATHDUEL_INIT);
            server.send("TODO Process math init " + msg, result.responder_sockaddr);
        } else if (result.starts(SYSMSG_MATHDUEL_ANS)) {
            std::string msg = result.get_sys_msg(SYSMSG_MATHDUEL_ANS);
            server.send("TODO Process math ans " + msg, result.responder_sockaddr);
        } else {
            server.send("Please use one of the commands as prefix to your message", result.responder_sockaddr);
        }
    }
};


int main() {
    Server server(SERVER_PORT);

    server.start();
    if (!server.is_working()) {
        std::cout << "main - Couldn't connect to the server" << std::endl;
        return 1;
    }

    // Start state machine
    StateMachine state_machine(server);

    // Start thread for reading user input and sending it to the server
    std::thread thread_server_listen([&]() {
        while (true) {
            sleep(SERVER_SLEEP_BETWEEN_RECEIVE);
            ReadResult result = server.read();
            if (result.is_empty) continue;
            state_machine.process(result);
        }
    });

    // Wait till the end
    while (server.is_working()) {}
    thread_server_listen.join();
    server.close();

    return 0;
}

