#include "Aimbot.h"
#include "Misc.h"
#include "Backtrack.h"
#include "Players.h"

#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../SDK/AnimState.h"
#include "../SDK/EngineTrace.h"
#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/Vector.h"
#include "../SDK/WeaponId.h"
#include "../SDK/GameEvent.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/ModelInfo.h"
#include "../SDK/PhysicsSurfaceProps.h"
#include "../SDK/WeaponData.h"

#include "../lib/Helpers.hpp"

static Vector targetAngle;
static int targetHandle;
static const Record *targetRecord;

int Aimbot::getTargetHandle() noexcept
{
	return targetHandle;
}

const Record *Aimbot::getTargetRecord() noexcept
{
	return targetRecord;
}

static int shots = 0;
static int hits = 0;

void Aimbot::missCounter(GameEvent *event) noexcept
{
	if (!localPlayer)
		return;

	if (event)
	{
		switch (fnv::hashRuntime(event->getName()))
		{
		case fnv::hash("weapon_fire"):
			if (localPlayer->getUserId() == event->getInt("userid"))
				shots++;
			break;
		case fnv::hash("player_hurt"):
			if (localPlayer->getUserId() == event->getInt("attacker"))
				hits++;
			break;
		}
		return;
	}
}

void Aimbot::resetMissCounter() noexcept
{
	shots = 0;
	hits = 0;
}

int Aimbot::getMisses() noexcept
{
    return shots - hits;
}

void Aimbot::predictPeek(UserCmd *cmd) noexcept
{
	if (!localPlayer)
		return;

	if (*memory->gameRules && (*memory->gameRules)->freezePeriod())
		return;

	const auto time = memory->globalVars->serverTime();
	if (localPlayer->nextAttack() > time || !localPlayer->isAlive() || localPlayer->isDefusing() || localPlayer->waitForNoAttack())
		return;

	const auto activeWeapon = localPlayer->getActiveWeapon();
	if (!activeWeapon || !activeWeapon->clip())
		return;

	if (localPlayer->shotsFired() > 0 && !activeWeapon->isFullAuto())
		return;

	const auto &cfg = Config::Aimbot::getRelevantConfig();
	
	if (!cfg.autoStop && !cfg.autoScope)
		return;

	if (!cfg.betweenShots && activeWeapon->nextPrimaryAttack() > time && (!activeWeapon->burstMode() || activeWeapon->nextBurstShot() > time))
		return;

	constexpr auto predictionFactor = 0.07f;

	for (int i = 1; i <= interfaces->engine->getMaxClients(); i++)
	{
		auto entity = interfaces->entityList->getEntity(i);

		if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive() || entity->gunGameImmunity())
			continue;

		const auto enemy = localPlayer->isOtherEnemy(entity);
		if (!cfg.friendlyFire && !enemy)
			continue;

		Record predictedGhost;
		predictedGhost.hasHelmet = entity->hasHelmet();
		predictedGhost.armor = entity->armor();

		bool occluded = true;
		bool occludedBacktrack = true;
		int damage = Helpers::findDamage(localPlayer.get(), entity, occluded, cfg.friendlyFire, predictionFactor);

		const auto &records = Backtrack::getRecords(entity->index());

		if (const auto it = std::find_if(records.rbegin(), records.rend(), [](const Record &record) noexcept { return Backtrack::valid(record.simulationTime); }); it != records.rend())
			damage = std::max(damage, Helpers::findDamage(localPlayer.get(), entity, occludedBacktrack, cfg.friendlyFire, predictionFactor, &(*it)));

		if (static Helpers::KeyBindState flag; !flag[cfg.bind]) return;

		if (config->players.spectatorFilter && config->players.filterAim && !Players::noSpectators)
			return;

		// if the players feature and aim filter is enabled, and the player was not flagged as a target, then don't target them
		if (config->players.enabled && config->players.filterAim && !Players::players[i].flagged)
			continue;
		
		if (damage > 0 && (!cfg.visibleOnly || !occluded || !occludedBacktrack))
		{
			if (cfg.autoScope && !localPlayer->isScoped() && activeWeapon->isSniperRifle())
				cmd->buttons |= UserCmd::Button_Attack2;

			if (cfg.autoStop)
				Misc::slowwalk(cmd);

			break;
		}
	}
}

#define REAL_OVER_LAG_RECORD_ADVANTAGE 5
#define DAMAGE_THRESHOLD_FRACTION 0.1f

