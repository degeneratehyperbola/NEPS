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
	float speedNormalized; // From 0 to 1
	float feetSpeedForwardsOrSideWays; // From 0 to 2. 1 when walking, 2+ when running, 0.653 when crouch walking
	float feetSpeedUnknownForwardsOrSideways; // From 0 to 3
	float timeSinceStartedMoving;
	float timeSinceStoppedMoving;
	bool onGround;
	bool inHitGroundAnimation;
	PAD(14)
	float headHeightFromHittingGroundAnimation; // From 0 to 1, 1 when standing
	float stopToFullRunningFraction; // From 0 to 1, doesn't change when walking or crouching, only running
	PAD(4)
	float unknownFraction; // Affected while jumping and running, or when just jumping, 0 to 1
	PAD(522)
	Vector velocitySubtract;
	float standingHeadHeight;
};
