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
	auto time = memory->globalVars->serverTime();
	static float nextLby;
	
	if (~localPlayer->flags() & Entity::FL_ONGROUND)
	{
		return false;
	} else if (localPlayer->velocity().length2D() > 0.1f)
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

	if (Helpers::attacking(cmd->buttons & UserCmd::IN_ATTACK, cmd->buttons & UserCmd::IN_ATTACK2))
		return false;

	if (localPlayer->moveType() == MoveType::NOCLIP || localPlayer->moveType() == MoveType::LADDER)
		return false;

	return true;
}

static void forceLbyUpdate(UserCmd *cmd) noexcept
{
	if (fabsf(cmd->sidemove) < 5.0f)
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

	const auto &cfg = config->antiAim;

	if (static Helpers::KeyBindState flag; cfg.fakeDuckPackets && flag[cfg.fakeDuck])
	{
		sendPacket = interfaces->engine->getNetworkChannel()->chokedPackets >= cfg.fakeDuckPackets;

		cmd->buttons |= UserCmd::IN_BULLRUSH;
		cmd->buttons &= ~UserCmd::IN_DUCK;

		if (interfaces->engine->getNetworkChannel()->chokedPackets > (cfg.fakeDuckPackets / 2))
			cmd->buttons |= UserCmd::IN_DUCK;
	} else if (static Helpers::KeyBindState flag; flag[cfg.choke] && cfg.chokedPackets)
		sendPacket = interfaces->engine->getNetworkChannel()->chokedPackets >= cfg.chokedPackets;

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
				cmd->viewangles.y += angle.y;
			}
		}
	}

	if (cfg.desync)
	{
		float fake = flip ? cfg.fakeYaw : -cfg.fakeYaw;
		float real = flip ? cfg.realYaw : -cfg.realYaw;
		const bool fakeLessThanReal = cfg.lbyBreaker ? fake < real : 0.0f < real;
		fake += fakeLessThanReal ? -100.0f : 100.0f;
		real += fakeLessThanReal ? 60.0f : -60.0f;

		if (cfg.lbyBreaker)
		{
			if (lbyUpdate())
			{
				sendPacket = false;
				cmd->viewangles.y += fake;
			} else if (!sendPacket)
			{
				cmd->viewangles.y += real;
			}
		} else
		{
			if (!sendPacket)
			{
				cmd->viewangles.y += real;
			}

			forceLbyUpdate(cmd);
		}
	}

	if (cfg.yaw)
		cmd->viewangles.y += cfg.yawAngle;
}

bool AntiAim::fakePitch(UserCmd *cmd) noexcept
{
	if (canAntiAim(cmd) && config->antiAim.fakeUp)
	{
		cmd->viewangles.x = -540.0f;
		cmd->forwardmove = -cmd->forwardmove;
		return true;
	}

	return false;
}