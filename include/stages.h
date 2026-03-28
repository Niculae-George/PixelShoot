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

class GameStage : public Stage {
public:
    GameStage(StageManager* manager, int screenWidth, int screenHeight);
    ~GameStage();

    void Update(float deltaTime) override;
    void Draw() override;

private:
    StageManager* manager;
    int screenWidth, screenHeight;
    int mapWidth, mapHeight;
    int gridSize;
    float playerSpeed, playerSize, bulletSpeed, bulletSize;

    std::vector<Entity> entities;
    Camera2D camera;
    RenderTexture2D maskTexture;
};
