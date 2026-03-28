#pragma once

#include <vector>
#include <string>
#include "entity.h"

// World generation parameters
struct WorldGenParams {
    int mapWidth;
    int mapHeight;
    int gridSize;
    int wallDensity;        // Percentage of grid cells that should have walls (0-100)
    int minPathWidth;       // Minimum width of pathways (in grid cells)
    int spawnSafeZone;      // Safe zone radius around player spawn (in grid cells)
    int numPlayers;         // Number of players
    unsigned int seed;      // Seed for deterministic generation
};

// Check if a path exists from start to end using BFS
bool IsPathClear(Vector2 start, Vector2 end, const std::vector<Entity>& entities, 
                int gridSize, int mapWidth, int mapHeight);

// Generate a random world with guaranteed passability and return player spawn points
std::vector<Vector2> GenerateRandomWorld(std::vector<Entity>& outWalls, const WorldGenParams& params);
