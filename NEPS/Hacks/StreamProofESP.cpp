#include "StreamProofESP.h"

#include <shared_lib/imgui/imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <shared_lib/imgui/imgui_internal.h>

#include "../Config.h"
#include "../GameData.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../lib/fnv.hpp"
#include "../lib/ImGuiCustom.hpp"
#include "../lib/Helpers.hpp"
#include "../SDK/Engine.h"
#include "../SDK/Entity.h"
#include "../SDK/GlobalVars.h"

#include <limits>
#include <tuple>
#include <sstream>

static constexpr auto operator-(float sub, const std::array<float, 3> &a) noexcept
{
	return Vector{sub - a[0], sub - a[1], sub - a[2]};
}

struct BoundingBox
{
private:
	bool valid;
public:
	ImVec2 min, max;
	std::array<ImVec2, 8> vertices;

	BoundingBox(const Vector &mins, const Vector &maxs, const std::array<float, 3> &scale, const Matrix3x4 *matrix = nullptr) noexcept
	{
		min.y = min.x = std::numeric_limits<float>::max();
		max.y = max.x = -std::numeric_limits<float>::max();

		const auto scaledMins = mins + (maxs - mins) * 2 * (0.25f - scale);
		const auto scaledMaxs = maxs - (maxs - mins) * 2 * (0.25f - scale);

		for (int i = 0; i < 8; ++i)
		{
			const Vector point{i & 1 ? scaledMaxs.x : scaledMins.x,
								i & 2 ? scaledMaxs.y : scaledMins.y,
								i & 4 ? scaledMaxs.z : scaledMins.z};

			if (!Helpers::worldToScreen(matrix ? point.transform(*matrix) : point, vertices[i]))
			{
				valid = false;
				return;
			}

			min.x = std::min(min.x, vertices[i].x);
			min.y = std::min(min.y, vertices[i].y);
			max.x = std::max(max.x, vertices[i].x);
			max.y = std::max(max.y, vertices[i].y);
		}
		valid = true;
	}

	BoundingBox(const BaseData &data, const std::array<float, 3> &scale) noexcept : BoundingBox{data.obbMins, data.obbMaxs, scale, &data.coordinateFrame} {}
	BoundingBox(const Vector &center) noexcept : BoundingBox{center - 2.0f, center + 2.0f, { 0.25f, 0.25f, 0.25f }} {}

	operator bool() const noexcept
	{
		return valid;
	}
};

static ImDrawList* drawList;

// Convex hull using Graham's scan
static std::vector<ImVec2> convexHull(std::vector<ImVec2> points) noexcept
{
	if (points.size() < 3)
		return {};

	std::swap(points[0], *std::min_element(points.begin(), points.end(), [](const auto &a, const auto &b) { return (a.x < b.x || (a.x == b.x && a.y < b.y)); }));

	constexpr auto orientation = [](const ImVec2 &a, const ImVec2 &b, const ImVec2 &c)
	{
		return (b.y - a.y) * (c.x - b.x) - (b.x - a.x) * (c.y - b.y);
	};

	std::sort(points.begin() + 1, points.end(), [&](const auto &a, const auto &b) { return orientation(points[0], a, b) > 0.0f; });

	std::vector<ImVec2> hull;

	for (const auto &p : points)
	{
		while (hull.size() >= 2 && orientation(hull[hull.size() - 2], hull[hull.size() - 1], p) < 0.0f)
			hull.pop_back();
		hull.emplace_back(p);
	}

	return hull;
}

static bool cullText(float distance, float cullDistance) noexcept
{
	if (cullDistance > 0)
		return distance > cullDistance;
	else if (cullDistance < 0)
		return distance < -cullDistance;

	return false;
}

