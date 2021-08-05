#pragma once

#include "AnimState.h"
#include "Engine.h"
#include "EntityList.h"
#include "GlobalVars.h"
#include "LocalPlayer.h"
#include "Matrix3x4.h"
#include "ModelInfo.h"
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
#define PLAYER_DUCK_EYE_HEIGHT 46.076218f
#define PLAYER_HEIGHT 72.0f
#define PLAYER_DUCK_HEIGHT 54.0f

#define MAX_ANIM_LAYERS 15

enum class MoveType
{
    NOCLIP = 8,
    LADDER = 9
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
    VIRTUAL_METHOD(const Vector&, obbMins, 1, (), (this))
    VIRTUAL_METHOD(const Vector&, obbMaxs, 2, (), (this))
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
	VIRTUAL_METHOD(void, release, 1, (), (this + 8))
	VIRTUAL_METHOD(ClientClass *, getClientClass, 2, (), (this + 8))
	VIRTUAL_METHOD(void, preDataUpdate, 6, (int updateType), (this + 8, updateType))
	VIRTUAL_METHOD(void, postDataUpdate, 7, (int updateType), (this + 8, updateType))
	VIRTUAL_METHOD(bool, isDormant, 9, (), (this + 8))
	VIRTUAL_METHOD(int, index, 10, (), (this + 8))
	VIRTUAL_METHOD(void, setDestroyedOnRecreateEntities, 13, (), (this + 8))

	VIRTUAL_METHOD(Vector &, getRenderOrigin, 1, (), (this + 4))
	VIRTUAL_METHOD(const Model *, getModel, 8, (), (this + 4))
	VIRTUAL_METHOD(const Matrix3x4 &, toWorldTransform, 32, (), (this + 4))

	VIRTUAL_METHOD(int&, handle, 2, (), (this))
	VIRTUAL_METHOD(Collideable *, getCollideable, 3, (), (this))
	VIRTUAL_METHOD(const Vector &, getAbsOrigin, 10, (), (this))
	VIRTUAL_METHOD(const Vector &, getAbsAngle, 11, (), (this))
	VIRTUAL_METHOD(void, setModelIndex, 75, (int index), (this, index))
	VIRTUAL_METHOD(int, health, 121, (), (this))
	VIRTUAL_METHOD(bool, isAlive, 155, (), (this))
	VIRTUAL_METHOD(bool, isPlayer, 157, (), (this))
	VIRTUAL_METHOD(bool, isWeapon, 165, (), (this))
	VIRTUAL_METHOD(int, getWeaponSubType, 281, (), (this))
	VIRTUAL_METHOD(ObsMode, getObserverMode, 293, (), (this))
	VIRTUAL_METHOD(Entity *, getObserverTarget, 294, (), (this))
	VIRTUAL_METHOD(Vector, getAimPunch, 345, (), (this))
	VIRTUAL_METHOD(float, getSpread, 452, (), (this))
	VIRTUAL_METHOD(WeaponType, getWeaponType, 454, (), (this))
	VIRTUAL_METHOD(WeaponInfo *, getWeaponData, 460, (), (this))
	VIRTUAL_METHOD(float, getInaccuracy, 482, (), (this))
	VIRTUAL_METHOD(void, updateClientSideAnimation, 223, (), (this))

	Entity *getActiveWeapon() noexcept
	{
		return interfaces->entityList->getEntityFromHandle(activeWeapon());
	}

	Vector getEyePosition() noexcept
	{
		return getAbsOrigin() + viewOffset();
	}

	
	enum PlayerFlags
	{
		FL_ONGROUND = 1 << 0, // At rest / on the ground
		FL_DUCKING = 1 << 1, // Player flag - player is fully crouched
		FL_WATERJUMP = 1 << 2, // Player jumping out of water
		FL_ONTRAIN = 1 << 3, // Player is controlling a train, so movement commands should be ignored on client during prediction
		FL_INRAIN = 1 << 4, // Indicates the entity is standing in rain
		FL_FROZEN = 1 << 5, // Player is frozen for 3rd person camera
		FL_ATCONTROLS = 1 << 6, // Player can't move, but keeps key inputs for controlling another entity
		FL_CLIENT = 1 << 7, // Is a player
		FL_FAKECLIENT = 1 << 8, // Fake client, simulated server side; don't send network messages to them. NON-PLAYER SPECIFIC (i.e. not used by GameMovement or the client.dll) - can still be applied to players, though
		FL_INWATER = 1 << 9, // In water
		FL_FLY = 1 << 10, // Changes the SV_Movestep() behavior to not need to be on ground
		FL_SWIM = 1 << 11, // Changes the SV_Movestep() behavior to not need to be on ground (but stay in water)
		FL_CONVEYOR = 1 << 12,
		FL_NPC = 1 << 13,
		FL_GODMODE = 1 << 14,
		FL_NOTARGET = 1 << 15,
		FL_AIMTARGET = 1 << 16, // Set if the crosshair needs to aim onto the entity
		FL_PARTIALGROUND = 1 << 17, // Not all corners are valid
		FL_STATICPROP = 1 << 18, // Eetsa static prop!
		FL_GRAPHED = 1 << 19, // Worldgraph has this ent listed as something that blocks a connection
		FL_GRENADE = 1 << 20,
		FL_STEPMOVEMENT = 1 << 21, // Changes the SV_Movestep() behavior to not do any processing
		FL_DONTTOUCH = 1 << 22, // Doesn't generate touch functions, generates Untouch() for anything it was touching when this flag was set
		FL_BASEVELOCITY = 1 << 23, // Base velocity has been applied this frame (used to convert base velocity into momentum)
		FL_WORLDBRUSH = 1 << 24, // Not moveable/removeable brush entity (really part of the world, but represented as an entity for transparency or something)
		FL_OBJECT = 1 << 25, // Terrible name. This is an object that NPCs should see. Missiles, for example
		FL_KILLME = 1 << 26, // This entity is marked for death -- will be freed by game DLL
		FL_ONFIRE = 1 << 27, // You know...
		FL_DISSOLVING = 1 << 28, // We're dissolving!
		FL_TRANSRAGDOLL = 1 << 29, // In the process of turning into a client side ragdoll
		FL_UNBLOCKABLE_BY_PLAYER = 1 << 30 // Pusher that can't be blocked by the player
	};

