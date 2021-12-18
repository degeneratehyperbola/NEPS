#pragma once

#include <cstddef>
#include "Pad.h"

struct UserCmd;

struct GlobalVars
{
	const float realTime;
	const int frameCount;
	const float absoluteFrameTime;
	PAD(4)
	float currentTime;
	float frameTime;
	const int maxClients;
	const int tickCount;
	const float intervalPerTick;

	float serverTime(UserCmd * = nullptr) const noexcept;
};