static void renderBox(const BoundingBox& bbox, const Box& config) noexcept
{
	if (!config.enabled)
		return;

    const ImU32 color = Helpers::calculateColor(config);
    const ImU32 fillColor = Helpers::calculateColor(config.secondaryColor);

	switch (config.type)
	{
	case Box::_2d:
		switch (config.secondary)
		{
		case Box::Fill:
			drawList->AddRectFilled(bbox.min + ImVec2{1.0f, 1.0f}, bbox.max - ImVec2{1.0f, 1.0f}, fillColor, config.rounding, ImDrawCornerFlags_All);
			break;
		case Box::Outline:
			drawList->AddRect(bbox.min, bbox.max, fillColor, config.rounding, ImDrawCornerFlags_All, 3.0f);
			break;
		}

		drawList->AddRect(bbox.min, bbox.max, color, config.rounding, ImDrawCornerFlags_All);
		break;
	case Box::_2dCorners:
		switch (config.secondary)
		{
		case Box::Fill:
			drawList->AddRectFilled(bbox.min + ImVec2{1.0f, 1.0f}, bbox.max - ImVec2{1.0f, 1.0f}, fillColor, config.rounding, ImDrawCornerFlags_All);
			break;
		case Box::Outline:
			drawList->AddLine(bbox.min, {bbox.min.x, IM_FLOOR(bbox.min.y * 0.75f + bbox.max.y * 0.25f)}, fillColor, 3.0f);
			drawList->AddLine(bbox.min, {IM_FLOOR(bbox.min.x * 0.75f + bbox.max.x * 0.25f), bbox.min.y}, fillColor, 3.0f);

			drawList->AddLine({bbox.max.x, bbox.min.y}, {IM_FLOOR(bbox.max.x * 0.75f + bbox.min.x * 0.25f), bbox.min.y}, fillColor, 3.0f);
			drawList->AddLine({bbox.max.x - 1.0f, bbox.min.y}, {bbox.max.x - 1.0f, IM_FLOOR(bbox.min.y * 0.75f + bbox.max.y * 0.25f)}, fillColor, 3.0f);

			drawList->AddLine({bbox.min.x, bbox.max.y}, {bbox.min.x, IM_FLOOR(bbox.max.y * 0.75f + bbox.min.y * 0.25f)}, fillColor, 3.0f);
			drawList->AddLine({bbox.min.x, bbox.max.y - 1.0f}, {IM_FLOOR(bbox.min.x * 0.75f + bbox.max.x * 0.25f), bbox.max.y - 1.0f}, fillColor, 3.0f);

			drawList->AddLine(bbox.max - ImVec2{0.5f, 1.0f}, {IM_FLOOR(bbox.max.x * 0.75f + bbox.min.x * 0.25f), bbox.max.y - 1.0f}, fillColor, 3.0f);
			drawList->AddLine(bbox.max - ImVec2{1.0f, 0.0f}, {bbox.max.x - 1.0f, IM_FLOOR(bbox.max.y * 0.75f + bbox.min.y * 0.25f)}, fillColor, 3.0f);
			break;
		}

		{
			drawList->AddLine(bbox.min, {bbox.min.x, IM_FLOOR(bbox.min.y * 0.75f + bbox.max.y * 0.25f)}, color);
			drawList->AddLine(bbox.min, {IM_FLOOR(bbox.min.x * 0.75f + bbox.max.x * 0.25f), bbox.min.y}, color);

			drawList->AddLine({bbox.max.x, bbox.min.y}, {IM_FLOOR(bbox.max.x * 0.75f + bbox.min.x * 0.25f), bbox.min.y}, color);
			drawList->AddLine({bbox.max.x - 1.0f, bbox.min.y}, {bbox.max.x - 1.0f, IM_FLOOR(bbox.min.y * 0.75f + bbox.max.y * 0.25f)}, color);

			drawList->AddLine({bbox.min.x, bbox.max.y}, {bbox.min.x, IM_FLOOR(bbox.max.y * 0.75f + bbox.min.y * 0.25f)}, color);
			drawList->AddLine({bbox.min.x, bbox.max.y - 1.0f}, {IM_FLOOR(bbox.min.x * 0.75f + bbox.max.x * 0.25f), bbox.max.y - 1.0f}, color);

			drawList->AddLine(bbox.max - ImVec2{0.5f, 1.0f}, {IM_FLOOR(bbox.max.x * 0.75f + bbox.min.x * 0.25f), bbox.max.y - 1.0f}, color);
			drawList->AddLine(bbox.max - ImVec2{1.0f, 0.0f}, {bbox.max.x - 1.0f, IM_FLOOR(bbox.max.y * 0.75f + bbox.min.y * 0.25f)}, color);
		}
		break;
	case Box::_3d:
		switch (config.secondary)
		{
		case Box::Fill:
		{
			const auto hull = convexHull({std::begin(bbox.vertices), std::end(bbox.vertices)});
			drawList->AddConvexPolyFilled(hull.data(), hull.size(), fillColor);
			break;
		}
		case Box::Outline:
			for (int i = 0; i < 8; ++i)
			{
				for (int j = 1; j <= 4; j <<= 1)
				{
					if (!(i & j))
						drawList->AddLine(bbox.vertices[i], bbox.vertices[i + j], fillColor, 3.0f);
				}
			}
			break;
		}

		for (int i = 0; i < 8; ++i)
		{
			for (int j = 1; j <= 4; j <<= 1)
			{
				if (!(i & j))
					drawList->AddLine(bbox.vertices[i], bbox.vertices[i + j], color);
			}
		}
		break;
	case Box::_3dCorners:
		switch (config.secondary)
		{
		case Box::Fill:
		{
			const auto hull = convexHull({std::begin(bbox.vertices), std::end(bbox.vertices)});
			drawList->AddConvexPolyFilled(hull.data(), hull.size(), fillColor);
			break;
		}
		case Box::Outline:
			for (int i = 0; i < 8; ++i)
			{
				for (int j = 1; j <= 4; j <<= 1)
				{
					if (!(i & j))
					{
						drawList->AddLine(bbox.vertices[i], ImVec2{bbox.vertices[i].x * 0.75f + bbox.vertices[i + j].x * 0.25f, bbox.vertices[i].y * 0.75f + bbox.vertices[i + j].y * 0.25f}, fillColor, 3.0f);
						drawList->AddLine(ImVec2{bbox.vertices[i].x * 0.25f + bbox.vertices[i + j].x * 0.75f, bbox.vertices[i].y * 0.25f + bbox.vertices[i + j].y * 0.75f}, bbox.vertices[i + j], fillColor, 3.0f);
					}
				}
			}
			break;
		}

		for (int i = 0; i < 8; ++i)
		{
			for (int j = 1; j <= 4; j <<= 1)
			{
				if (!(i & j))
				{
					drawList->AddLine(bbox.vertices[i], {bbox.vertices[i].x * 0.75f + bbox.vertices[i + j].x * 0.25f, bbox.vertices[i].y * 0.75f + bbox.vertices[i + j].y * 0.25f}, color);
					drawList->AddLine({bbox.vertices[i].x * 0.25f + bbox.vertices[i + j].x * 0.75f, bbox.vertices[i].y * 0.25f + bbox.vertices[i + j].y * 0.75f}, bbox.vertices[i + j], color);
				}
			}
		}
		break;
	}
}

