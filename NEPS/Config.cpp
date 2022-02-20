#include <fstream>

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#include <ShlObj.h>
#endif

#include "lib/nlohmann/json.hpp"

#include "Config.h"
#include "lib/Helpers.hpp"
#include "Gui.h"

#include "SDK/LocalPlayer.h"
#include "SDK/Entity.h"

#ifdef _WIN32
int CALLBACK fontCallback(const LOGFONTW *lpelfe, const TEXTMETRICW *, DWORD, LPARAM lParam)
{
	const wchar_t *const fontName = reinterpret_cast<const ENUMLOGFONTEXW *>(lpelfe)->elfFullName;

	if (fontName[0] == L'@')
		return TRUE;

	if (HFONT font = CreateFontW(0, 0, 0, 0,
		FW_NORMAL, FALSE, FALSE, FALSE,
		ANSI_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH, fontName))
	{

		DWORD fontData = GDI_ERROR;

		if (HDC hdc = CreateCompatibleDC(nullptr))
		{
			SelectObject(hdc, font);
			// Do not use TTC fonts as we only support TTF fonts
			fontData = GetFontData(hdc, 'fctt', 0, NULL, 0);
			DeleteDC(hdc);
		}
		DeleteObject(font);

		if (fontData == GDI_ERROR)
		{
			if (char buff[1024]; WideCharToMultiByte(CP_UTF8, 0, fontName, -1, buff, sizeof(buff), nullptr, nullptr))
				reinterpret_cast<std::vector<std::string> *>(lParam)->emplace_back(buff);
		}
	}
	return TRUE;
}
#endif

Config::Config(const char *name) noexcept
{
	#ifdef _WIN32
	if (PWSTR pathToDocuments; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &pathToDocuments)))
	{
		path = pathToDocuments;
		path /= name;
		CoTaskMemFree(pathToDocuments);
	}
	#endif

	listConfigs();

	#ifdef _WIN32
	LOGFONTW logfont;
	logfont.lfCharSet = ANSI_CHARSET;
	logfont.lfPitchAndFamily = DEFAULT_PITCH;
	logfont.lfFaceName[0] = L'\0';

	EnumFontFamiliesExW(GetDC(nullptr), &logfont, fontCallback, (LPARAM)&systemFonts, 0);
	#endif

	std::sort(std::next(systemFonts.begin()), systemFonts.end());
}

using json = nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int64_t, std::uint64_t, float>;
using value_t = json::value_t;

template <value_t Type, typename T>
static typename std::enable_if_t<!std::is_same_v<T, bool>> read(const json &j, const char *key, T &o) noexcept
{
	if (!j.contains(key))
		return;

	if (const auto &val = j[key]; val.type() == Type)
		val.get_to(o);
}

static void read(const json &j, const char *key, bool &o) noexcept
{
	if (!j.contains(key))
		return;

	if (const auto &val = j[key]; val.type() == value_t::boolean)
		val.get_to(o);
}

static void read(const json &j, const char *key, float &o) noexcept
{
	if (!j.contains(key))
		return;

	if (const auto &val = j[key]; val.type() == value_t::number_float)
		val.get_to(o);
}

static void read(const json &j, const char *key, int &o) noexcept
{
	if (!j.contains(key))
		return;

	if (const auto &val = j[key]; val.is_number_integer())
		val.get_to(o);
}

static void read(const json &j, const char *key, WeaponId &o) noexcept
{
	if (!j.contains(key))
		return;

	if (const auto &val = j[key]; val.is_number_integer())
		val.get_to(o);
}

template <typename T, size_t Size>
static void read_array_opt(const json &j, const char *key, std::array<T, Size> &o) noexcept
{
	if (j.contains(key) && j[key].type() == value_t::array)
	{
		std::size_t i = 0;
		for (const auto &e : j[key])
		{
			if (i >= o.size())
				break;

			if (e.is_null())
				continue;

			e.get_to(o[i]);
			++i;
		}
	}
}

template <typename T, size_t Size>
static void read(const json &j, const char *key, std::array<T, Size> &o) noexcept
{
	if (!j.contains(key))
		return;

	if (const auto &val = j[key]; val.type() == value_t::array && val.size() == o.size())
	{
		for (std::size_t i = 0; i < val.size(); ++i)
		{
			if (!val[i].empty())
				val[i].get_to(o[i]);
		}
	}
}

template <typename T>
static void read(const json &j, const char *key, std::unordered_map<std::string, T> &o) noexcept
{
	if (j.contains(key) && j[key].is_object())
	{
		for (auto &element : j[key].items())
			element.value().get_to(o[element.key()]);
	}
}

static void from_json(const json &j, KeyBind &k)
{
	read(j, "Key", k.key);
	read(j, "Key mode", k.keyMode);
}

static void from_json(const json &j, Color4 &c)
{
	read(j, "Color", c.color);
	read(j, "Rainbow", c.rainbow);
	read(j, "Rainbow Speed", c.rainbowSpeed);
}

static void from_json(const json &j, Color4Outline &co)
{
	from_json(j, static_cast<Color4 &>(co));
	read(j, "Border", co.outline);
}

static void from_json(const json &j, Color4OutlineToggle &cot)
{
	from_json(j, static_cast<Color4Outline &>(cot));
	read(j, "Enabled", cot.enabled);
}

static void from_json(const json &j, Color4OutlineToggleThickness &cott)
{
	from_json(j, static_cast<Color4OutlineToggle &>(cott));
	read(j, "Thickness", cott.thickness);
}

static void from_json(const json &j, Color4Toggle &ct)
{
	from_json(j, static_cast<Color4 &>(ct));
	read(j, "Enabled", ct.enabled);
}

static void from_json(const json &j, Color3 &c)
{
	read(j, "Color", c.color);
	read(j, "Rainbow", c.rainbow);
	read(j, "Rainbow Speed", c.rainbowSpeed);
}

static void from_json(const json &j, Color3Toggle &ct)
{
	from_json(j, static_cast<Color3 &>(ct));
	read(j, "Enabled", ct.enabled);
}

static void from_json(const json &j, Color4ToggleRounding &ctr)
{
	from_json(j, static_cast<Color4Toggle &>(ctr));
	read(j, "Rounding", ctr.rounding);
}

static void from_json(const json &j, Color4ToggleThickness &ctt)
{
	from_json(j, static_cast<Color4Toggle &>(ctt));
	read(j, "Thickness", ctt.thickness);
}

static void from_json(const json &j, Color4ToggleThicknessRounding &cttr)
{
	from_json(j, static_cast<Color4ToggleRounding &>(cttr));
	read(j, "Thickness", cttr.thickness);
}

static void from_json(const json &j, Font &f)
{
	read<value_t::string>(j, "Name", f.name);

	if (!f.name.empty())
		config->scheduleFontLoad(f.name);

	if (const auto it = std::find_if(config->getSystemFonts().begin(), config->getSystemFonts().end(), [&f](const auto &e) { return e == f.name; }); it != config->getSystemFonts().end())
		f.index = std::distance(config->getSystemFonts().begin(), it);
	else
		f.index = 0;
}

static void from_json(const json &j, Snapline &s)
{
	from_json(j, static_cast<Color4ToggleThickness &>(s));

	read(j, "Type", s.type);
}

static void from_json(const json &j, Box &b)
{
	from_json(j, static_cast<Color4ToggleRounding &>(b));

	read(j, "Type", b.type);
	read(j, "Scale", b.scale);
	read(j, "Secondary", b.secondary);
	read<value_t::object>(j, "Secondary color", b.secondaryColor);
}

