#include "KnifeBot.h"

#include "../Config.h"
#include "../GUI.h"
#include "../GameData.h"
#include "../Interfaces.h"
#include "../SDK/Entity.h"
#include "../SDK/EngineTrace.h"


bool IsPlayerBehind(Entity* player) noexcept
{
	Vector pOrigin = player->getAbsOrigin();
	Vector toTarget = (localPlayer->getAbsOrigin() - pOrigin);
	Vector playerViewAngles;
	Vector::AngleVectors(player->eyeAngles(), playerViewAngles);
	if (toTarget.dotProduct(playerViewAngles) > -0.5f)
		return false;
	else
		return true;
}

int GetKnifeDamageDone(Entity* player) noexcept
{
	//damage: unarmored/armored
	//leftclick: 39/33
	//rightclick: 55/65
	//backstab leftclick: 90/76
	//backstab rightclick: 180/153
	bool backstab = IsPlayerBehind(player);
	int armor = player->armor();
	if (!backstab)
	{
		if (armor > 0)
			return 33; // 21
		else
			return 39; // 25
	}
	else
	{
		if (armor > 0)
			return 76; // 76
		else
			return 90; // 90
	}
}

int GetKnife2DamageDone(Entity* player) noexcept
{
	//damage: unarmored/armored
	//leftclick: 39/33
	//rightclick: 55/65
	//backstab leftclick: 90/76
	//backstab rightclick: 180/153
	bool backstab = IsPlayerBehind(player);
	int armor = player->armor();
	if (!backstab)
	{
		if (armor > 0)
			return 55;
		else
			return 65;
	}
	else
	{
		return 100;
	}
}

void KnifeBot::run(UserCmd* cmd) noexcept
{
	if (!interfaces->engine->isInGame())
		return;

	if (static Helpers::KeyBindState flag; !flag[config->misc.knifeBot.enabled])
		return;

	if (!localPlayer || !localPlayer->isAlive())
		return;

	if (!localPlayer->getActiveWeapon()  || !localPlayer->getActiveWeapon()->isKnife())
		return;

	const auto startPos = localPlayer->getEyePosition();
	const auto endPos = startPos + Vector::fromAngle(cmd->viewangles + localPlayer->getAimPunch()) * localPlayer->getActiveWeapon()->getWeaponData()->range;

	Trace trace;
	trace.startPos = localPlayer->origin();
	TraceFilter filter = trace.entity;
	filter.skip = localPlayer.get();
	interfaces->engineTrace->traceRay({ startPos, endPos }, MASK_SHOT, filter, trace);

	Entity* player = trace.entity;

	if (!player || !player->isPlayer() || player->isDormant() || !player->isAlive())
		return;

	if (!config->misc.knifeBot.friendly && !localPlayer->isOtherEnemy(trace.entity))
		return;

	float playerDistance = localPlayer->origin().distTo(player->origin());
	if (localPlayer->getActiveWeapon()->nextPrimaryAttack() < memory->globalVars->currenttime)
	{
		if (playerDistance <= 60.f && GetKnife2DamageDone(player) >= player->health())
			cmd->buttons |= UserCmd::Button_Attack2;
		else if (IsPlayerBehind(player) && playerDistance <= 60.f)
			cmd->buttons |= UserCmd::Button_Attack2;
		else if (playerDistance <= 73.f)
		{
			if (IsPlayerBehind(player))
				return;
			if (playerDistance <= 60.f &&
				(2 * (GetKnifeDamageDone(player)) + GetKnife2DamageDone(player) - 13) < player->health())
				cmd->buttons |= UserCmd::Button_Attack2;
			else
				cmd->buttons |= UserCmd::Button_Attack;
			}
	}
}
