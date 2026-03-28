#include "net_utils.h"
#include "stages.h"

// --- MenuStage ---
void MenuStage::Update(float deltaTime) {
    menu.Update();
    if (menu.ShouldStartGame()) {
        bool isHost = menu.IsHost();
        std::string ip = menu.GetTargetIP();
        menu.ResetForGameStart();
        manager->ChangeStage(std::make_unique<LobbyStage>(manager, 1600, 900, isHost, ip));
    }
    else if (menu.ShouldQuit()) {
        manager->Quit();
    }
}

void MenuStage::Draw() {
    BeginDrawing();
    menu.Draw();
    EndDrawing();
}

// --- LobbyStage ---
LobbyStage::LobbyStage(StageManager* manager, int screenWidth, int screenHeight, bool isHost, const std::string& ip)
    : manager(manager), screenWidth(screenWidth), screenHeight(screenHeight), isHost(isHost), ipConfig(ip), clientHost(nullptr), serverPeer(nullptr), playerCount(0) {

    ENetHost* client = { 0 };
    client = enet_host_create(NULL /* create a client host */,
        1 /* only allow 1 outgoing connection */,
        2 /* allow up 2 channels to be used, 0 and 1 */,
        0 /* assume any amount of incoming bandwidth */,
        0 /* assume any amount of outgoing bandwidth */);

    clientHost = client;
    if (clientHost == NULL) {
        TraceLog(LOG_ERROR, "An error occurred while trying to create an ENet client host.");
    }

    if (isHost) {
        const char* appDir = GetApplicationDirectory();
#ifdef _WIN32
        // Kill any existing server.exe process and give the OS a moment to free the port
        system("taskkill /f /im server.exe >nul 2>&1");
        std::this_thread::sleep_for(std::chrono::milliseconds(300)); 

        // Start the server using the absolute path
        system(TextFormat("start \"\" \"%sserver.exe\"", appDir));
#else
        system("pkill server >/dev/null 2>&1");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        system(TextFormat("\"%sserver\" &", appDir));
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(300)); // give it a little time to start
    }

    if (clientHost) {
        ENetAddress address = { 0 };
        ENetEvent event = { 0 };

        std::string targetIP = isHost ? "127.0.0.1" : ipConfig;
        enet_address_set_host(&address, targetIP.c_str()); 
        address.port = 7777;

        TraceLog(LOG_INFO, "Attempting to connect to host at %s on port 7777...", targetIP.c_str());

        serverPeer = enet_host_connect(clientHost, &address, 2, 0);
        if (serverPeer == NULL) {
            TraceLog(LOG_ERROR, "No available peers for initiating an ENet connection.");
        } else {
            /* Wait up to 5 seconds for the connection attempt to succeed. */
            if (enet_host_service(clientHost, &event, 5000) > 0 &&
                event.type == ENET_EVENT_TYPE_CONNECT) {
                TraceLog(LOG_INFO, "Connection to %s:7777 succeeded.", targetIP.c_str());
            } else {
                enet_peer_reset(serverPeer);
                serverPeer = nullptr;
                TraceLog(LOG_ERROR, "Connection to %s:7777 failed.", targetIP.c_str());
            }
        }
    }
}

LobbyStage::~LobbyStage() {
    if (serverPeer) {
        enet_peer_disconnect(serverPeer, 0);

        ENetEvent event;
        bool disconnected = false;

        /* Allow up to 3 seconds for the disconnect to succeed
         * and drop any packets received.
         */
        while (enet_host_service(clientHost, &event, 3000) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                TraceLog(LOG_INFO, "Disconnection succeeded.");
                disconnected = true;
                break;
            default:
                break;
            }
        }

        // Drop connection, since disconnection didn't succeed
        if (!disconnected) {
            enet_peer_reset(serverPeer);
        }
    }

    if (clientHost) {
        enet_host_destroy(clientHost);
    }
}

