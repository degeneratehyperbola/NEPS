#include <clocale>
#include <Windows.h>
#include "Hooks.h"

// Last digit is always 0. Free byte for actual info
constexpr DWORD SIGNATURE = 0xEAC8FC70;

extern "C" BOOL WINAPI _CRT_INIT(HMODULE moduleHandle, DWORD reason, LPVOID reserved);

BOOL APIENTRY DllEntryPoint(HMODULE moduleHandle, DWORD reason, LPVOID reserved)
{
    if (!_CRT_INIT(moduleHandle, reason & 3, reserved))
        return FALSE;

	#define NS_DEBUG
	#ifdef NS_DEBUG
	if (reason & DLL_PROCESS_ATTACH)
	#else
	if (reason & DLL_PROCESS_ATTACH && reason & SIGNATURE)
	#endif // NS_DEBUG
	{
		std::setlocale(LC_CTYPE, ".utf8");
		hooks = std::make_unique<Hooks>(moduleHandle);
	} else if (reason & DLL_PROCESS_ATTACH)
	{
		MessageBoxA(nullptr, "Oh no! Somehow DLL got injected outside of loader. I'm sorry about that.", "NEPS.PP", MB_OK | MB_ICONERROR);
		_CRT_INIT(moduleHandle, DLL_PROCESS_DETACH, nullptr);
		FreeLibrary(moduleHandle);
	}

    return TRUE;
}
