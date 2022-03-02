#include <list>

#include "GameData.h"

#include "SDK/ClientClass.h"
#include "SDK/EngineTrace.h"
#include "SDK/Entity.h"
#include "SDK/Localize.h"
#include "SDK/ModelRender.h"
#include "SDK/NetworkChannel.h"
#include "SDK/PlayerResource.h"
#include "SDK/Radar.h"
#include "SDK/Sound.h"
#include "SDK/UtlVector.h"

#include "Hacks/Chams.h"

static Matrix4x4 viewMatrix;
static LocalPlayerData localPlayerData;
static std::vector<PlayerData> playerData;
static std::vector<ObserverData> observerData;
static std::vector<WeaponData> weaponData;
static std::vector<EntityData> entityData;
static std::vector<LootCrateData> lootCrateData;
static std::list<ProjectileData> projectileData;
static BombData bombData;
static std::vector<InfernoData> infernoData;
static std::vector<SmokeData> smokeData;
static SessionData sessionData;

static auto playerByHandleWritable(int handle) noexcept
{
	const auto it = std::find_if(playerData.begin(), playerData.end(), [handle](const auto &playerData) { return playerData.handle == handle; });
	return it != playerData.end() ? &(*it) : nullptr;
}

UtlVector<ActiveSoundInfo> activeSounds;

void GameData::update() noexcept
{
	static int lastFrame;
	if (lastFrame == memory->globalVars->frameCount)
		return;

	lastFrame = memory->globalVars->frameCount;

	Lock lock;

	observerData.clear();
	weaponData.clear();
	entityData.clear();
	lootCrateData.clear();
	infernoData.clear();
	smokeData.clear();

	localPlayerData.update();
	bombData.update();
	sessionData.update();

	if (!localPlayer)
	{
		playerData.clear();
		projectileData.clear();
		return;
	}

	viewMatrix = interfaces->engine->worldToScreenMatrix();

	const auto obsMode = localPlayer->getObserverMode();
	const auto obsTarget = obsMode != ObsMode::Roaming && obsMode != ObsMode::Deathcam ? localPlayer->getObserverTarget() : nullptr;

	const auto highestEntityIndex = interfaces->entityList->getHighestEntityIndex();
	for (int i = 1; i <= highestEntityIndex; ++i)
	{
		const auto entity = interfaces->entityList->getEntity(i);
		if (!entity)
			continue;

		Chams::toSlowPipeline(entity);

		if (entity->isPlayer())
		{
			if (entity == localPlayer.get())
				continue;

			if (const auto player = playerByHandleWritable(entity->handle()))
			{
				player->update(entity);
			} else
			{
				PlayerData dummy;
				dummy.update(entity);
				playerData.emplace_back(dummy);
			}

			if (!entity->isDormant() && !entity->isAlive())
			{
				const auto obs = entity->getObserverTarget();
				if (obs)
					observerData.emplace_back(entity, obs, obs == localPlayer.get(), obs == obsTarget);
			}
		} else
		{
			if (entity->isDormant())
				continue;

			if (entity->isWeapon())
			{
				if (entity->ownerEntity() == -1)
					weaponData.emplace_back(entity);
			} else
			{
				auto classId = entity->getClientClass()->classId;

				switch (classId)
				{
				case ClassId::GrenadeProjectile:
					if (entity->grenadeExploded())
					{
						if (const auto it = std::find(projectileData.begin(), projectileData.end(), entity->handle()); it != projectileData.end())
							it->exploded = true;
						break;
					}
					[[fallthrough]];
				case ClassId::SmokeGrenadeProjectile:
				case ClassId::BreachChargeProjectile:
				case ClassId::BumpMineProjectile:
				case ClassId::DecoyProjectile:
				case ClassId::MolotovProjectile:
				case ClassId::SensorGrenadeProjectile:
				case ClassId::SnowballProjectile:
					if (const auto it = std::find(projectileData.begin(), projectileData.end(), entity->handle()); it != projectileData.end())
						it->update(entity);
					else
						projectileData.emplace_front(entity);
					break;
				case ClassId::PropDynamic:
					if (const auto model = entity->getModel(); !model || !std::strstr(model->name, "challenge_coin"))
						break;
					[[fallthrough]];
				case ClassId::EconEntity:
				case ClassId::Chicken:
				case ClassId::PlantedC4:
				case ClassId::Hostage:
				case ClassId::Dronegun:
				case ClassId::Cash:
				case ClassId::AmmoBox:
				case ClassId::RadarJammer:
				case ClassId::SnowballPile:
					entityData.emplace_back(entity);
					break;
				case ClassId::LootCrate:
					lootCrateData.emplace_back(entity);
					break;
				case ClassId::Inferno:
					infernoData.emplace_back(entity);
					break;
				}

				if (classId == ClassId::SmokeGrenadeProjectile && entity->didSmokeEffect())
					smokeData.emplace_back(entity);
			}
		}
	}

	activeSounds.destructAll();
	interfaces->engineSound->getActiveSounds(activeSounds);

	for (auto &sound : activeSounds)
	{
		if (sound.soundSource < 1 || sound.soundSource > 64)
			continue;

		if (!sound.origin->notNull())
			continue;

		if (const auto entity = interfaces->entityList->getEntity(sound.soundSource); entity && entity->isPlayer())
		{
			if (auto player = playerByHandleWritable(entity->handle()); player)
			{
				player->becameDormant = memory->globalVars->realTime;
				player->audible = true;

				if (entity->isDormant())
				{
					const auto delta = *sound.origin - player->origin;
					player->coordinateFrame.setOrigin(*sound.origin);
					player->origin = *sound.origin;
					player->headMaxs += delta;
					player->headMins += delta;
					for (auto &bone : player->bones)
					{
						bone.first += delta;
						bone.second += delta;
					}
				}
			}
		}
	}

	std::sort(playerData.begin(), playerData.end());
	std::sort(weaponData.begin(), weaponData.end());
	std::sort(entityData.begin(), entityData.end());
	std::sort(lootCrateData.begin(), lootCrateData.end());

	std::for_each(projectileData.begin(), projectileData.end(), [](auto &projectile)
	{
		if (interfaces->entityList->getEntityFromHandle(projectile.handle) == nullptr)
			projectile.exploded = true;
	});

	std::erase_if(projectileData, [](const auto &projectile)
	{
		return interfaces->entityList->getEntityFromHandle(projectile.handle) == nullptr
			&& (projectile.trajectory.size() < 1 || projectile.trajectory[projectile.trajectory.size() - 1].first + 60.0f < memory->globalVars->realTime);
	});

	std::erase_if(playerData, [](const auto &player) { return interfaces->entityList->getEntityFromHandle(player.handle) == nullptr; });
}

