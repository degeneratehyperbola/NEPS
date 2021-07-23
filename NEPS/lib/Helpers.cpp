#include "Helpers.hpp"

#include "../Hacks/Backtrack.h"

#include "../GameData.h"
#include "../res_defaultfont.h"

#include "../SDK/EngineTrace.h"
#include "../SDK/Entity.h"
#include "../SDK/PhysicsSurfaceProps.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_internal.h"

#include <cwctype>

std::array<float, 3U> Helpers::rgbToHsv(float r, float g, float b) noexcept
{
	r = std::clamp(r, 0.0f, 1.0f);
	g = std::clamp(g, 0.0f, 1.0f);
	b = std::clamp(b, 0.0f, 1.0f);
	const auto max = std::max({r, g, b});
	const auto min = std::min({r, g, b});
	const auto delta = max - min;

	float hue = 0.0f, sat = 0.0f;

	if (delta)
	{
		if (max == r)
			hue = std::fmodf((g - b) / delta, 6.0f) / 6.0f;
		else if (max == g)
			hue = ((b - r) / delta + 2.0f) / 6.0f;
		else if (max == b)
			hue = ((r - g) / delta + 4.0f) / 6.0f;
	}

	if (max)
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
	float distance = 0.0f;

	constexpr auto didHit = [](const Trace &trace) noexcept
	{
		return trace.fraction < 1.0f || trace.allSolid || trace.startSolid;
	};

	constexpr auto didHitNonWorld = [](Entity *entity) noexcept
	{
		return entity != nullptr && entity == interfaces->entityList->getEntity(0);
	};

	while (distance <= 90.0f)
	{
		distance += 4.0f;
		end = start + direction * distance;

		int pointContents = interfaces->engineTrace->getPointContents(end, MASK_SHOT_HULL | CONTENTS_HITBOX);

		if (pointContents & MASK_SHOT_HULL && (!(pointContents & CONTENTS_HITBOX)))
			continue;

		auto march = end - (direction * 4.0f);

		interfaces->engineTrace->traceRay({end, march}, MASK_SHOT, nullptr, exitTrace);

		if (exitTrace.startSolid && exitTrace.surface.flags & SURF_HITBOX)
		{
			interfaces->engineTrace->traceRay({end, start}, MASK_SHOT & ~CONTENTS_HITBOX, exitTrace.entity, exitTrace);

			if (didHit(exitTrace) && !exitTrace.startSolid)
			{
				end = exitTrace.endPos;
				return true;
			}
			continue;
		}

		if (!didHit(exitTrace) || exitTrace.startSolid)
		{
			if (exitTrace.entity)
			{
				if (didHitNonWorld(enterTrace.entity))
					return true;
			}
			continue;
		}

		if (((exitTrace.surface.flags >> 7) & 1) && !((enterTrace.surface.flags >> 7) & 1))
			continue;

		if (exitTrace.planeNormal.dotProduct(direction) <= 1.0f)
		{
			float fraction = exitTrace.fraction * 4.0f;
			end = end - (direction * fraction);
			return true;
		}
	}
	return false;
}

float Helpers::handleBulletPenetration(SurfaceData *enterSurfaceData, const Trace &enterTrace, const Vector &direction, Vector &result, float penetration, float damage) noexcept
{
	Vector end;
	Trace exitTrace;
	if (!traceToExit(enterTrace, enterTrace.endPos, direction, end, exitTrace))
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

	damage -= 11.25f / penetration / penetrationModifier + damage * damageModifier + (exitTrace.endPos - enterTrace.endPos).squareLength() / 24.0f / penetrationModifier;

	result = exitTrace.endPos;
	return damage;
}

Vector Helpers::calculateRelativeAngle(const Vector &source, const Vector &destination, const Vector &viewAngles) noexcept
{
	return ((destination - source).toAngle() - viewAngles).normalize();
}

