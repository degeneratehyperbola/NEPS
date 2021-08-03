#include "AntiAim.h"

#include "../GameData.h"
#include "../Memory.h"
#include "../Interfaces.h"
#include "../SDK/Engine.h"
#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/UserCmd.h"
#include "../SDK/GlobalVars.h"

static bool lbyUpdate() noexcept
{
	if (!localPlayer)
		return false;

	auto time = memory->globalVars->serverTime();
	static float nextLby;
	
	if (localPlayer->velocity().length2D() > 0.1f || std::fabsf(localPlayer->velocity().z) > 100.0f)
	{
		nextLby = time + 0.22f;
		return false;
	} else if (time >= nextLby)
	{
		nextLby = time + 1.1f;
		return true;
	}
	
	return false;
}

static bool canAntiAim(UserCmd *cmd) noexcept
{
	if (!localPlayer)
		return false;

	if (*memory->gameRules && (*memory->gameRules)->freezePeriod())
		return false;

	if (cmd->buttons & UserCmd::IN_USE || !localPlayer->isAlive())
		return false;

	if (localPlayer->moveType() == MoveType::NOCLIP || localPlayer->moveType() == MoveType::LADDER)
		return false;

	return true;
}

static void microMovement(UserCmd *cmd) noexcept
{
	if (std::fabsf(cmd->sidemove) < 5.0f)
	{
		if (localPlayer->flags() & Entity::FL_DUCKING)
			cmd->sidemove = cmd->tickCount & 1 ? 3.25f : -3.25f;
		else
			cmd->sidemove = cmd->tickCount & 1 ? 1.1f : -1.1f;
	}
}

void AntiAim::run(UserCmd* cmd, const Vector& currentViewAngles, bool& sendPacket) noexcept
{
	if (!canAntiAim(cmd)) return;

	const auto networkChannel = interfaces->engine->getNetworkChannel();
	if (!networkChannel)
		return;

	const auto &cfg = config->antiAim;

	if (static Helpers::KeyBindState flag; cfg.fakeDuckPackets && flag[cfg.fakeDuck])
	{
		sendPacket = networkChannel->chokedPackets >= cfg.fakeDuckPackets;

		cmd->buttons |= UserCmd::IN_BULLRUSH;
		cmd->buttons &= ~UserCmd::IN_DUCK;

		if (networkChannel->chokedPackets < cfg.fakeDuckPackets / 2 || networkChannel->chokedPackets > cfg.fakeDuckPackets / 2 + 3)
			cmd->buttons &= ~UserCmd::IN_ATTACK;

		if (networkChannel->chokedPackets > (cfg.fakeDuckPackets / 2))
			cmd->buttons |= UserCmd::IN_DUCK;
	} else if (static Helpers::KeyBindState flag; flag[cfg.choke] && cfg.chokedPackets)
		sendPacket = networkChannel->chokedPackets >= cfg.chokedPackets;

	if (Helpers::attacking(cmd->buttons & UserCmd::IN_ATTACK, cmd->buttons & UserCmd::IN_ATTACK2))
		return;

	if (cfg.reduceSlide && localPlayer->velocity().length2D() > 10.0f)
		return;

	static bool flip = true;

	if (cfg.flipKey && GetAsyncKeyState(cfg.flipKey) & 1)
	{
		flip = !flip;
	}

	if (cfg.pitch && cmd->viewangles.x == currentViewAngles.x)
		cmd->viewangles.x = cfg.pitchAngle;

	if (cfg.lookAtEnemies && cmd->viewangles.y == currentViewAngles.y)
	{
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
				bestFov = fov;
				bestAngle = angle.y;
			}
		}

		cmd->viewangles.y += bestAngle;
	}

	if (cfg.desync)
	{
		const auto desync = localPlayer->getMaxDesyncAngle();
		float a = 0.0f;
		float b = flip ? 120.0f : -120.0f;
		switch (cfg.desyncType)
		{
		case 1: a = b < 0.0f ? -desync : desync; break;
		case 2: a = b > 0.0f ? -desync : desync; break;
		}
		const bool fakeLessThanReal = a < b;
		b += fakeLessThanReal ? 60.0f : -60.0f;

		if (cfg.desyncType && lbyUpdate())
		{
			sendPacket = false;
			cmd->viewangles.y += a;
		} else if (!cfg.desyncType) microMovement(cmd);

		if (!sendPacket)
		{
			cmd->viewangles.y += b;
		}
	}

	if (cfg.yaw)
		cmd->viewangles.y += cfg.yawAngle;
}

bool AntiAim::fakePitch(UserCmd *cmd) noexcept
{
	if (canAntiAim(cmd) && config->antiAim.fakeUp && !Helpers::attacking(cmd->buttons & UserCmd::IN_ATTACK, cmd->buttons & UserCmd::IN_ATTACK2))
	{
		cmd->viewangles.x = -540.0f;
		cmd->forwardmove = -cmd->forwardmove;
		return true;
	}

	return false;
}