	enum AnimLayerIndices
	{
		ANIMATION_LAYER_AIMMATRIX,
		ANIMATION_LAYER_WEAPON_ACTION,
		ANIMATION_LAYER_WEAPON_ACTION_RECROUCH,
		ANIMATION_LAYER_ADJUST,
		ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL,
		ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB,
		ANIMATION_LAYER_MOVEMENT_MOVE,
		ANIMATION_LAYER_MOVEMENT_STRAFECHANGE,
		ANIMATION_LAYER_WHOLE_BODY,
		ANIMATION_LAYER_FLASHED,
		ANIMATION_LAYER_FLINCH,
		ANIMATION_LAYER_ALIVELOOP,
		ANIMATION_LAYER_LEAN,
		ANIMATION_LAYER_COUNT
	};

	int getAnimationLayerCount() noexcept
	{
		return *reinterpret_cast<int *>((uintptr_t)this + 0x298C);
	}
	AnimLayer *animationLayers() noexcept
	{
		return *reinterpret_cast<AnimLayer **>((uintptr_t)this + 0x2980);
	}

	AnimState *getAnimState() noexcept
	{
		return *reinterpret_cast<AnimState **>((uintptr_t)this + 0x3914);
	}

	enum PoseParameterIndeces
	{
		POSE_PARAM_STRAFE_YAW,
		POSE_PARAM_STAND,
		POSE_PARAM_LEAN_YAW,
		POSE_PARAM_SPEED,
		POSE_PARAM_LADDER_YAW,
		POSE_PARAM_LADDER_SPEED,
		POSE_PARAM_JUMP_FALL,
		POSE_PARAM_MOVE_YAW,
		POSE_PARAM_MOVE_BLEND_CROUCH,
		POSE_PARAM_MOVE_BLEND_WALK,
		POSE_PARAM_MOVE_BLEND_RUN,
		POSE_PARAM_BODY_YAW,
		POSE_PARAM_BODY_PITCH,
		POSE_PARAM_AIM_BLEND_STAND_IDLE,
		POSE_PARAM_AIM_BLEND_STAND_WALK,
		POSE_PARAM_AIM_BLEND_STAND_RUN,
		POSE_PARAM_AIM_BLEND_COURCH_IDLE,
		POSE_PARAM_AIM_BLEND_CROUCH_WALK,
		POSE_PARAM_DEATH_YAW
	};

	std::array<float, 24> &poseParam() noexcept
	{
		return *reinterpret_cast<std::array<float, 24> *>((uintptr_t)this + netvars->operator[](fnv::hash("CBaseAnimating->m_flPoseParameter")));
	}
	float &poseParam(int index) noexcept
	{
		return reinterpret_cast<std::array<float, 24> *>((uintptr_t)this + netvars->operator[](fnv::hash("CBaseAnimating->m_flPoseParameter")))->operator[](index);
	}

