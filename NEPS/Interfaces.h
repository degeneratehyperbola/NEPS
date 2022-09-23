#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <Windows.h>

class Client;
class Cvar;
class Effects;
class Engine;
class EngineTrace;
class EntityList;
class FileSystem;
class GameEventManager;
class GameMovement;
class GameUI;
class InputSystem;
class Localize;
class MaterialSystem;
class ModelInfo;
class ModelRender;
class NetworkStringTableContainer;
class Panel;
class PhysicsSurfaceProps;
class Prediction;
class RenderView;
class Surface;
class EngineSound;
class SoundEmitter;
class StudioRender;

class Interfaces
{
public:
	#define GAME_INTERFACE(type, name, module, version) \
	type* name = reinterpret_cast<type*>(find(##module, version));

	GAME_INTERFACE(Client, client, "client", "VClient018")
	GAME_INTERFACE(Cvar, cvar, "vstdlib", "VEngineCvar007")
	GAME_INTERFACE(Effects, effects, "engine", "VEngineEffects001")
	GAME_INTERFACE(Engine, engine, "engine", "VEngineClient014")
	GAME_INTERFACE(EngineTrace, engineTrace, "engine", "EngineTraceClient004")
	GAME_INTERFACE(EntityList, entityList, "client", "VClientEntityList003")
	GAME_INTERFACE(FileSystem, fileSystem, "filesystem_stdio", "VFileSystem017")
	GAME_INTERFACE(GameEventManager, gameEventManager, "engine", "GAMEEVENTSMANAGER002")
	GAME_INTERFACE(GameMovement, gameMovement, "client", "GameMovement001")
	GAME_INTERFACE(GameUI, gameUI, "client", "GameUI011")
	GAME_INTERFACE(InputSystem, inputSystem, "inputsystem", "InputSystemVersion001")
	GAME_INTERFACE(Localize, localize, "localize", "Localize_001")
	GAME_INTERFACE(MaterialSystem, materialSystem, "materialsystem", "VMaterialSystem080")
	GAME_INTERFACE(ModelInfo, modelInfo, "engine", "VModelInfoClient004")
	GAME_INTERFACE(ModelRender, modelRender, "engine", "VEngineModel016")
	GAME_INTERFACE(NetworkStringTableContainer, networkStringTableContainer, "engine", "VEngineClientStringTable001")
	GAME_INTERFACE(Panel, panel, "vgui2", "VGUI_Panel009")
	GAME_INTERFACE(PhysicsSurfaceProps, physicsSurfaceProps, "vphysics", "VPhysicsSurfaceProps001")
	GAME_INTERFACE(Prediction, prediction, "client", "VClientPrediction001")
	GAME_INTERFACE(RenderView, renderView, "engine", "VEngineRenderView014")
	GAME_INTERFACE(Surface, surface, "vguimatsurface", "VGUI_Surface031")
	GAME_INTERFACE(EngineSound, engineSound, "engine", "IEngineSoundClient003")
	GAME_INTERFACE(SoundEmitter, soundEmitter, "soundemittersystem", "VSoundEmitter003")
	GAME_INTERFACE(StudioRender, studioRender, "studiorender", "VStudioRender026")

	#undef GAME_INTERFACE
private:
	static void *find(const char *moduleName, const char *name) noexcept
	{
		if (const auto createInterface = reinterpret_cast<std::add_pointer_t<void *__cdecl(const char *name, int *returnCode)>>(GetProcAddress(GetModuleHandleA(moduleName), "CreateInterface")))
		{
			if (void *foundInterface = createInterface(name, nullptr))
				return foundInterface;
		}

		MessageBoxA(nullptr, ("Failed to find " + std::string{name} + " interface!").c_str(), "NEPS.PP", MB_OK | MB_ICONERROR);
		std::exit(EXIT_FAILURE);
	}
};

inline std::unique_ptr<const Interfaces> interfaces;
