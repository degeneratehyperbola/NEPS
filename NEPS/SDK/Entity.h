#pragma once

#include "AnimState.h"
#include "Engine.h"
#include "EntityList.h"
#include "GlobalVars.h"
#include "LocalPlayer.h"
#include "Matrix3x4.h"
#include "ModelInfo.h"
#include "UtlVector.h"
#include "Vector.h"
#include "VirtualMethod.h"
#include "WeaponData.h"

#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../Netvars.h"

#include <functional>

struct ClientClass;
struct Model;
struct VarMap;

#define PLAYER_EYE_HEIGHT 64.093811f
#define PLAYER_EYE_HEIGHT_CROUCH 46.076218f
#define PLAYER_HEIGHT 72.0f
#define PLAYER_HEIGHT_CROUCH 54.0f

enum class MoveType
{
	Noclip = 8,
	Ladder = 9
};

enum PlayerFlag
{
	PlayerFlag_OnGround = 1 << 0, // At rest / on the ground
	PlayerFlag_Crouched = 1 << 1, // Player is fully crouched
	PlayerFlag_WaterJump = 1 << 2, // Player jumping out of water
	PlayerFlag_OnTrain = 1 << 3, // Player is controlling a train, so movement commands should be ignored on client during prediction
	PlayerFlag_InRain = 1 << 4, // Indicates the entity is standing in rain
	PlayerFlag_Frozen = 1 << 5, // Player is frozen for 3rd person camera
	PlayerFlag_AtControls = 1 << 6, // Player can't move, but keeps key inputs for controlling another entity
	PlayerFlag_Client = 1 << 7, // Is a player
	PlayerFlag_FakeClient = 1 << 8, // Fake client, simulated server side; don't send network messages to them. NON-PLAYER SPECIFIC (i.e. not used by GameMovement or the client.dll) - can still be applied to players, though
	PlayerFlag_InWater = 1 << 9, // In water
	PlayerFlag_Fly = 1 << 10, // Changes the SV_Movestep() behavior to not need to be on ground
	PlayerFlag_Swim = 1 << 11, // Changes the SV_Movestep() behavior to not need to be on ground (but stay in water)
	PlayerFlag_Conveyor = 1 << 12,
	PlayerFlag_NPC = 1 << 13,
	PlayerFlag_GodMode = 1 << 14,
	PlayerFlag_NoTarget = 1 << 15,
	PlayerFlag_AimTarget = 1 << 16, // Set if the crosshair needs to aim onto the entity
	PlayerFlag_PartialGround = 1 << 17, // Not all corners are valid
	PlayerFlag_StaticProp = 1 << 18, // Eetsa static prop!
	PlayerFlag_Graphed = 1 << 19, // Worldgraph has this ent listed as something that blocks a connection
	PlayerFlag_Grenade = 1 << 20,
	PlayerFlag_StepMovement = 1 << 21, // Changes the SV_Movestep() behavior to not do any processing
	PlayerFlag_NoTouch = 1 << 22, // Doesn't generate touch functions, generates Untouch() for anything it was touching when this flag was set
	PlayerFlag_BaseVelocity = 1 << 23, // Base velocity has been applied this frame (used to convert base velocity into momentum)
	PlayerFlag_WorldBrush = 1 << 24, // Not moveable/removeable brush entity (really part of the world, but represented as an entity for transparency or something)
	PlayerFlag_NPCSpecific = 1 << 25, // This is an object that NPCs should see. Missiles, for example
	PlayerFlag_KillMe = 1 << 26, // This entity is marked for death -- will be freed by game DLL
	PlayerFlag_OnFire = 1 << 27, // You know...
	PlayerFlag_Dissolving = 1 << 28, // We're dissolving!
	PlayerFlag_TransitionRagdoll = 1 << 29, // In the process of turning into a client side ragdoll
	PlayerFlag_NotBlockableByPlayer = 1 << 30 // Pusher that can't be blocked by the player
};