	float spawnTime() noexcept
	{
		return *(float *)((uintptr_t)this + 0xA370);
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
		if (config->misc.fixBoneMatrix && this == localPlayer.get())
		{
			int *render = reinterpret_cast<int *>(this + 0x274);
			int *shouldSkipFrame = reinterpret_cast<int *>(this + 0xA68);
			int backupRender = *render;
			int backupEffects = effectFlags();
			int backupShouldSkipFrame = *shouldSkipFrame;
			Vector absOrigin = getAbsOrigin();
			*render = 0;
			*shouldSkipFrame = 0;
			effectFlags() |= 8;
			memory->setAbsOrigin(this, origin());
			auto result = VirtualMethod::call<bool, 13>(this + 4, out, maxBones, boneMask, currentTime);
			memory->setAbsOrigin(this, absOrigin);
			*render = backupRender;
			*shouldSkipFrame = backupShouldSkipFrame;
			effectFlags() = backupEffects;
			return result;
		}
		if (config->misc.fixBoneMatrix)
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
	Vector getBonePosition(int bone) noexcept
	{
		if (Matrix3x4 boneMatrices[MAX_STUDIO_BONES]; setupBones(boneMatrices, MAX_STUDIO_BONES, BONE_USED_BY_ANYTHING, memory->globalVars->currenttime))
			return boneMatrices[bone].origin();
		else
			return Vector{};
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
    
	bool isOtherEnemy(Entity* other) noexcept;

	VarMap* getVarMap() noexcept
	{
		return reinterpret_cast<VarMap*>(this + 0x24);
	}

	float getMaxDesyncAngle() noexcept
	{
		const auto animState = getAnimState();

		if (!animState)
			return 0.0f;

		float yawModifier = (animState->stopToFullRunningFraction * -0.3f - 0.2f) * std::clamp(animState->feetSpeedForwardsOrSideWays, 0.0f, 1.0f) + 1.0f;

		if (animState->duckAmount > 0.0f)
			yawModifier += (animState->duckAmount * std::clamp(animState->feetSpeedUnknownForwardsOrSideways, 0.0f, 1.0f) * (0.5f - yawModifier));

		return animState->velocitySubtract.y * yawModifier;
	}

	bool isInReload() noexcept
	{
		return *reinterpret_cast<bool*>(uintptr_t(&clip()) + 0x41);
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

	bool canSee(Entity* other, const Vector& pos) noexcept;
	bool visibleTo(Entity* other) noexcept;

	bool grenadeExploded() noexcept
	{
		return *reinterpret_cast<bool *>(this + 0x29E8);
	}

	bool isFlashed() noexcept
	{
		return flashDuration() > 75.0f;
	}

	int &effectFlags() noexcept
	{
		return *reinterpret_cast<int *>(this + 0xF0);
	}

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
	NETVAR(weapons, "CBaseCombatCharacter", "m_hMyWeapons", int [64])
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
	NETVAR(lastPlaceName, "CBasePlayer", "m_szLastPlaceName", char [18])
	NETVAR_OFFSET(thirdPersonAngles, "CBasePlayer", "deadflag", 4, Vector)

	NETVAR(armor, "CCSPlayer", "m_ArmorValue", int)
	NETVAR(eyeAngles, "CCSPlayer", "m_angEyeAngles", Vector)
	NETVAR(isScoped, "CCSPlayer", "m_bIsScoped", bool)
	NETVAR(isDefusing, "CCSPlayer", "m_bIsDefusing", bool)
	NETVAR_OFFSET(flashDuration, "CCSPlayer", "m_flFlashMaxAlpha", -8, float)
	NETVAR(flashMaxAlpha, "CCSPlayer", "m_flFlashMaxAlpha", float)
	NETVAR(gunGameImmunity, "CCSPlayer", "m_bGunGameImmunity", bool)
	NETVAR(account, "CCSPlayer", "m_iAccount", int)
	NETVAR(inBombZone, "CCSPlayer", "m_bInBombZone", bool)
	NETVAR(hasDefuser, "CCSPlayer", "m_bHasDefuser", bool)
	NETVAR(hasHelmet, "CCSPlayer", "m_bHasHelmet", bool)
	NETVAR(lby, "CCSPlayer", "m_flLowerBodyYawTarget", float)
	NETVAR(ragdoll, "CCSPlayer", "m_hRagdoll", int)
	NETVAR(shotsFired, "CCSPlayer", "m_iShotsFired", int)
	NETVAR(waitForNoAttack, "CCSPlayer", "m_bWaitForNoAttack", bool)

	NETVAR(viewModelIndex, "CBaseCombatWeapon", "m_iViewModelIndex", int)
	NETVAR(worldModelIndex, "CBaseCombatWeapon", "m_iWorldModelIndex", int)
	NETVAR(worldDroppedModelIndex, "CBaseCombatWeapon", "m_iWorldDroppedModelIndex", int)
	NETVAR(weaponWorldModel, "CBaseCombatWeapon", "m_hWeaponWorldModel", int)
	NETVAR(clip, "CBaseCombatWeapon", "m_iClip1", int)
	NETVAR(reserveAmmoCount, "CBaseCombatWeapon", "m_iPrimaryReserveAmmoCount", int)
	NETVAR(nextPrimaryAttack, "CBaseCombatWeapon", "m_flNextPrimaryAttack", float)

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
	NETVAR(customName, "CBaseAttributableItem", "m_szCustomName", char [32])
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

	NETVAR(fireXDelta, "CInferno", "m_fireXDelta", int [100])
	NETVAR(fireYDelta, "CInferno", "m_fireYDelta", int [100])
	NETVAR(fireZDelta, "CInferno", "m_fireZDelta", int [100])
	NETVAR(fireIsBurning, "CInferno", "m_bFireIsBurning", bool [100])
	NETVAR(fireCount, "CInferno", "m_fireCount", int)
};

static_assert(sizeof(Entity) == 1);
