#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

#include <shared_lib/imgui/imgui.h>
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
		ImFont *font;
	};

	struct Aimbot
	{
		static Aimbot &getRelevantConfig() noexcept;

		KeyBind bind;
		bool aimlock = false;
		bool silent = false;
		bool friendlyFire = false;
		bool visibleOnly = true;
		bool scopedOnly = true;
		bool ignoreFlash = false;
		bool ignoreSmoke = false;
		bool autoShoot = false;
		bool autoScope = false;
		bool autoStop = false;
		bool multipoint = false;
		float multipointScale = 0.8f;
		int targeting = 0;
		float fov = 1.0f;
		float distance = 0.0f;
		int minDamage = 0;
		int minDamageAutoWall = 0;
		float hitchance = 0.0f;
		int hitGroup = 127;
		bool humanize = false;
		float acceleration = 0.6f;
		float friction = 2.0f;
		bool betweenShots = true;
		struct AimbotOverride
		{
			KeyBind bind;
			float multipointScale = 0.8f;
			int targeting = 0;
			float fov = 1.0f;
			float distance = 0.0f;
			int minDamage = 0;
			int minDamageAutoWall = 0;
			float hitchance = 0.0f;
			int hitGroup = 127;
		} aimbotOverride;
		float recoilReductionH = 100.0f;
		float recoilReductionV = 100.0f;
	};
	std::array<Aimbot, 40> aimbot;


	struct Triggerbot
	{
		static Triggerbot &getRelevantConfig() noexcept;

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
		static AntiAim &getRelevantConfig() noexcept;

		bool pitch = false;
		float pitchAngle = 0.0f;
		bool yaw = false;
		float yawAngle = 0.0f;
		bool lookAtEnemies = false;
		int direction = 0;
		int rightKey = 0;
		int backKey = 0;
		int leftKey = 0;
		Color4OutlineToggle visualizeDirection = {1.0f, 1.0f, 1.0f, 0.5f};
		int desync = 0;
		bool fakeUp = false;
		int flipKey = 0;
		Color4OutlineToggle visualizeSide = {1.0f, 1.0f, 1.0f, 0.5f};
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
	std::array<Glow, 20> glow;

	struct Chams
	{
		struct Material : Color4
		{
			bool enabled = false;
			bool healthBased = false;
			bool blinking = false;
			bool wireframe = false;
			bool cover = false;
			bool ignoreZ = false;
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
		Color4OutlineToggle accuracyCircle = {1.0f, 1.0f, 1.0f, 0.25f};
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

			bool enabled = false;
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
			bool enabled = false;
			std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f};
			float radius = 100.0f;
		};

		Dlights selfDlights;
		Dlights allyDlights;
		Dlights enemyDlights;

		Color4ToggleThickness molotovHull = {1.0f, 0.5f, 0.0f, 0.3f};
		Color4ToggleThickness smokeHull = {0.5f, 0.5f, 0.5f, 0.3f};
		Color4ToggleThickness playerBounds;
		Color4OutlineToggleThickness playerVelocity;

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
		int menuColors = 1;
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
		bool bypassPure = true;
	} exploits;

	struct Griefing
	{
		bool fakePrime = false;
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
		KeyBind chatNuke;
		KeyBind chatBasmala;

		struct Blockbot
		{
			KeyBind bind;
			float trajectoryFac = 1.0f;
			float distanceFac = 2.0f;
			Color4ToggleThickness visualize = {0.0f, 1.0f, 0.0f, 0.7f};
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

		struct QuickPeek
		{
			KeyBind bind;
			Color4Toggle visualizeIdle = {0.0f, 1.0f, 0.0f, 0.5f};
			Color4Toggle visualizeActive = {1.0f, 1.0f, 0.0f, 0.5f};
		} autoPeek;

	} movement;

	struct Misc
	{
		int menuKey = 0x2E; // VK_DELETE
		bool autoPistol = false;
		bool autoReload = false;
		bool autoAccept = false;
		bool fixAnimationLOD = true;
		bool fixBoneMatrices = true;
		bool fixMovement = true;
		bool fixLocalAnimations = true;
		bool disableModelOcclusion = true;
		bool noExtrapolate = true;
		bool disableIK = false;
		bool resolveLby = false;
		bool unlockInventory = false;
		bool disablePanoramablur = false;

		struct PreserveKillfeed
		{
			bool enabled = false;
			bool onlyHeadshots = false;
		} preserveKillfeed;

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

		struct BombTimer
		{
			bool enabled = false;
		} bombTimer;

		struct Indicators
		{
			bool enabled = false;
			bool noTitleBar = false;
		} indicators;

		struct SpectatorList
		{
			bool enabled = false;
			bool noTitleBar = false;
		} spectatorList;

		struct Watermark
		{
			bool enabled = false;
			int position = 1;
		} watermark;

		struct TeamDamageList
		{
			bool enabled = false;
			bool noTitleBar = false;
			bool progressBars = true;
		} teamDamageList;

		KeyBind prepareRevolver;
		int quickHealthshotKey = 0;
		float maxAngleDelta = 255.0f;

		bool revealRanks = false;
		bool revealMoney = false;
		bool revealSuspect = false;
		bool radarHack = false;
		bool quickReload = false;
		bool fastPlant = false;
		bool fixTabletSignal = false;
		bool nadePredict = false;
		int forceRelayCluster = 0;
		bool goFestive = false;
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
