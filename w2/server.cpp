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


class GameServer {
private:
    ENetHost* game_host;
    GameSession current_session;

public:
    GameServer() {
        if (enet_initialize() != 0) {
            throw std::runtime_error("Cannot init ENet");
        }
        atexit(enet_deinitialize);

        ENetAddress address;
        enet_address_set_host(&address, SERVER_ADDR.c_str());
        address.port = SERVER_PORT;

        game_host = enet_host_create(&address, MAX_PLAYERS, CHANNELS_AMOUNT, 0, 0);
        if (!game_host) {
            throw std::runtime_error("Cannot create game host");
        }

        std::cout << "Game server started on port " << SERVER_PORT << std::endl;
    }

    ~GameServer() {
        if (game_host) {
            enet_host_destroy(game_host);
        }
    }

    void run() {
        ENetEvent event;
        while (true) {
            while (enet_host_service(game_host, &event, SERVER_SERVICE_TIMEOUT_MS) > 0) {
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
        Player player(event.peer);
        player.generate_random_credentials();
        current_session.players.push_back(player);
        send(
            prepare_for_send(player.to_string_vector()),
            player.peer,
            CHANNEL_SERVER_PLAYER_CRED,
            true
        );
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
        std::cout << "Got data from " << event.peer->address.host << ":" << event.peer->address.port << " - " << event.packet->data << std::endl;
        const char* data = reinterpret_cast<const char*>(event.packet->data);

        // TODO
    }
};


int main() {
    GameServer server;
    server.run();

    return 0;
}
