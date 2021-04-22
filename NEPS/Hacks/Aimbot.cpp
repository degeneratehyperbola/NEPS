#include "Aimbot.h"
#include "Backtrack.h"
#include "../Config.h"
#include "../Interfaces.h"
#ifdef _DEBUG_NEPS
#include "../GameData.h"
#endif // _DEBUG_NEPS
#include "../Memory.h"
#include "../SDK/AnimState.h"
#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/Vector.h"
#include "../SDK/WeaponId.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/ModelInfo.h"
#include "../SDK/PhysicsSurfaceProps.h"
#include "../SDK/WeaponData.h"
#include "../Helpers.h"

static Vector targetPoint;
static int targetHandle = 0;
static const Backtrack::Record *targetRecord = nullptr;
static int weaponIndex = 0;

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

	weaponIndex = getWeaponIndex(activeWeapon->itemDefinitionIndex2());
	if (!weaponIndex)
		return;

	auto weaponClass = getWeaponClass(activeWeapon->itemDefinitionIndex2());
	if (!config->aimbot[weaponIndex].bind.keyMode)
		weaponIndex = weaponClass;

	if (!config->aimbot[weaponIndex].bind.keyMode)
		weaponIndex = 0;

	const auto weaponData = activeWeapon->getWeaponData();
	if (!weaponData)
		return;

	if (static Helpers::KeyBindState flag; !flag[config->aimbot[weaponIndex].bind]) return;

	if (!config->aimbot[weaponIndex].hitGroup)
		return;

    if (!config->aimbot[weaponIndex].betweenShots && activeWeapon->nextPrimaryAttack() > time && (!activeWeapon->burstMode() || activeWeapon->nextBurstShot() > time))
        return;

    if (!config->aimbot[weaponIndex].ignoreFlash && localPlayer->isFlashed())
        return;

    if ((cmd->buttons & UserCmd::IN_ATTACK || config->aimbot[weaponIndex].autoShot || config->aimbot[weaponIndex].aimlock) && activeWeapon->getInaccuracy() <= config->aimbot[weaponIndex].maxAimInaccuracy) {

		if (config->aimbot[weaponIndex].scopedOnly && activeWeapon->isSniperRifle() && !localPlayer->isScoped() && !config->aimbot[weaponIndex].autoScope)
			return;

		choseTarget(cmd);

		const auto target = interfaces->entityList->getEntityFromHandle(targetHandle);
		if (target && targetPoint.notNull())
		{
			static Vector lastAngles = cmd->viewangles;
			static int lastCommand = 0;

			const auto aimPunch = activeWeapon->requiresRecoilControl() ? localPlayer->getAimPunch() : Vector{};
			const auto localPlayerEyePosition = localPlayer->getEyePosition();

			if (lastCommand == cmd->commandNumber - 1 && lastAngles.notNull() && config->aimbot[weaponIndex].silent)
				cmd->viewangles = lastAngles;

			auto angle = Helpers::calculateRelativeAngle(localPlayerEyePosition, targetPoint, cmd->viewangles + aimPunch);
			bool clamped = false;

			if (std::abs(angle.x) > config->misc.maxAngleDelta || std::abs(angle.y) > config->misc.maxAngleDelta)
			{
				angle.x = std::clamp(angle.x, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
				angle.y = std::clamp(angle.y, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
				clamped = true;
			}

			if (config->aimbot[weaponIndex].interpolation == 2 || config->aimbot[weaponIndex].interpolation == 3)
				angle = angle * (1.0f - config->aimbot[weaponIndex].smooth);

			const auto l = angle.length();
			if ((config->aimbot[weaponIndex].interpolation == 1 || config->aimbot[weaponIndex].interpolation == 3) && l > config->aimbot[weaponIndex].linearSpeed)
				angle *= config->aimbot[weaponIndex].linearSpeed / l;

			if (angle.notNull())
			{
				cmd->viewangles += angle;

				if (!config->aimbot[weaponIndex].silent)
					interfaces->engine->setViewAngles(cmd->viewangles);
			}

			if (config->aimbot[weaponIndex].autoShot && activeWeapon->nextPrimaryAttack() <= time)
				cmd->buttons |= UserCmd::IN_ATTACK;

			if (clamped)
			{
				cmd->buttons &= ~UserCmd::IN_ATTACK;
				lastAngles = cmd->viewangles;
			} else lastAngles = Vector{};

			lastCommand = cmd->commandNumber;
		}
	}
}

void Aimbot::choseTarget(UserCmd *cmd) noexcept
{
	targetPoint = Vector{};
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
		doOverride = flag[config->aimbot[weaponIndex].damageOverride];
	}
	const auto minDamage = doOverride ? config->aimbot[weaponIndex].minDamageOverride : config->aimbot[weaponIndex].minDamage;
	const auto minDamageAutoWall = doOverride ? config->aimbot[weaponIndex].minDamageAutoWallOverride : config->aimbot[weaponIndex].minDamageAutoWall;

	auto bestFov = config->aimbot[weaponIndex].fov;
	auto bestDistance = config->aimbot[weaponIndex].distance ? config->aimbot[weaponIndex].distance : INFINITY;
	auto bestDamage = 0;
	auto bestHitchance = config->aimbot[weaponIndex].shotHitchance;

	std::array<Matrix3x4, MAX_STUDIO_BONES> bufferBones;

	for (int i = 1; i <= interfaces->engine->getMaxClients(); i++)
	{
		auto entity = interfaces->entityList->getEntity(i);

		if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive() || entity->gunGameImmunity())
			continue;

		const auto enemy = localPlayer->isOtherEnemy(entity);
		if (!config->aimbot[weaponIndex].friendlyFire && !enemy)
			continue;

		const auto hitboxSet = entity->getHitboxSet();

		if (!hitboxSet)
			continue;

		if (!entity->setupBones(bufferBones.data(), MAX_STUDIO_BONES, BONE_USED_BY_HITBOX, memory->globalVars->currenttime))
			continue;

		const Backtrack::Record *choosenRecord = nullptr;
		const auto doScope = config->aimbot[weaponIndex].autoScope && !localPlayer->isScoped() && activeWeapon->isSniperRifle();
		const auto doBacktrack = config->backtrack.aimAtRecords && config->backtrack.enabled && enemy;
		if (doScope || doBacktrack)
		{
			bool goesThroughWall = false;
			Trace trace;
			auto origin = bufferBones[8].origin();
			bool canHit = Helpers::canHit(origin, trace, config->aimbot[weaponIndex].friendlyFire, &goesThroughWall);

			if (doScope && canHit && trace.entity == entity && (!config->aimbot[weaponIndex].visibleOnly || !goesThroughWall))
			{
				if (config->aimbot[weaponIndex].autoScope && activeWeapon->isSniperRifle() && !localPlayer->isScoped())
					cmd->buttons |= UserCmd::IN_ATTACK2;
			}

			if (doBacktrack)
			{
				auto bestDistance = origin.distTo(localPlayerEyePosition);

				const auto records = Backtrack::getRecords(entity->index());
				for (const auto &record : records)
				{
					if (!Backtrack::valid(record.simulationTime))
						continue;

					if (record.shot && config->backtrack.onShot)
					{
						choosenRecord = &record;
						break;
					}

					const auto distance = record.matrix[8].origin().distTo(localPlayerEyePosition);
					if (goesThroughWall && distance < bestDistance)
					{
						bestDistance = distance;
						choosenRecord = &record;
					}
				}
			}

			if (choosenRecord)
				std::copy(std::begin(choosenRecord->matrix), std::end(choosenRecord->matrix), bufferBones.data());
		}

		auto allowedHitgroup = config->aimbot[weaponIndex].hitGroup;

		if (config->aimbot[weaponIndex].safeOnly && !Helpers::animDataAuthenticity(entity))
		{
			allowedHitgroup = config->aimbot[weaponIndex].safeHitGroup;
		}

		if (!allowedHitgroup)
			continue;

		for (int hitboxIdx = 0; hitboxIdx < hitboxSet->numHitboxes; hitboxIdx++)
		{
			if (hitboxIdx == Hitbox::LeftFoot ||
				hitboxIdx == Hitbox::RightFoot ||
				hitboxIdx == Hitbox::LeftHand ||
				hitboxIdx == Hitbox::RightHand ||
				hitboxIdx == Hitbox::Neck ||
				hitboxIdx == Hitbox::LowerChest ||
				hitboxIdx == Hitbox::Belly)
				continue;

			const auto hitbox = *hitboxSet->getHitbox(hitboxIdx);

			std::vector<Vector> points;

			if (config->aimbot[weaponIndex].multipoint)
			{
				switch (hitboxIdx)
				{
				case Hitbox::Head:
				{
					const float r = hitbox.capsuleRadius * config->aimbot[weaponIndex].multipointScale;
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
					const float r = hitbox.capsuleRadius * config->aimbot[weaponIndex].multipointScale;
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
					const float r = hitbox.capsuleRadius * config->aimbot[weaponIndex].multipointScale;
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
					const float r = hitbox.capsuleRadius * config->aimbot[weaponIndex].multipointScale;
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
				default:
					points.emplace_back(hitbox.bbMax.transform(bufferBones[hitbox.bone]));
					break;
				}
			} else
			{
				switch (hitboxIdx)
				{
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
				const auto damage = Helpers::findDamage(point, weaponData, trace, config->aimbot[weaponIndex].friendlyFire, allowedHitgroup, &goesThroughWall, choosenRecord, hitboxIdx);

				if (config->aimbot[weaponIndex].visibleOnly && goesThroughWall) continue;

				if (!choosenRecord && trace.entity != entity) continue;

				if (!goesThroughWall)
				{
					if (damage <= std::min(minDamage, entity->health() + config->aimbot[weaponIndex].killshot))
						continue;
					if (damage <= std::min(bestDamage, entity->health() + config->aimbot[weaponIndex].killshot))
						continue;
				} else
				{
					if (damage <= std::min(minDamageAutoWall, entity->health() + config->aimbot[weaponIndex].killshotAutoWall))
						continue;
					if (damage <= std::min(bestDamage, entity->health() + config->aimbot[weaponIndex].killshotAutoWall))
						continue;
				}

				if (!config->aimbot[weaponIndex].ignoreSmoke && memory->lineGoesThroughSmoke(localPlayerEyePosition, point, 1))
					continue;

				switch (config->aimbot[weaponIndex].targeting)
				{
				case 0:
					if (fov < bestFov)
					{
						bestFov = fov;
						targetPoint = point;
						targetHandle = entity->handle();
						targetRecord = choosenRecord;
					}
					break;
				case 1:
					if (damage > bestDamage)
					{
						bestDamage = damage;
						targetPoint = point;
						targetHandle = entity->handle();
						targetRecord = choosenRecord;
					}
					break;
				case 2:
					if (hitchance > bestHitchance)
					{
						bestHitchance = hitchance;
						targetPoint = point;
						targetHandle = entity->handle();
						targetRecord = choosenRecord;
					}
					break;
				case 3:
					if (distance < bestDistance)
					{
						bestDistance = distance;
						targetPoint = point;
						targetHandle = entity->handle();
						targetRecord = choosenRecord;
					}
					break;
				}
			}
		}
	}
}

Vector Aimbot::getTargetPoint() noexcept
{
	return targetPoint;
}

int Aimbot::getTargetHandle() noexcept
{
	return targetHandle;
}

const Backtrack::Record *Aimbot::getTargetRecord() noexcept
{
	return targetRecord;
}
