#include <functional>
#include <intrin.h>

#include "lib/imgui/imgui_impl_dx9.h"
#include "lib/imgui/imgui_impl_win32.h"

#include "lib/minhook/minhook.h"

#include "EventListener.h"
#include "GameData.h"
#include "GUI.h"
#include "Hooks.h"

#include "Hacks/Aimbot.h"
#include "Hacks/Animations.h"
#include "Hacks/AntiAim.h"
#include "Hacks/Backtrack.h"
#include "Hacks/Chams.h"
#include "Hacks/EnginePrediction.h"
#include "Hacks/StreamProofESP.h"
#include "Hacks/Glow.h"
#include "Hacks/Misc.h"
#include "Hacks/Triggerbot.h"
#include "Hacks/Visuals.h"

#include "SDK/ClientClass.h"
#include "SDK/Cvar.h"
#include "SDK/Entity.h"
#include "SDK/FrameStage.h"
#include "SDK/GameUI.h"
#include "SDK/Input.h"
#include "SDK/InputSystem.h"
#include "SDK/ModelRender.h"
#include "SDK/Panel.h"
#include "SDK/Sound.h"
#include "SDK/StudioRender.h"
#include "SDK/Surface.h"
#include "SDK/ViewSetup.h"

#define FRAME_ADDRESS ((std::uintptr_t)_AddressOfReturnAddress() - sizeof(std::uintptr_t))
#define RETURN_ADDRESS std::uintptr_t(_ReturnAddress())

static LRESULT __stdcall wndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	[[maybe_unused]] static const auto initGlobal = [](HWND window) noexcept
	{
		netvars = std::make_unique<Netvars>();
		eventListener = std::make_unique<EventListener>();

		ImGui::CreateContext();
		ImGui_ImplWin32_Init(window);
		config = std::make_unique<Config>("NEPS");
		gui = std::make_unique<GUI>();
		hooks->install();

		const bool loaded = config->load(u8"default", false);

		gui->updateColors();
		SkinChanger::scheduleHudUpdate();
		
		std::ostringstream welcomeMsg;
		welcomeMsg << "Let's get started!\n";
		welcomeMsg << "To open GUI press \"";
		welcomeMsg << interfaces->inputSystem->virtualKeyToString(config->misc.menuKey);
		welcomeMsg << "\" on your keyboard.\n\n";
		welcomeMsg << "Configs are stored in Documents/NEPS/ directory.\n";
		welcomeMsg << "NEPS tries to load a config named \"default\" on start-up,\nand it appears that it was ";
		welcomeMsg << (loaded ? "loaded successfuly." : "not found.");

		interfaces->gameUI->createCommandMsgBox("Welcome to NEPS", welcomeMsg.str().c_str());

		return true;
	}(window);

	if (msg == WM_KEYDOWN && LOWORD(wParam) == config->misc.menuKey
		|| ((msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) && config->misc.menuKey == VK_LBUTTON)
		|| ((msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) && config->misc.menuKey == VK_RBUTTON)
		|| ((msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) && config->misc.menuKey == VK_MBUTTON)
		|| ((msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK) && config->misc.menuKey == HIWORD(wParam) + 4))
	{
		gui->open = !gui->open;
		if (!gui->open)
			interfaces->inputSystem->resetInputState();
	}

	LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ImGui_ImplWin32_WndProcHandler(window, msg, wParam, lParam);

	interfaces->inputSystem->enableInput(!gui->open);

	return CallWindowProcW(hooks->originalWndProc, window, msg, wParam, lParam);
}