int Helpers::findDamage(const Vector &destination, const WeaponInfo *weaponData, Trace &trace, bool allowFriendlyFire, int hitgroupFlags, bool *goesThroughWall, const Backtrack::Record *ghost, int ghostHitbox) noexcept
{
	if (!localPlayer)
		return -1;

	if (!weaponData)
		return -1;

	float damage = static_cast<float>(weaponData->damage);

	Vector start = localPlayer->getEyePosition();
	Vector direction = destination - start;
	float traveled = direction.length();
	direction /= traveled;

	auto calcDepth = 4;

	constexpr auto hitboxToHitGroup = [](int hitbox) constexpr noexcept -> int
	{
		switch (hitbox)
		{
		case Hitbox::Head:
			return HitGroup::Head;
		case Hitbox::Neck:
		case Hitbox::UpperChest:
		case Hitbox::LowerChest:
		case Hitbox::Thorax:
			return HitGroup::Chest;
		case Hitbox::Belly:
		case Hitbox::Pelvis:
			return HitGroup::Stomach;
		case Hitbox::LeftThigh:
		case Hitbox::LeftCalf:
		case Hitbox::LeftFoot:
			return HitGroup::LeftLeg;
		case Hitbox::RightThigh:
		case Hitbox::RightCalf:
		case Hitbox::RightFoot:
			return HitGroup::RightLeg;
		case Hitbox::LeftUpperArm:
		case Hitbox::LeftForearm:
		case Hitbox::LeftHand:
			return HitGroup::LeftArm;
		case Hitbox::RightUpperArm:
		case Hitbox::RightForearm:
		case Hitbox::RightHand:
			return HitGroup::RightArm;
		}

		return -1;
	};

	constexpr auto calcDamage = [](const WeaponInfo *weaponData, bool hasHelmet, int armor, int hitGroup, float traveled, float &damage) noexcept
	{
		const auto m = std::strstr(weaponData->name, "Taser") ? 1.0f : HitGroup::getDamageMultiplier(hitGroup);
		damage = m * damage * std::powf(weaponData->rangeModifier, traveled / 500.0f);

		if (float armorRatio = weaponData->armorRatio / 2; HitGroup::isArmored(hitGroup, hasHelmet))
			damage -= (armor < damage * armorRatio / 2 ? armor * 4.0f : damage) * (1.0f - armorRatio);
	};

	while (damage >= 1.0f && calcDepth)
	{
		interfaces->engineTrace->traceRay({start, destination}, MASK_SHOT, localPlayer.get(), trace);

		if (trace.fraction == 1.0f)
		{
			if (!ghost)
				break;

			const auto hitGroup = hitboxToHitGroup(ghostHitbox);
			if (~hitgroupFlags & (1 << (hitGroup - 1)))
				break;

			calcDamage(weaponData, ghost->hasHelmet, ghost->armor, hitGroup, traveled, damage);
			return static_cast<int>(damage);
		}

		if (!ghost && trace.hitGroup > HitGroup::Generic && trace.hitGroup <= HitGroup::RightLeg)
		{
			if (!trace.entity || !trace.entity->isPlayer())
				break;

			if (!allowFriendlyFire && !localPlayer->isOtherEnemy(trace.entity))
				break;

			if (trace.entity->gunGameImmunity())
				break;

			if (~hitgroupFlags & (1 << (trace.hitGroup - 1)))
				break;

			calcDamage(weaponData, trace.entity->hasHelmet(), trace.entity->armor(), trace.hitGroup, traveled * trace.fraction, damage);
			return static_cast<int>(damage);
		}

		if (goesThroughWall)
			*goesThroughWall = true;

		const auto surfaceData = interfaces->physicsSurfaceProps->getSurfaceData(trace.surface.surfaceProps);

		if (surfaceData->penetrationmodifier < 0.1f)
			break;

		damage = handleBulletPenetration(surfaceData, trace, direction, start, weaponData->penetration, damage);
		calcDepth--;
	}

	return -1;
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

ImWchar *Helpers::getFontGlyphRanges() noexcept
{
	static ImVector<ImWchar> ranges;
	if (ranges.empty())
	{
		ImFontGlyphRangesBuilder builder;
		constexpr ImWchar baseRanges[] = {
			0x0100, 0x024F, // Latin Extended-A + Latin Extended-B
			0x0300, 0x03FF, // Combining Diacritical Marks + Greek/Coptic
			0x0600, 0x06FF, // Arabic
			0x0E00, 0x0E7F, // Thai
			//0x0000, 0xFFFF, // Fuck You
			0
		};
		builder.AddRanges(baseRanges);
		builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
		builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesDefault());
		builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesJapanese());
		builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesChineseSimplifiedCommon());
		// ★龍王™
		builder.AddChar(u'\u2605'); // ★
		builder.AddChar(u'\u9F8D'); // 龍
		builder.AddChar(u'\u738B'); // 王
		builder.AddChar(u'\u2122'); // ™
		builder.BuildRanges(&ranges);
	}
	return ranges.Data;
}

std::wstring Helpers::toWideString(const std::string &str) noexcept
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
	case Hitbox::LeftFoot:
	case Hitbox::RightFoot:
		return 4.30f;
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

bool Helpers::animDataAuthenticity(Entity *animatable) noexcept
{
	if (!animatable || !animatable->isPlayer())
		return false;

	if (~animatable->flags() & Entity::FL_ONGROUND) return true;
	if (animatable->moveType() == MoveType::LADDER) return true;
	if (animatable->moveType() == MoveType::NOCLIP) return true;
	if (animatable->isBot()) return true;
	const float simulationTime = animatable->simulationTime();
	const auto remoteActiveWeapon = animatable->getActiveWeapon();
	if (remoteActiveWeapon && Helpers::timeToTicks(remoteActiveWeapon->lastShotTime()) == Helpers::timeToTicks(simulationTime)) return true;
	const float oldSimulationTime = animatable->oldSimulationTime();
	if (!Helpers::timeToTicks(simulationTime - oldSimulationTime)) return true;

	return false;
}

std::string Helpers::decode(std::string in) noexcept
{
	std::string out;

	for (auto &c : in) --c;

	std::vector<int> T(256, -1);
	for (int i = 0; i < 64; i++) T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;

	int val = 0, valb = -8;
	for (unsigned char c : in)
	{
		if (T[c] == -1) break;
		val = (val << 6) + T[c];
		valb += 6;
		if (valb >= 0)
		{
			out.push_back(char((val >> valb) & 0xFF));
			valb -= 8;
		}
	}

	return out;
}

const void *Helpers::getDefaultFontData() noexcept
{
	return reinterpret_cast<const void *>(_compressedFontData);
}

std::size_t Helpers::getDefaultFontSize() noexcept
{
	return _compressedFontSize;
}