void GameData::clearProjectileList() noexcept
{
	Lock lock;
	projectileData.clear();
}

const Matrix4x4 &GameData::toScreenMatrix() noexcept
{
	return viewMatrix;
}

const LocalPlayerData &GameData::local() noexcept
{
	return localPlayerData;
}

const std::vector<PlayerData> &GameData::players() noexcept
{
	return playerData;
}

const std::vector<ObserverData> &GameData::observers() noexcept
{
	return observerData;
}

const std::vector<WeaponData> &GameData::weapons() noexcept
{
	return weaponData;
}

const std::vector<EntityData> &GameData::entities() noexcept
{
	return entityData;
}

const std::vector<LootCrateData> &GameData::lootCrates() noexcept
{
	return lootCrateData;
}

const std::list<ProjectileData> &GameData::projectiles() noexcept
{
	return projectileData;
}

const BombData &GameData::plantedC4() noexcept
{
	return bombData;
}

const std::vector<InfernoData> &GameData::infernos() noexcept
{
	return infernoData;
}

const std::vector<SmokeData> &GameData::smokes() noexcept
{
	return smokeData;
}

const SessionData &GameData::session() noexcept
{
	return sessionData;
}

void LocalPlayerData::update() noexcept
{
	observerTargetHandle = -1;

	if (!localPlayer)
	{
		exists = false;
		return;
	}

	exists = true;
	alive = localPlayer->isAlive();

	fov = localPlayer->fov() ? localPlayer->fov() : localPlayer->defaultFov();
	handle = localPlayer->handle();
	flashDuration = localPlayer->flashDuration();

	eyePosition = Vector{};
	aimPunch = Vector{};
	inaccuracy = Vector{};
	reloading = false;
	shooting = false;
	drawingScope = true;
	drawingCrosshair = true;

	const auto obsMode = localPlayer->getObserverMode();
	if (const auto obs = localPlayer->getObserverTarget(); obs && (obsMode == ObsMode::InEye || obsMode == ObsMode::Chase))
	{
		observerTargetHandle = obs->handle();

		origin = obs->getAbsOrigin();
		velocity = obs->velocity();

		const auto collidable = obs->getCollideable();
		if (collidable)
		{
			obbMaxs = collidable->obbMaxs();
			obbMins = collidable->obbMins();
		}

		eyePosition = obs->getEyePosition();
		aimPunch = eyePosition + Vector::fromAngle(interfaces->engine->getViewAngles() + obs->getAimPunch()) * 1000;

		if (const auto activeWeapon = obs->getActiveWeapon(); activeWeapon && obs->isAlive())
		{
			inaccuracy = eyePosition + Vector::fromAngle(interfaces->engine->getViewAngles() + Vector{Helpers::radiansToDegrees(activeWeapon->getInaccuracy() + activeWeapon->getSpread()), 0.0f, 0.0f}) * 1000;
			shooting = obs->shotsFired() > 1;
			reloading = activeWeapon->isInReload();
			nextAttack = std::fmaxf(activeWeapon->nextPrimaryAttack(), obs->nextAttack());
			drawingScope = (activeWeapon->isSniperRifle() || !config->visuals.noWeapons) && obs->isScoped();
			drawingCrosshair = (!activeWeapon->isSniperRifle() || config->visuals.forceCrosshair == 1) && config->visuals.forceCrosshair != 2;
		}
	} else
	{
		origin = localPlayer->getAbsOrigin();
		velocity = localPlayer->velocity();

		const auto collidable = localPlayer->getCollideable();
		if (collidable)
		{
			obbMaxs = collidable->obbMaxs();
			obbMins = collidable->obbMins();
		}

		eyePosition = localPlayer->getEyePosition();
		aimPunch = eyePosition + Vector::fromAngle(interfaces->engine->getViewAngles() + localPlayer->getAimPunch()) * 1000;

		if (const auto activeWeapon = localPlayer->getActiveWeapon(); activeWeapon && localPlayer->isAlive())
		{
			inaccuracy = eyePosition + Vector::fromAngle(interfaces->engine->getViewAngles() + Vector{Helpers::radiansToDegrees(activeWeapon->getInaccuracy() + activeWeapon->getSpread()), 0.0f, 0.0f}) * 1000;
			shooting = localPlayer->shotsFired() > 1;
			reloading = activeWeapon->isInReload();
			nextAttack = std::fmaxf(activeWeapon->nextPrimaryAttack(), localPlayer->nextAttack());
			drawingScope = (activeWeapon->isSniperRifle() || !config->visuals.noWeapons) && localPlayer->isScoped();
			drawingCrosshair = (!activeWeapon->isSniperRifle() || config->visuals.forceCrosshair == 1) && config->visuals.forceCrosshair != 2;
		}
	}
}

