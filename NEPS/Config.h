#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

#include "lib/imgui/imgui.h"
#include "Hacks/SkinChanger.h"
#include "ConfigStructs.h"

class Config
{
public:
	explicit Config(const char *) noexcept;

	bool load(const char8_t *name, bool incremental) noexcept;
	bool load(std::size_t, bool incremental) noexcept;
	void save(std::size_t) const noexcept;
	void add(const char *) noexcept;
	void remove(std::size_t) noexcept;
	void rename(std::size_t, const char *) noexcept;
	void reset() noexcept;
	void listConfigs() noexcept;
	void openConfigDir() const noexcept;

	constexpr auto &getConfigs() noexcept { return configs; }

	struct Font
	{
		ImFont *tiny;
		ImFont *medium;
		ImFont *big;
	};

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
		bool autoStop = false;
		bool multipoint = false;
		float multipointScale = 0.75f;
		int targeting = 0;
		float fov = 1.0f;
		float distance = 0.0f;
		int minDamage = 0;
		int minDamageAutoWall = 0;
		KeyBind damageOverride;
		int minDamageOverride = 0;
		int minDamageAutoWallOverride = 0;
		float shotHitchance = 0.0f;
		int interpolation = 0;
		float quadratic = 0.0f;
		float linear = 255.0f;
		int hitGroup = 127;
		bool betweenShots = true;
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
		int hitGroup = 127;
		int shotDelay = 0;
		float hitchance = 0.0f;
		float distance = 0.0f;
		int minDamage = 0;
		int minDamageAutoWall = 0;
		float burstTime = 0.0f;
	};
	std::array<Triggerbot, 40> triggerbot;

	struct Backtrack
	{
		bool enabled = false;
		bool ignoreSmoke = false;
		int timeLimit = 200;
	} backtrack;

	struct AntiAim
	{
		bool pitch = false;
		float pitchAngle = 0.0f;
		bool yaw = false;
		float yawAngle = 0.0f;
		bool hideHead = false;
		bool desync = false;
		int desyncType = 1;
		bool fakeUp = false;
		int flipKey = 0;
		KeyBind choke;
		int chokedPackets = 0;
	};
	std::unordered_map<std::string, AntiAim> antiAim;

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
		int smoke = 0;
		int inferno = 0;
		bool noBlur = false;
		bool noScopeOverlay = false;
		bool noGrass = false;
		bool noShadows = false;
		KeyBind zoom;
		int zoomFac = 50;
		KeyBind thirdPerson;
		int thirdpersonDistance = 150;
		bool thirdpersonCollision = true;
		KeyBind flashlight;
		float flashlightBrightness = 0.25f;
		int flashlightDistance = 750;
		int flashlightFov = 53;
		int fov = 90;
		bool forceFov = false;
		int farZ = 1200;
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
		Color4 overlayCrosshair;
		int overlayCrosshairType = 0;
		Color4 recoilCrosshair;
		int recoilCrosshairType = 0;
		int forceCrosshair = 0;

		struct Beams
		{
			enum Type
			{
				Line,
				Noise,
				Spiral
			};

			bool enabled;
			int sprite = 0;
			std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f};
			float width = 1.0f;
			float life = 3.0f;
			int type = 0;
			float amplitude = 5.0f;
			bool noiseOnce = true;
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
		int deathSound = 0;
		float hitSoundVol = 1.0f;
		float killSoundVol = 1.0f;
		float deathSoundVol = 1.0f;
		std::string customHitSound;
		std::string customKillSound;
		std::string customDeathSound;
	} sound;

	struct Style
	{
		int menuStyle = 0;
		int menuColors = 0;
		float scaling = 1.0f;
	} style;

	struct Exploits
	{
		KeyBind fakeDuck;
		int fakeDuckPackets = 0;
		KeyBind slowwalk;
		bool moonwalk = false;
		bool antiAfkKick = false;
		bool fastDuck = false;
		KeyBind doubletap;
		bool bypassPure = true;
	} exploits;

	struct Griefing
	{
		bool nameStealer = false;
		bool killMessage = false;
		bool revealVotes = false;
		char clanTag[16] = "\0";
		int animatedClanTag = 0;
		bool clocktag = false;
		bool customClanTag = false;
		std::string killMessageString;
		int banColor = 6;
		std::string banText = "Cheater has been permanently banned from official CS:GO servers.";
		bool spamUse = false;

		struct TeamDamageList
		{
			bool enabled = false;
			bool noTitleBar = false;
		} teamDamageList;

		struct Blockbot
		{
			KeyBind bind;
			KeyBind target;
			float trajectoryFac = 1.0f;
			float distanceFac = 2.0f;
			Color4ToggleThickness visualize = {0.0f, 0.5f, 1.0f, 1.0f};
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
		bool disableInterp = false;
		bool desyncResolver = false;
		bool unlockInvertory = false;
		bool disablePanoramablur = false;

		struct PreserveKillfeed
		{
			bool enabled = false;
			bool onlyHeadshots = false;
		} preserveKillfeed;

		Color3Toggle spectatorList;
		Color3Toggle watermark;
		int watermarkPos = 1;
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
		bool bombTimer = false;
		bool nadePredict = false;
		bool indicators = false;
		int forceRelayCluster = 0;
	} misc;

	void scheduleFontLoad(const std::string &name) noexcept;
	bool loadScheduledFonts() noexcept;
	const auto &getSystemFonts() const noexcept { return systemFonts; }
	const auto &getFonts() const noexcept { return fonts; }
	const auto &getPath() const noexcept { return path; }
private:
	std::vector<std::string> scheduledFonts = {"Default"};
	std::vector<std::string> systemFonts = {"Default"};
	std::unordered_map<std::string, Font> fonts;
	std::filesystem::path path;
	std::vector<std::string> configs;
};

inline std::unique_ptr<Config> config;
