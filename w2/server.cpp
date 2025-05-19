#include <cstdint>
#include <algorithm>
#include <chrono>
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
    uint64_t send_data_timer = 0;

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

                uint64_t current = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                uint64_t dt = current - send_data_timer;
                if (dt > SERVER_SEND_DATA_INTERVAL_MS * 1000) {
                    broadcast_player_data();
                    send_data_timer = current;
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
            prepare_for_send(player.to_string_vector({ .name = true, .id = true })),
            player.peer,
            CHANNEL_SERVER_PLAYER_CRED,
            true
        );

        // Send list of all players
        broadcast_player_lists();
    }

    void broadcast_player_lists() {
        std::cout << "Broadcasting player lists" << std::endl;
        for (int i = 0; i < current_session.players.size(); ++i) {
            // Generate combined list of all other players except for currently chosen
            std::vector<std::string> players_list_vector;
            for (int j = 0; j < current_session.players.size(); ++j) {
                if (i == j) continue;
                std::vector<std::string> a = current_session.players[j].to_string_vector({ .name = true, .id = true });
                players_list_vector.insert(players_list_vector.end(), a.begin(), a.end());
            }
            // Send to currently chosen player
            send(
                prepare_for_send(players_list_vector),
                current_session.players[i].peer,
                CHANNEL_SERVER_PLAYERS_LIST,
                true
            );
        }
    }

    void broadcast_player_data() {
        // COUT
        if (DEBUG) std::cout << "Broadcasting player data" << std::endl;
        // Generate combined list of all other players except for currently chosen
        std::vector<std::string> players_list_vector;
        for (int j = 0; j < current_session.players.size(); ++j) {
            std::vector<std::string> a = current_session.players[j].to_string_vector({ .name = true, .id = true, .pos = true, .ping = true });
            players_list_vector.insert(players_list_vector.end(), a.begin(), a.end());
        }
        for (int i = 0; i < current_session.players.size(); ++i) {
            // Send to currently chosen player
            send(
                prepare_for_send(players_list_vector),
                current_session.players[i].peer,
                CHANNEL_SERVER_PLAYERS_DATA,
                false
            );
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
            broadcast_player_lists();
        }
    }

    void handle_packet(const ENetEvent& event) {
        if (DEBUG) std::cout << "Got data from " << event.peer->address.host << ":" << event.peer->address.port << " - " << event.packet->data << std::endl;
        const char* data = reinterpret_cast<const char*>(event.packet->data);

        if (event.channelID == CHANNEL_SERVER_PING) {
            std::cout << "Sending pong to " << event.peer->address.host << ":" << event.peer->address.port << std::endl;
            send(
                "pong",
                event.peer,
                CHANNEL_SERVER_PING,
                false
            );
            return;
        }
        if (event.channelID == CHANNEL_SERVER_PLAYERS_DATA) {
            std::string msg(data);
            std::vector<std::string> parsed = parse_from_receive(msg);
            PlayerUseData use{ .pos = true, .ping = true };
            Player player = Player::from_string_vector(parsed, use);
            for (int i = 0; i < current_session.players.size(); ++i) {
                if (current_session.players[i].peer == event.peer) {
                    if (current_session.players[i].pos != player.pos) {
                        std::cout << "Position has changed for player id=" << current_session.players[i].id << " - "
                                  << "from " << current_session.players[i].pos.x << "," << current_session.players[i].pos.y << " "
                                  << "to " << player.pos.x << "," << player.pos.y << std::endl;
                    }
                    current_session.players[i].pos = player.pos;
                    current_session.players[i].ping = player.ping;
                }
            }
            return;
        }
    }
};


int main() {
    GameServer server;
    server.run();

    return 0;
}