static void from_json(const json &j, Shared &s)
{
	read(j, "Enabled", s.enabled);
	read<value_t::object>(j, "Font", s.font);
	read<value_t::object>(j, "Snapline", s.snapline);
	read<value_t::object>(j, "Box", s.box);
	read<value_t::object>(j, "Name", s.name);
	read(j, "Text Cull Distance", s.textCullDistance);
}

static void from_json(const json &j, Weapon &w)
{
	from_json(j, static_cast<Shared &>(w));

	read<value_t::object>(j, "Ammo", w.ammo);
}

static void from_json(const json &j, Trail &t)
{
	from_json(j, static_cast<Color4OutlineToggleThickness &>(t));

	read(j, "Type", t.type);
	read(j, "Time", t.time);
}

static void from_json(const json &j, Trails &t)
{
	read(j, "Enabled", t.enabled);
	read<value_t::object>(j, "Local Player", t.localPlayer);
	read<value_t::object>(j, "Allies", t.allies);
	read<value_t::object>(j, "Enemies", t.enemies);
}

static void from_json(const json &j, Projectile &p)
{
	from_json(j, static_cast<Shared &>(p));

	read<value_t::object>(j, "Trails", p.trails);
}

static void from_json(const json &j, Player &p)
{
	from_json(j, static_cast<Shared &>(p));

	read<value_t::object>(j, "Weapon", p.weapon);
	read<value_t::object>(j, "Flash Duration", p.flashDuration);
	read(j, "Audible Only", p.audibleOnly);
	read(j, "Spotted Only", p.spottedOnly);
	read<value_t::object>(j, "Health Bar", p.healthBar);
	read<value_t::object>(j, "Health", p.health);
	read<value_t::object>(j, "Skeleton", p.skeleton);
	read<value_t::object>(j, "Head Box", p.headBox);
	read<value_t::object>(j, "Flags", p.flags);
	read<value_t::object>(j, "Offscreen", p.offscreen);
	read<value_t::object>(j, "Looking at", p.lineOfSight);
}

static void from_json(const json &j, ImVec2 &v)
{
	read(j, "X", v.x);
	read(j, "Y", v.y);
}

static void from_json(const json &j, Config::Aimbot::AimbotOverride &v)
{
	read<value_t::object>(j, "Bind", v.bind);
	read(j, "Multipoint scale", v.multipointScale);
	read(j, "Targeting", v.targeting);
	read(j, "Hitgroup", v.hitGroup);
	read(j, "Fov", v.fov);
	read(j, "Distance", v.distance);
	read(j, "Hitchance", v.hitchance);
	read(j, "Min damage", v.minDamage);
	read(j, "Min damage auto-wall", v.minDamageAutoWall);
}

static void from_json(const json &j, Config::Aimbot &a)
{
	read<value_t::object>(j, "Bind", a.bind);
	read(j, "Aimlock", a.aimlock);
	read(j, "Multipoint", a.multipoint);
	read(j, "Multipoint scale", a.multipointScale);
	read(j, "Silent", a.silent);
	read(j, "Friendly fire", a.friendlyFire);
	read(j, "Visible only", a.visibleOnly);
	read(j, "Scoped only", a.scopedOnly);
	read(j, "Ignore flash", a.ignoreFlash);
	read(j, "Ignore smoke", a.ignoreSmoke);
	read(j, "Auto shot", a.autoShoot);
	read(j, "Auto scope", a.autoScope);
	read(j, "Auto stop", a.autoStop);
	read(j, "Targeting", a.targeting);
	read(j, "Hitgroup", a.hitGroup);
	read(j, "Fov", a.fov);
	read(j, "Distance", a.distance);
	read(j, "Hitchance", a.hitchance);
	read(j, "Min damage", a.minDamage);
	read(j, "Min damage auto-wall", a.minDamageAutoWall);
	read(j, "Humanize", a.humanize);
	read(j, "Acceleration", a.acceleration);
	read(j, "Friction", a.friction);
	read<value_t::object>(j, "Override", a.aimbotOverride);
	read(j, "Recoil reduction H", a.recoilReductionH);
	read(j, "Recoil reduction V", a.recoilReductionV);
	read(j, "Between shots", a.betweenShots);
}

static void from_json(const json &j, Config::Triggerbot &t)
{
	read<value_t::object>(j, "Bind", t.bind);
	read(j, "Friendly fire", t.friendlyFire);
	read(j, "Visible only", t.visibleOnly);
	read(j, "Scoped only", t.scopedOnly);
	read(j, "Ignore flash", t.ignoreFlash);
	read(j, "Ignore smoke", t.ignoreSmoke);
	read(j, "Hitgroup", t.hitGroup);
	read(j, "Hitchance", t.hitchance);
	read(j, "Distance", t.distance);
	read(j, "Shot delay", t.shotDelay);
	read(j, "Min damage", t.minDamage);
	read(j, "Min damage auto-wall", t.minDamageAutoWall);
	read(j, "Burst Time", t.burstTime);
}

static void from_json(const json &j, Config::Backtrack &b)
{
	read(j, "Enabled", b.enabled);
	read(j, "Ignore smoke", b.ignoreSmoke);
	read(j, "Time limit", b.timeLimit);
}

static void from_json(const json &j, Config::AntiAim &a)
{
	read(j, "Pitch", a.pitch);
	read(j, "Pitch angle", a.pitchAngle);
	read(j, "Yaw", a.yaw);
	read(j, "Yaw angle", a.yawAngle);
	read(j, "Look at enemies", a.lookAtEnemies);
	read(j, "Auto direction", a.direction);
	read(j, "Right key", a.rightKey);
	read(j, "Back key", a.backKey);
	read(j, "Left key", a.leftKey);
	read<value_t::object>(j, "Visualize direction", a.visualizeDirection);
	read(j, "Desync", a.desync);
	read(j, "Fake up", a.fakeUp);
	read(j, "Flip key", a.flipKey);
	read<value_t::object>(j, "Visualize side", a.visualizeSide);
	read<value_t::object>(j, "Choke", a.choke);
	read(j, "Choked packets", a.chokedPackets);
}

static void from_json(const json &j, Config::Glow &g)
{
	from_json(j, static_cast<Color4 &>(g));

	read(j, "Enabled", g.enabled);
	read(j, "Health based", g.healthBased);
	read(j, "Style", g.style);
	read(j, "Full bloom", g.full);
}

static void from_json(const json &j, Config::Chams::Material &m)
{
	from_json(j, static_cast<Color4 &>(m));

	read(j, "Enabled", m.enabled);
	read(j, "Health based", m.healthBased);
	read(j, "Blinking", m.blinking);
	read(j, "Wireframe", m.wireframe);
	read(j, "Cover", m.cover);
	read(j, "Ignore-Z", m.ignoreZ);
	read(j, "Material", m.material);
}

static void from_json(const json &j, Config::Chams &c)
{
	read_array_opt(j, "Materials", c.materials);
}

static void from_json(const json &j, Config::ESP &e)
{
	read(j, "Allies", e.allies);
	read(j, "Enemies", e.enemies);
	read(j, "Weapons", e.weapons);
	read(j, "Projectiles", e.projectiles);
	read(j, "Loot Crates", e.lootCrates);
	read(j, "Other Entities", e.otherEntities);
}

static void from_json(const json &j, Config::Visuals::ColorCorrection &c)
{
	read(j, "Enabled", c.enabled);
	read(j, "Blue", c.blue);
	read(j, "Red", c.red);
	read(j, "Mono", c.mono);
	read(j, "Saturation", c.saturation);
	read(j, "Ghost", c.ghost);
	read(j, "Green", c.green);
	read(j, "Yellow", c.yellow);
}

