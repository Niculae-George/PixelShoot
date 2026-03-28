// server_main.cpp
#include <iostream>
#include <vector>
#include <ctime>
#include <stdio.h>
#define ENET_IMPLEMENTATION
#include "enet.h"

enum class PacketType : uint8_t {
    PLAYER_COUNT = 0,
    START_GAME = 1
};

#define MAX_CLIENTS 32

int main() {
    if (enet_initialize () != 0) {
        printf("An error occurred while initializing ENet.\n");
        return 1;
    }
    atexit(enet_deinitialize);

    ENetAddress address = {0};

    address.host = ENET_HOST_ANY; /* Bind the server to the default localhost.     */
    address.port = 7777; /* Bind the server to port 7777. */

    /* create a server */
    ENetHost * server = enet_host_create(&address, MAX_CLIENTS, 2, 0, 0);

    if (server == NULL) {
        printf("An error occurred while trying to create an ENet server host.\n");
        std::cin.get();
        return 1;
    }

    printf("Started a server...\n");

    unsigned int mapSeed = (unsigned int)time(nullptr);
    printf("Using Map Seed: %u\n", mapSeed);

    int playerCount = 0;
    bool gameStarted = false;

    auto broadcastPlayerCount = [&]() {
        uint8_t buf[2];
        buf[0] = (uint8_t)PacketType::PLAYER_COUNT;
        buf[1] = (uint8_t)playerCount;
        ENetPacket* p = enet_packet_create(buf, 2, ENET_PACKET_FLAG_RELIABLE);
        enet_host_broadcast(server, 0, p);
    };

    ENetEvent event = {0};

    /* Wait up to 1000 milliseconds for an event. (WARNING: blocking) */
    while (true) {
        while (enet_host_service(server, &event, 1000) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    printf("A new client connected from %x:%u.\n", event.peer->address.host, event.peer->address.port);
                    event.peer->data = (void*)"Client information";
                    if (!gameStarted) {
                        playerCount++;
                        broadcastPlayerCount();
                    } else {
                        enet_peer_disconnect(event.peer, 0);
                    }
                    break;

                case ENET_EVENT_TYPE_RECEIVE:
                    if (event.packet->dataLength > 0) {
                        uint8_t type = event.packet->data[0];
                        if (type == (uint8_t)PacketType::START_GAME && !gameStarted) {
                            gameStarted = true;
                            // Broadcast start game with seed
                            uint8_t buf[5];
                            buf[0] = (uint8_t)PacketType::START_GAME;
                            memcpy(&buf[1], &mapSeed, sizeof(unsigned int));
                            ENetPacket* p = enet_packet_create(buf, 5, ENET_PACKET_FLAG_RELIABLE);
                            enet_host_broadcast(server, 0, p);
                        } else if (gameStarted) {
                            // Relay packet to all other peers during game
                            for (size_t i = 0; i < server->peerCount; ++i) {
                                ENetPeer* currentPeer = &server->peers[i];
                                if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
                                if (currentPeer == event.peer) continue;

                                ENetPacket* packet = enet_packet_create(event.packet->data, event.packet->dataLength, event.packet->flags);
                                enet_peer_send(currentPeer, 0, packet);
                            }
                        }
                    }
                    /* Clean up the packet now that we're done using it. */
                    enet_packet_destroy (event.packet);
                    break;

                case ENET_EVENT_TYPE_DISCONNECT:
                    printf("%s disconnected.\n", (char*)event.peer->data);
                    if (!gameStarted) {
                        playerCount--;
                        if (playerCount < 0) playerCount = 0;
                        broadcastPlayerCount();
                    }
                    /* Reset the peer's client information. */
                    event.peer->data = NULL;
                    break;

                case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                    printf("%s disconnected due to timeout.\n", (char*)event.peer->data);
                    if (!gameStarted) {
                        playerCount--;
                        if (playerCount < 0) playerCount = 0;
                        broadcastPlayerCount();
                    }
                    /* Reset the peer's client information. */
                    event.peer->data = NULL;
                    break;

                case ENET_EVENT_TYPE_NONE:
                    break;
            }
        }
    }

    enet_host_destroy(server);
    return 0;
}