enum EffectFlag
{
	EffectFlag_BoneMerge = 1 << 0, // Performs bone merge on client side
	EffectFlag_BrightLight = 1 << 1, // Dlight centered at entity origin
	EffectFlag_Flashlight = 1 << 2, // Player flashlight
	EffectFlag_NoInterp = 1 << 3, // Don't interpolate the next frame
	EffectFlag_NoShadow = 1 << 4, // Don't cast shadow
	EffectFlag_NoDraw = 1 << 5, // Don't draw entity
	EffectFlag_NoRecieveShadow = 1 << 6, // Don't receive shadow
	EffectFlag_BoneMergeFastCull = 1 << 7, // For use with EffectFlag_BoneMerge
	EffectFlag_ItemBlink = 1 << 8, // Blink an item so that the player notices it
	EffectFlag_ParentAnimates = 1 << 9, // Always assume that the parent entity is animating
	EffectFlag_FastReflection = 1 << 10, // Marks an entity for reflection rendering when using $reflectonlymarkedentities material variable
	EffectFlag_NoShadowDepth = 1 << 11, // Indicates this entity does not render into any shadow depthmap
	EffectFlag_NoShadowDepthCache = 1 << 12, // Indicates this entity cannot be cached in shadow depthmap and should render every frame
	EffectFlag_NoFlashlight = 1 << 13,
	EffectFlag_NoCSM = 1 << 14 // Indicates this entity does not render into the cascade shadow depthmap
};

enum class ObsMode
{
	None = 0,
	Deathcam,
	Freezecam,
	Fixed,
	InEye,
	Chase,
	Roaming
};

class Collideable
{
public:
	VIRTUAL_METHOD(const Vector &, obbMins, 1, (), (this))
	VIRTUAL_METHOD(const Vector &, obbMaxs, 2, (), (this))
};

enum class Team
{
	None = 0,
	Spectators,
	TT,
	CT
};

class Entity
{
public:
	VIRTUAL_METHOD(void, release, 1, (), (this + sizeof(uintptr_t) * 2))
	VIRTUAL_METHOD(ClientClass *, getClientClass, 2, (), (this + sizeof(uintptr_t) * 2))
	VIRTUAL_METHOD(void, preDataUpdate, 6, (int updateType), (this + sizeof(uintptr_t) * 2, updateType))
	VIRTUAL_METHOD(void, postDataUpdate, 7, (int updateType), (this + sizeof(uintptr_t) * 2, updateType))
	VIRTUAL_METHOD(bool, isDormant, 9, (), (this + sizeof(uintptr_t) * 2))
	VIRTUAL_METHOD(int, index, 10, (), (this + sizeof(uintptr_t) * 2))
	VIRTUAL_METHOD(void, setDestroyedOnRecreateEntities, 13, (), (this + sizeof(uintptr_t) * 2))

	VIRTUAL_METHOD(Vector &, getRenderOrigin, 1, (), (this + sizeof(uintptr_t)))
	VIRTUAL_METHOD(bool, shouldDraw, 3, (), (this + sizeof(uintptr_t)))
	VIRTUAL_METHOD(const Model *, getModel, 8, (), (this + sizeof(uintptr_t)))
	VIRTUAL_METHOD(const Matrix3x4 &, toWorldTransform, 32, (), (this + sizeof(uintptr_t)))

	VIRTUAL_METHOD(int &, handle, 2, (), (this))
	VIRTUAL_METHOD(Collideable *, getCollideable, 3, (), (this))
	VIRTUAL_METHOD(const Vector &, getAbsOrigin, 10, (), (this))
	VIRTUAL_METHOD(const Vector &, getAbsAngle, 11, (), (this))
	VIRTUAL_METHOD(void, setModelIndex, 75, (int index), (this, index))
	VIRTUAL_METHOD(bool, getAttachment, 84, (int index, Vector &origin), (this, index, std::ref(origin)))
	VIRTUAL_METHOD(int, health, 122, (), (this))
	VIRTUAL_METHOD(bool, isAlive, 156, (), (this))
	VIRTUAL_METHOD(bool, isPlayer, 158, (), (this))
	VIRTUAL_METHOD(bool, isWeapon, 166, (), (this))
	VIRTUAL_METHOD(void, updateClientSideAnimation, 224, (), (this))
	VIRTUAL_METHOD(int, getWeaponSubType, 282, (), (this))
	VIRTUAL_METHOD(ObsMode, getObserverMode, 294, (), (this))
	VIRTUAL_METHOD(float, getSpread, 453, (), (this))
	VIRTUAL_METHOD(WeaponType, getWeaponType, 455, (), (this))
	VIRTUAL_METHOD(WeaponInfo *, getWeaponData, 461, (), (this))
	VIRTUAL_METHOD(int, getMuzzleAttachmentIndex1stPerson, 468, (Entity *viewModel), (this, viewModel))
	VIRTUAL_METHOD(int, getMuzzleAttachmentIndex3rdPerson, 469, (), (this))
	VIRTUAL_METHOD(float, getInaccuracy, 483, (), (this))
	VIRTUAL_METHOD(void, updateInaccuracyPenalty, 484, (), (this))

