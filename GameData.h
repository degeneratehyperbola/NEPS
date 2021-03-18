#pragma once

#include <list>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>
#include <array>

#include "SDK/matrix3x4.h"
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

struct Indicators;

struct Global;

struct StudioHitboxSet;
struct Matrix4x4;

namespace GameData
{
    void update() noexcept;
    void clearProjectileList() noexcept;

    class Lock {
    public:
        Lock() noexcept : lock{ mutex } {};
    private:
        std::scoped_lock<std::mutex> lock;
        static inline std::mutex mutex;
    };

    // You have to acquire lock before using these getters
    const Matrix4x4& toScreenMatrix() noexcept;
    const LocalPlayerData& local() noexcept;
    const std::vector<PlayerData>& players() noexcept;
    const std::vector<ObserverData>& observers() noexcept;
    const std::vector<WeaponData>& weapons() noexcept;
    const std::vector<EntityData>& entities() noexcept;
    const std::vector<LootCrateData>& lootCrates() noexcept;
    const std::list<ProjectileData>& projectiles() noexcept;
    const PlayerData *playerByHandle(int handle) noexcept;
	const BombData &plantedC4() noexcept;
	const std::vector<InfernoData> &infernos() noexcept;
	Global &global() noexcept;
}

struct Indicators
{
	int blockTarget = 0;
	Vector serverHead = Vector{};
	Vector desyncHead = Vector{};
	std::vector<Vector> multipoints;
	float fakeLby = 0.0f;
	float realLby = 0.0f;
	float deltaLby = 0.0f;
};

struct Global
{
	UserCmd lastCmd = UserCmd{};
	bool sentPacket = true;
	matrix3x4 lerpedBones[MAXSTUDIOBONES];
	//matrix3x4 shotBones[MAXSTUDIOBONES];

	Indicators indicators;
};

struct LocalPlayerData {
    void update() noexcept;

    bool exists = false;
    bool alive = false;
    bool inReload = false;
    bool shooting = false;
    bool noScope = false;
    float nextWeaponAttack = 0.0f;
    int fov;
	int handle;
    float flashDuration;
    Vector aimPunch;
    Vector aimPunchAngle;
    Vector origin;
    Vector velocity;
};

class Entity;

struct BaseData {
    BaseData(Entity* entity) noexcept;

    constexpr auto operator<(const BaseData& other) const
    {
        return distanceToLocal > other.distanceToLocal;
    }

    float distanceToLocal;
    Vector obbMins, obbMaxs;
    matrix3x4 coordinateFrame;
};

struct EntityData final : BaseData {
    EntityData(Entity* entity) noexcept;
   
    const char* name;
};

struct ProjectileData : BaseData {
    ProjectileData(Entity* projectile) noexcept;

    void update(Entity* projectile) noexcept;

    constexpr auto operator==(int otherHandle) const noexcept
    {
        return handle == otherHandle;
    }

    bool exploded = false;
    bool thrownByLocalPlayer = false;
    bool thrownByEnemy = false;
    int handle;
    const char* name = nullptr;
    std::vector<std::pair<float, Vector>> trajectory;
};

struct PlayerData : BaseData {
	PlayerData(Entity* entity) noexcept;
	PlayerData(const PlayerData&) = delete;
	PlayerData& operator=(const PlayerData&) = delete;
	PlayerData(PlayerData&&) = default;
	PlayerData& operator=(PlayerData&&) = default;

	void update(Entity* entity) noexcept;

	bool dormant;
	bool enemy = false;
	bool visible = false;
	bool audible;
	bool spotted;
	bool inViewFrustum;
	bool alive;
	int uid;
	bool isBot;
	bool hasBomb;
	bool hasDefuser;
	bool ducking;
	bool scoped;
	bool reloading;
	float flashDuration;
	int health;
	int armor;
	int handle;
	char name[128];
	Vector headMins, headMaxs;
	Vector origin;
	Vector velocity;
	std::string activeWeapon;
	std::vector<std::pair<Vector, Vector>> bones;
	StudioHitboxSet *hitboxSet;
};

struct WeaponData : BaseData {
    WeaponData(Entity* entity) noexcept;

    int clip;
    int reserveAmmo;
    const char* group = "All";
    const char* name = "All";
    std::string displayName;
};

struct LootCrateData : BaseData {
    LootCrateData(Entity* entity) noexcept;

    const char* name = nullptr;
};

struct ObserverData {
    ObserverData(Entity* entity, Entity* obs, bool targetIsLocalPlayer, bool targetIsObservedByLocalPlayer) noexcept;

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
