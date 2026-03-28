#include "menu.h"

Menu::Menu(int screenWidth, int screenHeight)
    : screenWidth(screenWidth), screenHeight(screenHeight), 
      shouldStartGame(false), shouldQuit(false) 
{
    CreateButtons();
}

Menu::~Menu() {
}

void Menu::CreateButtons() {
    int buttonWidth = 200;
    int buttonHeight = 50;
    int centerX = (screenWidth - buttonWidth) / 2;
    int startY = screenHeight / 2 - 50;

    buttons.push_back(Button(
        {(float)centerX, (float)startY, (float)buttonWidth, (float)buttonHeight},
        "Start Game",
        DARKGRAY, LIGHTGRAY, WHITE
    ));

    buttons.push_back(Button(
        {(float)centerX, (float)(startY + buttonHeight + 20), (float)buttonWidth, (float)buttonHeight},
        "Quit",
        DARKGRAY, LIGHTGRAY, WHITE
    ));
}

void Menu::Update() {
    for (auto& btn : buttons) {
        btn.Update();
    }

    if (buttons[0].IsClicked()) {
        shouldStartGame = true;
    }
    if (buttons[1].IsClicked()) {
        shouldQuit = true;
    }
}

void Menu::Draw() {
    ClearBackground(RAYWHITE);
    
    int titleWidth = MeasureText("PixelFight", 60);
    DrawText("PixelFight", (screenWidth - titleWidth) / 2, screenHeight / 3, 60, DARKGRAY);

    for (const auto& btn : buttons) {
        btn.Draw();
    }
}
