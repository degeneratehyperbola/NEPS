#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

#include "imgui/imgui.h"
#include "Hacks/SkinChanger.h"
#include "ConfigStructs.h"

class Config
{
public:
	explicit Config(const char *) noexcept;
	bool load(const char8_t *name, bool incremental) noexcept;
	bool load(size_t, bool incremental) noexcept;
	void save(size_t) const noexcept;
	void add(const char *) noexcept;
	void remove(size_t) noexcept;
	void rename(size_t, const char *) noexcept;
	void reset() noexcept;
	void listConfigs() noexcept;
	void openConfigDir() const noexcept;

	constexpr auto &getConfigs() noexcept
	{
		return configs;
	}

	struct Aimbot
	{
		KeyBind bind;
		bool aimlock = false;
		bool silent = false;
		bool friendlyFire = false;
		bool visibleOnly = true;
		bool scopedOnly = true;
		bool ignoreFlash = false;
		bool ignoreSmoke = false;
		bool autoShot = false;
		bool autoScope = false;
		bool multipoint = false;
		float multipointScale = 0.75f;
		int targeting = 0;
		float fov = 1.0f;
		float distance = 0.0f;
		int minDamage = 0;
		int minDamageAutoWall = 0;
		int killshot = 10;
		int killshotAutoWall = 10;
		float shotHitchance = 0.0f;
		float maxAimInaccuracy = 1.0f;
		int interpolation = 0;
		float smooth = 0.0f;
		float linearSpeed = 255.0f;
		int hitgroup = (1 << 7) - 1;
		bool betweenShots = true;
		KeyBind safeOnly;
		int safeHitgroup = 0;
		bool targetStop = false;
	};
	std::array<Aimbot, 40> aimbot;

	struct Triggerbot
	{
		KeyBind bind;
		bool friendlyFire = false;
		bool visibleOnly = true;
		bool scopedOnly = true;
		bool ignoreFlash = false;
		bool ignoreSmoke = false;
		int hitgroup = (1 << 7) - 1;
		int shotDelay = 0;
		float maxShotInaccuracy = 1.0f;
		float hitchance = 0.0f;
		float distance = 0.0f;
		int minDamage = 0;
		int minDamageAutoWall = 0;
		int killshot = 10;
		int killshotAutoWall = 10;
		float burstTime = 0.0f;
	};
	std::array<Triggerbot, 40> triggerbot;

	struct Backtrack
	{
		bool enabled = false;
		bool ignoreSmoke = false;
		bool recoilBasedFov = true;
		int timeLimit = 200;
	} backtrack;

	struct AntiAim
	{
		bool pitch = false;
		float pitchAngle = 0.0f;
		bool yaw = false;
		float yawAngle = 0.0f;
		bool desync = false;
		bool corrected = true;
		bool clamped = false;
		bool extended = false;
		bool avoidOverlap = false;
		bool fakeUp = false;
		int flipKey = 0;
		KeyBind fakeDuck;
		int fakeDuckPackets = 0;
		int chokedPackets = 0;
		KeyBind choke;
	} antiAim;

	struct Glow : Color4
	{
		bool enabled = false;
		bool healthBased = false;
		int style = 0;
		bool full = false;
	};
	std::array<Glow, 21> glow;

	struct Chams
	{
		struct Material : Color4
		{
			bool enabled = false;
			bool healthBased = false;
			bool blinking = false;
			bool wireframe = false;
			bool cover = false;
			bool ignorez = false;
			int material = 0;
		};
		std::array<Material, 7> materials;
	};
	std::unordered_map<std::string, Chams> chams;

	struct ESP
	{
		std::unordered_map<std::string, Player> allies;
		std::unordered_map<std::string, Player> enemies;
		std::unordered_map<std::string, Weapon> weapons;
		std::unordered_map<std::string, Projectile> projectiles;
		std::unordered_map<std::string, Shared> lootCrates;
		std::unordered_map<std::string, Shared> otherEntities;
	} esp;

	struct Font
	{
		ImFont *tiny;
		ImFont *medium;
		ImFont *big;
	};

	struct Visuals
	{
		bool disablePostProcessing = false;
		bool inverseRagdollGravity = false;
		bool noFog = false;
		bool no3dSky = false;
		bool noAimPunch = false;
		bool noViewPunch = false;
		bool noHands = false;
		bool noSleeves = false;
		bool noWeapons = false;
		bool noSmoke = false;
		bool noFire = false;
		bool noBlur = false;
		bool noScopeOverlay = false;
		bool noGrass = false;
		bool noShadows = false;
		bool wireframeSmoke = false;
		KeyBind zoom;
		int zoomFac = 50;
		KeyBind thirdPerson;
		int thirdpersonDistance = 150;
		int fov = 90;
		bool forceFov = false;
		int farZ = 500;
		int flashReduction = 0;
		float brightness = 0.0f;
		int skybox = 0;
		Color4Toggle world;
		Color4Toggle props;
		Color3Toggle sky;
		bool deagleSpinner = false;
		int screenEffect = 0;
		int hitEffect = 0;
		float hitEffectTime = 0.6f;
		int killEffect = 0;
		float killEffectTime = 0.6f;
		int hitMarker = 0;
		float hitMarkerTime = 0.6f;
		int playerModelT = 0;
		int playerModelCT = 0;
		float aspectratio = 0;
		bool oppositeHandKnife = false;
		int bulletImpacts = 0;
		int accuracyTracers = 0;

		struct Beams
		{
			bool enabled;
			int sprite = 0;
			std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f};
			float width = 1.0f;
			float life = 3.0f;
			float noise = 5.0f;
			bool noiseOnce = true;
			bool railgun = true;
		};

