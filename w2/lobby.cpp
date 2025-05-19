#include <cstdint>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <enet/enet.h>
#include <string>
#include <vector>
#include <format>

#include "common.hpp"


struct GameSession {
    std::vector<Player> players;
    bool active = false;
};


class LobbyServer {
private:
    ENetHost* lobby_host;
    GameSession current_session;

public:
    LobbyServer() {
        if (enet_initialize() != 0) {
            throw std::runtime_error("Cannot init ENet");
        }
        atexit(enet_deinitialize);

        ENetAddress address;
        enet_address_set_host(&address, LOBBY_ADDR.c_str());
        address.port = LOBBY_PORT;

        lobby_host = enet_host_create(&address, MAX_PLAYERS, CHANNELS_AMOUNT, 0, 0);
        if (!lobby_host) {
            throw std::runtime_error("Cannot create lobby host");
        }

        std::cout << "Lobby server started on port " << LOBBY_PORT << std::endl;
    }

    ~LobbyServer() {
        if (lobby_host) {
            enet_host_destroy(lobby_host);
        }
    }

    void run() {
        ENetEvent event;
        while (true) {
            while (enet_host_service(lobby_host, &event, LOBBY_SERVICE_TIMEOUT_MS) > 0) {
                switch (event.type) {
                    case ENET_EVENT_TYPE_CONNECT:
                        std::cout << "Connected: " << event.peer->address.host << ":" << event.peer->address.port << std::endl;
                        handle_connect(event);
                        break;

                    case ENET_EVENT_TYPE_DISCONNECT:
                        std::cout << "Disconnected: " << event.peer->address.host << ":" << event.peer->address.port << std::endl;
                        handle_disconnect(event);
                        break;

                    case ENET_EVENT_TYPE_RECEIVE:
                        handle_packet(event);
                        enet_packet_destroy(event.packet);
                        break;

                    default:
                        break;
                }
            }
        }
    }

private:
    void handle_connect(const ENetEvent& event) {
        current_session.players.push_back(Player(event.peer));
        if (current_session.active) {
            send_game_server_info(event.peer);
        }
    }

    void handle_disconnect(const ENetEvent& event) {
        auto it = std::find_if(
            current_session.players.begin(),
            current_session.players.end(),
            [&](const Player& player) { return player.peer == event.peer; }
        );

        if (it != current_session.players.end()) {
            current_session.players.erase(it);
        }
    }

    void handle_packet(const ENetEvent& event) {
        if (DEBUG) std::cout << "Got data from " << event.peer->address.host << ":" << event.peer->address.port << " - " << event.packet->data << std::endl;
        const char* data = reinterpret_cast<const char*>(event.packet->data);

        if (data == SYSCMD_START) {
            handle_start_command(event.peer);
        } else {
            std::cout << "Got unexpected command from client" << std::endl;
        }
    }

    void handle_start_command(ENetPeer* peer) {
        // If game has already started - just ignore
        if (current_session.active) {
            return;
        }

        // Start the session
        current_session.active = true;

        // Notify all connected peers about the game server
        for (const auto& player : current_session.players) {
            send_game_server_info(player.peer);
        }
    }

    void send_game_server_info(ENetPeer* peer) {
        std::string message = prepare_for_send({SERVER_ADDR, std::to_string(SERVER_PORT)});
        ENetPacket* packet = enet_packet_create(message.c_str(), message.size() + 1, ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(peer, CHANNEL_LOBBY_START, packet);

        std::cout << "Sent game server info (" << SERVER_ADDR << ":" << SERVER_PORT << ") to " << peer->address.host << ":" << peer->address.port << std::endl;
    }
};


int main() {
    LobbyServer server;
    server.run();

    return 0;
}
