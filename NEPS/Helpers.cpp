#include <cmath>
#include <cwctype>
#include <tuple>

#include "imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_internal.h"

#include "Helpers.h"
#include "Config.h"
#include "ConfigStructs.h"
#include "GameData.h"
#include "Memory.h"
#include "SDK/GlobalVars.h"
#include "SDK/AnimState.h"
#include "SDK/Entity.h"
#include "SDK/PhysicsSurfaceProps.h"
#include "SDK/StudioRender.h"

std::array<float, 3U> Helpers::rgbToHsv(float r, float g, float b) noexcept
{
	r = std::clamp(r, 0.0f, 1.0f);
	g = std::clamp(g, 0.0f, 1.0f);
	b = std::clamp(b, 0.0f, 1.0f);
	const auto max = std::max({r, g, b});
	const auto min = std::min({r, g, b});
	const auto delta = max - min;

	float hue = 0.0f, sat = 0.0f;

	if (delta != 0.0f)
	{
		if (max == r)
			hue = std::fmodf((g - b) / delta, 6.0f) / 6.0f;
		else if (max == g)
			hue = ((b - r) / delta + 2.0f) / 6.0f;
		else if (max == b)
			hue = ((r - g) / delta + 4.0f) / 6.0f;
	}

	if (max != 0.0f)
		sat = delta / max;

	return {hue, sat, max};
}

std::array<float, 3U> Helpers::hsvToRgb(float h, float s, float v) noexcept
{
	h = h < 0.0f ? std::fmodf(h, 1.0f) + 1.0f : std::fmodf(h, 1.0f);
	s = std::clamp(s, 0.0f, 1.0f);
	v = std::clamp(v, 0.0f, 1.0f);
	const auto c = s * v;
	const auto x = c * (1.0f - std::fabsf(std::fmodf(h * 6.0f, 2.0f) - 1.0f));
	const auto m = v - c;

	float r = 0.0f, g = 0.0f, b = 0.0f;

	if (0.0f <= h && h < 1.0f / 6.0f)
		r = c, g = x;
	else if (1.0f / 6.0f <= h && h < 1.0f / 3.0f)
		r = x, g = c;
	else if (1.0f / 3.0f <= h && h < 0.5f)
		g = c, b = x;
	else if (0.5f <= h && h < 1.0f / 3.0f * 2.0f)
		g = x, b = c;
	else if (1.0f / 3.0f * 2.0f <= h && h < 1.0f / 6.0f * 5.0f)
		r = x, b = c;
	else if (1.0f / 6.0f * 5.0f <= h && h < 1.0f)
		r = c, b = x;

	return {r + m, g + m, b + m};
}

std::array<float, 3U> Helpers::rainbowColor(float speed) noexcept
{
	return hsvToRgb(speed * memory->globalVars->realtime * 0.1f, 1.0f, 1.0f);
}

std::array<float, 4U> Helpers::rainbowColor(float speed, float alpha) noexcept
{
	auto &&[r, g, b] = hsvToRgb(speed * memory->globalVars->realtime * 0.1f, 1.0f, 1.0f);
	return {r, g, b, alpha};
}

static bool traceToExit(const Trace &enterTrace, const Vector &start, const Vector &direction, Vector &end, Trace &exitTrace)
{
	bool result = false;
	#ifdef _WIN32
	const auto traceToExitFn = memory->traceToExit;
	__asm {
		push exitTrace
		mov eax, direction
		push[eax]Vector.z
		push[eax]Vector.y
		push[eax]Vector.x
		mov eax, start
		push[eax]Vector.z
		push[eax]Vector.y
		push[eax]Vector.x
		mov edx, enterTrace
		mov ecx, end
		call traceToExitFn
		add esp, 28
		mov result, al
	}
	#endif
	return result;
}