#ifndef NEPS_DEBUG
__forceinline
#endif
void chooseTarget(UserCmd *cmd) noexcept
{
	const auto &cfg = Config::Aimbot::getRelevantConfig();

	targetAngle = Vector{};
	targetHandle = 0;
	targetRecord = nullptr;

	const auto activeWeapon = localPlayer->getActiveWeapon();
	if (!activeWeapon)
		return;

	const auto weaponData = activeWeapon->getWeaponData();
	if (!weaponData)
		return;

	const auto aimPunch = activeWeapon->requiresRecoilControl() ? localPlayer->getAimPunch() * Vector{cfg.recoilReductionV / 100, cfg.recoilReductionH / 100, 1.0f} : Vector{};
	const auto localPlayerEyePosition = localPlayer->getEyePosition();
	bool doOverride = false;
	{
		static Helpers::KeyBindState flag;
		doOverride = flag[cfg.aimbotOverride.bind];
	}
	const auto targeting = doOverride ? cfg.aimbotOverride.targeting : cfg.targeting;
	const auto hitGroup = doOverride ? cfg.aimbotOverride.hitGroup : cfg.hitGroup;
	const auto multipointScale = doOverride ? cfg.aimbotOverride.multipointScale : cfg.multipointScale;

	const auto minDamage = doOverride ? cfg.aimbotOverride.minDamage : cfg.minDamage;
	const auto minDamageAutoWall = doOverride ? cfg.aimbotOverride.minDamageAutoWall : cfg.minDamageAutoWall;

	auto bestFov = doOverride ? cfg.aimbotOverride.fov : cfg.fov;
	auto bestDistance = doOverride ? (cfg.distance ? cfg.distance : INFINITY) : (cfg.aimbotOverride.distance ? cfg.aimbotOverride.distance : INFINITY);
	auto bestDamage = 0;
	auto bestHitchance = doOverride ? cfg.aimbotOverride.hitchance : cfg.hitchance;
	for (int i = 1; i <= interfaces->engine->getMaxClients(); i++)
	{
		auto entity = interfaces->entityList->getEntity(i);

		if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive() || entity->gunGameImmunity())
			continue;

		// if the players feature and aim filter is enabled, and the player was not flagged as a target, then don't target them
		if (config->players.enabled && config->players.filterAim && !Players::players[i].flagged)
			continue;

		const auto enemy = localPlayer->isOtherEnemy(entity);
		if (!cfg.friendlyFire && !enemy)
			continue;

		const auto hitboxSet = entity->getHitboxSet();
		if (!hitboxSet)
			continue;

		std::array<Matrix3x4, MAX_STUDIO_BONES> bones;
		{
			const auto &boneMatrices = entity->boneCache();
			std::copy(boneMatrices.memory, boneMatrices.memory + boneMatrices.size, bones.begin());
		}

		const Record *backtrackRecord = nullptr;
		if (config->backtrack.enabled && enemy)
		{
			const auto &records = Backtrack::getRecords(entity->index());

			if (const auto it = std::find_if(records.rbegin(), records.rend(), [](const Record &record) noexcept { return Backtrack::valid(record.simulationTime); }); it != records.rend())
				backtrackRecord = &(*it);

			[[maybe_unused]] bool occluded = true;
			if (Helpers::findDamage(localPlayer.get(), entity, occluded, cfg.friendlyFire, 0.0f) > Helpers::findDamage(localPlayer.get(), entity, occluded, cfg.friendlyFire, 0.0f, backtrackRecord) - REAL_OVER_LAG_RECORD_ADVANTAGE)
				backtrackRecord = nullptr;

			if (backtrackRecord)
				std::copy(std::begin(backtrackRecord->bones), std::end(backtrackRecord->bones), bones.begin());
		}

		for (int hitboxIdx = 0; hitboxIdx < hitboxSet->numHitboxes; hitboxIdx++)
		{
			if (hitboxIdx == Hitbox_LeftHand ||
				hitboxIdx == Hitbox_RightHand ||
				hitboxIdx == Hitbox_Neck ||
				hitboxIdx == Hitbox_LowerChest ||
				hitboxIdx == Hitbox_Belly)
				continue;

			if (~hitGroup & (1 << (Helpers::hitboxToHitGroup(hitboxIdx) - 1)))
				continue;

			const auto hitbox = *hitboxSet->getHitbox(hitboxIdx);

			std::vector<Vector> points;

			if (cfg.multipoint)
			{
				switch (hitboxIdx)
				{
				case Hitbox_Head:
				{
					const float r = hitbox.capsuleRadius * multipointScale;
					const Vector min = hitbox.bbMin.transform(bones[hitbox.bone]);
					const Vector max = hitbox.bbMax.transform(bones[hitbox.bone]);
					Vector mid = (min + max) * 0.5f;
					Vector axis = max - min;
					axis /= axis.length();

					Vector v1 = min.crossProduct(max);
					v1 /= v1.length();
					v1 *= r;
					Vector v2 = v1.rotate(axis, 120.0f);
					Vector v3 = v2.rotate(axis, 120.0f);

					points.emplace_back(mid);
					points.emplace_back(max + v1);
					points.emplace_back(max + v2);
					points.emplace_back(max + v3);
					points.emplace_back(max + axis * r);
					break;
				}
				case Hitbox_UpperChest:
				{
					const float r = hitbox.capsuleRadius * multipointScale;
					const Vector min = hitbox.bbMin.transform(bones[hitbox.bone]);
					const Vector max = hitbox.bbMax.transform(bones[hitbox.bone]);
					Vector axis = max - min;
					axis /= axis.length();
					Vector axisRel = hitbox.bbMax - hitbox.bbMin;
					axisRel /= axisRel.length();
					Vector midRel = (hitbox.bbMin + hitbox.bbMax) * 0.5f;

					Vector v1 = hitbox.bbMin.crossProduct(hitbox.bbMax);
					v1 /= v1.length();
					v1 *= r;

					axis *= r;

					points.emplace_back((midRel + v1).transform(bones[hitbox.bone]));
					points.emplace_back(max + axis);
					points.emplace_back(min - axis);
					break;
				}
				case Hitbox_Thorax:
				{
					const float r = hitbox.capsuleRadius * multipointScale;
					const Vector min = hitbox.bbMin.transform(bones[hitbox.bone]);
					const Vector max = hitbox.bbMax.transform(bones[hitbox.bone]);
					Vector mid = (min + max) * 0.5f;
					Vector axis = max - min;
					axis /= axis.length();
					axis *= r;

					points.emplace_back(mid);
					points.emplace_back(max + axis);
					points.emplace_back(min - axis);
					break;
				}
				case Hitbox_Pelvis:
				{
					const float r = hitbox.capsuleRadius * multipointScale;
					const Vector min = hitbox.bbMin.transform(bones[hitbox.bone]);
					const Vector max = hitbox.bbMax.transform(bones[hitbox.bone]);
					Vector axis = max - min;
					axis /= axis.length();
					Vector axisRel = hitbox.bbMax - hitbox.bbMin;
					axisRel /= axisRel.length();
					Vector midRel = (hitbox.bbMin + hitbox.bbMax) * 0.5f;

					Vector v1 = hitbox.bbMin.crossProduct(hitbox.bbMax);
					v1 /= v1.length();
					v1 *= r;

					axis *= r;

					points.emplace_back((midRel - v1).transform(bones[hitbox.bone]));
					points.emplace_back(max + axis);
					points.emplace_back(min - axis);
					break;
				}
				case Hitbox_LeftFoot:
				case Hitbox_RightFoot:
					points.emplace_back(((hitbox.bbMin + hitbox.bbMax) * 0.5f).transform(bones[hitbox.bone]));
					break;
				default:
					points.emplace_back(hitbox.bbMax.transform(bones[hitbox.bone]));
					break;
				}
			} else
			{
				switch (hitboxIdx)
				{
				case Hitbox_LeftFoot:
				case Hitbox_RightFoot:
				case Hitbox_Head:
				case Hitbox_UpperChest:
				case Hitbox_Thorax:
				case Hitbox_Pelvis:
					points.emplace_back(((hitbox.bbMin + hitbox.bbMax) * 0.5f).transform(bones[hitbox.bone]));
					break;
				default:
					points.emplace_back(hitbox.bbMax.transform(bones[hitbox.bone]));
					break;
				}
			}

			const float radius = Helpers::approxRadius(hitbox, hitboxIdx);

			for (const auto &point : points)
			{
				const auto extrapolatedPoint = point + entity->velocity() * memory->globalVars->intervalPerTick;

				const auto angle = Helpers::calculateRelativeAngle(localPlayerEyePosition, extrapolatedPoint, cmd->viewangles + aimPunch);

				const auto fov = std::hypot(angle.x, angle.y);
				if (fov >= bestFov)
					continue;

				const auto distance = localPlayerEyePosition.distTo(extrapolatedPoint);
				if (distance >= bestDistance || distance > weaponData->range)
					continue;

				const auto hitchance = Helpers::findHitchance(activeWeapon->getInaccuracy(), activeWeapon->getSpread(), radius, distance);
				if (hitchance <= bestHitchance)
					continue;

				Trace trace;
				const auto damage = Helpers::findDamage(point, localPlayer.get(), trace, cfg.friendlyFire, backtrackRecord, hitboxIdx);
				bool occluded = trace.startPos != localPlayerEyePosition;

				if (cfg.visibleOnly && occluded) continue;

				if (!backtrackRecord && trace.entity != entity) continue;

				const int targetHealth = entity->health() + static_cast<int>(entity->health() * DAMAGE_THRESHOLD_FRACTION);
				if (!occluded)
				{
					if (damage <= std::min(minDamage, targetHealth))
						continue;
					if (damage <= std::min(bestDamage, targetHealth))
						continue;
				} else
				{
					if (damage <= std::min(minDamageAutoWall, targetHealth))
						continue;
					if (damage <= std::min(bestDamage, targetHealth))
						continue;
				}

				if (!cfg.ignoreSmoke && memory->lineGoesThroughSmoke(localPlayerEyePosition, extrapolatedPoint, 1))
					continue;

				switch (targeting)
				{
				case 0:
					if (fov < bestFov)
					{
						bestFov = fov;
						targetAngle = angle;
						targetHandle = entity->handle();
						targetRecord = backtrackRecord;
					}
					break;
				case 1:
					if (damage > bestDamage)
					{
						bestDamage = damage;
						targetAngle = angle;
						targetHandle = entity->handle();
						targetRecord = backtrackRecord;
					}
					break;
				case 2:
					if (hitchance > bestHitchance)
					{
						bestHitchance = hitchance;
						targetAngle = angle;
						targetHandle = entity->handle();
						targetRecord = backtrackRecord;
					}
					break;
				case 3:
					if (distance < bestDistance)
					{
						bestDistance = distance;
						targetAngle = angle;
						targetHandle = entity->handle();
						targetRecord = backtrackRecord;
					}
					break;
				}
			}
		}
	}
}