	Entity *getObserverTarget() noexcept
	{
		Entity *entity = VirtualMethod::call<Entity *, 295>(this);
		if (entity)
			return entity->isPlayer() ? entity : nullptr;
		return nullptr;
	}

	Entity *getActiveWeapon() noexcept { return interfaces->entityList->getEntityFromHandle(activeWeapon()); }

	Vector getEyePosition() noexcept
	{
		if (this == localPlayer.get())
			return getAbsOrigin() + viewOffset();

		Vector v;
		VirtualMethod::call<void, 285>(this, std::ref(v));
		return v;
	}

	Vector getAimPunch() noexcept
	{
		Vector v;
		VirtualMethod::call<void, 346>(this, std::ref(v));
		return v;
	}

	UtlVector<Matrix3x4> &boneCache() noexcept { return *(UtlVector<Matrix3x4> *)((uintptr_t)this + 0x2914); }
	Vector getBonePosition(int bone) noexcept { return boneCache()[bone].origin(); }

	AnimLayer *animLayers() noexcept { return *reinterpret_cast<AnimLayer **>((uintptr_t)this + 0x2990); }
	int getAnimLayerCount() noexcept { return *reinterpret_cast<int *>((uintptr_t)this + 0x299C); }

	std::array<float, PoseParam_Count> &poseParams() noexcept
	{
		return *reinterpret_cast<std::array<float, PoseParam_Count> *>((uintptr_t)this + netvars->operator[](fnv::hash("CBaseAnimating->m_flPoseParameter")));
	}

	auto isPistol() noexcept { return getWeaponType() == WeaponType::Pistol; }
	auto isSniperRifle() noexcept { return getWeaponType() == WeaponType::SniperRifle; }
	auto isGrenade() noexcept { return getWeaponType() == WeaponType::Grenade; }
	auto isC4() noexcept { return getWeaponType() == WeaponType::C4; }
	auto isKnife() noexcept { return getWeaponType() == WeaponType::Knife; }
	auto isTablet() noexcept { return getWeaponType() == WeaponType::Tablet; }
	auto isShotgun() noexcept { return getWeaponType() == WeaponType::Shotgun; }

	auto isFullAuto() noexcept
	{
		const auto weaponData = getWeaponData();
		if (weaponData)
			return weaponData->fullAuto;
		return false;
	}

	auto requiresRecoilControl() noexcept
	{
		const auto weaponData = getWeaponData();
		if (weaponData)
			return weaponData->recoilMagnitude < 35.0f && weaponData->recoveryTimeStand > weaponData->cycletime;
		return false;
	}

	bool setupBones(Matrix3x4 *out, int maxBones, int boneMask, float currentTime) noexcept
	{
		if (config->misc.fixBoneMatrices && this == localPlayer.get())
		{
			int *render = reinterpret_cast<int *>(this + 0x274);
			int backupRender = *render;
			int backupEffects = effectFlags();
			int backupShouldSkipFrame = shouldSkipFrame();
			Vector absOrigin = getAbsOrigin();
			*render = 0;
			shouldSkipFrame() = 0;
			effectFlags() |= EffectFlag_NoInterp;
			memory->setAbsOrigin(this, origin());
			auto result = VirtualMethod::call<bool, 13>(this + 4, out, maxBones, boneMask, currentTime);
			memory->setAbsOrigin(this, absOrigin);
			*render = backupRender;
			shouldSkipFrame() = backupShouldSkipFrame;
			effectFlags() = backupEffects;
			return result;
		}
		if (config->misc.fixBoneMatrices)
		{
			int *render = reinterpret_cast<int *>(this + 0x274);
			int backup = *render;
			Vector absOrigin = getAbsOrigin();
			*render = 0;
			memory->setAbsOrigin(this, origin());
			auto result = VirtualMethod::call<bool, 13>(this + sizeof(uintptr_t), out, maxBones, boneMask, currentTime);
			memory->setAbsOrigin(this, absOrigin);
			*render = backup;
			return result;
		}

		return VirtualMethod::call<bool, 13>(this + 4, out, maxBones, boneMask, currentTime);
	}