		Beams selfBeams;
		Beams allyBeams;
		Beams enemyBeams;

		struct Dlights
		{
			bool enabled;
			std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f};
			float radius;
		};

		Dlights selfDlights;
		Dlights allyDlights;
		Dlights enemyDlights;

		Color4ToggleThickness molotovHull = {1.0f, 0.5f, 0.0f, 0.3f};
		Color4ToggleThickness smokeHull = {0.5f, 0.5f, 0.5f, 0.3f};
		Color4ToggleThickness playerBounds = {1.0f, 1.0f, 1.0f, 1.0f};
		Color4ToggleThickness playerVel = {1.0f, 1.0f, 1.0f, 1.0f};

		struct ColorCorrection
		{
			bool enabled = false;
			float blue = 0.0f;
			float red = 0.0f;
			float mono = 0.0f;
			float saturation = 0.0f;
			float ghost = 0.0f;
			float green = 0.0f;
			float yellow = 0.0f;
		} colorCorrection;

		struct Viewmodel
		{
			int fov = 0;
			bool enabled = false;
			float x = 0.0f;
			float y = 0.0f;
			float z = 0.0f;
			float roll = 0;
		} viewmodel;
	} visuals;

	std::array<item_setting, 36> skinChanger;

	struct Sound
	{
		int chickenVolume = 100;

		struct Player
		{
			int masterVolume = 100;
			int headshotVolume = 100;
			int weaponVolume = 100;
			int footstepVolume = 100;
		};

		std::array<Player, 3> players;

		int hitSound = 0;
		int killSound = 0;
		float hitSoundVol = 1.0f;
		float killSoundVol = 1.0f;
		std::string customKillSound;
		std::string customHitSound;
	} sound;

	struct Style
	{
		int menuStyle = 0;
		int menuColors = 2;
	} style;

	struct Exploits
	{
		bool moonwalk = false;
		bool antiAfkKick = false;
		KeyBind slowwalk;
		bool fastDuck = false;
		KeyBind doubletap;
		bool bypassPure = true;
	} exploits;

	struct Griefing
	{
		bool nameStealer = false;
		bool killMessage = false;
		bool fakePrime = false;
		bool revealVotes = false;
		char clanTag[16] = "\0";
		int animatedClanTag = 0;
		bool clocktag = false;
		bool customClanTag = false;
		std::string killMessageString;
		int banColor = 6;
		std::string banText = "Cheater has been permanently banned from official CS:GO servers.";

		struct Blockbot
		{
			KeyBind bind;
			KeyBind target;
			float trajectoryFac = 1.0f;
			float distanceFac = 2.0f;
			Color4ToggleThickness visualise = {0.0f, 0.5f, 1.0f, 1.0f};
		} blockbot;

		struct Reportbot
		{
			bool enabled = false;
			bool textAbuse = false;
			bool griefing = false;
			bool wallhack = true;
			bool aimbot = true;
			bool other = true;
			int target = 0;
			int delay = 1;
			int rounds = 99;
		} reportbot;

	} griefing;

	struct Movement
	{
		bool bunnyHop = false;
		bool autoStrafe = false;
		float steerSpeed = 18.5f;
		KeyBind edgeJump;
		bool fastStop = false;
	} movement;

	struct Misc
	{
		int menuKey = 0x2E; // VK_DELETE
		bool autoPistol = false;
		bool autoReload = false;
		bool autoAccept = false;
		bool fixAnimationLOD = true;
		bool fixBoneMatrix = true;
		bool fixMovement = true;
		bool fixAnimation = true;
		bool disableModelOcclusion = true;
		bool disablePanoramablur = false;
		struct PreserveKillfeed
		{
			bool enabled = false;
			bool onlyHeadshots = false;
		} preserveKillfeed;
		Color3Toggle spectatorList;
		Color4 specBg = {0.0f, 0.0f, 0.0f, 0.5f};
		Color3Toggle watermark;
		int watermarkPos;
		Color4 bg = {0.0f, 0.0f, 0.0f, 0.5f};
		KeyBind prepareRevolver;
		int quickHealthshotKey = 0;
		float maxAngleDelta = 255.0f;
		struct PurchaseList
		{
			bool enabled = false;
			bool onlyDuringFreezeTime = false;
			bool showPrices = false;
			bool noTitleBar = false;

			enum Mode
			{
				Details = 0,
				Summary
			};
			int mode = Details;
		} purchaseList;
		bool revealRanks = false;
		bool revealMoney = false;
		bool revealSuspect = false;
		bool radarHack = false;
		bool quickReload = false;
		bool fastPlant = false;
		bool fixTabletSignal = false;
		Color4Toggle noscopeCrosshair;
		Color4Toggle recoilCrosshair;
		bool bombTimer = false;
		Color4Toggle offscreenEnemies = {1.0f, 0.0f, 0.0f, 0.2f};
		bool nadePredict = false;
		bool spamUse = false;
		bool indicators = false;
	} misc;

	void scheduleFontLoad(const std::string &name) noexcept;
	bool loadScheduledFonts() noexcept;
	const auto &getSystemFonts() noexcept { return systemFonts; }
	const auto &getFonts() noexcept { return fonts; }
	const auto &getPath() noexcept { return path; }
private:
	std::vector<std::string> scheduledFonts = {"Default"};
	std::vector<std::string> systemFonts = {"Default"};
	std::unordered_map<std::string, Font> fonts;
	std::filesystem::path path;
	std::vector<std::string> configs;
};

inline std::unique_ptr<Config> config;