static void from_json(const json &j, Config::Visuals::Viewmodel &vxyz)
{
	read(j, "Enabled", vxyz.enabled);
	read(j, "Fov", vxyz.fov);
	read(j, "X", vxyz.x);
	read(j, "Y", vxyz.y);
	read(j, "Z", vxyz.z);
	read(j, "Roll", vxyz.roll);
}

static void from_json(const json &j, Config::Visuals::Beams &b)
{
	read(j, "Enabled", b.enabled);
	read(j, "Sprite", b.sprite);
	read(j, "Color", b.color);
	read(j, "Width", b.width);
	read(j, "Life", b.life);
	read(j, "Type", b.type);
	read(j, "Noise", b.amplitude);
	read(j, "Noise once", b.noiseOnce);
}

static void from_json(const json &j, Config::Visuals::Dlights &b)
{
	read(j, "Enabled", b.enabled);
	read(j, "Color", b.color);
	read(j, "Radius", b.radius);
}

static void from_json(const json &j, Config::Visuals &v)
{
	read(j, "Disable post-processing", v.disablePostProcessing);
	read(j, "Inverse ragdoll gravity", v.inverseRagdollGravity);
	read(j, "No fog", v.noFog);
	read(j, "No 3d sky", v.no3dSky);
	read(j, "No aim punch", v.noAimPunch);
	read(j, "No view punch", v.noViewPunch);
	read(j, "No hands", v.noHands);
	read(j, "No sleeves", v.noSleeves);
	read(j, "No weapons", v.noWeapons);
	read(j, "No smoke", v.smoke);
	read(j, "No fire", v.inferno);
	read(j, "No blur", v.noBlur);
	read(j, "No scope overlay", v.noScopeOverlay);
	read(j, "No grass", v.noGrass);
	read(j, "No shadows", v.noShadows);
	read<value_t::object>(j, "Viewmodel", v.viewmodel);
	read<value_t::object>(j, "Zoom", v.zoom);
	read(j, "Zoom factor", v.zoomFac);
	read<value_t::object>(j, "Thirdperson", v.thirdPerson);
	read(j, "Thirdperson distance", v.thirdpersonDistance);
	read(j, "Thirdperson collision", v.thirdpersonCollision);
	read<value_t::object>(j, "Flashlight", v.flashlight);
	read(j, "Flashlight brightness", v.flashlightBrightness);
	read(j, "Flashlight distance", v.flashlightDistance);
	read(j, "Flashlight fov", v.flashlightFov);
	read(j, "FOV", v.fov);
	read(j, "Force keep FOV", v.forceFov);
	read(j, "Far Z", v.farZ);
	read(j, "Flash reduction", v.flashReduction);
	read(j, "Brightness", v.brightness);
	read(j, "Skybox", v.skybox);
	read<value_t::object>(j, "World", v.world);
	read<value_t::object>(j, "Props", v.props);
	read<value_t::object>(j, "Sky", v.sky);
	read(j, "Deagle spinner", v.deagleSpinner);
	read(j, "Screen effect", v.screenEffect);
	read(j, "Hit effect", v.hitEffect);
	read(j, "Hit effect time", v.hitEffectTime);
	read(j, "Kill effect", v.killEffect);
	read(j, "Kill effect time", v.killEffectTime);
	read(j, "Hit marker", v.hitMarker);
	read(j, "Hit marker time", v.hitMarkerTime);
	read(j, "Playermodel T", v.playerModelT);
	read(j, "Playermodel CT", v.playerModelCT);
	read<value_t::object>(j, "Color correction", v.colorCorrection);
	read(j, "Aspect ratio", v.aspectratio);
	read(j, "Opposite hand knife", v.oppositeHandKnife);
	read(j, "Bullet impacts", v.bulletImpacts);
	read(j, "Accuracy tracers", v.accuracyTracers);
	read<value_t::object>(j, "Beams self", v.selfBeams);
	read<value_t::object>(j, "Beams ally", v.allyBeams);
	read<value_t::object>(j, "Beams enemy", v.enemyBeams);
	read<value_t::object>(j, "Dlights self", v.selfDlights);
	read<value_t::object>(j, "Dlights ally", v.allyDlights);
	read<value_t::object>(j, "Dlights enemy", v.enemyDlights);
	read<value_t::object>(j, "Inferno hull", v.molotovHull);
	read<value_t::object>(j, "Smoke hull", v.smokeHull);
	read<value_t::object>(j, "Player bounds", v.playerBounds);
	read<value_t::object>(j, "Player velocity", v.playerVelocity);
	read(j, "Noscope crosshair type", v.overlayCrosshairType);
	read<value_t::object>(j, "Noscope crosshair", v.overlayCrosshair);
	read(j, "Recoil crosshair type", v.recoilCrosshairType);
	read<value_t::object>(j, "Recoil crosshair", v.recoilCrosshair);
	read<value_t::object>(j, "Inaccuracy circle", v.accuracyCircle);
	read(j, "Force crosshair", v.forceCrosshair);
}

static void from_json(const json &j, sticker_setting &s)
{
	read(j, "Kit", s.kit);
	read(j, "Wear", s.wear);
	read(j, "Scale", s.scale);
	read(j, "Rotation", s.rotation);

	s.onLoad();
}

static void from_json(const json &j, item_setting &i)
{
	read(j, "Enabled", i.enabled);
	read(j, "Definition index", i.itemId);
	read(j, "Quality", i.quality);
	read(j, "Paint Kit", i.paintKit);
	read(j, "Definition override", i.definition_override_index);
	read(j, "Seed", i.seed);
	read(j, "StatTrak", i.stat_trak);
	read(j, "Wear", i.wear);
	if (j.contains("Custom name"))
		strncpy_s(i.custom_name, j["Custom name"].get<std::string>().c_str(), _TRUNCATE);
	read(j, "Stickers", i.stickers);

	i.onLoad();
}

static void from_json(const json &j, Config::Sound::Player &p)
{
	read(j, "Master volume", p.masterVolume);
	read(j, "Headshot volume", p.headshotVolume);
	read(j, "Weapon volume", p.weaponVolume);
	read(j, "Footstep volume", p.footstepVolume);
}

static void from_json(const json &j, Config::Sound &s)
{
	read(j, "Chicken volume", s.chickenVolume);
	read(j, "Players", s.players);
	read(j, "Hit sound", s.hitSound);
	read(j, "Kill sound", s.killSound);
	read(j, "Death sound", s.deathSound);
	read(j, "Hit sound volume", s.hitSoundVol);
	read(j, "Kill sound volume", s.killSoundVol);
	read(j, "Death sound volume", s.deathSoundVol);
	read<value_t::string>(j, "Custom hit sound", s.customHitSound);
	read<value_t::string>(j, "Custom kill sound", s.customKillSound);
	read<value_t::string>(j, "Custom death sound", s.customDeathSound);
}

static void from_json(const json &j, Config::Style &s)
{
	read(j, "Menu style", s.menuStyle);
	read(j, "Menu colors", s.menuColors);

	if (j.contains("Colors") && j["Colors"].is_object())
	{
		const auto &colors = j["Colors"];

		ImGuiStyle &style = ImGui::GetStyle();

		for (int i = 0; i < ImGuiCol_COUNT; ++i)
		{
			if (const char *name = ImGui::GetStyleColorName(i); colors.contains(name))
			{
				std::array<float, 4> temp;
				read(colors, name, temp);
				style.Colors[i].x = temp[0];
				style.Colors[i].y = temp[1];
				style.Colors[i].z = temp[2];
				style.Colors[i].w = temp[3];
			}
		}
	}
}

