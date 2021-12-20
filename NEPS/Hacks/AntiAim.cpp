#include "AntiAim.h"
#include "Backtrack.h"

#include "../GameData.h"
#include "../Memory.h"
#include "../Interfaces.h"
#include "../lib/ImguiCustom.hpp"
#include "../SDK/Engine.h"
#include "../SDK/EngineTrace.h"
#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/UserCmd.h"
#include "../SDK/GlobalVars.h"

static bool canAntiAim(UserCmd *cmd) noexcept
{
	if (!localPlayer)
		return false;

	if (*memory->gameRules && (*memory->gameRules)->freezePeriod())
		return false;

	if (cmd->buttons & UserCmd::Button_Use || !localPlayer->isAlive())
		return false;

	if (localPlayer->moveType() == MoveType::Noclip || localPlayer->moveType() == MoveType::Ladder)
		return false;

	return true;
}

static void microMovement(UserCmd *cmd) noexcept
{
	if (std::fabsf(cmd->sidemove) < 5.0f)
	{
		if (localPlayer->flags() & PlayerFlag_Crouched)
			cmd->sidemove = cmd->tickCount & 1 ? 3.25f : -3.25f;
		else
			cmd->sidemove = cmd->tickCount & 1 ? 1.1f : -1.1f;
	}
}

static signed char dir = 0;
static bool flip = true;

void AntiAim::run(UserCmd* cmd, const Vector& currentViewAngles, bool& sendPacket) noexcept
{
	if (!canAntiAim(cmd)) return;

	const auto networkChannel = interfaces->engine->getNetworkChannel();
	if (!networkChannel)
		return;

	const auto &cfg = Config::AntiAim::getRelevantConfig();
	const auto time = memory->globalVars->serverTime();

	bool fakeDucking = false;
	if (static Helpers::KeyBindState flag; config->exploits.fakeDuckPackets && flag[config->exploits.fakeDuck])
	{
		sendPacket = networkChannel->chokedPackets >= config->exploits.fakeDuckPackets;

		cmd->buttons |= UserCmd::Button_Bullrush;
		cmd->buttons &= ~UserCmd::Button_Duck;

		if (networkChannel->chokedPackets < config->exploits.fakeDuckPackets / 2 || networkChannel->chokedPackets > config->exploits.fakeDuckPackets / 2 + 3)
			cmd->buttons &= ~UserCmd::Button_Attack;

		if (networkChannel->chokedPackets > (config->exploits.fakeDuckPackets / 2))
			cmd->buttons |= UserCmd::Button_Duck;

		fakeDucking = true;
	}
	
	if (Helpers::attacking(cmd->buttons & UserCmd::Button_Attack, cmd->buttons & UserCmd::Button_Attack2))
		return;

	if (static Helpers::KeyBindState flag; flag[cfg.choke] && cfg.chokedPackets && !fakeDucking)
	{
		if (interfaces->engine->isVoiceRecording())
			sendPacket = networkChannel->chokedPackets >= std::min(3, cfg.chokedPackets);
		else
			sendPacket = networkChannel->chokedPackets >= cfg.chokedPackets;
	}

	if (cfg.pitch && cmd->viewangles.x == currentViewAngles.x)
		cmd->viewangles.x = cfg.pitchAngle;

	if (cfg.lookAtEnemies && cmd->viewangles.y == currentViewAngles.y)
	{
		Entity *bestTarget = nullptr;
		auto bestFov = 255.0f;
		auto bestAngle = 0.0f;

		for (int i = 1; i <= interfaces->engine->getMaxClients(); i++)
		{
			auto entity = interfaces->entityList->getEntity(i);

			if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive())
				continue;

			if (!localPlayer->isOtherEnemy(entity))
				continue;

			const auto angle = Helpers::calculateRelativeAngle(localPlayer->getEyePosition(), entity->getBonePosition(0), cmd->viewangles);

			const auto fov = std::hypot(angle.x, angle.y);
			if (fov < bestFov)
			{
				bestTarget = entity;
				bestFov = fov;
				bestAngle = angle.y;
			}
		}

		cmd->viewangles.y += bestAngle;
	}

	switch (cfg.direction)
	{
	case 0:
		dir = 0;
		break;
	case 1:
	{
		constexpr std::array positions = {-35.0f, 0.0f, 35.0f};
		std::array active = {false, false, false};
		const auto fwd = Vector::fromAngle2D(cmd->viewangles.y);
		const auto side = fwd.crossProduct(Vector::up());

		for (std::size_t i = 0; i < positions.size(); ++i)
		{
			const auto start = localPlayer->getEyePosition() + side * positions[i];
			const auto end = start + fwd * 100.0f;

			Trace trace;
			interfaces->engineTrace->traceRay({start, end}, CONTENTS_SOLID | CONTENTS_WINDOW, nullptr, trace);

			if (trace.fraction != 1.0f)
				active[i] = true;
		}

		if (active[0] && active[1] && !active[2])
			dir = -1;
		else if (!active[0] && active[1] && active[2])
			dir = 1;
		else
			dir = 0;
	}
		break;
	case 2:
	{
		if (cfg.rightKey && GetAsyncKeyState(cfg.rightKey))
			dir = -1;

		if (cfg.backKey && GetAsyncKeyState(cfg.backKey))
			dir = 0;

		if (cfg.leftKey && GetAsyncKeyState(cfg.leftKey))
			dir = 1;
	}
		break;
	}

	cmd->viewangles.y += 90.0f * dir;

	static float nextLbyUpdate;
	const bool lbyUpdate = Helpers::lbyUpdate(localPlayer.get(), nextLbyUpdate);

	if (cfg.desync)
	{
		float a = 0.0f;
		float b = 0.0f;
		switch (cfg.desync)
		{
		case 1:
			b = flip ? 120.0f : -120.0f;
			break;
		case 2:
			a = flip ? -120.0f : 120.0f;
			b = flip ? 120.0f : -120.0f;
			break;
		case 3:
			a = flip ? -120.0f : 120.0f;
			b = flip ? 60.0f : -60.0f;
			break;
		case 4:
		case 5:
			a = flip ? 120.0f : -120.0f;
			b = flip ? 120.0f : -120.0f;
			break;
		}

		if (cfg.flipKey && GetAsyncKeyState(cfg.flipKey) & 1 || cfg.desync == 5 && lbyUpdate)
			flip = !flip;

		if (cfg.desync == 1)
		{
			microMovement(cmd);

			if (!sendPacket)
				cmd->viewangles.y += b;
		}
		else if (lbyUpdate)
		{
			sendPacket = false;
			cmd->viewangles.y += a;
		} else if (!sendPacket)
			cmd->viewangles.y += b;
	}

	if (cfg.yaw)
		cmd->viewangles.y += cfg.yawAngle;
}

