#pragma once

struct UserCmd;
enum class FrameStage;
class GameEvent;
struct ViewSetup;

namespace Visuals
{
    void playerModel(FrameStage stage) noexcept;
    void colorWorld() noexcept;
    void modifySmoke(FrameStage stage) noexcept;
    void thirdperson() noexcept;
    void removeVisualRecoil(FrameStage stage) noexcept;
    void removeBlur(FrameStage stage) noexcept;
    void removeGrass(FrameStage stage) noexcept;
    void applyScreenEffects() noexcept;
    void hitEffect(GameEvent* event = nullptr) noexcept;
    void killEffect(GameEvent* event = nullptr) noexcept;
    void hitMarker(GameEvent* event = nullptr, ImDrawList* drawList = nullptr) noexcept;
    void disablePostProcessing(FrameStage stage) noexcept;
    void reduceFlashEffect() noexcept;
    bool removeHands(const char* modelName) noexcept;
    bool removeSleeves(const char* modelName) noexcept;
    bool removeWeapons(const char* modelName) noexcept;
    void skybox(FrameStage stage) noexcept;
    void bBeams(GameEvent *event);
    void drawMolotovHull(ImDrawList *drawList) noexcept;
}