static void from_json(const json &j, Config::Misc::PurchaseList &pl)
{
	read(j, "Enabled", pl.enabled);
	read(j, "Only During Freeze Time", pl.onlyDuringFreezeTime);
	read(j, "Show Prices", pl.showPrices);
	read(j, "No Title Bar", pl.noTitleBar);
	read(j, "Mode", pl.mode);
}

static void from_json(const json &j, Config::Misc::PreserveKillfeed &o)
{
	read(j, "Enabled", o.enabled);
	read(j, "Only Headshots", o.onlyHeadshots);
}

static void from_json(const json &j, Config::Misc::Watermark &w)
{
	read(j, "Enabled", w.enabled);
	read(j, "Position", w.position);
}

static void from_json(const json &j, Config::Misc::BombTimer &bt)
{
	read(j, "Enabled", bt.enabled);
}

static void from_json(const json &j, Config::Misc::Indicators &i)
{
	read(j, "Enabled", i.enabled);
}

static void from_json(const json &j, Config::Misc::SpectatorList &sl)
{
	read(j, "Enabled", sl.enabled);
}

static void from_json(const json &j, Config::Misc::TeamDamageList &tdl)
{
	read(j, "Enabled", tdl.enabled);
	read(j, "No Title Bar", tdl.noTitleBar);
	read(j, "Progress Bars", tdl.progressBars);
}

static void from_json(const json &j, Config::Misc &m)
{
	read(j, "Menu key", m.menuKey);
	read(j, "Auto pistol", m.autoPistol);
	read(j, "Auto reload", m.autoReload);
	read(j, "Auto accept", m.autoAccept);
	read(j, "Fix animation LOD", m.fixAnimationLOD);
	read(j, "Fix bone matrix", m.fixBoneMatrices);
	read(j, "Fix movement", m.fixMovement);
	read(j, "Fix animations", m.fixLocalAnimations);
	read(j, "Disable model occlusion", m.disableModelOcclusion);
	read(j, "Disable extrapolation", m.noExtrapolate);
	read(j, "Disable IK", m.disableIK);
	read(j, "Resolve LBY", m.resolveLby);
	read(j, "Unlock inventory", m.unlockInventory);
	read(j, "Disable HUD blur", m.disablePanoramablur);
	read<value_t::object>(j, "Prepare revolver", m.prepareRevolver);
	read(j, "Quick healthshot key", m.quickHealthshotKey);
	read(j, "Radar hack", m.radarHack);
	read(j, "Reveal ranks", m.revealRanks);
	read(j, "Reveal money", m.revealMoney);
	read(j, "Reveal suspect", m.revealSuspect);
	read(j, "Fast plant", m.fastPlant);
	read(j, "Quick reload", m.quickReload);
	read(j, "Fix tablet signal", m.fixTabletSignal);
	read(j, "Grenade predict", m.nadePredict);
	read(j, "Force relay cluster", m.forceRelayCluster);
	read(j, "NEPSmas", m.goFestive);
	read(j, "Aimstep", m.maxAngleDelta);
	read<value_t::object>(j, "Preserve killfeed", m.preserveKillfeed);
	read<value_t::object>(j, "Purchase list", m.purchaseList);
	read<value_t::object>(j, "Spectator list", m.spectatorList);
	read<value_t::object>(j, "Bomb timer", m.bombTimer);
	read<value_t::object>(j, "Watermark", m.watermark);
	read<value_t::object>(j, "Indicators", m.indicators);
	read<value_t::object>(j, "Team damage list", m.teamDamageList);
}

static void from_json(const json &j, Config::Exploits &e)
{
	read(j, "Anti AFK kick", e.antiAfkKick);
	read(j, "Fast duck", e.fastDuck);
	read<value_t::object>(j, "Fake duck", e.fakeDuck);
	read(j, "Fake duck packets", e.fakeDuckPackets);
	read(j, "Moonwalk", e.moonwalk);
	read<value_t::object>(j, "Slowwalk", e.slowwalk);
	read(j, "Bypass sv_pure", e.bypassPure);
}

static void from_json(const json &j, Config::Griefing &g)
{
	read(j, "Custom clantag", g.customClanTag);
	read(j, "Clock tag", g.clocktag);
	if (j.contains("Clantag"))
		strncpy_s(g.clanTag, j["Clantag"].get<std::string>().c_str(), _TRUNCATE);
	read(j, "Animated clantag", g.animatedClanTag);
	read(j, "Kill message", g.killMessage);
	read<value_t::string>(j, "Kill message string", g.killMessageString);
	read(j, "Name stealer", g.nameStealer);
	read(j, "Fake Prime", g.fakePrime);
	read(j, "Ban color", g.banColor);
	read<value_t::string>(j, "Ban text", g.banText);
	read<value_t::object>(j, "Reportbot", g.reportbot);
	read<value_t::object>(j, "Blockbot", g.blockbot);
	read(j, "Vote reveal", g.revealVotes);
	read(j, "Spam use", g.spamUse);
	read<value_t::object>(j, "Nuke chat", g.chatNuke);
	read<value_t::object>(j, "Basmala chat", g.chatBasmala);
}

static void from_json(const json &j, Config::Griefing::Reportbot &r)
{
	read(j, "Enabled", r.enabled);
	read(j, "Target", r.target);
	read(j, "Delay", r.delay);
	read(j, "Rounds", r.rounds);
	read(j, "Abusive Communications", r.textAbuse);
	read(j, "Griefing", r.griefing);
	read(j, "Wall Hacking", r.wallhack);
	read(j, "Aim Hacking", r.aimbot);
	read(j, "Other Hacking", r.other);
}

static void from_json(const json &j, Config::Griefing::Blockbot &b)
{
	read<value_t::object>(j, "Bind", b.bind);
	read(j, "Trajectory factor", b.trajectoryFac);
	read(j, "Distance factor", b.distanceFac);
	read<value_t::object>(j, "Visualise", b.visualize);
}

static void from_json(const json &j, Config::Movement &m)
{
	read(j, "Bunny hop", m.bunnyHop);
	read(j, "Auto strafe", m.autoStrafe);
	read<value_t::object>(j, "Edge jump", m.edgeJump);
	read(j, "Fast stop", m.fastStop);
	read<value_t::object>(j, "Quick peek", m.autoPeek);
}

static void from_json(const json &j, Config::Movement::QuickPeek &qp)
{
	read<value_t::object>(j, "Bind", qp.bind);
	read<value_t::object>(j, "Visualize idle", qp.visualizeIdle);
	read<value_t::object>(j, "Visualize active", qp.visualizeActive);
}

bool Config::load(const char8_t *name, bool incremental) noexcept
{
	json j;

	if (std::ifstream in{path / name}; in.good())
	{
		j = json::parse(in, nullptr, false);
		if (j.is_discarded())
			return false;
	} else return false;

	if (!incremental)
		reset();

	read(j, "Aimbot", aimbot);
	read(j, "Triggerbot", triggerbot);
	read<value_t::object>(j, "Backtrack", backtrack);
	read(j, "Anti aim", antiAim);
	read(j, "Glow", glow);
	read(j, "Chams", chams);
	read<value_t::object>(j, "ESP", esp);
	read<value_t::object>(j, "Visuals", visuals);
	read(j, "Skin changer", skinChanger);
	read<value_t::object>(j, "Sound", sound);
	read<value_t::object>(j, "Style", style);
	read<value_t::object>(j, "Misc", misc);
	read<value_t::object>(j, "Exploits", exploits);
	read<value_t::object>(j, "Movement", movement);
	read<value_t::object>(j, "Griefing", griefing);

	return true;
}

