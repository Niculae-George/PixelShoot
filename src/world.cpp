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
    // Clear existing walls and players since we will regenerate based on territories
    entities.erase(
        std::remove_if(entities.begin(), entities.end(), 
                      [](const Entity& e) { return e.type == EntityType::WALL || e.type == EntityType::PLAYER; }),
        entities.end()
    );

    int maxPlayers = params.numPlayers;
    if (maxPlayers < 1) maxPlayers = 1;

    // The number of territories must be even.
    int T = maxPlayers % 2 == 0 ? maxPlayers : maxPlayers + 1;
    if (T < 2) T = 2;

    int cols = T == 2 ? 2 : T / 2;
    int rows = T == 2 ? 1 : 2;

    int gridCellsX = params.mapWidth / params.gridSize;
    int gridCellsY = params.mapHeight / params.gridSize;

    int terrCellsX = gridCellsX / cols;
    int terrCellsY = gridCellsY / rows;

    float terrWidth = terrCellsX * params.gridSize;
    float terrHeight = terrCellsY * params.gridSize;

    std::vector<Vector2> playerSpawns;

    for (int i = 0; i < T; i++) {
        int col = i % cols;
        int row = i / rows; // Wait, row = i / cols; my bad, wait

        row = i / cols;

        float centerX = col * terrWidth + terrWidth / 2.0f;
        float centerY = row * terrHeight + terrHeight / 2.0f;

        if (i < maxPlayers) {
            float pSize = 32.0f;
            entities.push_back(Entity{{centerX, centerY}, {pSize, pSize}, EntityType::PLAYER});
            playerSpawns.push_back({centerX, centerY});
        }
    }

    // Vertical borders
    for (int col = 1; col < cols; col++) {
        int borderX = col * terrCellsX;
        for (int row = 0; row < rows; row++) {
            int startY = row * terrCellsY;
            int endY = startY + terrCellsY;

            int gapSize = std::max(3, params.minPathWidth + 2);
            int gapStart = GetRandomValue(startY + 2, endY - gapSize - 2);

            for (int y = startY; y < endY; y++) {
                if (y >= gapStart && y < gapStart + gapSize) continue;

                Vector2 wallPos = {borderX * params.gridSize + params.gridSize / 2.0f,
                                   y * params.gridSize + params.gridSize / 2.0f};
                entities.push_back(Entity{wallPos, {(float)params.gridSize, (float)params.gridSize}, EntityType::WALL});
            }
        }
    }

    // Horizontal borders
    for (int row = 1; row < rows; row++) {
        int borderY = row * terrCellsY;
        for (int col = 0; col < cols; col++) {
            int startX = col * terrCellsX;
            int endX = startX + terrCellsX;

            int gapSize = std::max(3, params.minPathWidth + 2);
            int gapStart = GetRandomValue(startX + 2, endX - gapSize - 2);

            for (int x = startX; x < endX; x++) {
                if (x >= gapStart && x < gapStart + gapSize) continue;
                if (col > 0 && x == col * terrCellsX) continue; // Avoid intersection overlaps

                Vector2 wallPos = {x * params.gridSize + params.gridSize / 2.0f,
                                   borderY * params.gridSize + params.gridSize / 2.0f};
                entities.push_back(Entity{wallPos, {(float)params.gridSize, (float)params.gridSize}, EntityType::WALL});
            }
        }
    }

    // Generate random walls
    int targetWallCount = (gridCellsX * gridCellsY * params.wallDensity) / 100;
    int wallsPlaced = 0;
    int maxAttempts = targetWallCount * 15;  
    int attempts = 0;

    while (wallsPlaced < targetWallCount && attempts < maxAttempts) {
        attempts++;

        int cellX = GetRandomValue(1, gridCellsX - 2);
        int cellY = GetRandomValue(1, gridCellsY - 2);

        // Avoid borders
        bool onBorder = false;
        for (int c = 1; c < cols; c++) if (cellX == c * terrCellsX) onBorder = true;
        for (int r = 1; r < rows; r++) if (cellY == r * terrCellsY) onBorder = true;
        if (onBorder) continue;

        int wallWidth = GetRandomValue(1, 3) * params.gridSize;
        int wallHeight = GetRandomValue(1, 3) * params.gridSize;

        Vector2 wallPos = {cellX * params.gridSize + params.gridSize / 2.0f, 
                           cellY * params.gridSize + params.gridSize / 2.0f};

        // Check distance to all player spawns
        bool nearSpawn = false;
        float safeZoneRadius = params.spawnSafeZone * params.gridSize;
        for (const auto& spawn : playerSpawns) {
            float distToPlayer = std::sqrt((wallPos.x - spawn.x) * (wallPos.x - spawn.x) + 
                                          (wallPos.y - spawn.y) * (wallPos.y - spawn.y));
            if (distToPlayer < safeZoneRadius) {
                nearSpawn = true;
                break;
            }
        }

        if (nearSpawn) continue;

        Rectangle newWallRect = {wallPos.x - wallWidth/2.0f, wallPos.y - wallHeight/2.0f, 
                                static_cast<float>(wallWidth), static_cast<float>(wallHeight)};

        bool overlaps = false;
        for (const auto& entity : entities) {
            if (entity.type == EntityType::WALL) {
                if (CheckCollisionRecs(newWallRect, entity.getRect())) {
                    overlaps = true;
                    break;
                }
            }
        }

        if (overlaps) {
            continue;
        }

        // Temporarily add wall to check if path still exists
        entities.push_back(Entity{wallPos, {static_cast<float>(wallWidth), static_cast<float>(wallHeight)}, EntityType::WALL});

        bool shouldValidatePath = (wallsPlaced >= 20 && wallsPlaced % 5 == 0);
        bool pathValid = true;

        if (shouldValidatePath && playerSpawns.size() > 1) {
            // Validate connection between the first and last spawned player
            pathValid = IsPathClear(playerSpawns[0], playerSpawns.back(), entities, params.gridSize, params.mapWidth, params.mapHeight);
        }

        if (pathValid) {
            wallsPlaced++;
        } else {
            entities.pop_back();
        }
    }
}