static HRESULT __stdcall present(IDirect3DDevice9 *device, const RECT *src, const RECT *dest, HWND windowOverride, const RGNDATA *dirtyRegion) noexcept
{
	[[maybe_unused]] static const bool imguiInit = ImGui_ImplDX9_Init(device);

	if (config->loadScheduledFonts())
		ImGui_ImplDX9_DestroyFontsTexture();

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	Visuals::drawSmokeHull(ImGui::GetBackgroundDrawList());
	Visuals::drawMolotovHull(ImGui::GetBackgroundDrawList());
	Visuals::playerBounds(ImGui::GetBackgroundDrawList());
	Visuals::playerVelocity(ImGui::GetBackgroundDrawList());
	Misc::visualizeBlockBot(ImGui::GetBackgroundDrawList());

	StreamProofESP::render();

	AntiAim::visualize(ImGui::GetBackgroundDrawList());
	Visuals::hitMarker(nullptr, ImGui::GetBackgroundDrawList());
	Misc::visualizeInaccuracy(ImGui::GetBackgroundDrawList());
	Misc::recoilCrosshair(ImGui::GetBackgroundDrawList());
	Misc::overlayCrosshair(ImGui::GetBackgroundDrawList());

	Misc::purchaseList();
	Misc::teamDamageList();
	Misc::drawBombTimer();
	Misc::indicators(ImGui::GetBackgroundDrawList());

	#ifndef LEGACY_WATERMARK
	Misc::spectatorList();
	Misc::watermark();
	#endif // !LEGACY_WATERMARK

	gui->render();

	ImGui::EndFrame();
	ImGui::Render();

	if (device->BeginScene() == D3D_OK)
	{
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		device->EndScene();
	}

	return hooks->originalPresent(device, src, dest, windowOverride, dirtyRegion);
}

static HRESULT __stdcall reset(IDirect3DDevice9 *device, D3DPRESENT_PARAMETERS *params) noexcept
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	return hooks->originalReset(device, params);
}

static void __fastcall checkFileCRC() noexcept
{
	if (config->exploits.bypassPure)
		return;

	hooks->originalCheckFileCRC();
}

static bool previousSendPacket;
static UserCmd previousCmd;

