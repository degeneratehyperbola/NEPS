#pragma once

#include "Pad.h"
#include <cstddef>
#include "Vector.h"

class Entity;

enum PoseParamIndeces
{
	PoseParam_StrafeYaw = 0,
	PoseParam_Stand,
	PoseParam_LeanYaw,
	PoseParam_Speed,
	PoseParam_LadderYaw,
	PoseParam_LadderSpeed,
	PoseParam_JumpFall,
	PoseParam_MoveYaw,
	PoseParam_MoveBlendCrouch,
	PoseParam_MoveBlendWalk,
	PoseParam_MoveBlendRun,
	PoseParam_BodyYaw,
	PoseParam_BodyPitch,
	PoseParam_AimBlendStandIdle,
	PoseParam_AimBlendStandWalk,
	PoseParam_AimBlendStandRun,
	PoseParam_AimBlendCrouchIdle,
	PoseParam_AimBlendCrouchWalk,
	PoseParam_DeathYaw,
	PoseParam_Count = 24
};

enum AnimLayerIndices
{
	AnimLayer_AimMatrix = 0,
	AnimLayer_WeaponAction,
	AnimLayer_WeaponActionReCrouch,
	AnimLayer_Adjust,
	AnimLayer_MovementJumpOrFall,
	AnimLayer_MovementLandOrClimb,
	AnimLayer_MovementMove,
	AnimLayer_MovementStrafeChange,
	AnimLayer_WholeBody,
	AnimLayer_Flashed,
	AnimLayer_Flinch,
	AnimLayer_AliveLoop,
	AnimLayer_Lean,
	AnimLayer_Count
};

struct AnimLayer
{
	bool clientBlend;
	float blendIn;
	void *studioHdr;
	int dispatchedSrc;
	int dispatchedDst;
	unsigned int order;
	unsigned int sequence;
	float prevCycle;
	float weight;
	float weightdeltarate;
	float playbackRate;
	float cycle;
	void *owner;
	int invalidatePhysicsBits;
};

struct AnimState
{
	void update(Vector viewangles) noexcept { memory->updateState(this, nullptr, viewangles.z, viewangles.y, viewangles.x, nullptr); }

	PAD(95)
	Entity *entity;
	Entity *activeWeapon;
	Entity *lastActiveWeapon;
	float previousUpdateTime;
	int previousUpdateFramecount;
	float previousUpdateIncrement;
	float eyeYaw;
	float eyePitch;
	float goalFeetYaw;
	float currentFeetYaw;
	float currentTorsoYaw;
	float velocityLean;
	float leanAmount;
	PAD(4)
	float feetCycle;
	float moveWeight;
	PAD(4)
	float duckAmount;
	float landingDuckAdditiveAmount;
	PAD(4)
	Vector origin;
	Vector previousOrigin;
	Vector velocity;
	PAD(24)
	float horizontalSpeed;
	float verticalSpeed;
	float speedAsPortionOfRunSpeed;
	float speedAsPortionOfWalkSpeed;
	float speedAsPortionOfCrouchWalkSpeed;
	float timeSinceStartedMoving;
	float timeSinceStoppedMoving;
	bool onGround;
	bool landing;
	PAD(14)
	float landAnimationMultiplier;
	float stopToFullRunningFraction;
	PAD(4)
	float inAirSmoothValue;
	bool onLadder;
	PAD(521)
	Vector velocitySubtract;
	float standingHeadHeight;
};