static void drawSnapline(const Snapline &config, const ImVec2 &min, const ImVec2 &max) noexcept
{
	if (!config.enabled)
		return;

	const auto &screenSize = ImGui::GetIO().DisplaySize;

	ImVec2 p1, p2;
	p1.x = screenSize.x / 2;
	p2.x = (min.x + max.x) / 2;

	switch (config.type)
	{
	case Snapline::Bottom:
		p1.y = screenSize.y;
		p2.y = max.y;
		break;
	case Snapline::Top:
		p1.y = 0.0f;
		p2.y = min.y;
		break;
	case Snapline::Crosshair:
		p1.y = screenSize.y / 2;
		p2.y = (min.y + max.y) / 2;
		break;
	default:
		return;
	}

	drawList->AddLine(p1, p2, Helpers::calculateColor(config), config.thickness);
}

struct FontPush
{
	FontPush(const std::string &name)
	{
		if (const auto it = config->getFonts().find(name); it != config->getFonts().end())
			ImGui::PushFont(it->second.font);
		else
			ImGui::PushFont(nullptr);
	}

	~FontPush()
	{
		ImGui::PopFont();
	}
};

static void drawHealthBar(const ImVec2 &pos, float height, int health, const Color4OutlineToggle &healthBarConfig, const Color4OutlineToggle &text, float distance, float cull) noexcept
{
	constexpr float width = 3.0f;

	if (healthBarConfig.enabled)
	{
		const auto textPos = ImGuiCustom::drawProgressBar(drawList, static_cast<float>(health) / 100, pos, ImVec2{width, height}, healthBarConfig.outline, true, healthBarConfig, true, false);

		if (text.enabled && !cullText(distance, cull))
			ImGuiCustom::drawText(drawList, std::to_string(health).c_str(), textPos, text, text.outline, true, false);
	} else if (text.enabled && !cullText(distance, cull))
		ImGuiCustom::drawText(drawList, std::to_string(health).c_str(), pos + ImVec2{width / 2, 0}, text, text.outline, true, false);
}

