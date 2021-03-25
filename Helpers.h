#pragma once

#include <array>
#include <numbers>

#include "Config.h"
#include "Memory.h"
#include "SDK/GlobalVars.h"

class Entity;
struct SurfaceData;
struct Trace;
struct Vector;
struct ImVec2;
struct WeaponInfo;
struct Color4;
struct AnimState;
struct StudioBbox;

#define PI std::numbers::pi_v<float>
const static inline float PI2 = 2.0f * PI;
const static inline float RAD90 = 0.5f * PI;
const static inline float RAD45 = 0.25f * PI;

namespace Helpers
{
	constexpr inline std::array skyboxList = {"Default", "cs_baggage_skybox_", "cs_tibet", "embassy", "italy", "jungle", "nukeblank", "office", "sky_cs15_daylight01_hdr", "sky_cs15_daylight02_hdr", "sky_cs15_daylight03_hdr", "sky_cs15_daylight04_hdr", "sky_csgo_cloudy01", "sky_csgo_night_flat", "sky_csgo_night02", "sky_day02_05_hdr", "sky_day02_05", "sky_dust", "sky_l4d_rural02_ldr", "sky_venice", "vertigo_hdr", "vertigo", "vertigoblue_hdr", "vietnam", "sky_lunacy", "sky_hr_aztec"};

	constexpr auto degreesToRadians = [](float degrees) constexpr noexcept { return degrees * PI / 180.0f; };
	constexpr auto radiansToDegrees = [](float radians) constexpr noexcept { return radians * 180.0f / PI; };

	constexpr auto timeToTicks = [](float time) noexcept { return static_cast<int>(0.5f + time / memory->globalVars->intervalPerTick); };

	std::tuple<float, float, float> rainbowColor(float speed) noexcept;

	Vector calculateRelativeAngle(const Vector &source, const Vector &destination, const Vector &viewAngles) noexcept;

	float handleBulletPenetration(SurfaceData *enterSurfaceData, const Trace &enterTrace, const Vector &direction, Vector &result, float penetration, float damage) noexcept;

	#define AUTOWALL_CALC_DEPTH 4
	#define AUTOWALL_MIN_PENETRATION 0.1f
	int findDamage(const Vector &destination, const WeaponInfo *weaponData, Trace &trace, bool allowFriendlyFire = false, int hitgroupFlags = 1 << 7, bool *goesThroughWall = nullptr) noexcept;
    bool canHit(const Vector &destination, Trace &trace, bool allowFriendlyFire, bool *goesThroughWall) noexcept;

	float findHitchance(float inaccuracy, float spread, float targetRadius, float distance) noexcept;

	unsigned int calculateColor(Color4 color) noexcept;
	unsigned int calculateColor(Color3 color) noexcept;
	unsigned int calculateColor(int r, int g, int b, int a) noexcept;
    unsigned int calculateColor(float r, float g, float b, float a) noexcept;

	constexpr auto units2meters(float units) noexcept
	{
		return units * 0.0254f;
	}

	ImWchar *getFontGlyphRanges() noexcept;

	constexpr int utf8SeqLen(char firstByte) noexcept
	{
		return (firstByte & 0x80) == 0x00 ? 1 :
			(firstByte & 0xE0) == 0xC0 ? 2 :
			(firstByte & 0xF0) == 0xE0 ? 3 :
			(firstByte & 0xF8) == 0xF0 ? 4 :
			-1;
	}

	constexpr auto utf8Substr(char *start, char *end, int n) noexcept
	{
		while (start < end && --n)
			start += utf8SeqLen(*start);
		return start;
	}

	std::wstring toWideString(const std::string &str) noexcept;
	std::wstring toUpper(std::wstring str) noexcept;

	struct KeyBindState
	{
	private:
		bool active = true, lastState = false;
	public:
		bool operator [](const KeyBind &bind)
		{
			if (bind.keyMode == 0)
				return false;
			else if (bind.keyMode == 1)
				return true;

			if (bind.key)
			{
				if (bind.keyMode == 2)
				{
					this->active = GetAsyncKeyState(bind.key);
				} else if (bind.keyMode == 3)
				{
					if (!this->lastState && GetAsyncKeyState(bind.key))
						this->active = !this->active;
				}

				this->lastState = GetAsyncKeyState(bind.key);
			}

			return this->active;
		}
	};

	float angleDiffDeg(float a1, float a2) noexcept;
	float angleDiffRad(float a1, float a2) noexcept;

	float approachAngleDeg(float target, float value, float speed) noexcept;
	float approachAngleRad(float target, float value, float speed) noexcept;

	void feetYaw(AnimState *state, float pursue, float &hold, float &current) noexcept;

	float approachValSmooth(float target, float value, float fraction);

	bool worldToScreen(const Vector &in, ImVec2 &out, bool floor = true) noexcept;

	// Oh my god this is so mind blowing,
	// if the next weapon attack == time
	// it would return false, but aimbot
	// does fire while it equals,
	// nibba,
	// the shit here is, in fact, tested
	// but epsilon of a floating point
	// number is just insane for a
	// genuine difference O_O
	//
	// Future me here, NO this is actually
	// wrong and aimbot was right!
	//
	// Future me again, nigga, please
	//
	// For local player
    bool attacking(bool cmdAttack, bool cmdAttack2) noexcept;

	bool replace(std::string &, const std::string &, const std::string &) noexcept;

	float approxRadius(const StudioBbox &hitbox, int i) noexcept;
}
