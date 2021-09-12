#pragma once

#include <array>
#include <algorithm>
#include <cmath>
#include <cwctype>
#include <numbers>
#include <vector>

#include "../Memory.h"
#include "../SDK/GlobalVars.h"
#include "../ConfigStructs.h"

#include <shared_lib/imgui/imgui.h>

class Entity;
struct AnimState;
struct Color4;
struct Record;
struct SurfaceData;
struct StudioBbox;
struct Trace;
struct TraceFilter;
struct Vector;
struct WeaponInfo;

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

	constexpr auto normalizeDeg = [](float a) noexcept { return std::isfinite(a) ? std::remainder(a, 360.0f) : 0.0f; };
	constexpr auto normalizeRad = [](float a) noexcept { return std::isfinite(a) ? std::remainder(a, PI * 2) : 0.0f; };

	std::array<float, 3U> rgbToHsv(float r, float g, float b) noexcept;
	std::array<float, 3U> hsvToRgb(float h, float s, float v) noexcept;

	std::array<float, 3U> rainbowColor(float speed) noexcept;
	std::array<float, 4U> rainbowColor(float speed, float alpha) noexcept;

	Vector calculateRelativeAngle(const Vector &source, const Vector &destination, const Vector &viewAngles) noexcept;

	float handleBulletPenetration(SurfaceData *enterSurfaceData, const Trace &enterTrace, const Vector &direction, Vector &result, float penetration, float damage) noexcept;

	int findDamage(const Vector &destination, const Vector &source, Entity *attacker, TraceFilter filter, Trace &trace, bool allowFriendlyFire = false, const Record *ghost = nullptr, int ghostHitbox = -1) noexcept;
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
	constexpr std::wstring toUpper(std::wstring str) noexcept
	{
		std::transform(str.begin(), str.end(), str.begin(), [](wchar_t w) { return std::towupper(w); });
		return str;
	}

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

	constexpr float angleDiffDeg(float a1, float a2) noexcept
	{
		float delta;

		delta = normalizeDeg(a1 - a2);
		if (a1 > a2)
		{
			if (delta >= 180)
				delta -= 360;
		} else
		{
			if (delta <= -180)
				delta += 360;
		}
		return delta;
	}

	constexpr float angleDiffRad(float a1, float a2) noexcept
	{
		float delta;

		delta = normalizeRad(a1 - a2);
		if (a1 > a2)
		{
			if (delta >= PI)
				delta -= PI * 2;
		} else
		{
			if (delta <= -PI)
				delta += PI * 2;
		}
		return delta;
	}

	float approachAngleDeg(float target, float value, float speed) noexcept;
	float approachAngleRad(float target, float value, float speed) noexcept;

	float approachValSmooth(float target, float value, float fraction);

	bool worldToScreen(const Vector &in, ImVec2 &out, bool floor = true) noexcept;

	bool attacking(bool cmdAttack, bool cmdAttack2) noexcept;

	constexpr int replace(std::string &str, const std::string &from, const std::string &to) noexcept
	{
		size_t startPos = str.find(from);
		if (startPos == std::string::npos)
			return -1;
		str.replace(startPos, from.length(), to);
		return startPos;
	}

	float approxRadius(const StudioBbox &hitbox, int i) noexcept;

	bool animDataAuthenticity(Entity *animatable) noexcept;

	template <typename T>
	constexpr std::vector<T> join(const std::vector<T> &first, const std::vector<T> &second) noexcept
	{
		std::vector<T> buffer;
		buffer.reserve(first.size() + second.size());
		buffer.insert(buffer.end(), first.begin(), first.end());
		buffer.insert(buffer.end(), second.begin(), second.end());
		return buffer;
	}

	std::string decode(std::string) noexcept;

	bool lbyUpdate(Entity *animatable, float &nextUpdate) noexcept;

	void drawTriangleFromCenter(ImDrawList *drawList, const ImVec2 &pos, const Color4 &colorCfg) noexcept;
	ImVec2 drawText(ImDrawList *drawList, float distance, float cullDistance, const Color4Border &textCfg, const char *text, const ImVec2 &pos, bool centered = true, bool adjustHeight = true) noexcept;
}
