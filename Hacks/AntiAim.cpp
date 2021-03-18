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

bool lbyUpdate()
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

void AntiAim::run(UserCmd* cmd, const Vector& currentViewAngles, bool& sendPacket) noexcept
{
	if (!localPlayer) return;

	if (*memory->gameRules && (*memory->gameRules)->freezePeriod())
		return;

	if (cmd->buttons & UserCmd::IN_USE)
		return;

	if (Helpers::attacking(cmd->buttons & UserCmd::IN_ATTACK, cmd->buttons & UserCmd::IN_ATTACK2))
		return;

	if (localPlayer->moveType() == MoveType::NOCLIP || localPlayer->moveType() == MoveType::LADDER)
		return;

	if (static Helpers::KeyBindState flag; flag[config->antiAim.choke] && config->antiAim.chokedPackets)
		sendPacket = interfaces->engine->getNetworkChannel()->chokedPackets >= config->antiAim.chokedPackets;

	if (static Helpers::KeyBindState flag; config->antiAim.chokedPackets && flag[config->antiAim.fakeDuck])
	{
		cmd->buttons |= UserCmd::IN_BULLRUSH;

		if (interfaces->engine->getNetworkChannel()->chokedPackets > (config->antiAim.chokedPackets / 2))
			cmd->buttons |= UserCmd::IN_DUCK;
		else
			cmd->buttons &= ~UserCmd::IN_DUCK;
	}

	static bool flip = true;

	if (config->antiAim.flipKey && GetAsyncKeyState(config->antiAim.flipKey) & 1)
		flip = !flip;

	if (config->antiAim.pitch && cmd->viewangles.x == currentViewAngles.x)
		cmd->viewangles.x = config->antiAim.pitchAngle;

	if (static Helpers::KeyBindState choke; choke[config->antiAim.choke] && config->antiAim.desync && config->antiAim.chokedPackets && cmd->viewangles.y == currentViewAngles.y)
	{
		if (config->antiAim.extended)
		{
			if (lbyUpdate())
			{
				sendPacket = false;
				cmd->viewangles.y += flip ? 180.0f : -180.0f;
			} else if (!sendPacket)
			{
				cmd->viewangles.y += flip ? -120.0f : 120.0f;
			}
		} else
		{
			const float add = config->antiAim.clamped ? (cmd->tickCount & 1 ? 34.25f : 28.75f) : 0.0f;
			const float desyncAngle = flip ? localPlayer->getMaxDesyncAngle() - add : -localPlayer->getMaxDesyncAngle() + add;

			if (!sendPacket && (!config->antiAim.corrected || localPlayer->velocity().length2D() < 30.0f))
			{
				cmd->viewangles.y += desyncAngle;
			}

			if (fabsf(cmd->sidemove) < 5.0f)
			{
				if (localPlayer->flags() & Entity::FL_DUCKING)
					cmd->sidemove = cmd->tickCount & 1 ? 3.25f : -3.25f;
				else
					cmd->sidemove = cmd->tickCount & 1 ? 1.1f : -1.1f;
			}
		}
	}

	if (config->antiAim.yaw)
		cmd->viewangles.y += config->antiAim.yawAngle;
}
