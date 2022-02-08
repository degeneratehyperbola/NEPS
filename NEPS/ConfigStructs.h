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
	operator Color3()
	{
		Color3 newColor;
		newColor.color[0] = color[0];
		newColor.color[1] = color[1];
		newColor.color[2] = color[2];
		newColor.rainbowSpeed = rainbowSpeed;
		newColor.rainbow = rainbow;
		return newColor;
	}

	std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f};
	float rainbowSpeed = 0.6f;
	bool rainbow = false;
};
#pragma pack(pop)

struct Color4Toggle : Color4
{
	bool enabled = false;
};

struct Color4Outline : Color4
{
	bool outline = true;
};

struct Color4OutlineToggle : Color4Outline
{
	bool enabled = false;
};

struct Color4OutlineToggleThickness : Color4OutlineToggle
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
	Color4OutlineToggle name;
	float textCullDistance = 0.0f;
};

struct Bar : Color4OutlineToggle
{
	// What
};

struct Player : Shared
{
	Color4OutlineToggle weapon;
	Color4OutlineToggle flashDuration;
	bool audibleOnly = false;
	bool spottedOnly = false;
	Color4OutlineToggle healthBar;
	Color4OutlineToggle health;
	Color4OutlineToggleThickness skeleton;
	Box headBox;
	Color4OutlineToggle flags;
	Color4OutlineToggle offscreen = {1.0f, 1.0f, 1.0f, 0.5f};
	Color4ToggleThickness lineOfSight;

	using Shared::operator=;
};

struct Weapon : Shared
{
	Color4OutlineToggle ammo;

	using Shared::operator=;
};

struct Trail : Color4OutlineToggleThickness
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