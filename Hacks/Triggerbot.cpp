#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../GameData.h"
#include "../Helpers.h"
#include "../SDK/Entity.h"
#include "../SDK/PhysicsSurfaceProps.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/UserCmd.h"
#include "../SDK/WeaponData.h"
#include "../SDK/WeaponId.h"
#include "Triggerbot.h"

void Triggerbot::run(UserCmd* cmd) noexcept
{
	if (!localPlayer) return;

    if (!localPlayer->isAlive() || localPlayer->nextAttack() > memory->globalVars->serverTime() || localPlayer->isDefusing() || localPlayer->waitForNoAttack())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip() || activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime())
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

	if (activeWeapon->getInaccuracy() > cfg.maxShotInaccuracy)
        return;

	if (!cfg.hitgroup)
		return;

    static auto lastTime = 0.0f;
    static auto lastContact = 0.0f;

	const auto now = memory->globalVars->realtime;

    if (now - lastContact < config->triggerbot[weaponIndex].burstTime) {
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

	Trace trace;
	const int damage = Helpers::findDamage(endPos, weaponData, trace, cfg.friendlyFire, cfg.hitgroup, cfg.visibleOnly);

	if (!trace.entity)
		return;

	const auto distance = startPos.distTo(trace.endpos);
	if (cfg.distance && cfg.distance < distance)
		return;

	const auto hitchance = Helpers::findHitchance(activeWeapon->getInaccuracy(), activeWeapon->getSpread(), 5.0f, distance);
	if (cfg.hitchance && cfg.hitchance > hitchance)
		return;

	lastTime = now;

    if (damage >= (cfg.killshot ? (trace.entity->health() + cfg.minDamage) : cfg.minDamage)) {
        cmd->buttons |= UserCmd::IN_ATTACK;
        lastTime = 0.0f;
        lastContact = now;
    }
}