BaseData::BaseData(Entity *entity) noexcept
{
	distanceToLocal = entity->getAbsOrigin().distTo(localPlayerData.origin);

	obbMins = Vector{};
	obbMaxs = Vector{};

	if (entity->isPlayer())
	{
		const auto collideable = entity->getCollideable();
		obbMins = collideable->obbMins();
		obbMaxs = collideable->obbMaxs();
	} else if (const auto model = entity->getModel())
	{
		obbMins = model->mins;
		obbMaxs = model->maxs;
	}

	coordinateFrame = entity->toWorldTransform();
}

EntityData::EntityData(Entity *entity) noexcept : BaseData{entity}
{
	name = [](Entity *entity)
	{
		switch (entity->getClientClass()->classId)
		{
		case ClassId::EconEntity: return "Defuse Kit";
		case ClassId::Chicken: return "Chicken";
		case ClassId::PlantedC4: return "Planted C4";
		case ClassId::Hostage: return "Hostage";
		case ClassId::Dronegun: return "Sentry";
		case ClassId::Cash: return "Cash";
		case ClassId::AmmoBox: return "Ammo Box";
		case ClassId::RadarJammer: return "Radar Jammer";
		case ClassId::SnowballPile: return "Snowball Pile";
		case ClassId::PropDynamic: return "Collectable Coin";
		default: assert(false); return "unknown";
		}
	}(entity);
}

