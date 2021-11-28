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

#define M_PHI 1.61803398874989484820; // golden ratio

static bool canAntiAim(UserCmd *cmd) noexcept
{
	if (!localPlayer)
		return false;

	auto weapon = localPlayer->getActiveWeapon();
	auto weaponClass = getWeaponClass(localPlayer->getActiveWeapon()->itemDefinitionIndex2());

	if (!weapon)
		return false;

	if (weaponClass != 40 && cmd->buttons & (UserCmd::Button_Attack | UserCmd::Button_Attack2))
		return false;

	if (localPlayer->throwing(cmd))
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

	const auto& cfg = Config::AntiAim::getRelevantConfig();
	const auto time = memory->globalVars->serverTime();

	if (static Helpers::KeyBindState flag; !flag[cfg.enabled])
		return;

	if (cfg.legitAA)
		AntiAim::legit(cmd, currentViewAngles, sendPacket);

	if (static Helpers::KeyBindState flag; config->exploits.fakeDuckPackets && flag[config->exploits.fakeDuck])
	{
		sendPacket = networkChannel->chokedPackets >= config->exploits.fakeDuckPackets;

		cmd->buttons |= UserCmd::Button_Bullrush;
		cmd->buttons &= ~UserCmd::Button_Duck;

		if (networkChannel->chokedPackets < config->exploits.fakeDuckPackets / 2 || networkChannel->chokedPackets > config->exploits.fakeDuckPackets / 2 + 3)
			cmd->buttons &= ~UserCmd::Button_Attack;

		if (networkChannel->chokedPackets > (config->exploits.fakeDuckPackets / 2))
			cmd->buttons |= UserCmd::Button_Duck;
	}
	else if (static Helpers::KeyBindState flag; flag[cfg.choke] && cfg.chokedPackets)
	{
		if (interfaces->engine->isVoiceRecording())
			sendPacket = networkChannel->chokedPackets >= std::min(3, cfg.chokedPackets);
		else
			sendPacket = networkChannel->chokedPackets >= cfg.chokedPackets;
	}

	if (Helpers::attacking(cmd->buttons & UserCmd::Button_Attack, cmd->buttons & UserCmd::Button_Attack2))
		return;

	if (cfg.pitch && cmd->viewangles.x == currentViewAngles.x)
		cmd->viewangles.x = cfg.pitchAngle;

	if (cfg.lookAtEnemies && cmd->viewangles.y == currentViewAngles.y)
	{
		Entity* bestTarget = nullptr;
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
		constexpr std::array positions = { -35.0f, 0.0f, 35.0f };
		std::array active = { false, false, false };
		const auto fwd = Vector::fromAngle2D(cmd->viewangles.y);
		const auto side = fwd.crossProduct(Vector::up());

		for (std::size_t i = 0; i < positions.size(); ++i)
		{
			const auto start = localPlayer->getEyePosition() + side * positions[i];
			const auto end = start + fwd * 100.0f;

			Trace trace;
			interfaces->engineTrace->traceRay({ start, end }, CONTENTS_SOLID | CONTENTS_WINDOW, nullptr, trace);

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
		float leftDesyncAngle = cfg.leftLimit * 2.f;
		float rightDesyncAngle = cfg.rightLimit * 2.f;
		float a = 0.0f;
		float b = 0.0f;

		switch (cfg.desyncType)
		{
		case 0:
			b = flip ? leftDesyncAngle : -rightDesyncAngle;
			break;
		case 1:
			a = flip ? -leftDesyncAngle : rightDesyncAngle;
			b = flip ? leftDesyncAngle : -rightDesyncAngle;
			break;
		case 2:
			a = flip ? -leftDesyncAngle : rightDesyncAngle;
			b = flip ? leftDesyncAngle / 2.f : -rightDesyncAngle / 2.f;
			break;
		case 3:
		case 4:
			a = flip ? leftDesyncAngle : -rightDesyncAngle;
			b = flip ? leftDesyncAngle : -rightDesyncAngle;
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
		}
		else if (!sendPacket)
			cmd->viewangles.y += b;
	}

	if (cfg.yaw)
		cmd->viewangles.y += cfg.yawAngle;

	
	double factor;
	static float trigger;
	int random;
	int maxJitter;
	float temp;
	Vector temp_qangle;

	if (cfg.dance)
	{
		static float pDance;
		pDance += 45.0f;
		if (pDance > 100)
			pDance = 0.0f;
		else if (pDance > 75.f)
			cmd->viewangles.x = -89.f;
		else if (pDance < 75.f)
			cmd->viewangles.x = 89.f;
	}

	switch (cfg.AAType)
	{
	case 0:
		cmd->viewangles.y -= 0;
		break;
	case 1:
		random = rand() % 100;
		maxJitter = rand() % (85 - 70 + 1) + 70;
		temp = maxJitter - (rand() % maxJitter);
		if (random < 35 + (rand() % 15))
			cmd->viewangles.y -= temp;
		else if (random < 85 + (rand() % 15))
			cmd->viewangles.y += temp;
		break;
	case 2:
		factor = 360.0 / M_PHI;
		factor *= 25;
		cmd->viewangles.y = fmodf(memory->globalVars->currenttime * factor, 360.0);
		break;
	case 3:
		trigger += 10.0f;
		cmd->viewangles.y -= trigger > 50.f ? -60.f : 60.f;

		if (trigger > 100.0f)
			trigger = 0.0f;
		break;
	case 4:
		temp = cmd->viewangles.y + 90.0f;
		temp_qangle = Vector{ 0.0f, temp, 0.0f };
		temp_qangle.clamp();
		temp = temp_qangle.y;

		if (temp > -45.0f)
			temp < 0.0f ? temp = -90.0f : temp < 45.0f ? temp = 90.0f : temp = temp;

		temp += 1800000.0f;
		cmd->viewangles.y = temp;
		break;
	}
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

void AntiAim::legit(UserCmd* cmd, const Vector& currentViewAngles, bool& sendPacket) noexcept
{
	if (!canAntiAim(cmd))
		return;

	static float nextLbyUpdate;
	const bool lbyUpdate = Helpers::lbyUpdate(localPlayer.get(), nextLbyUpdate);
	const auto& cfg = Config::AntiAim::getRelevantConfig();

	if (cmd->viewangles.y == currentViewAngles.y)
	{
		static Helpers::KeyBindState flag;
		bool invert = flag[cfg.invert];
		float desyncAngle = localPlayer->getMaxDesyncAngle() * 2.f;
		if (lbyUpdate && cfg.extend)
		{
			cmd->viewangles.y += !invert ? desyncAngle : -desyncAngle;
			sendPacket = false;
			if (fabsf(cmd->sidemove) < 5.0f)
			{
				if (cmd->buttons & UserCmd::Button_Duck)
					cmd->sidemove = cmd->tickCount & 1 ? 3.25f : -3.25f;
				else
					cmd->sidemove = cmd->tickCount & 1 ? 1.1f : -1.1f;
			}
			return;
		}

		if (fabsf(cmd->sidemove) < 5.0f && !cfg.extend)
		{
			if (cmd->buttons & UserCmd::Button_Duck)
				cmd->sidemove = cmd->tickCount & 1 ? 3.25f : -3.25f;
			else
				cmd->sidemove = cmd->tickCount & 1 ? 1.1f : -1.1f;
		}

		if (sendPacket)
			return;

		cmd->viewangles.y += invert ? desyncAngle : -desyncAngle;
	}
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
			ImGuiCustom::drawTriangleFromCenter(drawList, {-200, 0}, color);
			break;
		case 0:
			ImGuiCustom::drawTriangleFromCenter(drawList, {0, 100}, color);
			break;
		case 1:
			ImGuiCustom::drawTriangleFromCenter(drawList, {200, 0}, color);
			break;
		}
	}

	if (cfg.visualizeSide.enabled && cfg.desync)
	{
		const auto color = Helpers::calculateColor(cfg.visualizeSide);
		if (flip)
			ImGuiCustom::drawTriangleFromCenter(drawList, {100, 0}, color);
		else
			ImGuiCustom::drawTriangleFromCenter(drawList, {-100, 0}, color);
	}
}
