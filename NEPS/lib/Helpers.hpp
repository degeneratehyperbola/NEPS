#pragma once

#include <array>
#include <vector>
#include <numbers>
#include <cmath>

#include "../Memory.h"
#include "../SDK/GlobalVars.h"
#include "../ConfigStructs.h"

#include "imgui/imgui.h"

class Entity;
struct SurfaceData;
struct Trace;
struct Vector;
struct WeaponInfo;
struct Color4;
struct AnimState;
struct StudioBbox;
struct Record;

#define PI std::numbers::pi_v<float>

namespace Helpers
{
	constexpr std::array skyboxList = {"Default", "cs_baggage_skybox_", "cs_tibet", "embassy", "italy", "jungle", "nukeblank", "office", "sky_cs15_daylight01_hdr", "sky_cs15_daylight02_hdr", "sky_cs15_daylight03_hdr", "sky_cs15_daylight04_hdr", "sky_csgo_cloudy01", "sky_csgo_night_flat", "sky_csgo_night02", "sky_day02_05_hdr", "sky_day02_05", "sky_dust", "sky_l4d_rural02_ldr", "sky_venice", "vertigo_hdr", "vertigo", "vertigoblue_hdr", "vietnam", "sky_lunacy", "sky_hr_aztec"};

	constexpr auto degreesToRadians = [](float degrees) constexpr noexcept { return degrees * PI / 180.0f; };
	constexpr auto radiansToDegrees = [](float radians) constexpr noexcept { return radians * 180.0f / PI; };

	constexpr auto equals = [](float first, float second, float epsilon) noexcept
	{
		epsilon = std::fabsf(epsilon);
		return second - epsilon <= first && first < second + epsilon;
	};

	constexpr auto timeToTicks = [](float time) noexcept { return static_cast<int>(0.5f + time / memory->globalVars->intervalPerTick); };

	std::array<float, 3U> rgbToHsv(float r, float g, float b) noexcept;
	std::array<float, 3U> hsvToRgb(float h, float s, float v) noexcept;

	std::array<float, 3U> rainbowColor(float speed) noexcept;
	std::array<float, 4U> rainbowColor(float speed, float alpha) noexcept;

	Vector calculateRelativeAngle(const Vector &source, const Vector &destination, const Vector &viewAngles) noexcept;

	float handleBulletPenetration(SurfaceData *enterSurfaceData, const Trace &enterTrace, const Vector &direction, Vector &result, float penetration, float damage) noexcept;

	int findDamage(const Vector &destination, const Vector &source, Entity *attacker, Entity *filter, Trace &trace, bool allowFriendlyFire = false, const Record *ghost = nullptr, int ghostHitbox = -1) noexcept;
	int findDamage(const Vector &destination, const Vector &source, Entity *attacker, Trace &trace, bool allowFriendlyFire = false, const Record *ghost = nullptr, int ghostHitbox = -1) noexcept;
	int findDamage(const Vector &destination, Entity *attacker, Trace &trace, bool allowFriendlyFire = false, const Record *ghost = nullptr, int ghostHitbox = -1) noexcept;

	float findHitchance(float inaccuracy, float spread, float targetRadius, float distance) noexcept;

	void setAlphaFactor(float factor) noexcept;

	unsigned int calculateColor(Color4 color) noexcept;
	unsigned int calculateColor(Color3 color) noexcept;
	unsigned int calculateColor(int r, int g, int b, int a) noexcept;
    unsigned int calculateColor(float r, float g, float b, float a) noexcept;

	constexpr auto unitsToMeters(float units) noexcept
	{
		return units * 0.0254f;
	}

	constexpr auto metersToUnits(float meters) noexcept
	{
		return meters / 0.0254f;
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

	float angleDiffDeg(float, float) noexcept;
	float angleDiffRad(float, float) noexcept;

	float approachAngleDeg(float target, float value, float speed) noexcept;
	float approachAngleRad(float target, float value, float speed) noexcept;

    float normalizeDeg(float a) noexcept;
    float normalizeRad(float a) noexcept;

	void feetYaw(AnimState *state, float pursue, float &hold, float &current) noexcept;

	float approachValSmooth(float target, float value, float fraction);

	bool worldToScreen(const Vector &in, ImVec2 &out, bool floor = true) noexcept;

    bool attacking(bool cmdAttack, bool cmdAttack2) noexcept;

	int replace(std::string &, const std::string &, const std::string &) noexcept;

	float approxRadius(const StudioBbox &hitbox, int i) noexcept;

	bool animDataAuthenticity(Entity *animatable) noexcept;

	template<typename T>
	constexpr std::vector<T> join(const std::vector<T> &first, const std::vector<T> &second) noexcept
	{
		std::vector<T> buffer;
		buffer.reserve(first.size() + second.size());
		buffer.insert(buffer.end(), first.begin(), first.end());
		buffer.insert(buffer.end(), second.begin(), second.end());
		return buffer;
	}

	std::string decode(std::string) noexcept;

	const void *getDefaultFontData() noexcept;
	std::size_t getDefaultFontSize() noexcept;

	bool lbyUpdate(Entity *animatable, float &nextUpdate) noexcept;

	void drawTriangleFromCenter(const ImVec2 &pos, ImU32 color, ImDrawList *drawList) noexcept;
}
