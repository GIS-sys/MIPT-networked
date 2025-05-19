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


struct MathDuel {
protected:
    std::vector<ClientState> _participants;
    std::string _question;
    std::string _answer;

public:
    MathDuel() = default;
    MathDuel(ClientState participant) { add_opponent(participant); }

    const std::vector<ClientState>& participants() const { return _participants; }
    const std::string& question() const { return _question; }
    const std::string& answer() const { return _answer; }

    bool is_participant(const ClientState& clientstate) const {
        for (const ClientState& participant : _participants) {
            if (clientstate.id == participant.id) return true;
        }
        return false;
    }

    void add_opponent(ClientState participant) {
        _participants.push_back(participant);
        if (_participants.size() == 2) {
            _question = "TODO";
            _answer = "TODO";
        }
    }

    bool is_waiting_for_opponent() const { return _participants.size() < 2; }

    bool check(const std::string& attempt) const { return _answer == attempt; }

    ClientState other(const ClientState& clientstate) const {
        for (const ClientState& participant : _participants) {
            if (clientstate.id != participant.id) return participant;
        }
        return clientstate;
    }
};


class StateMachine {
protected:
    Server& server;

    std::map<std::pair<int, std::string>, ClientState> clients;
    std::vector<MathDuel> math_duels;

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
            // Disallow to participate in several math duels
            for (const MathDuel& math_duel : math_duels) {
                if (math_duel.is_participant(client)) {
                    if (math_duel.is_waiting_for_opponent()) {
                        server.send("You are already waiting for a math duel!", client.my_sockaddr);
                    } else {
                        server.send("You are already participating in a math duel! Reminder: " + math_duel.question(), client.my_sockaddr);
                    }
                    return;
                }
            }
            // Check if last duel is waiting for an opponent
            if (!math_duels.empty() && math_duels.back().is_waiting_for_opponent()) {
                math_duels.back().add_opponent(client);
                for (const ClientState& duelist : math_duels.back().participants()) {
                    server.send("A math duel has begun! Send answer to this question: " + math_duels.back().question(), duelist.my_sockaddr);
                }
                return;
            }
            // Else create a new duel
            math_duels.push_back(MathDuel(client));
            server.send("You have registered for a math duel. Waiting for an opponent...", client.my_sockaddr);
            return;
        }
        if (result.starts(SYSMSG_MATHDUEL_ANS)) {
            std::string msg = result.get_sys_msg(SYSMSG_MATHDUEL_ANS);
            trim(msg);
            // Find math duel
            int math_duel_index = -1;
            for (int i = 0; i < math_duels.size(); ++i) {
                if (math_duels[i].is_participant(client)) {
                    math_duel_index = i;
                    break;
                }
            }
            // If not participating
            if (math_duel_index == -1) {
                server.send("You are not taking part in a math duel! Register for it using command " + SYSMSG_MATHDUEL_INIT, client.my_sockaddr);
                return;
            }
            // Else check answer
            if (math_duels[math_duel_index].check(msg)) {
                server.send("Congratulations, you won the mathduel!", client.my_sockaddr);
                server.send("Unfortunately, you lost the mathduel :(", math_duels[math_duel_index].other(client).my_sockaddr);
                math_duels.erase(math_duels.begin() + math_duel_index); // probably need to rewrite using list, but oh well, let's assume not many ppl want to do math
            } else {
                server.send("Wrong answer, try again. Reminder: " + math_duels[math_duel_index].question(), client.my_sockaddr);
            }
            return;
        }

        // Unexpected message
        server.send("Please use one of the commands as prefix to your message", client.my_sockaddr);
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