static bool __stdcall createMove(float inputSampleTime, UserCmd *cmd) noexcept
{
	auto result = hooks->clientMode.callOriginal<bool, 24>(inputSampleTime, cmd);

	if (!cmd->commandNumber)
		return result;

	bool &sendPacket = *reinterpret_cast<bool *>(*reinterpret_cast<std::uintptr_t *>(FRAME_ADDRESS) - 0x1C);

	static auto previousViewAngles = cmd->viewangles;
	const auto currentViewAngles = cmd->viewangles;

	memory->globalVars->serverTime(cmd);
	Misc::changeConVarsTick();

	Misc::runReportbot();
	Misc::antiAfkKick(cmd);
	Misc::useSpam(cmd);
	Misc::autoPistol(cmd);
	Misc::autoReload(cmd);
	Misc::updateClanTag();
	Misc::fakeBan();
	Misc::stealNames();
	Misc::revealRanks(cmd);
	Misc::quickReload(cmd);
	Misc::quickHealthshot(cmd);
	Misc::fixTabletSignal();
	Misc::removeCrouchCooldown(cmd);
	Misc::fastStop(cmd);
	Misc::autoStrafe(cmd);
	Misc::bunnyHop(cmd);
	Misc::prepareRevolver(cmd);
	Aimbot::predictPeek(cmd);
	if (static Helpers::KeyBindState flag; flag[config->exploits.slowwalk]) Misc::slowwalk(cmd);

	EnginePrediction::run(cmd);

	Aimbot::run(cmd);
	Backtrack::run(cmd);
	Triggerbot::run(cmd);
	Misc::edgeJump(cmd);
	Misc::blockBot(cmd, currentViewAngles);
	Misc::fastPlant(cmd);

	AntiAim::run(cmd, currentViewAngles, sendPacket);

	auto viewAnglesDelta = cmd->viewangles - previousViewAngles;
	viewAnglesDelta.normalize();
	viewAnglesDelta.x = std::clamp(viewAnglesDelta.x, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
	viewAnglesDelta.y = std::clamp(viewAnglesDelta.y, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);

	cmd->viewangles = previousViewAngles + viewAnglesDelta;

	cmd->viewangles.normalize();

	Misc::fixMovement(cmd, currentViewAngles.y);
	Misc::moonwalk(cmd);
	Visuals::physicsTimescale();

	cmd->viewangles.x = std::clamp(cmd->viewangles.x, -89.0f, 89.0f);
	cmd->viewangles.y = std::clamp(cmd->viewangles.y, -180.0f, 180.0f);
	cmd->viewangles.z = 0.0f;
	cmd->forwardmove = std::clamp(cmd->forwardmove, -450.0f, 450.0f);
	cmd->sidemove = std::clamp(cmd->sidemove, -450.0f, 450.0f);

	bool fakePitchPerformed = AntiAim::fakePitch(cmd);

	previousViewAngles = cmd->viewangles;

	previousSendPacket = sendPacket;
	previousCmd = *cmd;

	if (fakePitchPerformed)
		cmd->viewangles.x = -89.0f; // Fake pitch visualization
	Animations::desyncedAnimations(*cmd, sendPacket);
	cmd->viewangles.x = previousViewAngles.x; // Restore view angles after visualizing fake pitch

	return false;
}

static int __stdcall doPostScreenEffects(int param) noexcept
{
	if (interfaces->engine->isInGame())
	{
		Visuals::thirdperson();
		Visuals::reduceFlashEffect();
		Glow::render();
	}
	return hooks->clientMode.callOriginal<int, 44>(param);
}

static float __stdcall getViewModelFov() noexcept
{
	float additionalFov = static_cast<float>(config->visuals.viewmodel.fov);
	if (localPlayer)
	{
		if (const auto activeWeapon = localPlayer->getActiveWeapon(); activeWeapon && activeWeapon->getClientClass()->classId == ClassId::Tablet || !config->visuals.viewmodel.enabled || localPlayer->isScoped())
			additionalFov = 0.0f;
	}

	return hooks->clientMode.callOriginal<float, 35>() + additionalFov;
}

static void __stdcall drawModelExecute(void *ctx, void *state, const ModelRenderInfo &info, Matrix3x4 *customBoneToWorld) noexcept
{
	if (interfaces->studioRender->isForcedMaterialOverride())
		return hooks->modelRender.callOriginal<void, 21>(ctx, state, std::cref(info), customBoneToWorld);

	if (Visuals::removeHands(info.model->name) || Visuals::removeSleeves(info.model->name) || Visuals::removeWeapons(info.model->name))
		return;

	static Chams chams;
	if (!chams.render(ctx, state, info, customBoneToWorld))
		hooks->modelRender.callOriginal<void, 21>(ctx, state, std::cref(info), customBoneToWorld);
	interfaces->studioRender->forcedMaterialOverride(nullptr);
}

static bool __fastcall svCheatsGetBool(void *thisptr) noexcept
{
	if (RETURN_ADDRESS == memory->cameraThink && config->visuals.thirdPerson.keyMode)
		return true;

	return hooks->svCheats.getOriginal<bool, 13>()(thisptr);
}

static void __stdcall paintTraverse(unsigned int panel, bool forceRepaint, bool allowForce) noexcept
{
	if (interfaces->panel->getName(panel) == "MatSystemTopPanel")
	{
		#ifdef LEGACY_WATERMARK
		Misc::spectatorList();
		Misc::watermark();
		#endif // LEGACY_WATERMARK
	}

	hooks->panel.callOriginal<void, 41>(panel, forceRepaint, allowForce);
}

static void __stdcall frameStageNotify(FrameStage stage) noexcept
{
	if (interfaces->engine->isConnected() && !interfaces->engine->isInGame())
		Misc::changeName(true, nullptr, 0.0f);

	Misc::changeConVarsFrame(stage);

	switch (stage)
	{
	case FrameStage::UNDEFINED:
		break;
	case FrameStage::START:
		GameData::update();
		break;
	case FrameStage::NET_UPDATE_START:
		break;
	case FrameStage::NET_UPDATE_POSTDATAUPDATE_START:
		break;
	case FrameStage::NET_UPDATE_POSTDATAUPDATE_END:
		break;
	case FrameStage::NET_UPDATE_END:
		break;
	case FrameStage::RENDER_START:
		Animations::fixAnimation(previousCmd, previousSendPacket);
		Misc::preserveKillfeed();
		Visuals::colorWorld();
		Misc::forceRelayCluster();
		break;
	case FrameStage::RENDER_END:
		break;
	}

	if (interfaces->engine->isInGame())
	{
		Visuals::flashlight(stage);
		Visuals::skybox(stage);
		Visuals::removeBlur(stage);
		Visuals::removeGrass(stage);
		Visuals::modifySmoke(stage);
		Visuals::modifyFire(stage);
		Visuals::playerModel(stage);
		Visuals::disablePostProcessing(stage);
		Visuals::removeVisualRecoil(stage);
		Misc::tweakPlayerAnim(stage);
		Backtrack::update(stage);
		SkinChanger::run(stage);
	}

	hooks->client.callOriginal<void, 37>(stage);
}

static int __stdcall emitSound(SoundParams params) noexcept
{
	auto modulateVolume = [&params](int(*get)(int))
	{
		if (const auto entity = interfaces->entityList->getEntity(params.entityIndex); localPlayer && entity && entity->isPlayer())
		{
			if (params.entityIndex == localPlayer->index())
				params.volume *= get(0) / 100.0f;
			else if (!entity->isOtherEnemy(localPlayer.get()))
				params.volume *= get(1) / 100.0f;
			else
				params.volume *= get(2) / 100.0f;
		}
	};

	modulateVolume([](int index) { return config->sound.players[index].masterVolume; });

	if (strstr(params.soundEntry, "Weapon") && strstr(params.soundEntry, "Single"))
	{
		modulateVolume([](int index) { return config->sound.players[index].weaponVolume; });
	} else if (config->misc.autoAccept && !strcmp(params.soundEntry, "UIPanorama.popup_accept_match_beep"))
	{
		memory->acceptMatch("");
		auto window = hooks->getProcessWindow();
		FLASHWINFO flash{sizeof(FLASHWINFO), window, FLASHW_TRAY | FLASHW_TIMERNOFG, 0, 0};
		FlashWindowEx(&flash);
		ShowWindow(window, SW_RESTORE);
	}
	params.volume = std::clamp(params.volume, 0.0f, 1.0f);
	return hooks->sound.callOriginal<int, 5>(params);
}

static bool __stdcall shouldDrawFog() noexcept
{
	if constexpr (std::is_same_v<HookType, MinHook>)
	{
		#ifdef NEPS_DEBUG
		// Check if we always get the same return address
		if (*static_cast<std::uint32_t *>(_ReturnAddress()) == 0x6274C084)
		{
			static const auto returnAddress = RETURN_ADDRESS;
			assert(returnAddress == RETURN_ADDRESS);
		}
		#endif // NEPS_DEBUG

		if (*static_cast<std::uint32_t *>(_ReturnAddress()) != 0x6274C084)
			return hooks->clientMode.callOriginal<bool, 17>();
	}

	return !config->visuals.noFog;
}

static bool __stdcall shouldDrawViewModel() noexcept
{
	if (localPlayer && localPlayer->fov() < 45 && localPlayer->fovStart() < 45)
		return false;
	return hooks->clientMode.callOriginal<bool, 27>();
}

static void __stdcall lockCursor() noexcept
{
	if (gui->open)
		return interfaces->surface->unlockCursor();
	return hooks->surface.callOriginal<void, 67>();
}

static void __stdcall setDrawColor(int r, int g, int b, int a) noexcept
{
	if (config->visuals.noScopeOverlay && (RETURN_ADDRESS == memory->scopeDust || RETURN_ADDRESS == memory->scopeArc))
		a = 0;

	hooks->surface.callOriginal<void, 15>(r, g, b, a);
}

static void __stdcall overrideView(ViewSetup *setup) noexcept
{
	const float fov = static_cast<float>(config->visuals.fov);
	const float zoomFov = fov * (1.0f - (static_cast<float>(config->visuals.zoomFac) / 100.0f));
	static float curFov = fov;
	static Helpers::KeyBindState zoom;
	curFov = zoom[config->visuals.zoom] ?
		Helpers::approachValSmooth(zoomFov, curFov, memory->globalVars->frametime * 10.0f) :
		Helpers::approachValSmooth(fov, curFov, memory->globalVars->frametime * 10.0f);

	if (localPlayer)
	{
		constexpr auto setViewmodel = [](Entity *viewModel, const Vector &angles) constexpr noexcept
		{
			if (viewModel)
			{
				Vector forward = Vector::fromAngle(angles);
				Vector up = Vector::fromAngle(angles - Vector{90.0f, 0.0f, 0.0f});
				Vector side = forward.crossProduct(up);
				Vector offset = side * config->visuals.viewmodel.x + forward * config->visuals.viewmodel.y + up * config->visuals.viewmodel.z;
				memory->setAbsOrigin(viewModel, viewModel->getRenderOrigin() + offset);
				memory->setAbsAngle(viewModel, angles + Vector{0.0f, 0.0f, config->visuals.viewmodel.roll});
			}
		};

		if (localPlayer->isAlive())
		{
			if ((!localPlayer->isScoped() || config->visuals.forceFov))
				setup->fov = curFov;

			if (static Helpers::KeyBindState fakeDuck; fakeDuck[config->exploits.fakeDuck])
				setup->origin.z = localPlayer->origin().z + PLAYER_EYE_HEIGHT;

			if (config->visuals.viewmodel.enabled && !localPlayer->isScoped() && !memory->input->isCameraInThirdPerson)
				setViewmodel(interfaces->entityList->getEntityFromHandle(localPlayer->viewModel()), setup->angles);
		} else if (auto observed = localPlayer->getObserverTarget(); observed && localPlayer->getObserverMode() == ObsMode::InEye)
		{
			if ((!observed->isScoped() || config->visuals.forceFov))
				setup->fov = curFov;

			if (config->visuals.viewmodel.enabled && !observed->isScoped())
				setViewmodel(interfaces->entityList->getEntityFromHandle(observed->viewModel()), setup->angles);
		} else
		{
			setup->fov = curFov;
		}
	}

	setup->farZ = float(config->visuals.farZ * 10);
	hooks->clientMode.callOriginal<void, 18>(setup);
}

struct RenderableInfo
{
	Entity *renderable;
	std::byte pad[18];
	uint16_t flags;
	uint16_t flags2;
};

static int __stdcall listLeavesInBox(const Vector &mins, const Vector &maxs, unsigned short *list, int listMax) noexcept
{
	if (RETURN_ADDRESS == memory->listLeaves)
	{
		if (const auto info = *reinterpret_cast<RenderableInfo **>(FRAME_ADDRESS + 0x18); info && info->renderable)
		{
			if (const auto ent = VirtualMethod::call<Entity *, 7>(info->renderable - 4); ent && ent->isPlayer())
			{
				if (config->misc.disableModelOcclusion)
				{
					// FIXME: sometimes players are rendered above smoke, maybe sort render list?
					info->flags &= ~0x100;
					info->flags2 |= 0x40;

					constexpr float maxCoord = 16384.0f;
					constexpr float minCoord = -maxCoord;
					constexpr Vector min = {minCoord, minCoord, minCoord};
					constexpr Vector max = {maxCoord, maxCoord, maxCoord};
					return hooks->bspQuery.callOriginal<int, 6>(std::cref(min), std::cref(max), list, listMax);
				}
			}
		}
	}
	return hooks->bspQuery.callOriginal<int, 6>(std::cref(mins), std::cref(maxs), list, listMax);
}

static int __fastcall dispatchSound(SoundInfo &soundInfo) noexcept
{
	if (const char *soundName = interfaces->soundEmitter->getSoundName(soundInfo.soundIndex))
	{
		auto modulateVolume = [&soundInfo](int(*get)(int))
		{
			if (auto entity{interfaces->entityList->getEntity(soundInfo.entityIndex)}; entity && entity->isPlayer())
			{
				if (localPlayer && soundInfo.entityIndex == localPlayer->index())
					soundInfo.volume *= get(0) / 100.0f;
				else if (!entity->isOtherEnemy(localPlayer.get()))
					soundInfo.volume *= get(1) / 100.0f;
				else
					soundInfo.volume *= get(2) / 100.0f;
			}
		};

		modulateVolume([](int index) { return config->sound.players[index].masterVolume; });

		if (!strcmp(soundName, "Player.DamageHelmetFeedback"))
			modulateVolume([](int index) { return config->sound.players[index].headshotVolume; });
		else if (strstr(soundName, "Step"))
			modulateVolume([](int index) { return config->sound.players[index].footstepVolume; });
		else if (strstr(soundName, "Chicken"))
			soundInfo.volume *= config->sound.chickenVolume / 100.0f;
	}
	soundInfo.volume = std::clamp(soundInfo.volume, 0.0f, 1.0f);
	return hooks->originalDispatchSound(soundInfo);
}

static int __stdcall render2dEffectsPreHud(int param) noexcept
{
	Visuals::applyScreenEffects();
	Visuals::hitEffect();
	Visuals::killEffect();
	return hooks->viewRender.callOriginal<int, 39>(param);
}

static const DemoPlaybackParameters *__stdcall getDemoPlaybackParameters() noexcept
{
	const auto params = hooks->engine.callOriginal<const DemoPlaybackParameters *, 218>();

	#ifdef NEPS_DEBUG
	// Check if we always get the same return address
	if (*static_cast<std::uint64_t *>(_ReturnAddress()) == 0x79801F74C985C88B)
	{
		static const auto returnAddress = RETURN_ADDRESS;
		assert(returnAddress == RETURN_ADDRESS);
	}
	#endif // NEPS_DEBUG

	if (params && config->misc.revealSuspect && *static_cast<std::uint64_t *>(_ReturnAddress()) != 0x79801F74C985C88B) // client.dll : 8B C8 85 C9 74 1F 80 79 10 00 , there game decides whether to show overwatch panel
	{
		static DemoPlaybackParameters customParams;
		customParams = *params;
		customParams.anonymousPlayerIdentity = false;
		return &customParams;
	}

	return params;
}

static bool __stdcall isPlayingDemo() noexcept
{
	if (config->misc.revealMoney && RETURN_ADDRESS == memory->demoOrHLTV && *reinterpret_cast<std::uintptr_t *>(FRAME_ADDRESS + 8) == memory->money)
		return true;

	return hooks->engine.callOriginal<bool, 82>();
}

static void __stdcall updateColorCorrectionWeights() noexcept
{
	hooks->clientMode.callOriginal<void, 58>();

	if (const auto &cfg = config->visuals.colorCorrection; cfg.enabled)
	{
		*reinterpret_cast<float *>(std::uintptr_t(memory->clientMode) + 0x49C) = cfg.blue;
		*reinterpret_cast<float *>(std::uintptr_t(memory->clientMode) + 0x4A4) = cfg.red;
		*reinterpret_cast<float *>(std::uintptr_t(memory->clientMode) + 0x4AC) = cfg.mono;
		*reinterpret_cast<float *>(std::uintptr_t(memory->clientMode) + 0x4B4) = cfg.saturation;
		*reinterpret_cast<float *>(std::uintptr_t(memory->clientMode) + 0x4C4) = cfg.ghost;
		*reinterpret_cast<float *>(std::uintptr_t(memory->clientMode) + 0x4CC) = cfg.green;
		*reinterpret_cast<float *>(std::uintptr_t(memory->clientMode) + 0x4D4) = cfg.yellow;
	}

	if (config->visuals.noScopeOverlay)
		*memory->vignette = 0.0f;
}

static float __stdcall getScreenAspectRatio(int width, int height) noexcept
{
	if (config->visuals.aspectratio)
		return config->visuals.aspectratio;
	return hooks->engine.callOriginal<float, 101>(width, height);
}

static void __stdcall renderSmokeOverlay(bool update) noexcept
{
	if (config->visuals.smoke)
		*reinterpret_cast<float *>(std::uintptr_t(memory->viewRender) + 0x588) = 0.0f;
	else
		hooks->viewRender.callOriginal<void, 41>(update);
}

static bool __stdcall isConnected() noexcept
{
	if (config->misc.unlockInvertory && RETURN_ADDRESS == memory->invertoryBlock)
		return false;

	return hooks->engine.callOriginal<bool, 27>();
}

Hooks::Hooks(HMODULE moduleHandle) noexcept
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    this->moduleHandle = moduleHandle;

    // Interfaces and memory shouldn't be initialized in wndProc because they show MessageBox on error which would cause deadlock
    interfaces = std::make_unique<const Interfaces>();
    memory = std::make_unique<const Memory>();

    window = FindWindowW(L"Valve001", nullptr);
    originalWndProc = WNDPROC(SetWindowLongPtrW(window, GWLP_WNDPROC, LONG_PTR(wndProc)));
}

