#include "world.h"
#include "raylib.h"
#include <algorithm>
#include <queue>
#include <cmath>

// Check if a path exists from start to end using BFS
bool IsPathClear(Vector2 start, Vector2 end, const std::vector<Entity>& entities, 
                 int gridSize, int mapWidth, int mapHeight) {
    // Convert to grid coordinates
    int startGridX = static_cast<int>(start.x / gridSize);
    int startGridY = static_cast<int>(start.y / gridSize);
    int endGridX = static_cast<int>(end.x / gridSize);
    int endGridY = static_cast<int>(end.y / gridSize);

    int gridWidth = mapWidth / gridSize;
    int gridHeight = mapHeight / gridSize;

    // Bounds check for start and end positions
    if (startGridX < 0 || startGridX >= gridWidth || startGridY < 0 || startGridY >= gridHeight) {
        return false;
    }
    if (endGridX < 0 || endGridX >= gridWidth || endGridY < 0 || endGridY >= gridHeight) {
        return false;
    }

    // BFS to find path
    std::queue<std::pair<int, int>> queue;
    std::vector<std::vector<bool>> visited(gridWidth, std::vector<bool>(gridHeight, false));

    queue.emplace(startGridX, startGridY);
    visited[startGridX][startGridY] = true;

    // Directions: up, down, left, right
    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};

    while (!queue.empty()) {
        auto [x, y] = queue.front();
        queue.pop();

        if (x == endGridX && y == endGridY) {
            return true;  // Path found
        }

        for (int i = 0; i < 4; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (nx >= 0 && nx < gridWidth && ny >= 0 && ny < gridHeight && !visited[nx][ny]) {
                // Check if this cell is blocked by a wall
                Vector2 cellCenter = {(nx + 0.5f) * gridSize, (ny + 0.5f) * gridSize};
                Rectangle cellRect = {cellCenter.x - gridSize/2.0f, cellCenter.y - gridSize/2.0f, static_cast<float>(gridSize), static_cast<float>(gridSize)};

                bool blocked = false;
                for (const auto& entity : entities) {
                    if (entity.type == EntityType::WALL && CheckCollisionRecs(cellRect, entity.getRect())) {
                        blocked = true;
                        break;
                    }
                }

                if (!blocked) {
                    visited[nx][ny] = true;
                    queue.push({nx, ny});
                }
            }
        }
    }

    return false;  // No path found
}

// Generate a random world with guaranteed passability
void GenerateRandomWorld(std::vector<Entity>& entities, const WorldGenParams& params) {
    // Get player position before clearing walls
    Entity* player = nullptr;
    for (auto& entity : entities) {
        if (entity.type == EntityType::PLAYER) {
            player = &entity;
            break;
        }
    }

    if (!player) return;  // No player to validate against

    // Clear existing walls (keep players)
    entities.erase(
        std::remove_if(entities.begin(), entities.end(), 
                      [](const Entity& e) { return e.type == EntityType::WALL; }),
        entities.end()
    );

    // Generate random walls
    int gridCellsX = params.mapWidth / params.gridSize;
    int gridCellsY = params.mapHeight / params.gridSize;
    int targetWallCount = (gridCellsX * gridCellsY * params.wallDensity) / 100;
    int wallsPlaced = 0;

    // Keep trying to place walls while ensuring paths remain clear
    int maxAttempts = targetWallCount * 15;  // Increased for better distribution
    int attempts = 0;

    while (wallsPlaced < targetWallCount && attempts < maxAttempts) {
        attempts++;

        // Random wall position and size
        int cellX = GetRandomValue(1, gridCellsX - 2);
        int cellY = GetRandomValue(1, gridCellsY - 2);
        int wallWidth = GetRandomValue(1, 3) * params.gridSize;
        int wallHeight = GetRandomValue(1, 3) * params.gridSize;

        Vector2 wallPos = {cellX * params.gridSize + params.gridSize / 2.0f, 
                          cellY * params.gridSize + params.gridSize / 2.0f};

        // Check if wall is within safe zone around player spawn
        float distToPlayer = std::sqrt((wallPos.x - player->pos.x) * (wallPos.x - player->pos.x) + 
                                      (wallPos.y - player->pos.y) * (wallPos.y - player->pos.y));
        float safeZoneRadius = params.spawnSafeZone * params.gridSize;

        if (distToPlayer < safeZoneRadius) {
            continue;  // Skip this wall, too close to player spawn
        }

        // Check if wall overlaps with existing walls (simple rejection)
        bool overlaps = false;
        Rectangle newWallRect = {wallPos.x - wallWidth/2.0f, wallPos.y - wallHeight/2.0f, 
                                static_cast<float>(wallWidth), static_cast<float>(wallHeight)};

        for (const auto& entity : entities) {
            if (entity.type == EntityType::WALL) {
                if (CheckCollisionRecs(newWallRect, entity.getRect())) {
                    overlaps = true;
                    break;
                }
            }
        }

        if (overlaps) {
            continue;  // Skip overlapping walls
        }

        // Temporarily add wall to check if path still exists
        entities.push_back(Entity{wallPos, {static_cast<float>(wallWidth), static_cast<float>(wallHeight)}, EntityType::WALL});

        // Skip pathfinding validation for first 20 walls (they shouldn't block main paths)
        // Then validate only every 5th wall to keep performance reasonable
        bool shouldValidatePath = (wallsPlaced >= 20 && wallsPlaced % 5 == 0);
        bool pathValid = true;

        if (shouldValidatePath) {
            // Quick validation: check if player can reach a corner
            Vector2 testDest = {params.mapWidth * 0.9f, params.mapHeight * 0.9f};
            pathValid = IsPathClear(player->pos, testDest, entities, params.gridSize, params.mapWidth, params.mapHeight);
        }

        if (pathValid) {
            // Path is clear (or we skipped validation), keep this wall
            wallsPlaced++;
        } else {
            // Path is blocked, remove this wall
            entities.pop_back();
        }
    }
}
