#pragma once

#include "raylib.h"
#include <vector>
#include <string>

// Game states
enum class GameState {
    MENU,
    PLAYING,
    GAME_OVER,
    PAUSED
};

// Button structure
struct Button {
    Rectangle rect;
    std::string text;
    Color color;
    Color hoverColor;
    Color textColor;
    bool isHovered;

    Button(Rectangle r, const std::string& t, Color c, Color hc, Color tc)
        : rect(r), text(t), color(c), hoverColor(hc), textColor(tc), isHovered(false) {}

    void Update() {
        Vector2 mousePos = GetMousePosition();
        isHovered = CheckCollisionPointRec(mousePos, rect);
    }

    void Draw() const {
        DrawRectangleRec(rect, isHovered ? hoverColor : color);
        DrawRectangleLinesEx(rect, 2, textColor);

        int textWidth = MeasureText(text.c_str(), 20);
        int textX = rect.x + (rect.width - textWidth) / 2;
        int textY = rect.y + (rect.height - 20) / 2;
        DrawText(text.c_str(), textX, textY, 20, textColor);
    }

    bool IsClicked() const {
        return isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    }
};

class Menu {
public:
    Menu(int screenWidth, int screenHeight);
    ~Menu();

    void Update();
    void Draw();

    // State queries
    bool ShouldStartGame() const { return shouldStartGame; }
    bool ShouldQuit() const { return shouldQuit; }

    void ResetForGameStart() { shouldStartGame = false; }

private:
    int screenWidth;
    int screenHeight;

    // Buttons
    std::vector<Button> buttons;

    // State
    bool shouldStartGame;
    bool shouldQuit;

    void CreateButtons();
};