bool Config::load(size_t id, bool incremental) noexcept
{
	return load((const char8_t *)configs[id].c_str(), incremental);
}

// WRITE macro requires:
// - json object named 'j'
// - object holding default values named 'dummy'
// - object to write to json named 'o'
#define WRITE(name, valueName) to_json(j[name], o.valueName, dummy.valueName)

template <typename T>
static void to_json(json &j, const T &o, const T &dummy)
{
	if (o != dummy)
		j = o;
}

static void to_json(json &j, const KeyBind &o, const KeyBind &dummy = {})
{
	WRITE("Key", key);
	WRITE("Key mode", keyMode);
}

static void to_json(json &j, const Color4 &o, const Color4 &dummy = {})
{
	WRITE("Color", color);
	WRITE("Rainbow", rainbow);
	WRITE("Rainbow Speed", rainbowSpeed);
}

static void to_json(json &j, const Color4Outline &o, const Color4Outline &dummy = {})
{
	to_json(j, static_cast<const Color4 &>(o), dummy);
	WRITE("Border", outline);
}

static void to_json(json &j, const Color4OutlineToggle &o, const Color4OutlineToggle &dummy = {})
{
	to_json(j, static_cast<const Color4Outline &>(o), dummy);
	WRITE("Enabled", enabled);
}

static void to_json(json &j, const Color4OutlineToggleThickness &o, const Color4OutlineToggleThickness &dummy = {})
{
	to_json(j, static_cast<const Color4OutlineToggle &>(o), dummy);
	WRITE("Thickness", thickness);
}

static void to_json(json &j, const Color4Toggle &o, const Color4Toggle &dummy = {})
{
	to_json(j, static_cast<const Color4 &>(o), dummy);
	WRITE("Enabled", enabled);
}

static void to_json(json &j, const Color3 &o, const Color3 &dummy = {})
{
	WRITE("Color", color);
	WRITE("Rainbow", rainbow);
	WRITE("Rainbow Speed", rainbowSpeed);
}

static void to_json(json &j, const Color3Toggle &o, const Color3Toggle &dummy = {})
{
	to_json(j, static_cast<const Color3 &>(o), dummy);
	WRITE("Enabled", enabled);
}

static void to_json(json &j, const Color4ToggleRounding &o, const Color4ToggleRounding &dummy = {})
{
	to_json(j, static_cast<const Color4Toggle &>(o), dummy);
	WRITE("Rounding", rounding);
}

static void to_json(json &j, const Color4ToggleThickness &o, const Color4ToggleThickness &dummy = {})
{
	to_json(j, static_cast<const Color4Toggle &>(o), dummy);
	WRITE("Thickness", thickness);
}

static void to_json(json &j, const Color4ToggleThicknessRounding &o, const Color4ToggleThicknessRounding &dummy = {})
{
	to_json(j, static_cast<const Color4ToggleRounding &>(o), dummy);
	WRITE("Thickness", thickness);
}

static void to_json(json &j, const Font &o, const Font &dummy = {})
{
	WRITE("Name", name);
}

static void to_json(json &j, const Snapline &o, const Snapline &dummy = {})
{
	to_json(j, static_cast<const Color4ToggleThickness &>(o), dummy);
	WRITE("Type", type);
}

static void to_json(json &j, const Box &o, const Box &dummy = {})
{
	to_json(j, static_cast<const Color4ToggleRounding &>(o), dummy);
	WRITE("Type", type);
	WRITE("Scale", scale);
	WRITE("Secondary", secondary);
	WRITE("Secondary color", secondaryColor);
}

static void to_json(json &j, const Shared &o, const Shared &dummy = {})
{
	WRITE("Enabled", enabled);
	WRITE("Font", font);
	WRITE("Snapline", snapline);
	WRITE("Box", box);
	WRITE("Name", name);
	WRITE("Text Cull Distance", textCullDistance);
}

static void to_json(json &j, const Player &o, const Player &dummy = {})
{
	to_json(j, static_cast<const Shared &>(o), dummy);
	WRITE("Weapon", weapon);
	WRITE("Flash Duration", flashDuration);
	WRITE("Audible Only", audibleOnly);
	WRITE("Spotted Only", spottedOnly);
	WRITE("Health Bar", healthBar);
	WRITE("Health", health);
	WRITE("Skeleton", skeleton);
	WRITE("Head Box", headBox);
	WRITE("Flags", flags);
	WRITE("Offscreen", offscreen);
	WRITE("Looking at", lineOfSight);
}

static void to_json(json &j, const Weapon &o, const Weapon &dummy = {})
{
	to_json(j, static_cast<const Shared &>(o), dummy);
	WRITE("Ammo", ammo);
}

static void to_json(json &j, const Trail &o, const Trail &dummy = {})
{
	to_json(j, static_cast<const Color4OutlineToggleThickness &>(o), dummy);
	WRITE("Type", type);
	WRITE("Time", time);
}

static void to_json(json &j, const Trails &o, const Trails &dummy = {})
{
	WRITE("Enabled", enabled);
	WRITE("Local Player", localPlayer);
	WRITE("Allies", allies);
	WRITE("Enemies", enemies);
}

static void to_json(json &j, const Projectile &o, const Projectile &dummy = {})
{
	j = static_cast<const Shared &>(o);

	WRITE("Trails", trails);
}

static void to_json(json &j, const ImVec2 &o, const ImVec2 &dummy = {})
{
	WRITE("X", x);
	WRITE("Y", y);
}

static void to_json(json &j, const Config::Aimbot::AimbotOverride &o, const Config::Aimbot::AimbotOverride &dummy = {})
{
	WRITE("Bind", bind);
	WRITE("Multipoint scale", multipointScale);
	WRITE("Targeting", targeting);
	WRITE("Hitgroup", hitGroup);
	WRITE("Fov", fov);
	WRITE("Distance", distance);
	WRITE("Hitchance", hitchance);
	WRITE("Min damage", minDamage);
	WRITE("Min damage auto-wall", minDamageAutoWall);
}

static void to_json(json &j, const Config::Aimbot &o, const Config::Aimbot &dummy = {})
{
	WRITE("Bind", bind);
	WRITE("Aimlock", aimlock);
	WRITE("Multipoint", multipoint);
	WRITE("Multipoint scale", multipointScale);
	WRITE("Silent", silent);
	WRITE("Friendly fire", friendlyFire);
	WRITE("Visible only", visibleOnly);
	WRITE("Scoped only", scopedOnly);
	WRITE("Ignore flash", ignoreFlash);
	WRITE("Ignore smoke", ignoreSmoke);
	WRITE("Auto shot", autoShoot);
	WRITE("Auto scope", autoScope);
	WRITE("Auto stop", autoStop);
	WRITE("Targeting", targeting);
	WRITE("Hitgroup", hitGroup);
	WRITE("Fov", fov);
	WRITE("Distance", distance);
	WRITE("Hitchance", hitchance);
	WRITE("Min damage", minDamage);
	WRITE("Min damage auto-wall", minDamageAutoWall);
	WRITE("Humanize", humanize);
	WRITE("Acceleration", acceleration);
	WRITE("Friction", friction);
	WRITE("Override", aimbotOverride);
	WRITE("Recoil reduction H", recoilReductionH);
	WRITE("Recoil reduction V", recoilReductionV);
	WRITE("Between shots", betweenShots);
}

