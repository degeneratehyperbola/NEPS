#include "AntiAim.h"
#include "Backtrack.h"

#include "../GameData.h"
#include "../Memory.h"
#include "../Interfaces.h"
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

	if (cmd->buttons & UserCmd::IN_USE || !localPlayer->isAlive())
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

static const Config::AntiAim &getCurrentConfig()
{
	constexpr std::array categories = {"Freestand", "Slowwalk", "Run", "Airborne"};

	if (localPlayer->flags() & PlayerFlag_OnGround)
	{
		if (localPlayer->velocity().length2D() < 5.0f)
			return config->antiAim[categories[0]];
		else if (static Helpers::KeyBindState flag; flag[config->exploits.slowwalk])
			return config->antiAim[categories[1]];
		else
			return config->antiAim[categories[2]];
	} else
		return config->antiAim[categories[3]];
}

void AntiAim::run(UserCmd* cmd, const Vector& currentViewAngles, bool& sendPacket) noexcept
{
	if (!canAntiAim(cmd)) return;

	const auto networkChannel = interfaces->engine->getNetworkChannel();
	if (!networkChannel)
		return;

	const auto &cfg = getCurrentConfig();
	const auto time = memory->globalVars->serverTime();

	if (static Helpers::KeyBindState flag; config->exploits.fakeDuckPackets && flag[config->exploits.fakeDuck])
	{
		sendPacket = networkChannel->chokedPackets >= config->exploits.fakeDuckPackets;

		cmd->buttons |= UserCmd::IN_BULLRUSH;
		cmd->buttons &= ~UserCmd::IN_DUCK;

		if (networkChannel->chokedPackets < config->exploits.fakeDuckPackets / 2 || networkChannel->chokedPackets > config->exploits.fakeDuckPackets / 2 + 3)
			cmd->buttons &= ~UserCmd::IN_ATTACK;

		if (networkChannel->chokedPackets > (config->exploits.fakeDuckPackets / 2))
			cmd->buttons |= UserCmd::IN_DUCK;
	} else if (static Helpers::KeyBindState flag; flag[cfg.choke] && cfg.chokedPackets)
		sendPacket = networkChannel->chokedPackets >= cfg.chokedPackets;

	if (Helpers::attacking(cmd->buttons & UserCmd::IN_ATTACK, cmd->buttons & UserCmd::IN_ATTACK2))
		return;

	if (cfg.pitch && cmd->viewangles.x == currentViewAngles.x)
		cmd->viewangles.x = cfg.pitchAngle;

	if (cfg.hideHead && cmd->viewangles.y == currentViewAngles.y)
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

		bestAngle = 0.0f;
		constexpr std::array positions = {-18.0f, 18.0f};
		std::array active = {false, false};
		const auto fwd = Vector::fromAngle2D(cmd->viewangles.y);
		const auto side = fwd.crossProduct(Vector::up());

		for (std::size_t i = 0; i < positions.size(); ++i)
		{
			const auto start = localPlayer->getEyePosition() + side * positions[i];
			const auto end = start + fwd * 50.0f;

			Trace trace;
			interfaces->engineTrace->traceRay({start, end}, CONTENTS_SOLID | CONTENTS_WINDOW, localPlayer.get(), trace);

			if (trace.fraction != 1.0f)
				active[i] = true;
		}

		if (active[0] && !active[1])
			bestAngle = -90.0f;
		else if (!active[0] && active[1])
			bestAngle = 90.0f;

		cmd->viewangles.y += bestAngle;
	}

	static float nextLbyUpdate;
	const bool lbyUpdate = Helpers::lbyUpdate(localPlayer.get(), nextLbyUpdate);

	if (cfg.desync)
	{
		static bool flip = true;

		float a = 0.0f;
		float b = 0.0f;
		switch (cfg.desyncType)
		{
		case 0:
			b = flip ? 120.0f : -120.0f;
			break;
		case 1:
			a = flip ? -120.0f : 120.0f;
			b = flip ? 120.0f : -120.0f;
			break;
		case 2:
			a = flip ? -120.0f : 120.0f;
			b = flip ? 60.0f : -60.0f;
			break;
		case 3:
		case 4:
			a = flip ? 120.0f : -120.0f;
			b = flip ? 120.0f : -120.0f;
			break;
		}

		if (cfg.flipKey && GetAsyncKeyState(cfg.flipKey) & 1 || cfg.desyncType == 4 && lbyUpdate)
			flip = !flip;

		if (cfg.desyncType == 0)
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

	const auto &cfg = getCurrentConfig();

	if (cfg.fakeUp && !Helpers::attacking(cmd->buttons & UserCmd::IN_ATTACK, cmd->buttons & UserCmd::IN_ATTACK2))
	{
		cmd->viewangles.x = -540.0f;
		cmd->forwardmove = -cmd->forwardmove;
		return true;
	}

	return false;
}