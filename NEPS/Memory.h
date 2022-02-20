#pragma once

#include <array>
#include <memory>
#include <string>
#include <type_traits>
#include <Windows.h>
#include <Psapi.h>
#include <cassert>

class ClientMode;
class Entity;
class GameEventDescriptor;
class GameEventManager;
class Input;
class ItemSystem;
class KeyValues;
class MoveHelper;
class MoveData;
class PlayerResource;
class ViewRender;
class WeaponSystem;
class ClientState;
template <typename T>
class UtlVector;

struct AnimState;
struct ActiveChannels;
struct Channel;
struct GlobalVars;
struct GlowObjectManager;
struct Trace;
struct Vector;
struct UserCmd;
struct ViewRenderBeams;

class Memory
{
public:
	Memory() noexcept;

	uintptr_t present;
	uintptr_t reset;

	ClientMode *clientMode;
	Input *input;
	GlobalVars *globalVars;
	GlowObjectManager *glowObjectManager;
	ClientState *clientState;
	UtlVector<Entity *> *plantedC4s;

	bool *disablePostProcessing;

	std::add_pointer_t<void __fastcall(const char *)> loadSky;
	std::add_pointer_t<void __fastcall(const char *, const char *)> setClanTag;
	uintptr_t cameraThink;
	uintptr_t inventoryBlock;
	std::add_pointer_t<bool __stdcall(const char *)> acceptMatch;
	std::add_pointer_t<bool __cdecl(Vector, Vector, short)> lineGoesThroughSmoke;
	int(__thiscall *getSequenceActivity)(void *, int);
	bool(__thiscall *isOtherEnemy)(Entity *, Entity *);
	uintptr_t hud;
	int *(__thiscall *findHudElement)(uintptr_t, const char *);
	int(__thiscall *clearHudWeapon)(int *, int);
	std::add_pointer_t<ItemSystem *__cdecl()> itemSystem;
	void(__thiscall *setAbsOrigin)(Entity *, const Vector &);
	void(__thiscall *setAbsAngle)(Entity *, const Vector &);
	uintptr_t listLeaves;
	int *dispatchSound;
	int *checkFileCRC;
	int *doProceduralFootPlant;
	ViewRender *viewRender;
	uintptr_t drawScreenEffectMaterial;
	std::uint8_t *fakePrime;
	std::add_pointer_t<bool __stdcall(const char *, const char *)> submitReport;
	std::add_pointer_t<void __cdecl(const char *msg, ...)> debugMsg;
	std::add_pointer_t<void __cdecl(const std::array<std::uint8_t, 4> &color, const char *msg, ...)> conColorMsg;
	float *vignette;
	int(__thiscall *equipWearable)(void *wearable, void *player);
	int *predictionRandomSeed;
	MoveData *moveData;
	MoveHelper *moveHelper;
	std::uintptr_t keyValuesFromString;
	KeyValues *(__thiscall *keyValuesFindKey)(KeyValues *keyValues, const char *keyName, bool create);
	void(__thiscall *keyValuesSetString)(KeyValues *keyValues, const char *value);
	WeaponSystem *weaponSystem;
	std::add_pointer_t<const char **__fastcall(const char *playerModelName)> getPlayerViewmodelArmConfigForPlayerModel;
	GameEventDescriptor *(__thiscall *getEventDescriptor)(GameEventManager *_this, const char *name, int *cookie);
	ActiveChannels *activeChannels;
	Channel *channels;
	PlayerResource **playerResource;
	Entity **gameRules;
	ViewRenderBeams *viewRenderBeams;
	uintptr_t scopeDust;
	uintptr_t scopeArc;
	uintptr_t demoOrHLTV;
	uintptr_t money;
	uintptr_t demoFileEndReached;
	std::string* relayCluster;
	const wchar_t *(__thiscall *getDecoratedPlayerName)(PlayerResource *pr, int index, wchar_t *buffer, int buffsize, int flags);
	void(__thiscall *createState)(AnimState *state, Entity *);
	void(__vectorcall *updateState)(AnimState *state, void *, float z, float y, float x, void *);
	void(__fastcall *invalidateBoneCache)(Entity *);
	void(__thiscall *_setOrAddAttributeValueByName)(std::uintptr_t, const char *attribute);

	void setOrAddAttributeValueByName(std::uintptr_t attributeList, const char *attribute, float value) const noexcept
	{
		__asm movd xmm2, value
		_setOrAddAttributeValueByName(attributeList, attribute);
	}

	void setOrAddAttributeValueByName(std::uintptr_t attributeList, const char *attribute, int value) const noexcept
	{
		setOrAddAttributeValueByName(attributeList, attribute, *reinterpret_cast<float *>(&value));
	}
};

inline std::unique_ptr<const Memory> memory;