static void renderPlayerBox(const PlayerData &playerData, const Player &config) noexcept
{
	if (!playerData.inViewFrustum)
		return;

	const BoundingBox bbox = {playerData, config.box.scale};

	if (!bbox)
		return;

	renderBox(bbox, config.box);

	ImVec2 offsetMins{}, offsetMaxs{};

	FontPush font{config.font.name};

	drawHealthBar(bbox.min - ImVec2{5.0f, 0.0f}, (bbox.max.y - bbox.min.y), playerData.health, config.healthBar, config.health, playerData.distanceToLocal, config.textCullDistance);

	if (config.name.enabled && !cullText(playerData.distanceToLocal, config.textCullDistance))
	{
		const auto nameSize = ImGuiCustom::drawText(drawList, playerData.name.c_str(), {(bbox.min.x + bbox.max.x) * 0.5f, bbox.min.y}, config.name, config.name.outline);
		offsetMins.y -= nameSize.y;
	}

	if (config.flags.enabled)
	{
		std::ostringstream flags;

		#ifdef NEPS_DEBUG
		if (playerData.chokedPackets)
			flags << "CHK " << playerData.chokedPackets << "\n";
		#endif // NEPS_DEBUG

		if (playerData.isBot)
			flags << "BOT\n";
		if (playerData.hasBomb)
			flags << "C4\n";
		if (playerData.isVip)
			flags << "VIP\n";
		if (playerData.hasDefuser)
			flags << "KIT\n";
		if (playerData.ducking)
			flags << "DUCK\n";
		if (playerData.reloading)
			flags << "RELOAD\n";
		if (playerData.armor)
			flags << "AMR " << playerData.armor << "\n";

		if (!flags.str().empty() && !cullText(playerData.distanceToLocal, config.textCullDistance))
			ImGuiCustom::drawText(drawList, flags.str().c_str(), {bbox.max.x + 1.0f, bbox.min.y}, config.flags, config.flags.outline, false, false);
	}

	if (config.flashDuration.enabled && playerData.flashDuration > 0.0f)
	{
		const auto radius = std::max(6.0f - playerData.distanceToLocal / 600.0f, 2.0f);
		ImVec2 flashDurationPos{(bbox.min.x + bbox.max.x) / 2, bbox.min.y + offsetMins.y - radius * 1.5f};

		const auto color = Helpers::calculateColor(config.flashDuration);
		constexpr float pi = std::numbers::pi_v<float>;
		drawList->PathArcTo(flashDurationPos, radius, pi / 2 - (playerData.flashDuration / 255.0f * pi), pi / 2 + (playerData.flashDuration / 255.0f * pi), 40);
		if (config.flashDuration.outline)
			drawList->AddPolyline(drawList->_Path.Data, drawList->_Path.Size, color & IM_COL32_A_MASK, false, 3.0f);
		drawList->PathStroke(color, false);

		offsetMins.y -= radius * 2.5f;
	}

	if (config.weapon.enabled && !playerData.activeWeapon.empty() && !cullText(playerData.distanceToLocal, config.textCullDistance))
	{
		const auto weaponTextSize = ImGuiCustom::drawText(drawList, playerData.activeWeapon.c_str(), {(bbox.min.x + bbox.max.x) * 0.5f, bbox.max.y}, config.weapon, config.weapon.outline, true, false);
		offsetMaxs.y += weaponTextSize.y;
	}

	drawSnapline(config.snapline, bbox.min + offsetMins, bbox.max + offsetMaxs);
}

static void renderWeaponBox(const WeaponData &weaponData, const Weapon &config) noexcept
{
	const BoundingBox bbox{weaponData, config.box.scale};

	if (!bbox)
		return;

	renderBox(bbox, config.box);
	drawSnapline(config.snapline, bbox.min, bbox.max);

	FontPush font{config.font.name};

	if (config.name.enabled && !weaponData.displayName.empty() && !cullText(weaponData.distanceToLocal, config.textCullDistance))
		ImGuiCustom::drawText(drawList, weaponData.displayName.c_str(), {(bbox.min.x + bbox.max.x) / 2, bbox.min.y}, config.name, config.name.outline);

	if (config.ammo.enabled && weaponData.clip != -1 && !cullText(weaponData.distanceToLocal, config.textCullDistance))
	{
		const auto text{std::to_string(weaponData.clip) + " / " + std::to_string(weaponData.reserveAmmo)};
		ImGuiCustom::drawText(drawList, text.c_str(), {(bbox.min.x + bbox.max.x) / 2, bbox.max.y}, config.ammo, config.ammo.outline, true, false);
	}
}

