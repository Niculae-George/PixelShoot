#include "stages.h"

// --- MenuStage ---
void MenuStage::Update(float deltaTime) {
    menu.Update();
    if (menu.ShouldStartGame()) {
        menu.ResetForGameStart();
        manager->ChangeStage(std::make_unique<GameStage>(manager, 1600, 900));
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

// --- GameStage ---
GameStage::GameStage(StageManager* manager, int screenWidth, int screenHeight)
    : manager(manager), screenWidth(screenWidth), screenHeight(screenHeight) {
    gridSize = 40;
    mapWidth = 60 * gridSize;
    mapHeight = 50 * gridSize;

    playerSpeed = 250.0f;
    playerSize = 32.0f;
    bulletSpeed = 500.0f;
    bulletSize = 8.0f;

    // Generate random world with walls and multiple players
    WorldGenParams worldParams = {
        mapWidth, 
        mapHeight, 
        gridSize,
        15,     // 15% wall density
        1,      // min path width (1 grid cell)
        5,      // safe zone radius around player spawn (5 grid cells = 200 pixels)
        4       // numPlayers (e.g. 4 for testing 4 territories)
    };
    GenerateRandomWorld(entities, worldParams);

    // Setup camera
    camera = { 0 };
    camera.zoom = 1.0f;

    // Setup mask texture
    maskTexture = LoadRenderTexture(screenWidth, screenHeight);
}

GameStage::~GameStage() {
    UnloadRenderTexture(maskTexture);
}

void GameStage::Update(float deltaTime) {
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
