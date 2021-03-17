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
class MemAlloc;

struct AnimState;
struct ActiveChannels;
struct Channel;
struct GlobalVars;
struct GlowObjectManager;
struct Trace;
struct Vector;
struct UserCmd;
struct ViewRenderBeams;

static std::pair<void *, std::size_t> getModuleInformation(const char *name) noexcept
{
	if (HMODULE handle = GetModuleHandleA(name))
	{
		if (MODULEINFO moduleInfo; GetModuleInformation(GetCurrentProcess(), handle, &moduleInfo, sizeof(moduleInfo)))
			return std::make_pair(moduleInfo.lpBaseOfDll, moduleInfo.SizeOfImage);
	}
	return {};
}

class Memory {
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
	std::add_pointer_t<bool __cdecl(float, float, float, float, float, float, Trace &)> traceToExit;
	ViewRender *viewRender;
	uintptr_t drawScreenEffectMaterial;
    std::add_pointer_t<bool __stdcall(const char*, const char*)> submitReport;
	uint8_t *fakePrime;
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
    ActiveChannels* activeChannels;
	Channel *channels;
	PlayerResource **playerResource;
	Entity **gameRules;
	uintptr_t scopeDust;
	uintptr_t scopeArc;
	uintptr_t demoOrHLTV;
	uintptr_t money;
	uintptr_t demoFileEndReached;
	const wchar_t *(__thiscall *getDecoratedPlayerName)(PlayerResource *pr, int index, wchar_t *buffer, int buffsize, int flags);
	//void(__fastcall *sendMove)();
	//void  writeUserCmdDeltaToBufferReturn;
	//void(__fastcall *writeUserCmd)(void *buf, UserCmd *in, UserCmd *out);
	void(__thiscall *createState)(AnimState *state, Entity *);
	void(__vectorcall *updateState)(AnimState *state, void *, float x, float y, float z, void *);
	void(__fastcall *invalidateBoneCache)(Entity *);

	MemAlloc *memalloc;
	ViewRenderBeams *viewRenderBeams;

	[[nodiscard]] static auto generateBadCharTable(std::string_view pattern) noexcept
	{
		assert(!pattern.empty());

		std::array<std::size_t, (std::numeric_limits<std::uint8_t>::max)() + 1> table;

		auto lastWildcard = pattern.rfind('?');
		if (lastWildcard == std::string_view::npos)
			lastWildcard = 0;

		const auto defaultShift = (std::max)(std::size_t(1), pattern.length() - 1 - lastWildcard);
		table.fill(defaultShift);

		for (auto i = lastWildcard; i < pattern.length() - 1; ++i)
			table[static_cast<std::uint8_t>(pattern[i])] = pattern.length() - 1 - i;

		return table;
	}

	static std::uintptr_t findPattern(const char *moduleName, std::string pattern) noexcept
	{
		static auto id = 0;
		++id;

		const auto [moduleBase, moduleSize] = getModuleInformation(moduleName);

		if (moduleBase && moduleSize)
		{
			const auto lastIdx = pattern.length() - 1;
			const auto badCharTable = generateBadCharTable(pattern);

			auto start = static_cast<const char *>(moduleBase);
			const auto end = start + moduleSize - pattern.length();

			while (start <= end)
			{
				int i = lastIdx;
				while (i >= 0 && (pattern[i] == '?' || start[i] == pattern[i]))
					--i;

				if (i < 0)
					return reinterpret_cast<std::uintptr_t>(start);

				start += badCharTable[static_cast<std::uint8_t>(start[lastIdx])];
			}
		}

		#ifdef _WIN32
		MessageBoxA(NULL, ("Failed to find pattern #" + std::to_string(id) + '!').c_str(), "NEPS", MB_OK | MB_ICONWARNING);
		#endif

		return 0;
	}

	static std::uintptr_t findPattern(const wchar_t *moduleName, const char *pattern) noexcept
	{
		static auto id = 0;
		++id;

		if (HMODULE moduleHandle = GetModuleHandleW(moduleName))
		{
			if (MODULEINFO moduleInfo; GetModuleInformation(GetCurrentProcess(), moduleHandle, &moduleInfo, sizeof(moduleInfo)))
			{
				auto start = static_cast<const char *>(moduleInfo.lpBaseOfDll);
				const auto end = start + moduleInfo.SizeOfImage;
				auto first = start;
				auto second = pattern;
				while (first < end && *second)
				{
					if (*first == *second || *second == '?')
					{
						++first;
						++second;
					} else
					{
						first = ++start;
						second = pattern;
					}
				}
				if (!*second)
					return reinterpret_cast<std::uintptr_t>(start);
			}
		}

		#ifdef _WIN32
		MessageBoxA(NULL, ("Failed to find pattern #" + std::to_string(id) + '!').c_str(), "NEPS", MB_OK | MB_ICONWARNING);
		#endif

		return 0;
	}
};

inline std::unique_ptr<const Memory> memory;
