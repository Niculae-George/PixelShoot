#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOGDI
#define NOUSER
#endif

#include "enet.h"

#ifdef _WIN32
#undef FAR
#undef NEAR
#undef PlaySound
#endif

// Initialize ENet at program start (if not already done)
struct ENetContext {
    ENetContext() {
        enet_initialize();
    }
    ~ENetContext() {
        enet_deinitialize();
    }
};

inline void InitENet() {
    static ENetContext ctx;
}