void Hooks::install() noexcept
{
	originalPresent = **reinterpret_cast<decltype(originalPresent) **>(memory->present);
	**reinterpret_cast<decltype(present) ***>(memory->present) = present;
	originalReset = **reinterpret_cast<decltype(originalReset) **>(memory->reset);
	**reinterpret_cast<decltype(reset) ***>(memory->reset) = reset;

	if constexpr (std::is_same_v<HookType, MinHook>)
		MH_Initialize();

	bspQuery.init(interfaces->engine->getBSPTreeQuery());
	client.init(interfaces->client);
	clientMode.init(memory->clientMode);
	engine.init(interfaces->engine);
	modelRender.init(interfaces->modelRender);
	panel.init(interfaces->panel);
	sound.init(interfaces->sound);
	surface.init(interfaces->surface);
	svCheats.init(interfaces->cvar->findVar("sv_cheats"));
	viewRender.init(memory->viewRender);

	bspQuery.hookAt(6, listLeavesInBox);
	client.hookAt(37, frameStageNotify);
	clientMode.hookAt(17, shouldDrawFog);
	clientMode.hookAt(18, overrideView);
	clientMode.hookAt(24, createMove);
	clientMode.hookAt(27, shouldDrawViewModel);
	clientMode.hookAt(35, getViewModelFov);
	clientMode.hookAt(44, doPostScreenEffects);
	clientMode.hookAt(58, updateColorCorrectionWeights);
	engine.hookAt(27, isConnected);
	engine.hookAt(82, isPlayingDemo);
	engine.hookAt(101, getScreenAspectRatio);
	engine.hookAt(218, getDemoPlaybackParameters);
	modelRender.hookAt(21, drawModelExecute);
	panel.hookAt(41, paintTraverse);
	sound.hookAt(5, emitSound);
	surface.hookAt(15, setDrawColor);
	surface.hookAt(67, lockCursor);
	svCheats.hookAt(13, svCheatsGetBool);
	viewRender.hookAt(39, render2dEffectsPreHud);
	viewRender.hookAt(41, renderSmokeOverlay);

	if (DWORD oldProtection; VirtualProtect(memory->dispatchSound, 4, PAGE_EXECUTE_READWRITE, &oldProtection))
	{
		originalDispatchSound = decltype(originalDispatchSound)(uintptr_t(memory->dispatchSound + 1) + *memory->dispatchSound);
		*memory->dispatchSound = uintptr_t(dispatchSound) - uintptr_t(memory->dispatchSound + 1);
		VirtualProtect(memory->dispatchSound, 4, oldProtection, nullptr);
	}

	if (DWORD oldProtection; VirtualProtect(memory->checkFileCRC, 4, PAGE_EXECUTE_READWRITE, &oldProtection))
	{
		originalCheckFileCRC = decltype(originalCheckFileCRC)(uintptr_t(memory->checkFileCRC + 1) + *memory->checkFileCRC);
		*memory->checkFileCRC = uintptr_t(checkFileCRC) - uintptr_t(memory->checkFileCRC + 1);
		VirtualProtect(memory->checkFileCRC, 4, oldProtection, nullptr);
	}

	if constexpr (std::is_same_v<HookType, MinHook>)
		MH_EnableHook(MH_ALL_HOOKS);
}