ProjectileData::ProjectileData(Entity *projectile) noexcept : BaseData{projectile}
{
	name = [](Entity *projectile)
	{
		switch (projectile->getClientClass()->classId)
		{
		case ClassId::GrenadeProjectile:
			if (const auto model = projectile->getModel(); model && strstr(model->name, "flashbang"))
				return "Flashbang";
			else
				return "HE Grenade";
		case ClassId::BreachChargeProjectile: return "Breach Charge";
		case ClassId::BumpMineProjectile: return "Bump Mine";
		case ClassId::DecoyProjectile: return "Decoy Grenade";
		case ClassId::MolotovProjectile: return "Molotov";
		case ClassId::SensorGrenadeProjectile: return "TA Grenade";
		case ClassId::SmokeGrenadeProjectile: return "Smoke Grenade";
		case ClassId::SnowballProjectile: return "Snowball";
		default: assert(false); return "unknown";
		}
	}(projectile);

	if (const auto thrower = interfaces->entityList->getEntityFromHandle(projectile->thrower()); thrower && localPlayer)
	{
		if (thrower == localPlayer.get())
			thrownByLocalPlayer = true;
		else
			thrownByEnemy = localPlayer->isOtherEnemy(thrower);
	}

	handle = projectile->handle();
}

void ProjectileData::update(Entity *projectile) noexcept
{
	static_cast<BaseData &>(*this) = {projectile};

	if (const auto &pos = projectile->getAbsOrigin(); trajectory.size() < 1 || trajectory[trajectory.size() - 1].second != pos)
		trajectory.emplace_back(memory->globalVars->realTime, pos);
}

void PlayerData::update(Entity *entity) noexcept
{
	handle = entity->handle();
	name = entity->getPlayerName();
	inViewFrustum = !interfaces->engine->cullBox(obbMins + origin, obbMaxs + origin);
	audible = false;

	if (entity->isDormant())
	{
		if (!dormant)
			becameDormant = memory->globalVars->realTime;
		dormant = true;
		return;
	}

	dormant = false;

	static_cast<BaseData &>(*this) = {entity};
	origin = entity->getAbsOrigin();
	velocity = entity->velocity();
	alive = entity->isAlive();

	enemy = localPlayer->isOtherEnemy(entity);
	if (localPlayer->isAlive() || localPlayer->observerMode() != ObsMode::InEye)
		visible = alive && entity->visibleTo(localPlayer.get());
	else if (const auto obs = localPlayer->getObserverTarget(); obs)
		visible = alive && entity->visibleTo(obs);
	
	spotted = entity->spotted();
	health = entity->health();
	armor = entity->armor();

	isBot = entity->isBot();
	hasBomb = entity->hasC4();
	isVip = entity->isVip();
	hasDefuser = entity->hasDefuser();
	ducking = entity->flags() & PlayerFlag_Crouched;

	if (previousUpdateTick != memory->globalVars->tickCount)
	{
		previousUpdateTick = memory->globalVars->tickCount;
		lbyUpdate = entity->trackLbyUpdate(nextLbyUpdate);
		if (entity->isChokingPackets())
		{
			if (chokedPackets < 0) chokedPackets = 0;
			chokedPackets++;
		}
		else
		{
			if (chokedPackets > 0) chokedPackets = 0;
			chokedPackets--;
		}
	}

	{
		const Vector start = entity->getEyePosition();
		const Vector end = start + Vector::fromAngle(entity->eyeAngles()) * 1000.0f;

		Trace trace;
		interfaces->engineTrace->traceRay({start, end}, ALL_VISIBLE_CONTENTS | CONTENTS_MOVEABLE | CONTENTS_DETAIL, entity, trace);
		lookingAt = trace.endPos;
	}

	scoped = false;
	reloading = false;

	if (const auto activeWeapon = entity->getActiveWeapon())
	{
		scoped = entity->isScoped();
		reloading = activeWeapon->isInReload();
	}

	flashDuration = entity->flashDuration();

	if (const auto weapon = entity->getActiveWeapon())
	{
		if (const auto weaponInfo = weapon->getWeaponData())
			activeWeapon = interfaces->localize->findAsUTF8(weaponInfo->name);
	}

	if (!inViewFrustum) return;

	const auto model = entity->getModel();
	if (!model)
		return;

	const auto studioModel = interfaces->modelInfo->getStudioModel(model);
	if (!studioModel)
		return;

	const auto &boneMatrices = entity->boneCache();

	bones.clear();
	bones.reserve(20);

	for (int i = 0; i < studioModel->numBones; ++i)
	{
		const auto bone = studioModel->getBone(i);

		if (!bone || bone->parent == -1 || !(bone->flags & BONE_USED_BY_HITBOX))
			continue;

		bones.emplace_back(boneMatrices[i].origin(), boneMatrices[bone->parent].origin());
	}

	const auto set = studioModel->getHitboxSet(entity->hitboxSet());

	if (!set)
		return;

	const auto headBox = set->getHitbox(0);

	headMins = headBox->bbMin.transform(boneMatrices[headBox->bone]);
	headMaxs = headBox->bbMax.transform(boneMatrices[headBox->bone]);

	if (headBox->capsuleRadius > 0.0f)
	{
		headMins -= headBox->capsuleRadius;
		headMaxs += headBox->capsuleRadius;
	}
}

