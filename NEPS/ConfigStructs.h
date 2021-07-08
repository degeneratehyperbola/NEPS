#pragma once

#include <array>
#include <string>

#pragma pack(push, 1)
struct Color3
{
	std::array<float, 3> color = {1.0f, 1.0f, 1.0f};
	bool rainbow = false;
	float rainbowSpeed = 0.6f;
};
#pragma pack(pop)

struct Color3Toggle : public Color3
{
	bool enabled = false;
};

#pragma pack(push, 1)
struct Color4
{
	std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f};
	float rainbowSpeed = 0.6f;
	bool rainbow = false;
};
#pragma pack(pop)

struct Color4Toggle : Color4
{
	bool enabled = false;
};

struct Color4Border : Color4
{
	bool border = true;
};

struct Color4BorderToggle : Color4Border
{
	bool enabled = false;
};

struct Color4BorderToggleThickness : Color4BorderToggle
{
	float thickness = 1.0f;
};

struct Color4ToggleThickness : Color4Toggle
{
	float thickness = 1.0f;
};

struct Color4ToggleRounding : Color4Toggle
{
	float rounding = 0.0f;
};

struct Color4ToggleThicknessRounding : Color4ToggleRounding
{
	float thickness = 1.0f;
};

struct Font
{
	int index = 0; // Do not save
	std::string name;
};

struct Snapline : Color4ToggleThickness
{
	enum
	{
		Bottom,
		Top,
		Crosshair
	};

	int type = Bottom;
};

struct Box : Color4ToggleRounding
{
	enum Type
	{
		_2d,
		_2dCorners,
		_3d,
		_3dCorners
	};

	enum SecondaryType
	{
		None,
		Outline,
		Fill
	};

	int type = _2d;
	std::array<float, 3> scale = {0.25f, 0.25f, 0.25f};
	int secondary = Outline;
	Color4 secondaryColor = {1.0f, 0.0f, 0.0f, 1.0f};
};

struct Shared
{
	bool enabled = false;
	Font font;
	Snapline snapline;
	Box box;
	Color4BorderToggle name;
	float textCullDistance = 0.0f;
};

struct Bar : Color4BorderToggle
{
	// What
};

struct Player : Shared
{
	Color4BorderToggle weapon;
	Color4BorderToggle flashDuration;
	bool audibleOnly = false;
	bool spottedOnly = false;
	Color4BorderToggle healthBar;
	Color4BorderToggle health;
	Color4BorderToggleThickness skeleton;
	Box headBox;
	Color4BorderToggle flags;
	Color4Toggle offscreen;
	Color4ToggleThickness lineOfSight;

	using Shared::operator=;
};

struct Weapon : Shared
{
	Color4BorderToggle ammo;

	using Shared::operator=;
};

struct Trail : Color4BorderToggleThickness
{
	enum Type
	{
		Line = 0,
		Circles,
		FilledCircles
	};

	int type = Line;
	float time = 2.0f;
};

struct Trails
{
	bool enabled = false;

	Trail localPlayer;
	Trail allies;
	Trail enemies;
};

struct Projectile : Shared
{
	Trails trails;

	using Shared::operator=;
};

struct KeyBind
{
	int key = 0;
	int keyMode = 0;
};