void LobbyStage::Update(float deltaTime) {
    if (clientHost) {
        ENetEvent event;
        while (enet_host_service(clientHost, &event, 0) > 0) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                uint8_t type = event.packet->data[0];
                if (type == 0) { // PLAYER_COUNT
                    playerCount = event.packet->data[1];
                } else if (type == 1) { // START_GAME
                    unsigned int seed;
                    memcpy(&seed, &event.packet->data[1], sizeof(unsigned int));

                    _ENetHost* ch = clientHost;
                    _ENetPeer* sp = serverPeer;
                    clientHost = nullptr; // prevent destroy
                    serverPeer = nullptr;

                    manager->ChangeStage(std::make_unique<GameStage>(manager, screenWidth, screenHeight, ch, sp, seed, playerCount));
                    enet_packet_destroy(event.packet);
                    return;
                }
                enet_packet_destroy(event.packet);
            } else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                serverPeer = nullptr;
                manager->ChangeStage(std::make_unique<MenuStage>(manager, screenWidth, screenHeight));
                return;
            }
        }
    }

    if (isHost) {
        Vector2 mousePos = GetMousePosition();
        Rectangle startBtn = {(float)(screenWidth - 200) / 2, (float)(screenHeight / 2 + 100), 200, 50};
        if (CheckCollisionPointRec(mousePos, startBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (serverPeer) {
                uint8_t type = 1; // START_GAME
                ENetPacket* p = enet_packet_create(&type, 1, ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(serverPeer, 0, p);
            }
        }
    }
}

void LobbyStage::Draw() {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    int textW = MeasureText("Lobby", 60);
    DrawText("Lobby", (screenWidth - textW) / 2, screenHeight / 4, 60, DARKGRAY);

    std::string countStr = "Players in lobby: " + std::to_string(playerCount);
    int countW = MeasureText(countStr.c_str(), 30);
    DrawText(countStr.c_str(), (screenWidth - countW) / 2, screenHeight / 2, 30, DARKGRAY);

    if (isHost) {
        std::string serverStatus = "Server Status: Auto-Launched | IP: 127.0.0.1 | Port: 7777";
        int statusW = MeasureText(serverStatus.c_str(), 20);
        DrawText(serverStatus.c_str(), (screenWidth - statusW) / 2, screenHeight / 2 + 50, 20, DARKGREEN);

        Rectangle startBtn = {static_cast<float>(screenWidth - 200) / 2, static_cast<float>(screenHeight / 2 + 100), 200, 50};
        Vector2 mousePos = GetMousePosition();
        bool hovered = CheckCollisionPointRec(mousePos, startBtn);

        DrawRectangleRec(startBtn, hovered ? LIGHTGRAY : DARKGRAY);
        DrawRectangleLinesEx(startBtn, 2, WHITE);

        int btnW = MeasureText("Start Game", 20);
        DrawText("Start Game", startBtn.x + (200 - btnW) / 2, startBtn.y + 15, 20, WHITE);
    } else {
        std::string connStr = "Connecting to IP: " + ipConfig + " | Port: 7777";
        int connW = MeasureText(connStr.c_str(), 20);
        DrawText(connStr.c_str(), (screenWidth - connW) / 2, screenHeight / 2 + 50, 20, DARKGREEN);

        std::string waitStr = "Waiting for host to start...";
        int waitW = MeasureText(waitStr.c_str(), 20);
        DrawText(waitStr.c_str(), (screenWidth - waitW) / 2, screenHeight / 2 + 100, 20, DARKGRAY);
    }

    EndDrawing();
}

// --- GameStage ---
GameStage::GameStage(StageManager* manager, int screenWidth, int screenHeight, _ENetHost* ch, _ENetPeer* sp, unsigned int seed, int pCount)
    : manager(manager), screenWidth(screenWidth), screenHeight(screenHeight), clientHost(ch), serverPeer(sp), mapSeed(seed), numPlayers(pCount) {
    gridSize = 40;
    mapWidth = 60 * gridSize;
    mapHeight = 50 * gridSize;

    playerSpeed = 250.0f;
    playerSize = 32.0f;
    bulletSpeed = 500.0f;
    bulletSize = 8.0f;

    isPlaying = false;
    mapGenerated = false;

    // Start map generation in secondary thread
    mapThread = std::thread([this]() {
        WorldGenParams worldParams = {
            mapWidth, 
            mapHeight, 
            gridSize,
            15,     // 15% wall density
            1,      // min path width (1 grid cell)
            5,      // safe zone radius around player spawn
            numPlayers, // numPlayers
            mapSeed
        };
        genSpawns = GenerateRandomWorld(genEntities, worldParams);
        mapGenerated = true;
    });

    // Setup camera
    camera = { 0 };
    camera.zoom = 1.0f;

    // Setup mask texture
    maskTexture = LoadRenderTexture(screenWidth, screenHeight);
}

GameStage::~GameStage() {
    if (serverPeer) {
        enet_peer_disconnect(serverPeer, 0);
        // Wait for disconnect
        ENetEvent event;
        while (enet_host_service(clientHost, &event, 300) > 0) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                enet_packet_destroy(event.packet);
            } else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                break;
            }
        }
    }
    if (clientHost) {
        enet_host_destroy(clientHost);
    }

    if (mapThread.joinable()) {
        mapThread.join();
    }
    UnloadRenderTexture(maskTexture);
}

