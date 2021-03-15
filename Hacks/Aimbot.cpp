#include "Aimbot.h"
#include "../Config.h"
#include "../Interfaces.h"
#include "../GameData.h"
#include "../Memory.h"
#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/Vector.h"
#include "../SDK/WeaponId.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/ModelInfo.h"
#include "../SDK/PhysicsSurfaceProps.h"
#include "../SDK/WeaponData.h"
#include "../Helpers.h"

void Aimbot::run(UserCmd *cmd) noexcept
{
	if (!localPlayer) return;

	if (localPlayer->nextAttack() > memory->globalVars->serverTime() || localPlayer->isDefusing() || localPlayer->waitForNoAttack())
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

	if (static Helpers::KeyBindState flag; !flag[config->aimbot[weaponIndex].bind]) return;

	if (!config->aimbot[weaponIndex].hitgroup)
		return;

    if (!config->aimbot[weaponIndex].betweenShots && activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime())
        return;

    if (!config->aimbot[weaponIndex].ignoreFlash && localPlayer->isFlashed())
        return;

	GameData::global().indicators.aimbot = true;

    if ((cmd->buttons & UserCmd::IN_ATTACK || config->aimbot[weaponIndex].autoShot || config->aimbot[weaponIndex].aimlock) && activeWeapon->getInaccuracy() <= config->aimbot[weaponIndex].maxAimInaccuracy) {

        if (config->aimbot[weaponIndex].scopedOnly && activeWeapon->isSniperRifle() && !localPlayer->isScoped())
            return;

		const auto aimPunch = activeWeapon->requiresRecoilControl() ? localPlayer->getAimPunch() : Vector{ };

        auto bestFov = config->aimbot[weaponIndex].fov;
        auto bestDistance = config->aimbot[weaponIndex].distance ? config->aimbot[weaponIndex].distance : INFINITY;
        auto bestDamage = config->aimbot[weaponIndex].minDamage;
        auto bestHitchance = config->aimbot[weaponIndex].shotHitchance;
        Vector bestTarget{ };

        const auto localPlayerEyePosition = localPlayer->getEyePosition();

		matrix3x4 *bufferBones = new matrix3x4[MAXSTUDIOBONES];
		std::vector<Vector> multipoints;

		GameData::Lock lock;
		for (auto &player : GameData::players())
		{
			if (!player.hitboxSet)
				continue;

			if (player.dormant || !player.alive || !player.enemy && !config->aimbot[weaponIndex].friendlyFire)
				continue;

			auto entity = interfaces->entityList->getEntityFromHandle(player.handle);

			if (!entity || entity->gunGameImmunity() || entity->isDormant())
				continue;

			if (static Helpers::KeyBindState flag; flag[config->aimbot[weaponIndex].safeOnly])
			{
				const auto remoteActiveWep = entity->getActiveWeapon();
				if (remoteActiveWep && config->aimbot[weaponIndex].onShot && remoteActiveWep->lastShotTime() == entity->simulationTime());
				else if (config->aimbot[weaponIndex].onMove && entity->velocity().length2D() > 90.0f);
				else continue;
			}

			if (!entity->setupBones(bufferBones, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, memory->globalVars->currenttime))
				continue;

			for (int i = 0; i < player.hitboxSet->numHitboxes; i++)
			{
				if (i == Hitbox::LeftFoot ||
					i == Hitbox::RightFoot ||
					i == Hitbox::LeftHand ||
					i == Hitbox::RightHand ||
					i == Hitbox::Neck ||
					i == Hitbox::LowerChest ||
					i == Hitbox::Belly)
					continue;

				const auto hitbox = *player.hitboxSet->getHitbox(i);

				std::vector<Vector> points;

				if (config->aimbot[weaponIndex].multipoint)
				{
					switch (i)
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
						Vector v2 = v1.rotate(axis, 120.0f);
						Vector v3 = v2.rotate(axis, 120.0f);
						v1 /= v1.length();
						v2 /= v2.length();
						v3 /= v3.length();

						points.emplace_back(mid);
						points.emplace_back(min + v1 * r);
						points.emplace_back(max + v1 * r);
						points.emplace_back(min + v2 * r);
						points.emplace_back(max + v2 * r);
						points.emplace_back(min + v3 * r);
						points.emplace_back(max + v3 * r);
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

						points.emplace_back((midRel + v1 * r).transform(bufferBones[hitbox.bone]));
						points.emplace_back(max + axis * r);
						points.emplace_back(min - axis * r);
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

						points.emplace_back((midRel + v1 * -r).transform(bufferBones[hitbox.bone]));
						points.emplace_back(max + axis * r);
						points.emplace_back(min - axis * r);
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

						points.emplace_back(mid);
						points.emplace_back(max + axis * r);
						points.emplace_back(min - axis * r);
						break;
					}
					default:
						const Vector max = hitbox.bbMax.transform(bufferBones[hitbox.bone]);
						points.emplace_back(max);
						break;
					}
				} else
				{
					points.emplace_back(((hitbox.bbMin + hitbox.bbMax) * 0.5f).transform(bufferBones[hitbox.bone]));
				}

				multipoints.insert(multipoints.end(), points.begin(), points.end());

				for (auto &point : points)
				{
					const auto angle = Helpers::calculateRelativeAngle(localPlayerEyePosition, point, cmd->viewangles + aimPunch);

					const auto fov = std::hypot(angle.x, angle.y);
					if (fov >= bestFov)
						continue;

					const auto distance = localPlayerEyePosition.distTo(point);
					if (distance >= bestDistance)
						continue;

					const auto hitchance = Helpers::findHitchance(activeWeapon->getInaccuracy(), activeWeapon->getSpread(), hitbox.capsuleRadius, distance);
					if (hitchance <= bestHitchance)
						continue;

					Trace trace;
					const auto damage = Helpers::findDamage(point, activeWeapon->getWeaponData(), trace, config->aimbot
						[weaponIndex].friendlyFire, config->aimbot[weaponIndex].hitgroup, config->aimbot[weaponIndex].visibleOnly);

					if (trace.entity != entity) continue;

					if (damage <= bestDamage)
						continue;
					if (config->aimbot[weaponIndex].killshot && (damage < (player.health + config->aimbot[weaponIndex].minDamage)))
						continue;

					if (!config->aimbot[weaponIndex].ignoreSmoke && memory->lineGoesThroughSmoke(localPlayerEyePosition, point, 1))
						continue;

					switch (config->aimbot[weaponIndex].targeting)
					{
					case 0:
						if (fov < bestFov)
						{
							bestFov = fov;
							bestTarget = point;
						}
						break;
					case 1:
						if (damage > bestDamage)
						{
							bestDamage = damage;
							bestTarget = point;
						}
						break;
					case 2:
						if (hitchance > bestHitchance)
						{
							bestHitchance = hitchance;
							bestTarget = point;
						}
						break;
					case 3:
						if (distance < bestDistance)
						{
							bestDistance = distance;
							bestTarget = point;
						}
						break;
					}
				}
			}
		}

		GameData::global().indicators.multipoints.clear();
		GameData::global().indicators.multipoints = multipoints;

		if (bestTarget.notNull())
		{
			static Vector lastAngles{cmd->viewangles};
			static int lastCommand{ };

			if (lastCommand == cmd->commandNumber - 1 && lastAngles.notNull() && config->aimbot[weaponIndex].silent)
				cmd->viewangles = lastAngles;

			auto angle = Helpers::calculateRelativeAngle(localPlayerEyePosition, bestTarget, cmd->viewangles + aimPunch);
			bool clamped = false;

			if (config->aimbot[weaponIndex].autoScope && activeWeapon->isSniperRifle() && !localPlayer->isScoped())
				cmd->buttons |= UserCmd::IN_ATTACK2;

			if (std::abs(angle.x) > config->misc.maxAngleDelta || std::abs(angle.y) > config->misc.maxAngleDelta)
			{
				angle.x = std::clamp(angle.x, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
				angle.y = std::clamp(angle.y, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
				clamped = true;
			}

			if (config->aimbot[weaponIndex].interpolation == 2)
				angle = angle * (1.0f - config->aimbot[weaponIndex].smooth);
			else if (config->aimbot[weaponIndex].interpolation == 1 && angle.length() > config->aimbot[weaponIndex].linearSpeed)
			{
				angle /= angle.length();
				angle = angle * config->aimbot[weaponIndex].linearSpeed;
				clamped = true;
			}

			cmd->viewangles += angle;

			if (!config->aimbot[weaponIndex].silent)
				interfaces->engine->setViewAngles(cmd->viewangles);

			if (config->aimbot[weaponIndex].autoShot && activeWeapon->nextPrimaryAttack() <= memory->globalVars->serverTime() && !clamped)
				cmd->buttons |= UserCmd::IN_ATTACK;

			if (clamped)
			{
				cmd->buttons &= ~UserCmd::IN_ATTACK;
				lastAngles = cmd->viewangles;
			} else lastAngles = Vector{};

			lastCommand = cmd->commandNumber;
		}

		delete[] bufferBones;
	}
}