	StudioHitboxSet *getHitboxSet()
	{
		const auto model = getModel();
		if (!model) return nullptr;
		const auto studioModel = interfaces->modelInfo->getStudioModel(model);
		if (!studioModel) return nullptr;
		return studioModel->getHitboxSet(hitboxSet());
	}
	StudioBbox *getHitbox(int i)
	{
		const auto set = getHitboxSet();
		if (!set) return nullptr;
		return set->getHitbox(i);
	}

	bool isOtherEnemy(Entity *other) noexcept;

	VarMap *getVarMap() noexcept { return reinterpret_cast<VarMap *>(this + 0x24); }

	int isChokingPackets() noexcept { return simulationTime() == oldSimulationTime(); };

	bool trackLbyUpdate(float &nextUpdate) noexcept
	{
		if (!isPlayer())
			return false;

		const auto time = memory->globalVars->serverTime();

		if (velocity().length2D() > 0.1f || std::fabsf(velocity().z) > 100.0f)
			nextUpdate = time + 0.22f;
		else if (time >= nextUpdate)
		{
			nextUpdate = time + 1.1f;
			return true;
		}

		return false;
	}

	float getMaxDesyncAngle() noexcept
	{
		const auto state = animState();
		if (!state)
			return 0.0f;

		float yawModifier = (state->stopToFullRunningFraction * -0.3f - 0.2f) * std::clamp(state->speedAsPortionOfWalkSpeed, 0.0f, 1.0f) + 1.0f;

		if (state->duckAmount > 0.0f)
			yawModifier += (state->duckAmount * std::clamp(state->speedAsPortionOfCrouchWalkSpeed, 0.0f, 1.0f) * (0.5f - yawModifier));

		return state->velocitySubtract.y * yawModifier;
	}

	auto getUserId() noexcept
	{
		if (PlayerInfo playerInfo; interfaces->engine->getPlayerInfo(index(), playerInfo))
			return playerInfo.userId;

		return -1;
	}

	bool isBot()
	{
		if (PlayerInfo playerInfo; interfaces->engine->getPlayerInfo(index(), playerInfo))
			return playerInfo.fakeplayer;

		return true;
	}

	bool hasC4() noexcept;
	bool isVip() noexcept;

	void getPlayerName(char(&out)[128]) noexcept;
	[[nodiscard]] std::string getPlayerName() noexcept
	{
		char name[128];
		getPlayerName(name);
		return name;
	}

	bool canSee(Entity *other, const Vector &pos) noexcept;
	bool visibleTo(Entity *other) noexcept;

	bool grenadeExploded() noexcept { return *reinterpret_cast<bool *>(this + 0x29E8); }

	bool isFlashed() noexcept { return flashDuration() > 75.0f; }

	int &effectFlags() noexcept { return *reinterpret_cast<int *>(this + 0xF0); }

	int &shouldSkipFrame() noexcept { return *reinterpret_cast<int *>(this + 0xA68); }

	NETVAR(clientAnimations, "CBaseAnimating", "m_bClientSideAnimation", bool)
	NETVAR(body, "CBaseAnimating", "m_nBody", int)
	NETVAR(hitboxSet, "CBaseAnimating", "m_nHitboxSet", int)
	NETVAR(frozen, "CBaseAnimating", "m_flFrozen", float)
	NETVAR_OFFSET(useFastPipeline, "CBaseAnimating", "m_flFrozen", 4, bool)

	NETVAR(modelIndex, "CBaseEntity", "m_nModelIndex", unsigned)
	NETVAR(origin, "CBaseEntity", "m_vecOrigin", Vector)
	NETVAR(rotation, "CBaseEntity", "m_angRotation", Vector)
	NETVAR_OFFSET(moveType, "CBaseEntity", "m_nRenderMode", 1, MoveType)
	NETVAR(simulationTime, "CBaseEntity", "m_flSimulationTime", float)
	NETVAR_OFFSET(oldSimulationTime, "CBaseEntity", "m_flSimulationTime", 4, float)
	NETVAR(ownerEntity, "CBaseEntity", "m_hOwnerEntity", int)
	NETVAR(team, "CBaseEntity", "m_iTeamNum", Team)
	NETVAR(spotted, "CBaseEntity", "m_bSpotted", bool)

