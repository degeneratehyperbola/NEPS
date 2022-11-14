#pragma once

#include <d3d9.h>
#include <memory>
#include <type_traits>
#include <Windows.h>

#include "Hooks/minhook.h"
#include "Hooks/VmtHook.h"
#include "Hooks/VmtSwap.h"

#include "FootprintCleaner.hpp"

struct SoundInfo;
struct Vector;

// Easily switch hooking method for all hooks, choose between MinHook/VmtHook/VmtSwap
using HookType = MinHook;

class Hooks
{
public:
	Hooks(HMODULE moduleHandle) noexcept;

	void install() noexcept;
	void uninstall() noexcept;

	HMODULE getDllHandle() noexcept { return moduleHandle; }

	HWND getProcessWindow() noexcept { return window; }

	WNDPROC originalWndProc;
	std::add_pointer_t<HRESULT __stdcall(IDirect3DDevice9 *, const RECT *, const RECT *, HWND, const RGNDATA *)> originalPresent;
	std::add_pointer_t<HRESULT __stdcall(IDirect3DDevice9 *, D3DPRESENT_PARAMETERS *)> originalReset;
	std::add_pointer_t<int __fastcall(SoundInfo &)> originalDispatchSound;
	std::add_pointer_t<void __fastcall()> originalCheckFileCRC;
	std::add_pointer_t<void __fastcall(void *, void *, void *, void *, void *, void *)> originalDoProceduralFootPlant;

	HookType bspQuery;
	HookType client;
	HookType clientMode;
	HookType engine;
	HookType modelRender;
	HookType panel;
	HookType engineSound;
	HookType surface;
	HookType viewRender;
	HookType svCheats;
private:
	HMODULE moduleHandle;
	HWND window;
};

inline std::unique_ptr<FootprintCleaner> footprintCleaner;
inline std::unique_ptr<Hooks> hooks;