WeaponData::WeaponData(Entity *entity) noexcept : BaseData{entity}
{
	clip = entity->clip();
	reserveAmmo = entity->reserveAmmoCount();

	if (const auto weaponInfo = entity->getWeaponData())
	{
		group = [](WeaponType type, WeaponId weaponId)
		{
			switch (type)
			{
			case WeaponType::Pistol: return "Pistols";
			case WeaponType::SubMachinegun: return "SMGs";
			case WeaponType::Rifle: return "Rifles";
			case WeaponType::SniperRifle: return "Sniper Rifles";
			case WeaponType::Shotgun: return "Shotguns";
			case WeaponType::Machinegun: return "Machineguns";
			case WeaponType::Grenade: return "Grenades";
			case WeaponType::Melee: return "Melee";
			default:
				switch (weaponId)
				{
				case WeaponId::C4:
				case WeaponId::Healthshot:
				case WeaponId::BumpMine:
				case WeaponId::ZoneRepulsor:
				case WeaponId::Shield:
					return "Other";
				default: return "All";
				}
			}
		}(weaponInfo->type, entity->itemDefinitionIndex2());
		name = [](WeaponId weaponId)
		{
			switch (weaponId)
			{
			default: return "All";

			case WeaponId::Glock: return "Glock-18";
			case WeaponId::Hkp2000: return "P2000";
			case WeaponId::Usp_s: return "USP-S";
			case WeaponId::Elite: return "Dual Berettas";
			case WeaponId::P250: return "P250";
			case WeaponId::Tec9: return "Tec-9";
			case WeaponId::Fiveseven: return "Five-SeveN";
			case WeaponId::Cz75a: return "CZ75-Auto";
			case WeaponId::Deagle: return "Desert Eagle";
			case WeaponId::Revolver: return "R8 Revolver";

			case WeaponId::Mac10: return "MAC-10";
			case WeaponId::Mp9: return "MP9";
			case WeaponId::Mp7: return "MP7";
			case WeaponId::Mp5sd: return "MP5-SD";
			case WeaponId::Ump45: return "UMP-45";
			case WeaponId::P90: return "P90";
			case WeaponId::Bizon: return "PP-Bizon";

			case WeaponId::GalilAr: return "Galil AR";
			case WeaponId::Famas: return "FAMAS";
			case WeaponId::Ak47: return "AK-47";
			case WeaponId::M4A1: return "M4A4";
			case WeaponId::M4a1_s: return "M4A1-S";
			case WeaponId::Sg553: return "SG 553";
			case WeaponId::Aug: return "AUG";

			case WeaponId::Ssg08: return "SSG 08";
			case WeaponId::Awp: return "AWP";
			case WeaponId::G3SG1: return "G3SG1";
			case WeaponId::Scar20: return "SCAR-20";

			case WeaponId::Nova: return "Nova";
			case WeaponId::Xm1014: return "XM1014";
			case WeaponId::Sawedoff: return "Sawed-Off";
			case WeaponId::Mag7: return "MAG-7";

			case WeaponId::M249: return "M249";
			case WeaponId::Negev: return "Negev";

			case WeaponId::Flashbang: return "Flashbang";
			case WeaponId::HeGrenade: return "HE Grenade";
			case WeaponId::SmokeGrenade: return "Smoke Grenade";
			case WeaponId::Molotov: return "Molotov";
			case WeaponId::Decoy: return "Decoy Grenade";
			case WeaponId::IncGrenade: return "Incendiary";
			case WeaponId::TaGrenade: return "TA Grenade";
			case WeaponId::Firebomb: return "Fire Bomb";
			case WeaponId::Diversion: return "Diversion";
			case WeaponId::FragGrenade: return "Frag Grenade";
			case WeaponId::Snowball: return "Snowball";

			case WeaponId::Axe: return "Axe";
			case WeaponId::Hammer: return "Hammer";
			case WeaponId::Spanner: return "Wrench";

			case WeaponId::C4: return "C4";
			case WeaponId::Healthshot: return "Healthshot";
			case WeaponId::BumpMine: return "Bump Mine";
			case WeaponId::ZoneRepulsor: return "Zone Repulsor";
			case WeaponId::Shield: return "Shield";
			}
		}(entity->itemDefinitionIndex2());

		displayName = interfaces->localize->findAsUTF8(weaponInfo->name);
	}
}

