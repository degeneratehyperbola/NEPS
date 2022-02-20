#pragma once

enum class FrameStage;
enum class UserMessageType;
class GameEvent;
struct ImDrawList;
struct UserCmd;
struct Vector;

namespace Misc
{
	void edgeJump(UserCmd *cmd) noexcept;
	void slowwalk(UserCmd *cmd) noexcept;
	void autoPeek(UserCmd* cmd) noexcept;
	void visualizeQuickPeek(ImDrawList *drawlist) noexcept;
	void updateClanTag() noexcept;
	void overlayCrosshair(ImDrawList *drawlist) noexcept;
	void recoilCrosshair(ImDrawList *drawList) noexcept;
	void visualizeAccuracy(ImDrawList *drawList) noexcept;
	void prepareRevolver(UserCmd *) noexcept;
	void fastPlant(UserCmd *) noexcept;
	void fastStop(UserCmd *) noexcept;
	void stealNames() noexcept;
	void quickReload(UserCmd *) noexcept;
	bool changeName(bool, const char *, float) noexcept;
	void bunnyHop(UserCmd *) noexcept;
	void fakeBan() noexcept;
	void changeConVarsTick() noexcept;
	void changeConVarsFrame(FrameStage stage);
	void quickHealthshot(UserCmd *) noexcept;
	void fixTabletSignal() noexcept;
	void killMessage(GameEvent &event) noexcept;
	void fixMovement(UserCmd *cmd, float yaw) noexcept;
	void antiAfkKick(UserCmd *cmd) noexcept;
	void tweakPlayerAnimations() noexcept;
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
	void blockBot(UserCmd *cmd, const Vector &) noexcept;
	void visualizeBlockBot(ImDrawList *drawList) noexcept;
	void useSpam(UserCmd *cmd) noexcept;
	void onPlayerVote(GameEvent &event) noexcept;
	void onVoteChange(UserMessageType type, const void *data = nullptr, int size = 0) noexcept;
	void forceRelayCluster() noexcept;
	void runChatSpammer(unsigned char test = 0) noexcept;
	void fakePrime() noexcept;
	void velocityGraph() noexcept;
	void purchaseList(GameEvent *event = nullptr) noexcept;
	void teamDamageList(GameEvent *event = nullptr);
	void drawBombTimer() noexcept;
	void indicators() noexcept;
	void spectatorList() noexcept;
	void watermark() noexcept;
}