extern "C" BOOL WINAPI _CRT_INIT(HMODULE moduleHandle, DWORD reason, LPVOID reserved);

static DWORD WINAPI unload(HMODULE moduleHandle) noexcept
{
	Sleep(100);

	Animations::releaseState();
	interfaces->inputSystem->enableInput(true);
	eventListener->remove();

	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	_CRT_INIT(moduleHandle, DLL_PROCESS_DETACH, nullptr);

	FreeLibraryAndExitThread(moduleHandle, 0);
}

void Hooks::uninstall() noexcept
{
	if constexpr (std::is_same_v<HookType, MinHook>)
	{
		MH_DisableHook(MH_ALL_HOOKS);
		MH_Uninitialize();
	}

	bspQuery.restore();
	panel.restore();
	client.restore();
	clientMode.restore();
	engine.restore();
	modelRender.restore();
	sound.restore();
	surface.restore();
	svCheats.restore();
	viewRender.restore();

	netvars->restore();

	Glow::clearCustomObjects();

	SetWindowLongPtrW(window, GWLP_WNDPROC, LONG_PTR(originalWndProc));
	**reinterpret_cast<void ***>(memory->present) = originalPresent;
	**reinterpret_cast<void ***>(memory->reset) = originalReset;

	if (DWORD oldProtection; VirtualProtect(memory->dispatchSound, 4, PAGE_EXECUTE_READWRITE, &oldProtection))
	{
		*memory->dispatchSound = uintptr_t(originalDispatchSound) - uintptr_t(memory->dispatchSound + 1);
		VirtualProtect(memory->dispatchSound, 4, oldProtection, nullptr);
	}

	if (DWORD oldProtection; VirtualProtect(memory->checkFileCRC, 4, PAGE_EXECUTE_READWRITE, &oldProtection))
	{
		*memory->checkFileCRC = uintptr_t(originalCheckFileCRC) - uintptr_t(memory->checkFileCRC + 1);
		VirtualProtect(memory->checkFileCRC, 4, oldProtection, nullptr);
	}

	if (HANDLE thread = CreateThread(nullptr, 0, LPTHREAD_START_ROUTINE(unload), moduleHandle, 0, nullptr))
		CloseHandle(thread);
}