void GameStage::Update(float deltaTime) {
    if (clientHost) {
        ENetEvent event;
        while (enet_host_service(clientHost, &event, 0) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    // Parse in-game packets here later
                    enet_packet_destroy(event.packet);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    TraceLog(LOG_INFO, "Disconnected from server.");
                    serverPeer = nullptr;
                    break;
                default:
                    break;
            }
        }
    }

    if (!isPlaying) {
        if (mapGenerated) {
            if (mapThread.joinable()) {
                mapThread.join();
            }
            entities = std::move(genEntities);

            // Add local player using the first generated spawn point
            if (!genSpawns.empty()) {
                entities.push_back(Entity{genSpawns[0], {playerSize, playerSize}, EntityType::PLAYER});
            }

            isPlaying = true;
        }
        return;
    }

    // Get player entity
    Entity* player = GetPlayerEntity(entities);
    if (!player) return;  // Safety check

    // Player movement with precise axis-aligned collision detection
    Vector2 newPos = player->pos;

    if (IsKeyDown( KEY_RIGHT ) || IsKeyDown( KEY_D )) newPos.x += playerSpeed * deltaTime;
    if (IsKeyDown( KEY_LEFT ) || IsKeyDown( KEY_A ))  newPos.x -= playerSpeed * deltaTime;
    if (IsKeyDown( KEY_DOWN ) || IsKeyDown( KEY_S ))  newPos.y += playerSpeed * deltaTime;
    if (IsKeyDown( KEY_UP ) || IsKeyDown( KEY_W ))    newPos.y -= playerSpeed * deltaTime;

    // Try X movement first
    Rectangle playerRectX = {newPos.x - playerSize / 2, player->pos.y - playerSize / 2, playerSize, playerSize};
    if (!CheckEntityCollision(playerRectX, entities, EntityType::WALL)) {
        player->pos.x = newPos.x;
    }

    // Try Y movement independently
    Rectangle playerRectY = {player->pos.x - playerSize / 2, newPos.y - playerSize / 2, playerSize, playerSize};
    if (!CheckEntityCollision(playerRectY, entities, EntityType::WALL)) {
        player->pos.y = newPos.y;
    }

    // Handle shooting 
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mouseScreenPos = GetMousePosition();
        Vector2 mouseWorldPos = GetScreenToWorld2D(mouseScreenPos, camera);

        Vector2 direction = {mouseWorldPos.x - player->pos.x, mouseWorldPos.y - player->pos.y};
        float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        if (distance > 0.0f) {
            direction.x /= distance;
            direction.y /= distance;
            Vector2 bulletVelocity = {direction.x * bulletSpeed, direction.y * bulletSpeed};
            entities.push_back(Entity{player->pos, {bulletSize, bulletSize}, EntityType::BULLET, bulletVelocity});
        }
    }

    // Update bullets
    for (auto& entity : entities) {
        if (entity.type == EntityType::BULLET) {
            entity.pos.x += entity.velocity.x * deltaTime;
            entity.pos.y += entity.velocity.y * deltaTime;
        }
    }

    // Remove bullets that are out of bounds or hit walls
    int mw = mapWidth, mh = mapHeight;
    entities.erase(
        std::remove_if(entities.begin(), entities.end(),
                      [mw, mh, this](const Entity& e) {
                          if (e.type != EntityType::BULLET) return false;

                          if (e.pos.x < 0 || e.pos.x > mw ||
                              e.pos.y < 0 || e.pos.y > mh) {
                              return true;
                          }

                          Rectangle bulletRect = e.getRect();
                          for (const auto& entity : entities) {
                              if (entity.type == EntityType::WALL &&
                                  CheckCollisionRecs(bulletRect, entity.getRect())) {
                                  return true;
                              }
                          }
                          return false;
                      }),
        entities.end()
    );

    // Keep the player safely inside the map boundaries
    player->pos.x = std::max<float>(player->pos.x, playerSize / 2);
    player->pos.x = std::min<float>(player->pos.x, mapWidth - playerSize / 2);
    player->pos.y = std::max<float>(player->pos.y, playerSize / 2);
    player->pos.y = std::min<float>(player->pos.y, mapHeight - playerSize / 2);

    // Update camera to follow player (centered on player)
    camera.target = player->pos;
    camera.offset = {.x = screenWidth / 2.0f, .y = screenHeight / 2.0f };

    // Clamp camera to stay within map bounds
    camera.target.x = std::max(camera.target.x, camera.offset.x / camera.zoom);
    camera.target.x = std::min(camera.target.x, mapWidth - camera.offset.x / camera.zoom);
    camera.target.y = std::max(camera.target.y, camera.offset.y / camera.zoom);
    camera.target.y = std::min(camera.target.y, mapHeight - camera.offset.y / camera.zoom);
}