static void to_json(json &j, const Config::Triggerbot &o, const Config::Triggerbot &dummy = {})
{
	WRITE("Bind", bind);
	WRITE("Friendly fire", friendlyFire);
	WRITE("Visible only", visibleOnly);
	WRITE("Scoped only", scopedOnly);
	WRITE("Ignore flash", ignoreFlash);
	WRITE("Ignore smoke", ignoreSmoke);
	WRITE("Hitgroup", hitGroup);
	WRITE("Hitchance", hitchance);
	WRITE("Distance", distance);
	WRITE("Shot delay", shotDelay);
	WRITE("Min damage", minDamage);
	WRITE("Min damage auto-wall", minDamageAutoWall);
	WRITE("Burst Time", burstTime);
}

static void to_json(json &j, const Config::Backtrack &o, const Config::Backtrack &dummy = {})
{
	WRITE("Enabled", enabled);
	WRITE("Ignore smoke", ignoreSmoke);
	WRITE("Time limit", timeLimit);
}

static void to_json(json &j, const Config::AntiAim &o, const Config::AntiAim &dummy = {})
{
	WRITE("Pitch", pitch);
	WRITE("Pitch angle", pitchAngle);
	WRITE("Yaw", yaw);
	WRITE("Yaw angle", yawAngle);
	WRITE("Look at enemies", lookAtEnemies);
	WRITE("Auto direction", direction);
	WRITE("Right key", rightKey);
	WRITE("Back key", backKey);
	WRITE("Left key", leftKey);
	WRITE("Visualize direction", visualizeDirection);
	WRITE("Desync", desync);
	WRITE("Fake up", fakeUp);
	WRITE("Flip key", flipKey);
	WRITE("Visualize side", visualizeSide);
	WRITE("Choke", choke);
	WRITE("Choked packets", chokedPackets);
}

static void to_json(json &j, const Config::Glow &o, const Config::Glow &dummy = {})
{
	to_json(j, static_cast<const Color4 &>(o), dummy);
	WRITE("Enabled", enabled);
	WRITE("Health based", healthBased);
	WRITE("Style", style);
	WRITE("Full bloom", full);
}

static void to_json(json &j, const Config::Chams::Material &o)
{
	const Config::Chams::Material dummy;

	to_json(j, static_cast<const Color4 &>(o), dummy);
	WRITE("Enabled", enabled);
	WRITE("Health based", healthBased);
	WRITE("Blinking", blinking);
	WRITE("Wireframe", wireframe);
	WRITE("Cover", cover);
	WRITE("Ignore-Z", ignoreZ);
	WRITE("Material", material);
}

static void to_json(json &j, const Config::Chams &o)
{
	j["Materials"] = o.materials;
}

static void to_json(json &j, const Config::ESP &o)
{
	j["Allies"] = o.allies;
	j["Enemies"] = o.enemies;
	j["Weapons"] = o.weapons;
	j["Projectiles"] = o.projectiles;
	j["Loot Crates"] = o.lootCrates;
	j["Other Entities"] = o.otherEntities;
}

static void to_json(json &j, const Config::Sound::Player &o)
{
	const Config::Sound::Player dummy;

	WRITE("Master volume", masterVolume);
	WRITE("Headshot volume", headshotVolume);
	WRITE("Weapon volume", weaponVolume);
	WRITE("Footstep volume", footstepVolume);
}

static void to_json(json &j, const Config::Sound &o)
{
	const Config::Sound dummy;

	WRITE("Chicken volume", chickenVolume);
	j["Players"] = o.players;
	WRITE("Hit sound", hitSound);
	WRITE("Kill sound", killSound);
	WRITE("Death sound", deathSound);
	WRITE("Hit sound volume", hitSoundVol);
	WRITE("Kill sound volume", killSoundVol);
	WRITE("Death sound volume", deathSoundVol);
	WRITE("Custom hit sound", customHitSound);
	WRITE("Custom kill sound", customKillSound);
	WRITE("Custom death sound", customDeathSound);
}

static void to_json(json &j, const Config::Misc::PurchaseList &o, const Config::Misc::PurchaseList &dummy = {})
{
	WRITE("Enabled", enabled);
	WRITE("Only During Freeze Time", onlyDuringFreezeTime);
	WRITE("Show Prices", showPrices);
	WRITE("No Title Bar", noTitleBar);
	WRITE("Mode", mode);
}

static void to_json(json &j, const Config::Misc::PreserveKillfeed &o, const Config::Misc::PreserveKillfeed &dummy = {})
{
	WRITE("Enabled", enabled);
	WRITE("Only Headshots", onlyHeadshots);
}

static void to_json(json &j, const Config::Misc::Watermark &o, const Config::Misc::Watermark &dummy = {})
{
	WRITE("Enabled", enabled);
	WRITE("Position", position);
}

static void to_json(json &j, const Config::Misc::BombTimer &o, const Config::Misc::BombTimer &dummy = {})
{
	WRITE("Enabled", enabled);
}

static void to_json(json &j, const Config::Misc::Indicators &o, const Config::Misc::Indicators &dummy = {})
{
	WRITE("Enabled", enabled);
}

static void to_json(json &j, const Config::Misc::SpectatorList &o, const Config::Misc::SpectatorList &dummy = {})
{
	WRITE("Enabled", enabled);
}

static void to_json(json &j, const Config::Misc::TeamDamageList &o, const  Config::Misc::TeamDamageList &dummy = {})
{
	WRITE("Enabled", enabled);
	WRITE("No Title Bar", noTitleBar);
	WRITE("Progress Bars", progressBars);
}

static void to_json(json &j, const Config::Misc &o)
{
	const Config::Misc dummy = {};

	WRITE("Menu key", menuKey);
	WRITE("Auto pistol", autoPistol);
	WRITE("Auto reload", autoReload);
	WRITE("Auto accept", autoAccept);
	WRITE("Fix animation LOD", fixAnimationLOD);
	WRITE("Fix bone matrix", fixBoneMatrices);
	WRITE("Fix movement", fixMovement);
	WRITE("Fix animations", fixLocalAnimations);
	WRITE("Disable model occlusion", disableModelOcclusion);
	WRITE("Disable extrapolation", noExtrapolate);
	WRITE("Disable IK", disableIK);
	WRITE("Resolve LBY", resolveLby);
	WRITE("Unlock inventory", unlockInventory);
	WRITE("Disable HUD blur", disablePanoramablur);
	WRITE("Prepare revolver", prepareRevolver);
	WRITE("Quick healthshot key", quickHealthshotKey);
	WRITE("Reveal ranks", revealRanks);
	WRITE("Reveal money", revealMoney);
	WRITE("Reveal suspect", revealSuspect);
	WRITE("Radar hack", radarHack);
	WRITE("Fast plant", fastPlant);
	WRITE("Quick reload", quickReload);
	WRITE("Fix tablet signal", fixTabletSignal);
	WRITE("Grenade predict", nadePredict);
	WRITE("Force relay cluster", forceRelayCluster);
	WRITE("NEPSmas", goFestive);
	WRITE("Aimstep", maxAngleDelta);
	WRITE("Preserve killfeed", preserveKillfeed);
	WRITE("Purchase list", purchaseList);
	WRITE("Spectator list", spectatorList);
	WRITE("Bomb timer", bombTimer);
	WRITE("Watermark", watermark);
	WRITE("Indicators", indicators);
	WRITE("Team damage list", teamDamageList);
}

static void to_json(json &j, const Config::Exploits &o)
{
	const Config::Exploits dummy;

	WRITE("Anti AFK kick", antiAfkKick);
	WRITE("Fast duck", fastDuck);
	WRITE("Fake duck", fakeDuck);
	WRITE("Fake duck packets", fakeDuckPackets);
	WRITE("Moonwalk", moonwalk);
	WRITE("Slowwalk", slowwalk);
	WRITE("Bypass sv_pure", bypassPure);
}