float Helpers::handleBulletPenetration(SurfaceData *enterSurfaceData, const Trace &enterTrace, const Vector &direction, Vector &result, float penetration, float damage) noexcept
{
	Vector end;
	Trace exitTrace;
	if (!traceToExit(enterTrace, enterTrace.endpos, direction, end, exitTrace))
		return -1.0f;

	SurfaceData *exitSurfaceData = interfaces->physicsSurfaceProps->getSurfaceData(exitTrace.surface.surfaceProps);

	float damageModifier = 0.16f;
	float penetrationModifier = (enterSurfaceData->penetrationmodifier + exitSurfaceData->penetrationmodifier) / 2.0f;

	if (enterSurfaceData->material == 71 || enterSurfaceData->material == 89)
	{
		damageModifier = 0.05f;
		penetrationModifier = 3.0f;
	} else if (enterTrace.contents >> 3 & 1 || enterTrace.surface.flags >> 7 & 1)
	{
		penetrationModifier = 1.0f;
	}

	if (enterSurfaceData->material == exitSurfaceData->material)
	{
		if (exitSurfaceData->material == 85 || exitSurfaceData->material == 87)
			penetrationModifier = 3.0f;
		else if (exitSurfaceData->material == 76)
			penetrationModifier = 2.0f;
	}

	damage -= 11.25f / penetration / penetrationModifier + damage * damageModifier + (exitTrace.endpos - enterTrace.endpos).squareLength() / 24.0f / penetrationModifier;

	result = exitTrace.endpos;
	return damage;
}

Vector Helpers::calculateRelativeAngle(const Vector &source, const Vector &destination, const Vector &viewAngles) noexcept
{
	return ((destination - source).toAngle() - viewAngles).normalize();
}

int Helpers::findDamage(const Vector &destination, const WeaponInfo *weaponData, Trace &trace, bool allowFriendlyFire, int hitgroupFlags, bool *goesThroughWall) noexcept
{
	if (!localPlayer)
		return -1;

	if (!weaponData)
		return -1;

	float damage{static_cast<float>(weaponData->damage)};

	Vector start = localPlayer->getEyePosition();
	Vector direction = destination - start;
	float traveled = direction.length();
	direction /= traveled;

	auto calcDepth = AUTOWALL_CALC_DEPTH;

	while (damage >= 1.0f && calcDepth)
	{
		interfaces->engineTrace->traceRay({start, destination}, 0x4600400B, localPlayer.get(), trace);

		if (trace.fraction == 1.0f)
			break;

		if (trace.hitgroup > HitGroup::Generic && trace.hitgroup <= HitGroup::RightLeg)
		{
			if (!trace.entity || !trace.entity->isPlayer())
				break;

			if (!allowFriendlyFire && !localPlayer->isOtherEnemy(trace.entity))
				break;

			if (trace.entity->gunGameImmunity())
				break;

			if (~hitgroupFlags & (1 << (trace.hitgroup - 1)))
				break;

			const auto m = std::strstr(weaponData->name, "Taser") ? 1.0f : HitGroup::getDamageMultiplier(trace.hitgroup);
			damage = m * damage * std::powf(weaponData->rangeModifier, trace.fraction * traveled / 500.0f);

			if (float armorRatio{weaponData->armorRatio / 2.0f}; HitGroup::isArmored(trace.hitgroup, trace.entity->hasHelmet()))
				damage -= (trace.entity->armor() < damage * armorRatio / 2.0f ? trace.entity->armor() * 4.0f : damage) * (1.0f - armorRatio);

			return static_cast<int>(damage);
		}

		if (goesThroughWall)
			*goesThroughWall = true;

		const auto surfaceData = interfaces->physicsSurfaceProps->getSurfaceData(trace.surface.surfaceProps);

		if (surfaceData->penetrationmodifier < AUTOWALL_MIN_PENETRATION)
			break;

		damage = handleBulletPenetration(surfaceData, trace, direction, start, weaponData->penetration, damage);
		calcDepth--;
	}

	return -1;
}

bool Helpers::canHit(const Vector &destination, Trace &trace, bool allowFriendlyFire, bool *goesThroughWall) noexcept
{
	if (!localPlayer)
		return false;

	Vector start = localPlayer->getEyePosition();
	Vector direction = destination - start;
	direction /= direction.length();

	auto calcDepth = AUTOWALL_CALC_DEPTH;

	while (calcDepth)
	{
		interfaces->engineTrace->traceRay({start, destination}, 0x4600400B, localPlayer.get(), trace);

		if (trace.fraction == 1.0f)
			break;

		if (trace.hitgroup > HitGroup::Generic && trace.hitgroup <= HitGroup::RightLeg)
		{
			if (!trace.entity || !trace.entity->isPlayer())
				break;

			if (!allowFriendlyFire && !localPlayer->isOtherEnemy(trace.entity))
				break;

			if (trace.entity->gunGameImmunity())
				break;

			return true;
		}

		if (goesThroughWall)
			*goesThroughWall = true;

		const auto surfaceData = interfaces->physicsSurfaceProps->getSurfaceData(trace.surface.surfaceProps);

		if (surfaceData->penetrationmodifier < AUTOWALL_MIN_PENETRATION)
			break;

		Vector end;
		Trace exitTrace;
		if (!traceToExit(trace, trace.endpos, direction, end, exitTrace))
			break;

		start = exitTrace.endpos;
		calcDepth--;
	}

	return false;
}