void GameStage::Draw() {
    BeginDrawing();
    ClearBackground( RAYWHITE );

    if (!isPlaying) {
        int textWidth = MeasureText("Generating World... Please wait.", 40);
        DrawText("Generating World... Please wait.", (screenWidth - textWidth) / 2, screenHeight / 2, 40, DARKGRAY);
        EndDrawing();
        return;
    }

    BeginMode2D( camera );
    {
        int startGridX = static_cast<int>(std::floor((camera.target.x - camera.offset.x / camera.zoom) / gridSize) * gridSize);
        int startGridY = static_cast<int>(std::floor((camera.target.y - camera.offset.y / camera.zoom) / gridSize) * gridSize);
        int endGridX = static_cast<int>(std::ceil((camera.target.x + camera.offset.x / camera.zoom) / gridSize) * gridSize);
        int endGridY = static_cast<int>(std::ceil((camera.target.y + camera.offset.y / camera.zoom) / gridSize) * gridSize);

        for (int i = startGridX; i <= endGridX; i += gridSize) {
            if (i >= 0 && i <= mapWidth) {
                DrawLine( i, startGridY, i, endGridY, LIGHTGRAY );
            }
        }

        for (int i = startGridY; i <= endGridY; i += gridSize) {
            if (i >= 0 && i <= mapHeight) {
                DrawLine( startGridX, i, endGridX, i, LIGHTGRAY );
            }
        }

        for (const auto& entity : entities) {
            if (entity.type == EntityType::WALL) {
                DrawRectangleRec(entity.getRect(), RED);
            }
            else if (entity.type == EntityType::PLAYER) {
                DrawRectangleRec(entity.getRect(), BLUE);
            }
            else if (entity.type == EntityType::BULLET) {
                DrawRectangleRec(entity.getRect(), YELLOW);
            }
        }
    }
    EndMode2D();

    BeginTextureMode(maskTexture);
    ClearBackground(BLACK);

    Entity* player = GetPlayerEntity(entities);
    if (player) {
        constexpr float visionRadius = 10.0f * 40; // 40 = gridSize
        const Vector2 playerScreenPos = GetWorldToScreen2D(player->pos, camera);

        DrawCircleV(playerScreenPos, visionRadius, WHITE);

        BeginBlendMode(BLEND_ALPHA);
        for (int i = 1; i <= 30; i++) {
            const float currentRadius = visionRadius + (i * 1.5f);
            const unsigned char alpha = static_cast<unsigned char>(255 * (1.0f - (i / 30.0f)));
            DrawCircleV(playerScreenPos, currentRadius, Color{0, 0, 0, alpha});
        }
        EndBlendMode();
    }
    EndTextureMode();

    DrawTextureRec(maskTexture.texture, {0, 0, static_cast<float>(screenWidth), static_cast<float>(-screenHeight)}, {0, 0}, WHITE);

    if (player) {
        std::string positionText = "X: " + std::to_string(static_cast<int>(player->pos.x)) +
            " \n\nY: " + std::to_string(static_cast<int>(player->pos.y));
        const int textWidth = MeasureText(positionText.c_str(), 20);
        DrawText(positionText.c_str(), screenWidth - textWidth - 10, 10, 20, DARKGRAY);
    }
    
    DrawText( "Use WASD or Arrow Keys to move | Left Click to shoot", 10, 10, 20, DARKGRAY );

    EndDrawing();
}