static void to_json(json &j, const Config::Griefing::Reportbot &o, const Config::Griefing::Reportbot &dummy = {})
{
	WRITE("Enabled", enabled);
	WRITE("Target", target);
	WRITE("Delay", delay);
	WRITE("Rounds", rounds);
	WRITE("Abusive Communications", textAbuse);
	WRITE("Griefing", griefing);
	WRITE("Wall Hacking", wallhack);
	WRITE("Aim Hacking", aimbot);
	WRITE("Other Hacking", other);
}

static void to_json(json &j, const Config::Griefing::Blockbot &o, const Config::Griefing::Blockbot &dummy = {})
{
	WRITE("Bind", bind);
	WRITE("Trajectory factor", trajectoryFac);
	WRITE("Distance factor", distanceFac);
	WRITE("Visualise", visualize);
}

static void to_json(json &j, const Config::Griefing &o)
{
	const Config::Griefing dummy;

	WRITE("Custom clantag", customClanTag);
	WRITE("Clock tag", clocktag);

	if (o.clanTag[0])
		j["Clantag"] = o.clanTag;

	WRITE("Animated clantag", animatedClanTag);
	WRITE("Name stealer", nameStealer);
	WRITE("Fake Prime", fakePrime);
	WRITE("Kill message", killMessage);
	WRITE("Kill message string", killMessageString);
	WRITE("Ban color", banColor);
	WRITE("Ban text", banText);
	WRITE("Reportbot", reportbot);
	WRITE("Blockbot", blockbot);
	WRITE("Vote reveal", revealVotes);
	WRITE("Spam use", spamUse);
	WRITE("Nuke chat", chatNuke);
	WRITE("Basmala chat", chatBasmala);
}

static void to_json(json& j, const Config::Movement::QuickPeek& o, const Config::Movement::QuickPeek& dummy = {})
{
	WRITE("Bind", bind);
	WRITE("Visualize idle", visualizeIdle);
	WRITE("Visualize active", visualizeActive);
}

static void to_json(json &j, const Config::Movement &o)
{
	const Config::Movement dummy;

	WRITE("Bunny hop", bunnyHop);
	WRITE("Auto strafe", autoStrafe);
	WRITE("Edge jump", edgeJump);
	WRITE("Fast stop", fastStop);
	WRITE("Quick peek", autoPeek);
}

static void to_json(json &j, const Config::Visuals::ColorCorrection &o, const Config::Visuals::ColorCorrection &dummy)
{
	WRITE("Enabled", enabled);
	WRITE("Blue", blue);
	WRITE("Red", red);
	WRITE("Mono", mono);
	WRITE("Saturation", saturation);
	WRITE("Ghost", ghost);
	WRITE("Green", green);
	WRITE("Yellow", yellow);
}

static void to_json(json &j, const Config::Visuals::Viewmodel &o, const Config::Visuals::Viewmodel &dummy)
{
	WRITE("Enabled", enabled);
	WRITE("Fov", fov);
	WRITE("X", x);
	WRITE("Y", y);
	WRITE("Z", z);
	WRITE("Roll", roll);
}

static void to_json(json &j, const Config::Visuals::Beams &o, const Config::Visuals::Beams &dummy)
{
	WRITE("Enabled", enabled);
	WRITE("Sprite", sprite);
	WRITE("Color", color);
	WRITE("Width", width);
	WRITE("Life", life);
	WRITE("Type", type);
	WRITE("Noise", amplitude);
	WRITE("Noise once", noiseOnce);
}

static void to_json(json &j, const Config::Visuals::Dlights &o, const Config::Visuals::Dlights &dummy)
{
	WRITE("Enabled", enabled);
	WRITE("Color", color);
	WRITE("Radius", radius);
}

static void to_json(json &j, const Config::Visuals &o)
{
	const Config::Visuals dummy = {};

	WRITE("Aspect ratio", aspectratio);
	WRITE("Opposite hand knife", oppositeHandKnife);
	WRITE("Disable post-processing", disablePostProcessing);
	WRITE("Inverse ragdoll gravity", inverseRagdollGravity);
	WRITE("No fog", noFog);
	WRITE("No 3d sky", no3dSky);
	WRITE("No aim punch", noAimPunch);
	WRITE("No view punch", noViewPunch);
	WRITE("No hands", noHands);
	WRITE("No sleeves", noSleeves);
	WRITE("No weapons", noWeapons);
	WRITE("No smoke", smoke);
	WRITE("No fire", inferno);
	WRITE("No blur", noBlur);
	WRITE("No scope overlay", noScopeOverlay);
	WRITE("No grass", noGrass);
	WRITE("No shadows", noShadows);
	WRITE("Viewmodel", viewmodel);
	WRITE("Zoom", zoom);
	WRITE("Zoom factor", zoomFac);
	WRITE("Thirdperson", thirdPerson);
	WRITE("Thirdperson distance", thirdpersonDistance);
	WRITE("Thirdperson collision", thirdpersonCollision);
	WRITE("Flashlight", flashlight);
	WRITE("Flashlight brightness", flashlightBrightness);
	WRITE("Flashlight distance", flashlightDistance);
	WRITE("Flashlight fov", flashlightFov);
	WRITE("FOV", fov);
	WRITE("Force keep FOV", forceFov);
	WRITE("Far Z", farZ);
	WRITE("Flash reduction", flashReduction);
	WRITE("Brightness", brightness);
	WRITE("Skybox", skybox);
	WRITE("World", world);
	WRITE("Props", props);
	WRITE("Sky", sky);
	WRITE("Deagle spinner", deagleSpinner);
	WRITE("Screen effect", screenEffect);
	WRITE("Hit effect", hitEffect);
	WRITE("Hit effect time", hitEffectTime);
	WRITE("Kill effect", killEffect);
	WRITE("Kill effect time", killEffectTime);
	WRITE("Hit marker", hitMarker);
	WRITE("Hit marker time", hitMarkerTime);
	WRITE("Playermodel T", playerModelT);
	WRITE("Playermodel CT", playerModelCT);
	WRITE("Color correction", colorCorrection);
	WRITE("Bullet impacts", bulletImpacts);
	WRITE("Accuracy tracers", accuracyTracers);
	WRITE("Beams self", selfBeams);
	WRITE("Beams ally", allyBeams);
	WRITE("Beams enemy", enemyBeams);
	WRITE("Dlights self", selfDlights);
	WRITE("Dlights ally", allyDlights);
	WRITE("Dlights enemy", enemyDlights);
	WRITE("Inferno hull", molotovHull);
	WRITE("Smoke hull", smokeHull);
	WRITE("Player bounds", playerBounds);
	WRITE("Player velocity", playerVelocity);
	WRITE("Noscope crosshair type", overlayCrosshairType);
	WRITE("Noscope crosshair", overlayCrosshair);
	WRITE("Recoil crosshair type", recoilCrosshairType);
	WRITE("Recoil crosshair", recoilCrosshair);
	WRITE("Inaccuracy circle", accuracyCircle);
	WRITE("Force crosshair", forceCrosshair);
}

static void to_json(json &j, const ImVec4 &o)
{
	j[0] = o.x;
	j[1] = o.y;
	j[2] = o.z;
	j[3] = o.w;
}

static void to_json(json &j, const Config::Style &o)
{
	const Config::Style dummy;

	WRITE("Menu style", menuStyle);
	WRITE("Menu colors", menuColors);

	auto &colors = j["Colors"];
	ImGuiStyle &style = ImGui::GetStyle();

	for (int i = 0; i < ImGuiCol_COUNT; ++i)
		colors[ImGui::GetStyleColorName(i)] = style.Colors[i];
}

