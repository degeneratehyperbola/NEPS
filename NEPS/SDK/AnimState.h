#pragma once

#include "Pad.h"
#include <cstddef>
#include "Vector.h"

class Entity;

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
	PAD(95)
	Entity *entity;
	void *activeWeapon;
	void *lastActiveWeapon;
	float lastClientSideAnimationUpdateTime;
	int lastClientSideAnimationUpdateFramecount;
	float eyePitch;
	float eyeYaw;
	float pitch;
	float feetYaw; // Lower body yaw or LBY
	float lastFeetYaw;
	float currentTorsoYaw;
	float unknownVelocityLean; // Changes when moving/jumping/hitting ground
	float leanAmount;
	PAD(4)
	float feetCycle; // From 0 to 1
	float feetYawRate; // From 0 to 1
	PAD(4)
	float duckAmount;
	float landingDuckAdditiveAmount;
	PAD(4)
	Vector origin;
	Vector lastOrigin;
	float velocityX;
	float velocityY;
	PAD(28)
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
	float stopToFullRunningFraction; // From 0 to 1, doesn't change when walking or crouching, only running
	PAD(8)
	bool onLadder;
	PAD(521)
	Vector velocitySubtract;
	float standingHeadHeight;
};
