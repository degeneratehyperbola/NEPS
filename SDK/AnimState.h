#pragma once

#include <cstddef>
#include "Vector.h"

struct AnimLayer
{
public:
	bool clientblend;
	float blendin;
	void *studioHdr;
	int dispatchedsrc;
	int dispatcheddst;
	unsigned int order;
	unsigned int sequence;
	float prevcycle;
	float weight;
	float weightdeltarate;
	float playbackRate;
	float cycle;
	void *owner;
	int invalidatephysicsbits;
};

struct AnimState
{
	std::byte pad2[95];
	void *entity; //0x60
	void *activeWeapon; //0x64
	void *lastActiveWeapon; //0x68
	float lastClientSideAnimationUpdateTime; //0x6C
	int lastClientSideAnimationUpdateFramecount; //0x70
	float eyePitch; //0x74
	float eyeYaw; //0x78
	float pitch; //0x7C
	float feetYaw; //0x80
	float feetYawLast; //0x84
	float currentTorsoYaw; //0x88
	float unknownVelocityLean; //0x8C //changes when moving/jumping/hitting ground
	float leanAmount; //0x90
	std::byte pad4[4]; //NaN
	float feetCycle; //0x98 0 to 1
	float feetYawRate; //0x9C 0 to 1
	std::byte pad5[4];
	float duckAmount; //0xA4
	float landingDuckAdditiveSomething; //0xA8
	std::byte pad6[4];
	Vector origin; //0xB0, 0xB4, 0xB8
	Vector lastOrigin; //0xBC, 0xC0, 0xC4
	float velocityX; //0xC8
	float velocityY; //0xCC
	std::byte pad7[28];
	float horizontalSpeed; //0xEC
	float verticalSpeed; //0xF0
	float speedNormalized; //0xF4 //from 0 to 1
	float feetSpeedForwardsOrSideWays; //0xF8 //from 0 to 2. something  is 1 when walking, 2.something when running, 0.653 when crouch walking
	float feetSpeedUnknownForwardsOrSideways; //0xFC //from 0 to 3. something
	float timeSinceStartedMoving; //0x100
	float timeSinceStoppedMoving; //0x104
	bool onGround; //0x108
	bool inHitGroundAnimation; //0x109
	std::byte pad8[14]; //NaN
	float headHeightFromHittingGroundAnimation; //0x118 from 0 to 1, is 1 when standing
	float stopToFullRunningFraction; //0x11C from 0 to 1, doesnt change when walking or crouching, only running
	std::byte pad9[4];
	float unknownFraction; //0x124 affected while jumping and running, or when just jumping, 0 to 1
	std::byte pad10[522];
	Vector velocitySubtract;
	float standingHeadHeight;
};
