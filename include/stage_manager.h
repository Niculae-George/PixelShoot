#pragma once
#include <memory>

class Stage {
public:
    virtual ~Stage() = default;
    virtual void Update(float deltaTime) = 0;
    virtual void Draw() = 0;
};

class StageManager {
public:
    StageManager() : shouldQuit(false) {}

    void ChangeStage(std::unique_ptr<Stage> newStage) {
        currentStage = std::move(newStage);
    }

    void Update(float deltaTime) {
        if (currentStage) {
            currentStage->Update(deltaTime);
        }
    }

    void Draw() {
        if (currentStage) {
            currentStage->Draw();
        }
    }

    bool HasStage() const {
        return currentStage != nullptr;
    }

    void Quit() {
        shouldQuit = true;
    }

    bool ShouldQuit() const {
        return shouldQuit;
    }

private:
    std::unique_ptr<Stage> currentStage;
    bool shouldQuit;
};