static void renderEntityBox(const BaseData &entityData, const char *name, const Shared &config) noexcept
{
	const BoundingBox bbox{entityData, config.box.scale};

	if (!bbox)
		return;

	renderBox(bbox, config.box);
	drawSnapline(config.snapline, bbox.min, bbox.max);

	FontPush font{config.font.name};

	if (config.name.enabled && !cullText(entityData.distanceToLocal, config.textCullDistance))
		ImGuiCustom::drawText(drawList, name, {(bbox.min.x + bbox.max.x) / 2, bbox.min.y - 5}, config.name, config.name.outline);
}

static void drawProjectileTrajectory(const Trail &config, const std::vector<std::pair<float, Vector>> &trajectory) noexcept
{
	if (!config.enabled)
		return;

	std::vector<ImVec2> points, shadowPoints;

	const auto color = Helpers::calculateColor(config);

	for (const auto &[time, point] : trajectory)
	{
		if (ImVec2 pos; time + config.time >= memory->globalVars->realTime && Helpers::worldToScreen(point, pos, false))
		{
			if (config.type == Trail::Line)
			{
				points.emplace_back(pos);
				shadowPoints.emplace_back(pos);
			} else if (config.type == Trail::Circles)
			{
				if (config.outline)
					drawList->AddCircle(pos, 4.0f - point.distTo(GameData::local().origin) / 700.0f, color & IM_COL32_A_MASK, 12, config.thickness + 2.0f);
				drawList->AddCircle(pos, 4.0f - point.distTo(GameData::local().origin) / 700.0f, color, 12, config.thickness);
			} else if (config.type == Trail::FilledCircles)
			{
				if (config.outline)
					drawList->AddCircleFilled(pos, 5.0f - point.distTo(GameData::local().origin) / 700.0f, color & IM_COL32_A_MASK);
				drawList->AddCircleFilled(pos, 4.0f - point.distTo(GameData::local().origin) / 700.0f, color);
			}
		}
	}

	if (config.type == Trail::Line)
	{
		if (config.outline)
			drawList->AddPolyline(shadowPoints.data(), shadowPoints.size(), color & IM_COL32_A_MASK, false, config.thickness + 2.0f);
		drawList->AddPolyline(points.data(), points.size(), color, false, config.thickness);
	}
}

static void drawPlayerSkeleton(const Color4OutlineToggleThickness &config, const PlayerData &playerData) noexcept
{
	if (!config.enabled)
		return;

	if (!playerData.inViewFrustum)
		return;

	const auto color = Helpers::calculateColor(config);

	std::vector<std::pair<ImVec2, ImVec2>> points, shadowPoints;

	for (const auto &[bone, parent] : playerData.bones)
	{
		ImVec2 bonePoint;
		if (!Helpers::worldToScreen(bone, bonePoint))
			continue;

		ImVec2 parentPoint;
		if (!Helpers::worldToScreen(parent, parentPoint))
			continue;

		points.emplace_back(bonePoint, parentPoint);
		shadowPoints.emplace_back(bonePoint, parentPoint);
	}

	if (config.outline)
		for (const auto &[bonePoint, parentPoint] : shadowPoints)
			drawList->AddLine(bonePoint, parentPoint, color & IM_COL32_A_MASK, config.thickness + 2.0f);

	for (const auto &[bonePoint, parentPoint] : points)
		drawList->AddLine(bonePoint, parentPoint, color, config.thickness);
}

static void drawOffscreen(const Color4OutlineToggle &config, const PlayerData &playerData) noexcept
{
	if (!config.enabled)
		return;

	if (playerData.inViewFrustum)
		return;

	const auto yaw = Helpers::degreesToRadians(interfaces->engine->getViewAngles().y);

	const auto positionDiff = GameData::local().origin - playerData.origin;

	const auto sin = std::sin(yaw);
	const auto cos = std::cos(yaw);
	ImVec2 pos = {cos * positionDiff.y - sin * positionDiff.x, cos * positionDiff.x + sin * positionDiff.y};
	const auto l = std::sqrtf(ImLengthSqr(pos));
	if (!l) return;
	pos /= l;

	const auto color = Helpers::calculateColor(config);
	ImGuiCustom::drawTriangleFromCenter(drawList, pos * 300, color, config.outline);
}

static void drawLineOfSight(const Color4ToggleThickness &config, const PlayerData &playerData)
{
	if (!config.enabled)
		return;

	const auto color = Helpers::calculateColor(config);

	ImVec2 start, end;
	
	bool draw = Helpers::worldToScreen((playerData.headMaxs + playerData.headMins) / 2, start);
	draw = draw && Helpers::worldToScreen(playerData.lookingAt, end);

	if (draw)
	{
		drawList->AddLine(start, end, color, config.thickness);
		drawList->AddCircle(end, 5.0f, color, 0, config.thickness);
	}
}