bool AntiAim::fakePitch(UserCmd *cmd) noexcept
{
	if (!canAntiAim(cmd))
		return false;

	const auto &cfg = Config::AntiAim::getRelevantConfig();

	if (cfg.fakeUp && !Helpers::attacking(cmd->buttons & UserCmd::Button_Attack, cmd->buttons & UserCmd::Button_Attack2))
	{
		cmd->viewangles.x = -540.0f;
		cmd->forwardmove = -cmd->forwardmove;
		return true;
	}

	return false;
}

void AntiAim::visualize(ImDrawList *drawList) noexcept
{
	if (!localPlayer)
		return;

	if (!localPlayer->isAlive())
		return;

	if (localPlayer->moveType() == MoveType::Noclip || localPlayer->moveType() == MoveType::Ladder)
		return;

	const auto &cfg = Config::AntiAim::getRelevantConfig();

	if (cfg.visualizeDirection.enabled && cfg.direction)
	{
		const auto color = Helpers::calculateColor(cfg.visualizeDirection);
		switch (dir)
		{
		case -1:
			ImGuiCustom::drawTriangleFromCenter(drawList, {-200, 0}, color, cfg.visualizeDirection.outline);
			break;
		case 0:
			ImGuiCustom::drawTriangleFromCenter(drawList, {0, 100}, color, cfg.visualizeDirection.outline);
			break;
		case 1:
			ImGuiCustom::drawTriangleFromCenter(drawList, {200, 0}, color, cfg.visualizeDirection.outline);
			break;
		}
	}

	if (cfg.visualizeSide.enabled && cfg.desync)
	{
		const auto color = Helpers::calculateColor(cfg.visualizeSide);
		if (flip)
			ImGuiCustom::drawTriangleFromCenter(drawList, {100, 0}, color, cfg.visualizeSide.outline);
		else
			ImGuiCustom::drawTriangleFromCenter(drawList, {-100, 0}, color, cfg.visualizeSide.outline);
	}
}
