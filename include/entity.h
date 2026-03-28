#pragma once

#include <vector>
#include "raylib.h"

// Entity types
enum class EntityType {
    PLAYER,
    WALL,
    BULLET
};

// Simple entity structure
struct Entity {
    Vector2 pos;
    Vector2 size;
    EntityType type;
    Vector2 velocity = {0, 0};  // For bullets

    Rectangle getRect() const {
        return {pos.x - size.x / 2, pos.y - size.y / 2, size.x, size.y};
    }
};

// Collision detection
inline bool CheckEntityCollision(const Rectangle& rect, const std::vector<Entity>& entities, EntityType typeToCheck) {
    for (const auto& entity : entities) {
        if (entity.type == typeToCheck) {
            if (CheckCollisionRecs(rect, entity.getRect())) {
                return true;
            }
        }
    }
    return false;
}

// Get player entity (assumes first entity is the player)
inline Entity* GetPlayerEntity(std::vector<Entity>& entities) {
    for (auto& entity : entities) {
        if (entity.type == EntityType::PLAYER) {
            return &entity;
        }
    }
    return nullptr;
}
