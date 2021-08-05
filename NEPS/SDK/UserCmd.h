#pragma once

#include "Vector.h"
#include "Pad.h"

struct UserCmd
{
	enum
	{
		IN_ATTACK = 1 << 0,
		IN_JUMP = 1 << 1,
		IN_DUCK = 1 << 2,
		IN_FORWARD = 1 << 3,
		IN_BACK = 1 << 4,
		IN_USE = 1 << 5,
		IN_MOVELEFT = 1 << 9,
		IN_MOVERIGHT = 1 << 10,
		IN_ATTACK2 = 1 << 11,
		IN_SCORE = 1 << 16,
		IN_BULLRUSH = 1 << 22
	};
	PAD(4)
	int commandNumber;
	int tickCount;
	Vector viewangles;
	Vector aimdirection;
	float forwardmove;
	float sidemove;
	float upmove;
	int buttons;
	char impulse;
	int weaponSelect;
	int weaponSubtype;
	int randomSeed;
	short mousedx;
	short mousedy;
	bool hasBeenPredicted;
};
