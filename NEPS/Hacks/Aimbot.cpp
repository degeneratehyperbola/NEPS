#include "Aimbot.h"
#include "Animations.h"
#include "Backtrack.h"

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
static const Backtrack::Record *targetRecord;

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

static __forceinline void chooseTarget(const Config::Aimbot &cfg, UserCmd *cmd) noexcept
{
	targetAngle = Vector{};
	targetHandle = 0;
	targetRecord = nullptr;

	const auto activeWeapon = localPlayer->getActiveWeapon();
	if (!activeWeapon || !activeWeapon->clip())
		return;

	const auto weaponData = activeWeapon->getWeaponData();
	if (!weaponData)
		return;

	const auto aimPunch = activeWeapon->requiresRecoilControl() ? localPlayer->getAimPunch() : Vector{};
	const auto localPlayerEyePosition = localPlayer->getEyePosition();
	bool doOverride = false;
	{
		static Helpers::KeyBindState flag;
		doOverride = flag[cfg.damageOverride];
	}
	const auto minDamage = doOverride ? cfg.minDamageOverride : cfg.minDamage;
	const auto minDamageAutoWall = doOverride ? cfg.minDamageAutoWallOverride : cfg.minDamageAutoWall;

	auto bestFov = cfg.fov;
	auto bestDistance = cfg.distance ? cfg.distance : INFINITY;
	auto bestDamage = 0;
	auto bestHitchance = cfg.shotHitchance;

	std::array<Matrix3x4, MAX_STUDIO_BONES> bufferBones;

	for (int i = 1; i <= interfaces->engine->getMaxClients(); i++)
	{
		auto entity = interfaces->entityList->getEntity(i);

		if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive() || entity->gunGameImmunity())
			continue;

		const auto enemy = localPlayer->isOtherEnemy(entity);
		if (!cfg.friendlyFire && !enemy)
			continue;

		const auto hitboxSet = entity->getHitboxSet();
		if (!hitboxSet)
			continue;

		if (cfg.desyncResolver)
			Animations::resolveLBY(entity, shots - hits);

		if (!entity->setupBones(bufferBones.data(), MAX_STUDIO_BONES, BONE_USED_BY_HITBOX, memory->globalVars->currenttime))
			continue;

		auto allowedHitgroup = cfg.hitGroup;

		if (cfg.safeOnly && !Helpers::animDataAuthenticity(entity))
		{
			allowedHitgroup = cfg.safeHitGroup;
		}

		if (!allowedHitgroup)
			continue;

		const Backtrack::Record *backtrackRecord = nullptr;
		const auto doScope = cfg.autoScope && !localPlayer->isScoped() && activeWeapon->isSniperRifle();
		const auto doStop = cfg.autoStop && localPlayer->flags() & Entity::FL_ONGROUND && localPlayer->moveType() != MoveType::NOCLIP && localPlayer->moveType() != MoveType::LADDER;
		const auto doBacktrack = config->backtrack.aimAtRecords && config->backtrack.enabled && enemy;
		if (doScope || doBacktrack || doStop)
		{
			bool goesThroughWall = false;
			Trace trace;
			auto origin = bufferBones[8].origin();
			bool canHit = Helpers::findDamage(origin, weaponData, trace, cfg.friendlyFire, 128, &goesThroughWall);

			if (canHit && trace.entity == entity && (!cfg.visibleOnly || !goesThroughWall))
			{
				if (doScope)
					cmd->buttons |= UserCmd::IN_ATTACK2;

				if (doStop)
				{
					const float maxSpeed = (localPlayer->isScoped() ? weaponData->maxSpeedAlt : weaponData->maxSpeed) / 4;

					if (cmd->forwardmove && cmd->sidemove)
					{
						const float maxSpeedRoot = maxSpeed * static_cast<float>(M_SQRT1_2);
						cmd->forwardmove = cmd->forwardmove < 0.0f ? -maxSpeedRoot : maxSpeedRoot;
						cmd->sidemove = cmd->sidemove < 0.0f ? -maxSpeedRoot : maxSpeedRoot;
					} else if (cmd->forwardmove)
					{
						cmd->forwardmove = cmd->forwardmove < 0.0f ? -maxSpeed : maxSpeed;
					} else if (cmd->sidemove)
					{
						cmd->sidemove = cmd->sidemove < 0.0f ? -maxSpeed : maxSpeed;
					}
				}
			}

			if (doBacktrack)
			{
				auto bestDistance = origin.distTo(localPlayerEyePosition);

				const auto records = Backtrack::getRecords(entity->index());
				for (const auto &record : records)
				{
					if (!Backtrack::valid(record.simulationTime) || !record.important)
						continue;

					const auto distance = record.matrix[8].origin().distTo(localPlayerEyePosition);
					if (goesThroughWall && distance < bestDistance)
					{
						bestDistance = distance;
						backtrackRecord = &record;
					}
				}
			}

			if (backtrackRecord)
				std::copy(std::begin(backtrackRecord->matrix), std::end(backtrackRecord->matrix), bufferBones.data());
		}

		for (int hitboxIdx = 0; hitboxIdx < hitboxSet->numHitboxes; hitboxIdx++)
		{
			if (hitboxIdx == Hitbox::LeftHand ||
				hitboxIdx == Hitbox::RightHand ||
				hitboxIdx == Hitbox::Neck ||
				hitboxIdx == Hitbox::LowerChest ||
				hitboxIdx == Hitbox::Belly)
				continue;

			const auto hitbox = *hitboxSet->getHitbox(hitboxIdx);

			std::vector<Vector> points;

			if (cfg.multipoint)
			{
				switch (hitboxIdx)
				{
				case Hitbox::Head:
				{
					const float r = hitbox.capsuleRadius * cfg.multipointScale;
					const Vector min = hitbox.bbMin.transform(bufferBones[hitbox.bone]);
					const Vector max = hitbox.bbMax.transform(bufferBones[hitbox.bone]);
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
				case Hitbox::UpperChest:
				{
					const float r = hitbox.capsuleRadius * cfg.multipointScale;
					const Vector min = hitbox.bbMin.transform(bufferBones[hitbox.bone]);
					const Vector max = hitbox.bbMax.transform(bufferBones[hitbox.bone]);
					Vector axis = max - min;
					axis /= axis.length();
					Vector axisRel = hitbox.bbMax - hitbox.bbMin;
					axisRel /= axisRel.length();
					Vector midRel = (hitbox.bbMin + hitbox.bbMax) * 0.5f;

					Vector v1 = hitbox.bbMin.crossProduct(hitbox.bbMax);
					v1 /= v1.length();
					v1 *= r;

					axis *= r;

					points.emplace_back((midRel + v1).transform(bufferBones[hitbox.bone]));
					points.emplace_back(max + axis);
					points.emplace_back(min - axis);
					break;
				}
				case Hitbox::Thorax:
				{
					const float r = hitbox.capsuleRadius * cfg.multipointScale;
					const Vector min = hitbox.bbMin.transform(bufferBones[hitbox.bone]);
					const Vector max = hitbox.bbMax.transform(bufferBones[hitbox.bone]);
					Vector mid = (min + max) * 0.5f;
					Vector axis = max - min;
					axis /= axis.length();
					axis *= r;

					points.emplace_back(mid);
					points.emplace_back(max + axis);
					points.emplace_back(min - axis);
					break;
				}
				case Hitbox::Pelvis:
				{
					const float r = hitbox.capsuleRadius * cfg.multipointScale;
					const Vector min = hitbox.bbMin.transform(bufferBones[hitbox.bone]);
					const Vector max = hitbox.bbMax.transform(bufferBones[hitbox.bone]);
					Vector axis = max - min;
					axis /= axis.length();
					Vector axisRel = hitbox.bbMax - hitbox.bbMin;
					axisRel /= axisRel.length();
					Vector midRel = (hitbox.bbMin + hitbox.bbMax) * 0.5f;

					Vector v1 = hitbox.bbMin.crossProduct(hitbox.bbMax);
					v1 /= v1.length();
					v1 *= r;

					axis *= r;

					points.emplace_back((midRel - v1).transform(bufferBones[hitbox.bone]));
					points.emplace_back(max + axis);
					points.emplace_back(min - axis);
					break;
				}
				case Hitbox::LeftFoot:
				case Hitbox::RightFoot:
					points.emplace_back(((hitbox.bbMin + hitbox.bbMax) * 0.5f).transform(bufferBones[hitbox.bone]));
					break;
				default:
					points.emplace_back(hitbox.bbMax.transform(bufferBones[hitbox.bone]));
					break;
				}
			} else
			{
				switch (hitboxIdx)
				{
				case Hitbox::LeftFoot:
				case Hitbox::RightFoot:
				case Hitbox::Head:
				case Hitbox::UpperChest:
				case Hitbox::Thorax:
				case Hitbox::Pelvis:
					points.emplace_back(((hitbox.bbMin + hitbox.bbMax) * 0.5f).transform(bufferBones[hitbox.bone]));
					break;
				default:
					points.emplace_back(hitbox.bbMax.transform(bufferBones[hitbox.bone]));
					break;
				}
			}

			const float radius = Helpers::approxRadius(hitbox, hitboxIdx);

			for (auto &point : points)
			{
				const auto angle = Helpers::calculateRelativeAngle(localPlayerEyePosition, point, cmd->viewangles + aimPunch);

				const auto fov = std::hypot(angle.x, angle.y);
				if (fov >= bestFov)
					continue;

				const auto distance = localPlayerEyePosition.distTo(point);
				if (distance >= bestDistance || distance > weaponData->range)
					continue;

				const auto hitchance = Helpers::findHitchance(activeWeapon->getInaccuracy(), activeWeapon->getSpread(), radius, distance);
				if (hitchance <= bestHitchance)
					continue;

				bool goesThroughWall = false;
				Trace trace;
				const auto damage = Helpers::findDamage(point, weaponData, trace, cfg.friendlyFire, allowedHitgroup, &goesThroughWall, backtrackRecord, hitboxIdx);

				if (cfg.visibleOnly && goesThroughWall) continue;

				if (!backtrackRecord && trace.entity != entity) continue;

				if (!goesThroughWall)
				{
					if (damage <= std::min(minDamage, entity->health()))
						continue;
					if (damage <= std::min(bestDamage, entity->health()))
						continue;
				} else
				{
					if (damage <= std::min(minDamageAutoWall, entity->health()))
						continue;
					if (damage <= std::min(bestDamage, entity->health()))
						continue;
				}

				if (!cfg.ignoreSmoke && memory->lineGoesThroughSmoke(localPlayerEyePosition, point, 1))
					continue;

				switch (cfg.targeting)
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
	if (!activeWeapon || !activeWeapon->clip())
		return;

	if (localPlayer->shotsFired() > 0 && !activeWeapon->isFullAuto())
		return;

	auto weaponIndex = getWeaponIndex(activeWeapon->itemDefinitionIndex2());
	if (!weaponIndex)
		return;

	auto weaponClass = getWeaponClass(activeWeapon->itemDefinitionIndex2());
	if (!config->aimbot[weaponIndex].bind.keyMode)
		weaponIndex = weaponClass;

	if (!config->aimbot[weaponIndex].bind.keyMode)
		weaponIndex = 0;

	const auto &cfg = config->aimbot[weaponIndex];

	const auto weaponData = activeWeapon->getWeaponData();
	if (!weaponData)
		return;

	if (static Helpers::KeyBindState flag; !flag[cfg.bind]) return;

	if (!cfg.hitGroup)
		return;

    if (!cfg.betweenShots && activeWeapon->nextPrimaryAttack() > time && (!activeWeapon->burstMode() || activeWeapon->nextBurstShot() > time))
        return;

    if (!cfg.ignoreFlash && localPlayer->isFlashed())
        return;

    if ((cmd->buttons & UserCmd::IN_ATTACK || cfg.autoShot || cfg.aimlock)) {

		if (cfg.scopedOnly && activeWeapon->isSniperRifle() && !localPlayer->isScoped() && !cfg.autoScope)
			return;

		static auto prevTargetHandle = targetHandle;

		if (prevTargetHandle != targetHandle)
			resetMissCounter();

		chooseTarget(cfg, cmd);

		const auto target = interfaces->entityList->getEntityFromHandle(targetHandle);
		if (target && targetAngle.notNull())
		{
			static Vector lastAngles = cmd->viewangles;
			static int lastCommand = 0;

			const auto aimPunch = activeWeapon->requiresRecoilControl() ? localPlayer->getAimPunch() : Vector{};

			if (lastCommand == cmd->commandNumber - 1 && lastAngles.notNull() && cfg.silent)
				cmd->viewangles = lastAngles;

			bool clamped = false;

			if (std::abs(targetAngle.x) > config->misc.maxAngleDelta || std::abs(targetAngle.y) > config->misc.maxAngleDelta)
			{
				targetAngle.x = std::clamp(targetAngle.x, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
				targetAngle.y = std::clamp(targetAngle.y, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
				clamped = true;
			}

			if (cfg.interpolation == 2 || cfg.interpolation == 3)
				targetAngle = targetAngle * (1.0f - cfg.quadratic);

			const auto l = targetAngle.length();
			if ((cfg.interpolation == 1 || cfg.interpolation == 3) && l > cfg.linear)
				targetAngle *= cfg.linear / l;

			if (targetAngle.notNull())
			{
				cmd->viewangles += targetAngle;

				if (!cfg.silent)
					interfaces->engine->setViewAngles(cmd->viewangles);
			}

			if (cfg.autoShot && activeWeapon->nextPrimaryAttack() <= time)
				cmd->buttons |= UserCmd::IN_ATTACK;

			if (clamped)
			{
				cmd->buttons &= ~UserCmd::IN_ATTACK;
				lastAngles = cmd->viewangles;
			} else lastAngles = Vector{};

			lastCommand = cmd->commandNumber;
		}

		prevTargetHandle = targetHandle;
	}
}

int Aimbot::getTargetHandle() noexcept
{
	return targetHandle;
}

const Backtrack::Record *Aimbot::getTargetRecord() noexcept
{
	return targetRecord;
}