static void to_json(json &j, const sticker_setting &o)
{
	const sticker_setting dummy;

	WRITE("Kit", kit);
	WRITE("Wear", wear);
	WRITE("Scale", scale);
	WRITE("Rotation", rotation);
}

static void to_json(json &j, const item_setting &o)
{
	const item_setting dummy;

	WRITE("Enabled", enabled);
	WRITE("Definition index", itemId);
	WRITE("Quality", quality);
	WRITE("Paint Kit", paintKit);
	WRITE("Definition override", definition_override_index);
	WRITE("Seed", seed);
	WRITE("StatTrak", stat_trak);
	WRITE("Wear", wear);
	if (o.custom_name[0])
		j["Custom name"] = o.custom_name;
	WRITE("Stickers", stickers);
}

void removeEmptyObjects(json &j) noexcept
{
	for (auto it = j.begin(); it != j.end();)
	{
		auto &val = it.value();
		if (val.is_object() || val.is_array())
			removeEmptyObjects(val);
		if (val.empty() && !j.is_array())
			it = j.erase(it);
		else
			++it;
	}
}

void Config::save(size_t id) const noexcept
{
	std::error_code ec;
	std::filesystem::create_directory(path, ec);

	if (std::ofstream out{path / (const char8_t *)configs[id].c_str()}; out.good())
	{
		json j;

		j["Aimbot"] = aimbot;
		j["Triggerbot"] = triggerbot;
		j["Backtrack"] = backtrack;
		j["Anti aim"] = antiAim;
		j["Glow"] = glow;
		j["Chams"] = chams;
		j["ESP"] = esp;
		j["Sound"] = sound;
		j["Visuals"] = visuals;
		j["Misc"] = misc;
		j["Style"] = style;
		j["Skin changer"] = skinChanger;
		j["Exploits"] = exploits;
		j["Movement"] = movement;
		j["Griefing"] = griefing;

		removeEmptyObjects(j);
		out << std::setw(2) << j;
	}
}

void Config::add(const char *name) noexcept
{
	if (*name && std::find(configs.cbegin(), configs.cend(), name) == configs.cend())
	{
		configs.emplace_back(name);
		save(configs.size() - 1);
	}
}

void Config::remove(size_t idx) noexcept
{
	std::error_code ec;
	std::filesystem::remove(path / (const char8_t *)configs[idx].c_str(), ec);
	configs.erase(configs.cbegin() + idx);
}

void Config::rename(size_t idx, const char *newName) noexcept
{
	std::error_code ec;
	std::filesystem::rename(path / (const char8_t *)configs[idx].c_str(), path / (const char8_t *)newName, ec);
	configs[idx] = newName;
}

void Config::reset() noexcept
{
	aimbot = {};
	antiAim = {};
	triggerbot = {};
	backtrack = {};
	glow = {};
	chams = {};
	esp = {};
	visuals = {};
	skinChanger = {};
	sound = {};
	style = {};
	exploits = {};
	griefing = {};
	movement = {};
	misc = {};
}

void Config::listConfigs() noexcept
{
	configs.clear();

	std::error_code ec;
	std::transform(std::filesystem::directory_iterator{path, ec},
		std::filesystem::directory_iterator{},
		std::back_inserter(configs),
		[](const auto &entry) { return std::string{(const char *)entry.path().filename().u8string().c_str()}; });
}

void Config::openConfigDir() const noexcept
{
	std::error_code ec;
	std::filesystem::create_directory(path, ec);
	ShellExecuteW(nullptr, L"open", path.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void Config::scheduleFontLoad(const std::string &name) noexcept
{
	scheduledFonts.push_back(name);
}

#ifdef _WIN32
static auto getFontData(const std::string &fontName) noexcept
{
	HFONT font = CreateFontA(0, 0, 0, 0,
		FW_NORMAL, FALSE, FALSE, FALSE,
		ANSI_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH, fontName.c_str());

	std::unique_ptr<std::byte[]> data;
	DWORD dataSize = GDI_ERROR;

	if (font)
	{
		HDC hdc = CreateCompatibleDC(nullptr);

		if (hdc)
		{
			SelectObject(hdc, font);
			dataSize = GetFontData(hdc, 0, 0, nullptr, 0);

			if (dataSize != GDI_ERROR)
			{
				data = std::make_unique<std::byte[]>(dataSize);
				dataSize = GetFontData(hdc, 0, 0, data.get(), dataSize);

				if (dataSize == GDI_ERROR)
					data.reset();
			}
			DeleteDC(hdc);
		}
		DeleteObject(font);
	}
	return std::make_pair(std::move(data), dataSize);
}
#endif

bool Config::loadScheduledFonts() noexcept
{
	bool result = false;

	for (const auto &fontName : scheduledFonts)
	{
		constexpr auto fontBig = 13.0f;

		if (fontName == "Default")
		{
			if (fonts.find("Default") == fonts.cend())
			{
				Font newFont;
				newFont.font = gui->getFont();

				fonts.emplace(fontName, newFont);
				result = true;
			}
			continue;
		}

		#ifdef _WIN32
		const auto [fontData, fontDataSize] = getFontData(fontName);
		if (fontDataSize == GDI_ERROR)
			continue;

		auto loadedFont = fonts.find(fontName);
		if (loadedFont == fonts.cend())
		{
			ImFontConfig cfg;
			cfg.FontDataOwnedByAtlas = false;
			cfg.OversampleH = cfg.OversampleV = 2;
			cfg.PixelSnapH = false;
			cfg.SizePixels = fontBig;
			if (cfg.Name[0] == '\0')
				std::sprintf(cfg.Name, "ESP %s, %dpx", fontName.c_str(), static_cast<int>(cfg.SizePixels));

			Font newFont;
			newFont.font = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(fontData.get(), fontDataSize, cfg.SizePixels, &cfg, Helpers::getFontGlyphRanges());
			fonts.emplace(fontName, newFont);
			result = true;
		}
		#endif
	}
	scheduledFonts.clear();
	return result;
}

Config::Aimbot &Config::Aimbot::getRelevantConfig() noexcept
{
	const auto activeWeapon = localPlayer->getActiveWeapon();

	auto weaponIndex = getWeaponIndex(activeWeapon->itemDefinitionIndex2());
	if (!config->aimbot[weaponIndex].bind.keyMode)
		weaponIndex = getWeaponClass(activeWeapon->itemDefinitionIndex2());

	if (!config->aimbot[weaponIndex].bind.keyMode)
		weaponIndex = 0;

	return config->aimbot[weaponIndex];
}

Config::Triggerbot &Config::Triggerbot::getRelevantConfig() noexcept
{
	const auto activeWeapon = localPlayer->getActiveWeapon();

	auto weaponIndex = getWeaponIndex(activeWeapon->itemDefinitionIndex2());
	if (!config->triggerbot[weaponIndex].bind.keyMode)
		weaponIndex = getWeaponClass(activeWeapon->itemDefinitionIndex2());

	if (!config->triggerbot[weaponIndex].bind.keyMode)
		weaponIndex = 0;

	return config->triggerbot[weaponIndex];
}

Config::AntiAim &Config::AntiAim::getRelevantConfig() noexcept
{
	constexpr std::array categories = {"Freestand", "Slowwalk", "Run", "Airborne"};

	if (localPlayer->flags() & PlayerFlag_OnGround)
	{
		if (localPlayer->velocity().length2D() < 5.0f)
			return config->antiAim[categories[0]];
		else if (static Helpers::KeyBindState flag; flag[config->exploits.slowwalk])
			return config->antiAim[categories[1]];
		else
			return config->antiAim[categories[2]];
	} else
		return config->antiAim[categories[3]];
}