float Helpers::findHitchance(float inaccuracy, float spread, float targetRadius, float distance) noexcept
{
	float f = targetRadius / (std::tan(spread + inaccuracy) * distance);
	return std::clamp(f, 0.0f, 1.0f) * 100.0f;
}

static float alphaFactor = 1.0f;

void Helpers::setAlphaFactor(float factor) noexcept
{
	alphaFactor = factor;
}

unsigned int Helpers::calculateColor(Color4 color) noexcept
{
	color.color[3] *= (255.0f - GameData::local().flashDuration) / 255.0f;
	color.color[3] *= alphaFactor;
	auto &&[r, g, b, a] = color.rainbow ? rainbowColor(color.rainbowSpeed, color.color[3]) : color.color;
	return ImGui::ColorConvertFloat4ToU32({r, g, b, a});
}

unsigned int Helpers::calculateColor(Color3 color) noexcept
{
	auto &&[r, g, b] = color.rainbow ? rainbowColor(color.rainbowSpeed) : color.color;
	return ImGui::ColorConvertFloat4ToU32({r, g, b, 1.0f});
}

unsigned int Helpers::calculateColor(int r, int g, int b, int a) noexcept
{
	a -= static_cast<int>(a * GameData::local().flashDuration / 255.0f);
	a = static_cast<int>(a * alphaFactor);
	return IM_COL32(r, g, b, a);
}

unsigned int Helpers::calculateColor(float r, float g, float b, float a) noexcept
{
	a -= a * GameData::local().flashDuration / 255.0f;
	a *= alphaFactor;
	return ImGui::ColorConvertFloat4ToU32({r, g, b, a});
}

ImWchar* Helpers::getFontGlyphRanges() noexcept
{
    static ImVector<ImWchar> ranges;
    if (ranges.empty()) {
        ImFontGlyphRangesBuilder builder;
        constexpr ImWchar baseRanges[]{
            0x0100, 0x024F, // Latin Extended-A + Latin Extended-B
            0x0300, 0x03FF, // Combining Diacritical Marks + Greek/Coptic
            0x0600, 0x06FF, // Arabic
            0x0E00, 0x0E7F, // Thai
			0x2605, 0x2605, // ★
            0
        };
        builder.AddRanges(baseRanges);
        builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
        builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesChineseSimplifiedCommon());
        builder.AddText("\u9F8D\u738B\u2122");
        builder.BuildRanges(&ranges);
    }
    return ranges.Data;
}

std::wstring Helpers::toWideString(const std::string& str) noexcept
{
    std::wstring upperCase(str.length(), L'\0');
    if (const auto newLen = std::mbstowcs(upperCase.data(), str.c_str(), upperCase.length()); newLen != static_cast<std::size_t>(-1))
        upperCase.resize(newLen);
    return upperCase;
}

std::wstring Helpers::toUpper(std::wstring str) noexcept
{
    std::transform(str.begin(), str.end(), str.begin(), [](wchar_t w) { return std::towupper(w); });
    return str;
}