LootCrateData::LootCrateData(Entity *entity) noexcept : BaseData{entity}
{
	const auto model = entity->getModel();
	if (!model)
		return;

	name = [](const char *modelName) -> const char *
	{
		switch (fnv::hashRuntime(modelName))
		{
		case fnv::hash("models/props_survival/cases/case_pistol.mdl"): return "Pistol Case";
		case fnv::hash("models/props_survival/cases/case_light_weapon.mdl"): return "Light Case";
		case fnv::hash("models/props_survival/cases/case_heavy_weapon.mdl"): return "Heavy Case";
		case fnv::hash("models/props_survival/cases/case_explosive.mdl"): return "Explosive Case";
		case fnv::hash("models/props_survival/cases/case_tools.mdl"): return "Tools Case";
		case fnv::hash("models/props_survival/cash/dufflebag.mdl"): return "Cash Dufflebag";
		default: return nullptr;
		}
	}(model->name);
}

const PlayerData *GameData::playerByHandle(int handle) noexcept
{
	return playerByHandleWritable(handle);
}

ObserverData::ObserverData(Entity *entity, Entity *obs, bool targetIsLocalPlayer, bool targetIsObservedByLocalPlayer) noexcept
{
	entity->getPlayerName(name);
	obs->getPlayerName(target);
	this->targetIsLocalPlayer = targetIsLocalPlayer;
	this->targetIsObservedByLocalPlayer = targetIsObservedByLocalPlayer;
}

void BombData::update() noexcept
{
	if (memory->plantedC4s->size > 0 && (!*memory->gameRules || (*memory->gameRules)->mapHasBombTarget()))
	{
		if (Entity *bomb = (*memory->plantedC4s)[0]; bomb && bomb->c4Ticking())
		{
			blowTime = bomb->c4BlowTime();
			timerLength = bomb->c4TimerLength();
			defuserHandle = bomb->c4Defuser();
			if (defuserHandle != -1)
			{
				defuseCountDown = bomb->c4DefuseCountDown();
				defuseLength = bomb->c4DefuseLength();
			}

			if (*memory->playerResource)
			{
				const auto &bombOrigin = bomb->origin();
				bombsite = bombOrigin.distTo((*memory->playerResource)->bombsiteCenterA()) > bombOrigin.distTo((*memory->playerResource)->bombsiteCenterB());
			}
			return;
		}
	}
	defuserHandle = bombsite = -1;
	blowTime = 0.0f;
}

InfernoData::InfernoData(Entity *inferno) noexcept
{
	const auto &origin = inferno->getAbsOrigin();

	points.reserve(inferno->fireCount());
	for (int i = 0; i < inferno->fireCount(); ++i)
	{
		if (inferno->fireIsBurning()[i])
			points.emplace_back(Vector{inferno->fireXDelta()[i] + origin.x, inferno->fireYDelta()[i] + origin.y, inferno->fireZDelta()[i] + origin.z});
	}
}

SmokeData::SmokeData(Entity *smoke) noexcept
{
	origin = smoke->getAbsOrigin();
}

void SessionData::update() noexcept
{
	const auto networkChannel = interfaces->engine->getNetworkChannel();
	if (networkChannel)
	{
		connected = true;
		address = networkChannel->getAddress();
		latency = static_cast<int>(networkChannel->getLatency(0) * 1000);
	} else
		connected = false;

	tickrate = static_cast<int>(1 / memory->globalVars->intervalPerTick);
	levelName = interfaces->engine->getLevelName();
}
