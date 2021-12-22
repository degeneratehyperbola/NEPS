#pragma once

#include "Pad.h"
#include "Vector.h"

struct RadarPlayer
{
public:
	Vector origin;
	Vector angle;
	Vector spottedAngle;
	PAD(0x10)
	float spottedTime;
	float spottedFraction;
	float time;
	PAD(4)
	int playerIndex;
	int entityIndex;
	PAD(4)
	int health;
	char name[32];
	PAD(0x75)
	bool spotted;
	PAD(0x8A)
};

struct HudRadar
{
	PAD(0x14C)
	RadarPlayer radarInfo[65];
};