float Helpers::angleDiffDeg(float a1, float a2) noexcept
{
	float delta;

	delta = std::remainder(a1 - a2, 360.0f);
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

float Helpers::angleDiffRad(float a1, float a2) noexcept
{
	float delta;

	delta = std::remainder(a1 - a2, PI * 2);
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

float Helpers::approachAngleDeg(float target, float value, float speed) noexcept
{
	target = std::remainder(target, 360.0f);
	value = std::remainder(value, 360.0f);

	float delta = target - value;

	// Speed is assumed to be positive
	if (speed < 0)
		speed = -speed;

	if (delta < -180.0f)
		delta += 360.0f;
	else if (delta > 180.0f)
		delta -= 360.0f;

	if (delta > speed)
		value += speed;
	else if (delta < -speed)
		value -= speed;
	else
		value = target;

	return value;
}

float Helpers::approachAngleRad(float target, float value, float speed) noexcept
{
	target = std::remainder(target, PI * 2);
	value = std::remainder(value, PI * 2);

	float delta = target - value;

	// Speed is assumed to be positive
	if (speed < 0)
		speed = -speed;

	if (delta < -PI)
		delta += PI * 2;
	else if (delta > PI)
		delta -= PI * 2;

	if (delta > speed)
		value += speed;
	else if (delta < -speed)
		value -= speed;
	else
		value = target;

	return value;
}

void Helpers::feetYaw(AnimState *state, float pursue, float &hold, float &current) noexcept
{
	if (state->onGround)
	{
		static float realignTimer = 0.0f;

		if (state->horizontalSpeed > 0.1f)
		{
			const float deltaTime = std::max(0.0f, memory->globalVars->currenttime - state->lastClientSideAnimationUpdateTime);

			current = approachAngleDeg(pursue, current, deltaTime * (30.0f + 20.0f * state->stopToFullRunningFraction));

			realignTimer = memory->globalVars->currenttime + 0.22f;
			hold = pursue;
		} else
		{
			const float deltaTime = std::max(0.0f, memory->globalVars->currenttime - state->lastClientSideAnimationUpdateTime);

			current = approachAngleDeg(hold, current, deltaTime * 100.0f);

			if (memory->globalVars->currenttime > realignTimer && std::abs(angleDiffDeg(current, pursue)) > 35.0f)
			{
				realignTimer = memory->globalVars->currenttime + 1.1f;
				hold = pursue;
			}
		}
	}
}

float Helpers::approachValSmooth(float target, float value, float fraction)
{
	float delta = target - value;
	fraction = std::clamp(fraction, 0.0f, 1.0f);
	delta *= fraction;
	return value + delta;
}

bool Helpers::worldToScreen(const Vector &in, ImVec2 &out, bool floor) noexcept
{
	const auto &matrix = GameData::toScreenMatrix();

	const auto w = matrix._41 * in.x + matrix._42 * in.y + matrix._43 * in.z + matrix._44;
	if (w < 0.001f)
		return false;

	out = ImGui::GetIO().DisplaySize / 2.0f;
	out.x *= 1.0f + (matrix._11 * in.x + matrix._12 * in.y + matrix._13 * in.z + matrix._14) / w;
	out.y *= 1.0f - (matrix._21 * in.x + matrix._22 * in.y + matrix._23 * in.z + matrix._24) / w;
	if (floor)
		out = ImFloor(out);
	return true;
}

bool Helpers::attacking(bool cmdAttack, bool cmdAttack2) noexcept
{
	if (!localPlayer) return false;
	const float time = memory->globalVars->serverTime();

	if (localPlayer->nextAttack() <= time && !localPlayer->waitForNoAttack() && !localPlayer->isDefusing())
	{
		const auto activeWeapon = localPlayer->getActiveWeapon();
		if (activeWeapon)
		{
			if (activeWeapon->itemDefinitionIndex2() == WeaponId::Healthshot)
				return false;

			if (activeWeapon->isGrenade())
				return !activeWeapon->pinPulled() && activeWeapon->throwTime() > 0.0f && activeWeapon->throwTime() <= time;

			if (activeWeapon->burstMode() && activeWeapon->nextBurstShot() > 0.0f && activeWeapon->nextBurstShot() <= time)
				return true;

			if (activeWeapon->clip() && activeWeapon->nextPrimaryAttack() <= time)
			{
				if (activeWeapon->isKnife() && (cmdAttack || cmdAttack2))
					return true;

				if (activeWeapon->itemDefinitionIndex2() == WeaponId::Revolver)
				{
					return cmdAttack2 || cmdAttack && activeWeapon->nextPrimaryAttack() < time && activeWeapon->postponeFireReadyTime() <= time;
				}

				return (!localPlayer->shotsFired() || activeWeapon->isFullAuto()) && cmdAttack;
			}
		}
	}

	return false;
}

int Helpers::replace(std::string &str, const std::string &from, const std::string &to) noexcept
{
	size_t startPos = str.find(from);
	if (startPos == std::string::npos)
		return -1;
	str.replace(startPos, from.length(), to);
	return startPos;
}

float Helpers::approxRadius(const StudioBbox &hitbox, int i) noexcept
{
	switch (i)
	{
	case Hitbox::Thorax:
		return 7.89f;
	case Hitbox::UpperChest:
		return 6.75f;
	case Hitbox::Pelvis:
		return 6.08f;
	case Hitbox::Head:
		return 4.17f;
	}

	return hitbox.capsuleRadius;
}
