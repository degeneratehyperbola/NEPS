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
struct SessionData;

struct StudioHitboxSet;
struct Matrix4x4;

namespace GameData
{
	void update() noexcept;
	void clearProjectileList() noexcept;
	void clearPlayersLastLocation() noexcept;

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
	const std::forward_list<ProjectileData> &projectiles() noexcept;
	const PlayerData *playerByHandle(int handle) noexcept;
	const BombData &plantedC4() noexcept;
	const std::vector<InfernoData> &infernos() noexcept;
	const std::vector<SmokeData> &smokes() noexcept;
	const SessionData &session() noexcept;
}

struct LocalPlayerData
{
	void update() noexcept;

	bool exists;
	bool alive;
	bool shooting;
	bool reloading;
	float nextAttack;
	bool drawingScope;
	bool drawingCrosshair;
	int fov;
	int handle;
	float flashDuration;
	int observerTargetHandle;
	Vector eyePosition;
	Vector aimPunch;
	Vector inaccuracy;
	Vector origin;
	Vector velocity;
	Vector colMaxs, colMins;
};

class Entity;

struct BaseData
{
	BaseData() noexcept = default;
	BaseData(Entity *entity) noexcept;

	constexpr auto operator<(const BaseData &other) const
	{
		return distanceToLocal > other.distanceToLocal;
	}

	float distanceToLocal;
	Vector obbMins, obbMaxs;
	Matrix3x4 coordinateFrame;
};

struct EntityData final : BaseData
{
	EntityData(Entity *entity) noexcept;

	const char *name;
};

struct ProjectileData : BaseData
{
	ProjectileData(Entity* projectile) noexcept;

	void update(Entity* projectile) noexcept;

	constexpr auto operator==(int otherHandle) const noexcept
	{
		return handle == otherHandle;
	}

	bool exploded = false;
	float explosionTime = 0.0f;
	bool thrownByLocalPlayer = false;
	bool thrownByEnemy = false;
	int handle;
	const char* name = nullptr;
	std::vector<std::pair<float, Vector>> trajectory;
};
struct PlayerData : BaseData
{
	void update(Entity *entity) noexcept;

	int handle;
	bool dormant;
	float becameDormant;
	bool enemy;
	bool visible;
	bool scoped;
	bool reloading;
	bool audible;
	bool spotted;
	bool inViewFrustum;
	bool alive;
	bool isBot;
	bool hasBomb;
	bool isVip;
	bool hasDefuser;
	bool ducking;
	float flashDuration;
	int health;
	int armor;
	int money;
	int userId;
	std::string team;
	uint64_t steamID;
	Vector headMins, headMaxs;
	Vector colMins, colMaxs;
	Vector origin;
	Vector velocity;
	Vector lookingAt;
	std::string name;
	std::string activeWeapon;
	std::vector<std::pair<Vector, Vector>> bones;
	std::string lastPlaceName;
};

struct WeaponData : BaseData
{
	WeaponData(Entity *entity) noexcept;

	int clip;
	int reserveAmmo;
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
	bool targetIsLocalPlayer;
	bool targetIsObservedByLocalPlayer;
};

struct BombData
{
	void update() noexcept;

	float blowTime;
	float timerLength;
	int defuserHandle;
	float defuseCountDown;
	float defuseLength;
	int bombsite;
};

struct InfernoData
{
	InfernoData(Entity *inferno) noexcept;

	std::vector<Vector> points;
};

#define SMOKEGRENADE_LIFETIME 17.5f

struct SmokeData {
	SmokeData(const Vector& origin, int handle) noexcept;

	[[nodiscard]] float fadingAlpha() const noexcept;

	Vector origin;
	float explosionTime;
	int handle;
};

struct SessionData
{
	void update() noexcept;

	bool connected;
	std::string levelName;
	std::string address;
	int latency;
	int tickrate;
};