static void renderPlayerEsp(const PlayerData &playerData, const Player &playerConfig) noexcept
{
	if (!playerConfig.enabled)
		return;

	if (playerConfig.audibleOnly && !playerData.audible && !playerConfig.spottedOnly
		|| playerConfig.spottedOnly && !playerData.spotted && !(playerConfig.audibleOnly && playerData.audible)) // If both "Audible only" and "Spotted only" are on treat them as audible OR spotted
		return;

	if (playerData.dormant)
	{
		const float factor = 1.0f - (memory->globalVars->realTime - playerData.becameDormant) * 0.25f;

		if (factor <= 0.0f) return;

		Helpers::setAlphaFactor(factor);
	}

	renderPlayerBox(playerData, playerConfig);
	drawPlayerSkeleton(playerConfig.skeleton, playerData);

	if (const BoundingBox headBbox = {playerData.headMins, playerData.headMaxs, playerConfig.headBox.scale}; headBbox && playerData.inViewFrustum)
		renderBox(headBbox, playerConfig.headBox);

	drawLineOfSight(playerConfig.lineOfSight, playerData);
	drawOffscreen(playerConfig.offscreen, playerData);

	Helpers::setAlphaFactor(1);
	return;
}

static void renderWeaponEsp(const WeaponData &weaponData, const Weapon &parentConfig, const Weapon &itemConfig) noexcept
{
	const auto &config = itemConfig.enabled ? itemConfig : (parentConfig.enabled ? parentConfig : ::config->esp.weapons["All"]);
	if (config.enabled)
	{
		renderWeaponBox(weaponData, config);
	}
}

static void renderEntityEsp(const BaseData &entityData, const std::unordered_map<std::string, Shared> &map, const char *name) noexcept
{
	if (const auto cfg = map.find(name); cfg != map.cend() && cfg->second.enabled)
	{
		renderEntityBox(entityData, name, cfg->second);
	} else if (const auto cfg = map.find("All"); cfg != map.cend() && cfg->second.enabled)
	{
		renderEntityBox(entityData, name, cfg->second);
	}
}

static void renderProjectileEsp(const ProjectileData &projectileData, const Projectile &parentConfig, const Projectile &itemConfig, const char *name) noexcept
{
	const auto &config = itemConfig.enabled ? itemConfig : parentConfig;

	if (config.enabled)
	{
		if (!projectileData.exploded)
			renderEntityBox(projectileData, name, config);

		if (config.trails.enabled)
		{
			if (projectileData.thrownByLocalPlayer)
				drawProjectileTrajectory(config.trails.localPlayer, projectileData.trajectory);
			else if (!projectileData.thrownByEnemy)
				drawProjectileTrajectory(config.trails.allies, projectileData.trajectory);
			else
				drawProjectileTrajectory(config.trails.enemies, projectileData.trajectory);
		}
	}
}

void StreamProofESP::render() noexcept
{
	drawList = ImGui::GetBackgroundDrawList();

	GameData::Lock lock;

	for (const auto &weapon : GameData::weapons())
		renderWeaponEsp(weapon, config->esp.weapons[weapon.group], config->esp.weapons[weapon.name]);

	for (const auto &entity : GameData::entities())
		renderEntityEsp(entity, config->esp.otherEntities, entity.name);

	for (const auto &lootCrate : GameData::lootCrates())
	{
		if (lootCrate.name)
			renderEntityEsp(lootCrate, config->esp.lootCrates, lootCrate.name);
	}

	for (const auto &projectile : GameData::projectiles())
		renderProjectileEsp(projectile, config->esp.projectiles["All"], config->esp.projectiles[projectile.name], projectile.name);

	for (const auto &player : GameData::players())
	{
		if (!player.alive) continue;
		if (player.handle == GameData::local().observerTargetHandle) continue;

		auto &playerConfig = player.enemy ? config->esp.enemies : config->esp.allies;

		if (player.dormant && playerConfig["Dormant"].enabled)
			renderPlayerEsp(player, playerConfig["Dormant"]);
		else if (player.visible && playerConfig["Visible"].enabled)
			renderPlayerEsp(player, playerConfig["Visible"]);
		else if (!player.visible && playerConfig["Occluded"].enabled)
			renderPlayerEsp(player, playerConfig["Occluded"]);
		else
			renderPlayerEsp(player, playerConfig["All"]);
	}
}
