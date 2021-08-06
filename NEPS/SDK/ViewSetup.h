#pragma once

#include "Pad.h"

struct ViewSetup
{
	PAD(172)
	void *csmVolumeCuller;
	float fov;
	float fovViewmodel;
	Vector origin;
	Vector angles;
	float nearZ;
	float farZ;
};