void Aimbot::run(UserCmd *cmd) noexcept
{
	if (!localPlayer)
		return;

	if (*memory->gameRules && (*memory->gameRules)->freezePeriod())
		return;

	const auto time = memory->globalVars->serverTime();
	if (localPlayer->nextAttack() > time || !localPlayer->isAlive() || localPlayer->isDefusing() || localPlayer->waitForNoAttack())
		return;

	const auto activeWeapon = localPlayer->getActiveWeapon();
	if (!activeWeapon || !activeWeapon->clip() || activeWeapon->isKnife())
		return;

	const auto &cfg = Config::Aimbot::getRelevantConfig();

	const auto weaponData = activeWeapon->getWeaponData();
	if (!weaponData)
		return;

	if (static Helpers::KeyBindState flag; !flag[cfg.bind]) return;

	if (config->players.spectatorFilter && config->players.filterAim && !Players::noSpectators)
		return;

    if (!cfg.betweenShots && activeWeapon->nextPrimaryAttack() > time && (!activeWeapon->burstMode() || activeWeapon->nextBurstShot() > time))
        return;

    if (!cfg.ignoreFlash && localPlayer->isFlashed())
        return;

    if (cmd->buttons & UserCmd::Button_Attack || cfg.autoShoot || cfg.aimlock) {

		if (cfg.scopedOnly && activeWeapon->isSniperRifle() && !localPlayer->isScoped() && !cfg.autoScope)
			return;

		chooseTarget(cmd);

		static auto previousTargetHandle = targetHandle;

		if (previousTargetHandle != targetHandle)
			resetMissCounter();

		static Vector aimVelocity = Vector{};

		if (targetAngle.notNull())
		{
			static Vector lastAngles = cmd->viewangles;
			static int lastCommand = 0;

			if (lastCommand == cmd->commandNumber - 1 && lastAngles.notNull() && cfg.silent)
				cmd->viewangles = lastAngles;

			bool clamped = false;

			if (std::abs(targetAngle.x) > config->misc.maxAngleDelta || std::abs(targetAngle.y) > config->misc.maxAngleDelta)
			{
				targetAngle.x = std::clamp(targetAngle.x, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
				targetAngle.y = std::clamp(targetAngle.y, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
				clamped = true;
			}

			if (cfg.humanize)
			{
				const auto l = targetAngle.length();
				if (l > cfg.acceleration)
					aimVelocity += targetAngle / l * cfg.acceleration;
				else
					aimVelocity += targetAngle;
			}
			else if (targetAngle.notNull())
			{
				cmd->viewangles += targetAngle;

				if (!cfg.silent)
					interfaces->engine->setViewAngles(cmd->viewangles);
			}

			if (cfg.autoShoot && activeWeapon->nextPrimaryAttack() <= time)
				cmd->buttons |= UserCmd::Button_Attack;

			if (clamped)
			{
				cmd->buttons &= ~UserCmd::Button_Attack;
				lastAngles = cmd->viewangles;
			} else lastAngles = Vector{};

			lastCommand = cmd->commandNumber;
		}

		if (!cfg.humanize) aimVelocity = Vector{};

		if (aimVelocity.notNull())
		{
			aimVelocity /= cfg.friction;

			cmd->viewangles += aimVelocity;

			if (!cfg.silent)
				interfaces->engine->setViewAngles(cmd->viewangles);
		}

		previousTargetHandle = targetHandle;
	}
}
