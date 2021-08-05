#include "Triggerbot.h"

#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../GameData.h"

#include "../SDK/Entity.h"
#include "../SDK/EngineTrace.h"
#include "../SDK/PhysicsSurfaceProps.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/UserCmd.h"
#include "../SDK/WeaponData.h"
#include "../SDK/WeaponId.h"
#include "../SDK/StudioRender.h"

#include "../lib/Helpers.hpp"

void Triggerbot::run(UserCmd* cmd) noexcept
{
	if (!localPlayer) return;

	const auto time = memory->globalVars->serverTime();
    if (!localPlayer->isAlive() || localPlayer->nextAttack() > time || localPlayer->isDefusing() || localPlayer->waitForNoAttack())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip() || activeWeapon->nextPrimaryAttack() > time)
        return;

    if (localPlayer->shotsFired() > 0 && !activeWeapon->isFullAuto())
        return;

    auto weaponIndex = getWeaponIndex(activeWeapon->itemDefinitionIndex2());
    if (!weaponIndex)
        return;

    if (!config->triggerbot[weaponIndex].bind.keyMode)
        weaponIndex = getWeaponClass(activeWeapon->itemDefinitionIndex2());

    if (!config->triggerbot[weaponIndex].bind.keyMode)
        weaponIndex = 0;

    const auto& cfg = config->triggerbot[weaponIndex];

	if (static Helpers::KeyBindState flag; !flag[cfg.bind]) return;

	if (!cfg.hitGroup)
		return;

    static auto lastTime = 0.0f;
    static auto lastContact = 0.0f;

	const auto now = memory->globalVars->realtime;

    if (now - lastContact < cfg.burstTime) {
        cmd->buttons |= UserCmd::IN_ATTACK;
        return;
    }
    lastContact = 0.0f;

    if (now - lastTime < cfg.shotDelay / 1000.0f)
        return;

    if (!cfg.ignoreFlash && localPlayer->isFlashed())
        return;

    if (cfg.scopedOnly && activeWeapon->isSniperRifle() && !localPlayer->isScoped())
        return;

    const auto weaponData = activeWeapon->getWeaponData();
    if (!weaponData)
        return;

    const auto startPos = localPlayer->getEyePosition();
    const auto endPos = startPos + Vector::fromAngle(cmd->viewangles + localPlayer->getAimPunch()) * weaponData->range;

    if (!cfg.ignoreSmoke && memory->lineGoesThroughSmoke(startPos, endPos, 1))
        return;

	bool goesThroughWall = false;
	Trace trace;
	const int damage = Helpers::findDamage(endPos, localPlayer.get(), trace, cfg.friendlyFire, &goesThroughWall);

	lastTime = now;
	
	if (~cfg.hitGroup & (1 << (trace.hitGroup - 1)))
		return;

	if (cfg.visibleOnly && goesThroughWall)
		return;

	if (!trace.entity || !trace.entity->isPlayer())
		return;

	const auto distance = trace.fraction * weaponData->range;
	if (cfg.distance && cfg.distance < distance)
		return;

	if (cfg.hitchance)
	{
		const auto hitbox = trace.entity->getHitbox(trace.hitbox);
		if (!hitbox) return;
		const auto hitchance = Helpers::findHitchance(activeWeapon->getInaccuracy(), activeWeapon->getSpread(), Helpers::approxRadius(*hitbox, trace.hitbox), distance);
		if (cfg.hitchance > hitchance)
			return;
	}

	auto minDamage = goesThroughWall ?
		std::min(cfg.minDamageAutoWall, trace.entity->health()) :
		std::min(cfg.minDamage, trace.entity->health());

    if (damage >= minDamage) {
        cmd->buttons |= UserCmd::IN_ATTACK;
        lastTime = 0.0f;
        lastContact = now;
    }
}
