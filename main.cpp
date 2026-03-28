//main.cpp
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "raylib.h"
#include "entity.h"
#include "world.h"

int main() {
    // 1. Initialize Window
    constexpr int screenWidth = 1600;
    constexpr int screenHeight = 900;
    InitWindow( screenWidth , screenHeight , "PixelFight" );
    
    // 2. Map Setup
    constexpr int gridSize = 40;
    constexpr int mapWidth = 60 * gridSize;
    constexpr int mapHeight = 50 * gridSize;

    // 3. Player Setup
    constexpr float playerSpeed = 250.0f;
    constexpr float playerSize = 32.0f;  // Smaller than grid cell (40.0f)

    // Bullet Setup
    constexpr float bulletSpeed = 500.0f;
    constexpr float bulletSize = 8.0f;

    // 6. Entity system - walls, bullets, and players
    std::vector<Entity> entities;

    // Create player as an entity
    entities.push_back(Entity{{static_cast<float>(mapWidth) / 2, static_cast<float>(mapHeight) / 2}, 
                             {playerSize, playerSize}, EntityType::PLAYER});

    // Generate random world with walls
    WorldGenParams worldParams = {
        mapWidth, 
        mapHeight, 
        gridSize,
        15,     // 15% wall density (reduced for more open space)
        1,      // min path width (1 grid cell)
        5       // safe zone radius around player spawn (5 grid cells = 200 pixels)
    };
    GenerateRandomWorld(entities, worldParams);

    // 4. Camera Setup
    Camera2D camera = { 0 };
    camera.zoom = 1.0f;

    // 5. Create render texture for visibility mask
    RenderTexture2D maskTexture = LoadRenderTexture(screenWidth, screenHeight);

    SetTargetFPS( 60 );

    while (!WindowShouldClose()) {

        // --- UPDATE LOGIC ---
        const float deltaTime = GetFrameTime();

        // Get player entity
        Entity* player = GetPlayerEntity(entities);
        if (!player) return 1;  // Safety check

        // Player movement with precise axis-aligned collision detection
        Vector2 newPos = player->pos;

        if (IsKeyDown( KEY_RIGHT ) || IsKeyDown( KEY_D )) newPos.x += playerSpeed * deltaTime;
        if (IsKeyDown( KEY_LEFT ) || IsKeyDown( KEY_A ))  newPos.x -= playerSpeed * deltaTime;
        if (IsKeyDown( KEY_DOWN ) || IsKeyDown( KEY_S ))  newPos.y += playerSpeed * deltaTime;
        if (IsKeyDown( KEY_UP ) || IsKeyDown( KEY_W ))    newPos.y -= playerSpeed * deltaTime;

        // Check collision separately for X and Y axes (allows sliding along walls)
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

        // Handle shooting - left mouse click to fire bullet toward mouse
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouseScreenPos = GetMousePosition();
            Vector2 mouseWorldPos = GetScreenToWorld2D(mouseScreenPos, camera);

            // Calculate direction from player to mouse
            Vector2 direction = {mouseWorldPos.x - player->pos.x, mouseWorldPos.y - player->pos.y};
            float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

            if (distance > 0.0f) {
                // Normalize direction
                direction.x /= distance;
                direction.y /= distance;

                // Create bullet with velocity pointing toward mouse
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
        entities.erase(
            std::remove_if(entities.begin(), entities.end(),
                          [mapWidth, mapHeight, &entities](const Entity& e) {
                              if (e.type != EntityType::BULLET) return false;

                              // Remove if out of bounds
                              if (e.pos.x < 0 || e.pos.x > mapWidth ||
                                  e.pos.y < 0 || e.pos.y > mapHeight) {
                                  return true;
                              }

                              // Remove if bullet hits a wall
                              Rectangle bulletRect = e.getRect();
                              for (const auto& entity : entities) {
                                  if (entity.type == EntityType::WALL &&
                                      CheckCollisionRecs(bulletRect, entity.getRect())) {
                                      return true;  // Remove this bullet
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

        // --- DRAWING LOGIC ---
        BeginDrawing();
        ClearBackground( RAYWHITE );

        // Step 1: Draw game scene normally
        BeginMode2D( camera );
        {
            // Draw the background grid
            // Calculate which grid cells are visible
            int startGridX = static_cast<int>(std::floor((camera.target.x - camera.offset.x / camera.zoom) / gridSize) * gridSize);
            int startGridY = static_cast<int>(std::floor((camera.target.y - camera.offset.y / camera.zoom) / gridSize) * gridSize);
            int endGridX = static_cast<int>(std::ceil((camera.target.x + camera.offset.x / camera.zoom) / gridSize) * gridSize);
            int endGridY = static_cast<int>(std::ceil((camera.target.y + camera.offset.y / camera.zoom) / gridSize) * gridSize);

            // Draw vertical grid lines
            for (int i = startGridX; i <= endGridX; i += gridSize) {
                if (i >= 0 && i <= mapWidth) {
                    DrawLine( i, startGridY, i, endGridY, LIGHTGRAY );
                }
            }

            // Draw horizontal grid lines
            for (int i = startGridY; i <= endGridY; i += gridSize) {
                if (i >= 0 && i <= mapHeight) {
                    DrawLine( startGridX, i, endGridX, i, LIGHTGRAY );
                }
            }

            // Draw all walls
            for (const auto& entity : entities) {
                if (entity.type == EntityType::WALL) {
                    DrawRectangleRec(entity.getRect(), RED);
                }
            }

            // Draw all players
            for (const auto& entity : entities) {
                if (entity.type == EntityType::PLAYER) {
                    DrawRectangleRec(entity.getRect(), BLUE);
                }
            }

            // Draw all bullets
            for (const auto& entity : entities) {
                if (entity.type == EntityType::BULLET) {
                    DrawRectangleRec(entity.getRect(), YELLOW);
                }
            }
        }
        EndMode2D();

		// Step 2: Create visibility mask in render texture
		BeginTextureMode(maskTexture);
		ClearBackground(BLACK);

		constexpr float visionRadius = 10.0f * gridSize;
		const Vector2 playerScreenPos = GetWorldToScreen2D(player->pos, camera);

		// Draw white circle (visible area)
		DrawCircleV(playerScreenPos, visionRadius, WHITE);

		// Draw fade gradient
		BeginBlendMode(BLEND_ALPHA);
		for (int i = 1; i <= 30; i++) {
			const float currentRadius = visionRadius + (i * 1.5f);
			const unsigned char alpha = static_cast<unsigned char>(255 * (1.0f - (i / 30.0f)));
			DrawCircleV(playerScreenPos, currentRadius, Color{0, 0, 0, alpha});
		}
		EndBlendMode();

		EndTextureMode();

		// Step 3: Draw mask texture as overlay
		DrawTextureRec(maskTexture.texture, {0, 0, static_cast<float>(screenWidth), static_cast<float>(-screenHeight)}, {0, 0}, WHITE);

		// Draw UI on screen (not affected by camera)
		std::string positionText;
		positionText = "X: " + std::to_string(static_cast<int>(player->pos.x)) +
			" \n\nY: " + std::to_string(static_cast<int>(player->pos.y));
		const int textWidth = MeasureText(positionText.c_str(), 20);
		DrawText(positionText.c_str(), screenWidth - textWidth - 10, 10, 20, DARKGRAY);

        // Instructions
        DrawText( "Use WASD or Arrow Keys to move | Left Click to shoot", 10, 10, 20, DARKGRAY );

        EndDrawing();
    }

    // Cleanup
    UnloadRenderTexture(maskTexture);
    CloseWindow();
    return 0;
}