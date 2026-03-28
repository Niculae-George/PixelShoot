//main.cpp
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "raylib.h"
#include "entity.h"
#include "world.h"
#include "stage_manager.h"
#include "stages.h"

int main() {
	// 1. Initialize Window
	constexpr int screenWidth = 1600;
	constexpr int screenHeight = 900;
	InitWindow( screenWidth , screenHeight , "PixelFight" );
	SetTargetFPS( 60 );

	StageManager sm;
	sm.ChangeStage(std::make_unique<MenuStage>(&sm, screenWidth, screenHeight));

	while (!WindowShouldClose() && !sm.ShouldQuit()) {
		const float deltaTime = GetFrameTime();

		if (sm.HasStage()) {
			sm.Update(deltaTime);
			sm.Draw();
		} else {
			break;
		}
	}

	CloseWindow();
	return 0;
}