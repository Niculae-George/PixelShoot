#include "menu.h"

Menu::Menu(int screenWidth, int screenHeight)
    : screenWidth(screenWidth), screenHeight(screenHeight), 
      shouldStartGame(false), shouldQuit(false), isHost(false) 
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
        "Host Lobby",
        DARKGRAY, LIGHTGRAY, WHITE
    ));

    buttons.push_back(Button(
        {(float)centerX, (float)(startY + buttonHeight + 20), (float)buttonWidth, (float)buttonHeight},
        "Join Lobby",
        DARKGRAY, LIGHTGRAY, WHITE
    ));

    buttons.push_back(Button(
        {(float)centerX, (float)(startY + (buttonHeight + 20) * 2), (float)buttonWidth, (float)buttonHeight},
        "Quit",
        DARKGRAY, LIGHTGRAY, WHITE
    ));
}

void Menu::Update() {
    for (auto& btn : buttons) {
        btn.Update();
    }

    if (buttons[0].IsClicked()) {
        shouldStartGame = true; // For now treat Host as Start Game
        isHost = true;
    }
    if (buttons[1].IsClicked()) {
        shouldStartGame = true;
        isHost = false;
    }
    if (buttons[2].IsClicked()) {
        shouldQuit = true;
    }

    // IP Input Logic
    int startY = screenHeight / 2 - 50;
    Rectangle ipBox = {(float)(screenWidth - 300) / 2, (float)(startY + 210), 300, 40};

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        typingIP = CheckCollisionPointRec(mousePos, ipBox);
    }

    if (typingIP) {
        int key = GetCharPressed();
        while (key > 0) {
            // allow numbers and dots
            if (((key >= '0' && key <= '9') || key == '.') && targetIP.length() < 15) {
                targetIP += (char)key;
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE) && targetIP.length() > 0) {
            targetIP.pop_back();
        }
    }
}

void Menu::Draw() {
    ClearBackground(RAYWHITE);

    int titleWidth = MeasureText("PixelFight", 60);
    DrawText("PixelFight", (screenWidth - titleWidth) / 2, screenHeight / 3 - 50, 60, DARKGRAY);

    for (const auto& btn : buttons) {
        btn.Draw();
    }

    // Draw IP Input Box
    int startY = screenHeight / 2 - 50;
    Rectangle ipBox = {(float)(screenWidth - 300) / 2, (float)(startY + 210), 300, 40};
    DrawRectangleRec(ipBox, typingIP ? LIGHTGRAY : RAYWHITE);
    DrawRectangleLinesEx(ipBox, 2, typingIP ? BLUE : DARKGRAY);

    DrawText("Target IP (Join only):", ipBox.x, ipBox.y - 25, 20, DARKGRAY);

    std::string displayText = targetIP;
    if (typingIP && (static_cast<int>(GetTime() * 2) % 2 == 0)) {
        displayText += "_";
    }
    DrawText(displayText.c_str(), ipBox.x + 10, ipBox.y + 10, 20, DARKGRAY);
}
