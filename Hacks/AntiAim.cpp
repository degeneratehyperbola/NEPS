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

static bool canDoFakePitch = false;

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
	canDoFakePitch = false;

	if (!localPlayer)
		return;

	if (*memory->gameRules && (*memory->gameRules)->freezePeriod())
		return;

	if (cmd->buttons & UserCmd::IN_USE)
		return;

	if (Helpers::attacking(cmd->buttons & UserCmd::IN_ATTACK, cmd->buttons & UserCmd::IN_ATTACK2))
		return;

	if (localPlayer->moveType() == MoveType::NOCLIP || localPlayer->moveType() == MoveType::LADDER)
		return;

	canDoFakePitch = true;

	if (static Helpers::KeyBindState flag; config->antiAim.fakeDuckPackets && flag[config->antiAim.fakeDuck])
	{
		sendPacket = interfaces->engine->getNetworkChannel()->chokedPackets >= config->antiAim.fakeDuckPackets;

		cmd->buttons |= UserCmd::IN_BULLRUSH;
		cmd->buttons &= ~UserCmd::IN_DUCK;

		if (interfaces->engine->getNetworkChannel()->chokedPackets > (config->antiAim.fakeDuckPackets / 2))
			cmd->buttons |= UserCmd::IN_DUCK;
	} else if (static Helpers::KeyBindState flag; flag[config->antiAim.choke] && config->antiAim.chokedPackets)
		sendPacket = interfaces->engine->getNetworkChannel()->chokedPackets >= config->antiAim.chokedPackets;

	static bool flip = true;

	if (config->antiAim.flipKey && GetAsyncKeyState(config->antiAim.flipKey) & 1)
		flip = !flip;

	if (config->antiAim.pitch && cmd->viewangles.x == currentViewAngles.x)
		cmd->viewangles.x = config->antiAim.pitchAngle;

	if (config->antiAim.desync && cmd->viewangles.y == currentViewAngles.y)
	{
		const float add = config->antiAim.clamped ? 30.0f : 0.0f;
		const float desyncAngle = flip ? localPlayer->getMaxDesyncAngle() - add : -localPlayer->getMaxDesyncAngle() + add;

		if (config->antiAim.extended)
		{
			if (lbyUpdate())
			{
				cmd->viewangles.y -= desyncAngle;
				sendPacket = false;
			} else if (!sendPacket)
			{
				cmd->viewangles.y += desyncAngle;
			}
		} else
		{
			if (!sendPacket && (!config->antiAim.corrected || localPlayer->velocity().length2D() < 0.1f))
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

bool AntiAim::fakePitch(UserCmd *cmd) noexcept
{
	if (canDoFakePitch && config->antiAim.fakeUp)
	{
		cmd->viewangles.x = -540.0f;
		cmd->forwardmove = -cmd->forwardmove;
		return true;
	}

	return false;
}