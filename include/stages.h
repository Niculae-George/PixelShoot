#pragma once
#include "stage_manager.h"
#include "menu.h"
#include "raylib.h"
#include "entity.h"
#include "world.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>
#include <thread>
#include <atomic>

struct _ENetHost;
struct _ENetPeer;

class GameStage;

class MenuStage : public Stage {
public:
    MenuStage(StageManager* manager, int screenWidth, int screenHeight) 
        : manager(manager), menu(screenWidth, screenHeight) {}

    void Update(float deltaTime) override;
    void Draw() override;

private:
    StageManager* manager;
    Menu menu;
};

class LobbyStage : public Stage {
public:
    LobbyStage(StageManager* manager, int screenWidth, int screenHeight, bool isHost, const std::string& ip = "127.0.0.1");
    ~LobbyStage();

    void Update(float deltaTime) override;
    void Draw() override;

private:
    StageManager* manager;
    int screenWidth, screenHeight;
    bool isHost;
    std::string ipConfig;

    _ENetHost* clientHost;
    _ENetPeer* serverPeer;
    int playerCount;
};

class GameStage : public Stage {
public:
    GameStage(StageManager* manager, int screenWidth, int screenHeight, _ENetHost* clientHost, _ENetPeer* serverPeer, unsigned int seed, int numPlayers);
    ~GameStage();

    void Update(float deltaTime) override;
    void Draw() override;

private:
    StageManager* manager;
    int screenWidth, screenHeight;
    int mapWidth, mapHeight;
    int gridSize;
    float playerSpeed, playerSize, bulletSpeed, bulletSize;
    unsigned int mapSeed;
    int numPlayers;

    std::vector<Entity> entities;
    Camera2D camera;
    RenderTexture2D maskTexture;

    // Multi-threading generation
    bool isPlaying;
    std::atomic<bool> mapGenerated;
    std::thread mapThread;
    std::vector<Entity> genEntities;
    std::vector<Vector2> genSpawns;

    _ENetHost* clientHost;
    _ENetPeer* serverPeer;
};