	NETVAR(activeWeapon, "CBaseCombatCharacter", "m_hActiveWeapon", int)
	NETVAR(weapons, "CBaseCombatCharacter", "m_hMyWeapons", int[64])
	PNETVAR(wearables, "CBaseCombatCharacter", "m_hMyWearables", int)
	NETVAR(nextAttack, "CBaseCombatCharacter", "m_flNextAttack", float)

	NETVAR(viewModel, "CBasePlayer", "m_hViewModel[0]", int)
	NETVAR(fov, "CBasePlayer", "m_iFOV", int)
	NETVAR(fovStart, "CBasePlayer", "m_iFOVStart", int)
	NETVAR(defaultFov, "CBasePlayer", "m_iDefaultFOV", int)
	NETVAR(flags, "CBasePlayer", "m_fFlags", int)
	NETVAR(viewOffset, "CBasePlayer", "m_vecViewOffset[0]", Vector)
	NETVAR(tickBase, "CBasePlayer", "m_nTickBase", int)
	NETVAR(aimPunchAngle, "CBasePlayer", "m_aimPunchAngle", Vector)
	NETVAR(viewPunchAngle, "CBasePlayer", "m_viewPunchAngle", Vector)
	NETVAR(velocity, "CBasePlayer", "m_vecVelocity[0]", Vector)
	NETVAR(observerMode, "CBasePlayer", "m_iObserverMode", ObsMode)
	NETVAR(lastPlaceName, "CBasePlayer", "m_szLastPlaceName", char[18])
	NETVAR_OFFSET(thirdPersonAngles, "CBasePlayer", "deadflag", 4, Vector)

	NETVAR(armor, "CCSPlayer", "m_ArmorValue", int)
	NETVAR(eyeAngles, "CCSPlayer", "m_angEyeAngles", Vector)
	NETVAR_OFFSET(animState, "CCSPlayer", "m_bIsScoped", -20, AnimState *)
	NETVAR(isScoped, "CCSPlayer", "m_bIsScoped", bool)
	NETVAR_OFFSET(spawnTime, "CCSPlayer", "m_iAddonBits", -4, float)
	NETVAR(isDefusing, "CCSPlayer", "m_bIsDefusing", bool)
	NETVAR_OFFSET(flashDuration, "CCSPlayer", "m_flFlashMaxAlpha", -8, float)
	NETVAR(flashMaxAlpha, "CCSPlayer", "m_flFlashMaxAlpha", float)
	NETVAR(gunGameImmunity, "CCSPlayer", "m_bGunGameImmunity", bool)
	NETVAR(account, "CCSPlayer", "m_iAccount", int)
	NETVAR(inBombZone, "CCSPlayer", "m_bInBombZone", bool)
	NETVAR(hasDefuser, "CCSPlayer", "m_bHasDefuser", bool)
	NETVAR(hasHelmet, "CCSPlayer", "m_bHasHelmet", bool)
	NETVAR(lbyTarget, "CCSPlayer", "m_flLowerBodyYawTarget", float)
	NETVAR(ragdoll, "CCSPlayer", "m_hRagdoll", int)
	NETVAR(shotsFired, "CCSPlayer", "m_iShotsFired", int)
	NETVAR(waitForNoAttack, "CCSPlayer", "m_bWaitForNoAttack", bool)

	NETVAR(viewModelIndex, "CBaseCombatWeapon", "m_iViewModelIndex", int)
	NETVAR(worldModelIndex, "CBaseCombatWeapon", "m_iWorldModelIndex", int)
	NETVAR(worldDroppedModelIndex, "CBaseCombatWeapon", "m_iWorldDroppedModelIndex", int)
	NETVAR(weaponWorldModel, "CBaseCombatWeapon", "m_hWeaponWorldModel", int)
	NETVAR(clip, "CBaseCombatWeapon", "m_iClip1", int)
	NETVAR_OFFSET(isInReload, "CBaseCombatWeapon", "m_iClip1", 65, bool)
	NETVAR(reserveAmmoCount, "CBaseCombatWeapon", "m_iPrimaryReserveAmmoCount", int)
	NETVAR(nextPrimaryAttack, "CBaseCombatWeapon", "m_flNextPrimaryAttack", float)
	NETVAR(nextSecondaryAttack, "CBaseCombatWeapon", "m_flNextSecondaryAttack", float)

