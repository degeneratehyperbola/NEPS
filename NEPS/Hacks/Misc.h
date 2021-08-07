#pragma once

enum class FrameStage;
class GameEvent;
struct ImDrawList;
struct UserCmd;

namespace Misc
{
	void edgeJump(UserCmd *cmd) noexcept;
	void slowwalk(UserCmd *cmd) noexcept;
	void updateClanTag() noexcept;
	void overlayCrosshair(ImDrawList *drawlist) noexcept;
	void recoilCrosshair(ImDrawList *drawList) noexcept;
    void visualizeInaccuracy(ImDrawList *drawList) noexcept;
	void prepareRevolver(UserCmd *) noexcept;
	void fastPlant(UserCmd *) noexcept;
	void fastStop(UserCmd *) noexcept;
	void stealNames() noexcept;
	void quickReload(UserCmd *) noexcept;
	bool changeName(bool, const char *, float) noexcept;
	void bunnyHop(UserCmd *) noexcept;
	void fakeBan(bool = false) noexcept;
	void changeConVarsTick() noexcept;
	void changeConVarsFrame(FrameStage stage);
	void quickHealthshot(UserCmd *) noexcept;
	void fixTabletSignal() noexcept;
	void killMessage(GameEvent &event) noexcept;
	void fixMovement(UserCmd *cmd, float yaw) noexcept;
	void antiAfkKick(UserCmd *cmd) noexcept;
	void tweakPlayerAnim(FrameStage stage) noexcept;
	void autoPistol(UserCmd *cmd) noexcept;
	void autoReload(UserCmd *cmd) noexcept;
	void revealRanks(UserCmd *cmd) noexcept;
	void autoStrafe(UserCmd *cmd) noexcept;
	void removeCrouchCooldown(UserCmd *cmd) noexcept;
	void moonwalk(UserCmd *cmd) noexcept;
	void playHitSound(GameEvent &event) noexcept;
	void playKillSound(GameEvent &event) noexcept;
    void playDeathSound(GameEvent &event) noexcept;
	void runReportbot() noexcept;
	void resetReportbot() noexcept;
	void preserveKillfeed(bool roundStart = false) noexcept;
	void blockBot(UserCmd *cmd) noexcept;
	void visualizeBlockBot(ImDrawList *drawList) noexcept;
	void useSpam(UserCmd *cmd) noexcept;
	void indicators(ImDrawList *drawList) noexcept;
	void drawBombTimer() noexcept;
	void purchaseList(GameEvent *event = nullptr) noexcept;
	void teamDamageList(GameEvent *event = nullptr);
	void spectatorList() noexcept;
	void watermark() noexcept;
	void voteRevealer(GameEvent &event) noexcept;
	void forceRelayCluster() noexcept;
}
