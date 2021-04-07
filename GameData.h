#pragma once

#include <list>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>
#include <array>

#include "SDK/Matrix3x4.h"
#include "SDK/Vector.h"
#include "SDK/AnimState.h"
#include "SDK/ModelInfo.h"
#include "SDK/UserCmd.h"

struct LocalPlayerData;

struct AnimState;
struct PlayerData;
struct ObserverData;
struct WeaponData;
struct EntityData;
struct LootCrateData;
struct ProjectileData;
struct BombData;
struct InfernoData;
struct SmokeData;
struct MatchData;

struct Indicators;

struct GlobalData;

struct StudioHitboxSet;
struct Matrix4x4;

namespace GameData
{
	void update() noexcept;
	void clearProjectileList() noexcept;

	class Lock
	{
	public:
		Lock() noexcept : lock{mutex} {};
	private:
		std::scoped_lock<std::mutex> lock;
		static inline std::mutex mutex;
	};

	// You have to acquire lock before using these getters
	// Do not aquire lock twice :)
	const Matrix4x4 &toScreenMatrix() noexcept;
	const LocalPlayerData &local() noexcept;
	const std::vector<PlayerData> &players() noexcept;
	const std::vector<ObserverData> &observers() noexcept;
	const std::vector<WeaponData> &weapons() noexcept;
	const std::vector<EntityData> &entities() noexcept;
	const std::vector<LootCrateData> &lootCrates() noexcept;
	const std::list<ProjectileData> &projectiles() noexcept;
	const PlayerData *playerByHandle(int handle) noexcept;
	const BombData &plantedC4() noexcept;
	const std::vector<InfernoData> &infernos() noexcept;
	const std::vector<SmokeData> &smokes() noexcept;
	const MatchData &match() noexcept;
	GlobalData &global() noexcept;
}

struct Indicators
{
	int blockTarget = 0;
	Vector serverHead;
	Vector desyncHead;
	float deltaLby = 0.0f;

	#ifdef _DEBUG_NEPS
	std::vector<Vector> multipoints;
	#endif // _DEBUG_NEPS
};

struct GlobalData
{
	UserCmd lastCmd;
	bool sentPacket = true;
	Matrix3x4 lerpedBones[MAXSTUDIOBONES];
	
	Indicators indicators;
};

struct LocalPlayerData
{
	void update() noexcept;

	bool exists = false;
	bool alive = false;
	bool reloading = false;
	bool shooting = false;
	bool scopeOverlay = false;
	float nextAttack = 0.0f;
	int fov = 90;
	int handle = 0;
	float flashDuration = 0.0f;
	int observerTargetHandle = 0;
	Vector aimPunch;
	Vector aimPunchAngle;
	Vector origin;
	Vector velocity;
	Vector colMaxs, colMins;
};

class Entity;

struct BaseData
{
	BaseData(Entity *entity) noexcept;

	constexpr auto operator<(const BaseData &other) const
	{
		return distanceToLocal > other.distanceToLocal;
	}

	float distanceToLocal = 0.0f;
	Vector obbMins, obbMaxs;
	Matrix3x4 coordinateFrame;
};

struct EntityData final : BaseData
{
	EntityData(Entity *entity) noexcept;

	const char *name = nullptr;
};

struct ProjectileData : BaseData
{
	ProjectileData(Entity *projectile) noexcept;

	void update(Entity *projectile) noexcept;

	constexpr auto operator==(int otherHandle) const noexcept
	{
		return handle == otherHandle;
	}

	bool exploded = false;
	bool thrownByLocalPlayer = false;
	bool thrownByEnemy = false;
	int handle = 0;
	const char *name = nullptr;
	std::vector<std::pair<float, Vector>> trajectory;
};

struct PlayerData : BaseData
{
	PlayerData(Entity *entity) noexcept;
	PlayerData(const PlayerData &) = delete;
	PlayerData &operator=(const PlayerData &) = delete;
	PlayerData(PlayerData &&) = default;
	PlayerData &operator=(PlayerData &&) = default;

	void update(Entity *entity) noexcept;

	bool dormant = true;
	float becameDormant = 0.0f;
	bool enemy = false;
	bool visible = false;
	bool scoped = false;
	bool reloading = false;
	bool audible = false;
	bool spotted = false;
	bool inViewFrustum = false;
	bool alive = false;
	bool isBot = false;
	bool hasBomb = false;
	bool isVip = false;
	bool hasDefuser = false;
	bool ducking = false;
	float flashDuration = 0.0f;
	int health = 0;
	int armor = 0;
	int handle = 0;
	char name[128] = "\0";
	Vector headMins, headMaxs;
	Vector colMins, colMaxs;
	Vector origin;
	Vector velocity;
	std::string activeWeapon;
	std::vector<std::pair<Vector, Vector>> bones;
};

struct WeaponData : BaseData
{
	WeaponData(Entity *entity) noexcept;

	int clip = -1;
	int reserveAmmo = -1;
	const char *group = "All";
	const char *name = "All";
	std::string displayName;
};

struct LootCrateData : BaseData
{
	LootCrateData(Entity *entity) noexcept;

	const char *name = nullptr;
};

struct ObserverData
{
	ObserverData(Entity *entity, Entity *obs, bool targetIsLocalPlayer, bool targetIsObservedByLocalPlayer) noexcept;

	char name[128];
	char target[128];
	bool targetIsLocalPlayer = false;
	bool targetIsObservedByLocalPlayer = false;
};

struct BombData
{
	void update() noexcept;

	float blowTime = 0.0f;
	float timerLength = 0.0f;
	int defuserHandle = 0;
	float defuseCountDown = 0.0f;
	float defuseLength = 0.0f;
	int bombsite = -1;
};

struct InfernoData
{
	InfernoData(Entity *inferno) noexcept;

	std::vector<Vector> points;
};

struct SmokeData
{
	SmokeData(Entity *smoke) noexcept;

	Vector origin;
};

struct MatchData
{
	void update() noexcept;

	std::string levelName;
};
