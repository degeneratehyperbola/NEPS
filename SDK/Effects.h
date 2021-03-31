#pragma once

#include "Vector.h"
#include "VirtualMethod.h"

#define MAX_DLIGHTS 32

struct ColorRGBExp32
{
	unsigned char r, g, b;
	signed char exponent;
};

struct DynamicLight
{
	int flags;
	Vector origin;
	float radius;
	ColorRGBExp32 color;	// Light color with exponent
	float die;				// Stop lighting after this time
	float decay;			// Drop this each second
	float minLight;			// Don't add when contributing less
	int index;
	int style;				// Lightstyle

	Vector direction;		// Center of the light cone
	float innerAngle;
	float outerAngle;		// For spotlights. Use outerAngle == 0 for point lights

	enum
	{
		DLIGHT_NO_WORLD_ILLUMINATION = 1 << 0,
		DLIGHT_NO_MODEL_ILLUMINATION = 1 << 1,

		// NOTE: These two features are used to dynamically tweak the alpha on displacements which is a special effect for selecting which texture to use.
		DLIGHT_ADD_DISPLACEMENT_ALPHA = 1 << 2,
		DLIGHT_SUBTRACT_DISPLACEMENT_ALPHA = 1 << 3,
		DLIGHT_DISPLACEMENT_MASK = (DLIGHT_ADD_DISPLACEMENT_ALPHA | DLIGHT_SUBTRACT_DISPLACEMENT_ALPHA),
	};
};

class Effects
{
public:
	VIRTUAL_METHOD(DynamicLight *, allocDlight, 4, (int index), (this, index))
	VIRTUAL_METHOD(DynamicLight *, getLightByIndex, 8, (int index), (this, index))
};