	NETVAR(lastShotTime, "CWeaponCSBase", "m_fLastShotTime", float)
	NETVAR(burstMode, "CWeaponCSBase", "m_bBurstMode", bool)
	NETVAR(postponeFireReadyTime, "CWeaponCSBase", "m_flPostponeFireReadyTime", float)

	NETVAR(burstShotsRemaining, "CWeaponCSBaseGun", "m_iBurstShotsRemaining", int)
	NETVAR_OFFSET(nextBurstShot, "CWeaponCSBaseGun", "m_iBurstShotsRemaining", 4, float)

	NETVAR(accountID, "CBaseAttributableItem", "m_iAccountID", int)
	NETVAR(itemDefinitionIndex, "CBaseAttributableItem", "m_iItemDefinitionIndex", short)
	NETVAR(itemDefinitionIndex2, "CBaseAttributableItem", "m_iItemDefinitionIndex", WeaponId)
	NETVAR(itemIDHigh, "CBaseAttributableItem", "m_iItemIDHigh", int)
	NETVAR(entityQuality, "CBaseAttributableItem", "m_iEntityQuality", int)
	NETVAR(customName, "CBaseAttributableItem", "m_szCustomName", char[32])
	NETVAR(fallbackPaintKit, "CBaseAttributableItem", "m_nFallbackPaintKit", unsigned)
	NETVAR(fallbackSeed, "CBaseAttributableItem", "m_nFallbackSeed", unsigned)
	NETVAR(fallbackWear, "CBaseAttributableItem", "m_flFallbackWear", float)
	NETVAR(fallbackStatTrak, "CBaseAttributableItem", "m_nFallbackStatTrak", unsigned)
	NETVAR(initialized, "CBaseAttributableItem", "m_bInitialized", bool)

	NETVAR(owner, "CBaseViewModel", "m_hOwner", int)
	NETVAR(weapon, "CBaseViewModel", "m_hWeapon", int)

	NETVAR(c4StartedArming, "CC4", "m_bStartedArming", bool)

	NETVAR(c4BlowTime, "CPlantedC4", "m_flC4Blow", float)
	NETVAR(c4TimerLength, "CPlantedC4", "m_flTimerLength", float)
	NETVAR(c4BombSite, "CPlantedC4", "m_nBombSite", int)
	NETVAR(c4Ticking, "CPlantedC4", "m_bBombTicking", bool)
	NETVAR(c4DefuseCountDown, "CPlantedC4", "m_flDefuseCountDown", float)
	NETVAR(c4DefuseLength, "CPlantedC4", "m_flDefuseLength", float)
	NETVAR(c4Defuser, "CPlantedC4", "m_hBombDefuser", int)

	NETVAR(tabletReceptionIsBlocked, "CTablet", "m_bTabletReceptionIsBlocked", bool)

	NETVAR(droneTarget, "CDrone", "m_hMoveToThisEntity", int)

	NETVAR(thrower, "CBaseGrenade", "m_hThrower", int)
	NETVAR(pinPulled, "CBaseCSGrenade", "m_bPinPulled", bool)
	NETVAR(throwTime, "CBaseCSGrenade", "m_fThrowTime", float)

	NETVAR(didSmokeEffect, "CSmokeGrenadeProjectile", "m_bDidSmokeEffect", bool)

	NETVAR(mapHasBombTarget, "CCSGameRulesProxy", "m_bMapHasBombTarget", bool)
	NETVAR(freezePeriod, "CCSGameRulesProxy", "m_bFreezePeriod", bool)

	NETVAR(fireXDelta, "CInferno", "m_fireXDelta", int[100])
	NETVAR(fireYDelta, "CInferno", "m_fireYDelta", int[100])
	NETVAR(fireZDelta, "CInferno", "m_fireZDelta", int[100])
	NETVAR(fireIsBurning, "CInferno", "m_bFireIsBurning", bool[100])
	NETVAR(fireCount, "CInferno", "m_fireCount", int)
};

static_assert(sizeof(Entity) == 1);
