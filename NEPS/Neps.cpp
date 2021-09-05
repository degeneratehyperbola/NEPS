#include <clocale>
#include "Hooks.h"

extern "C" BOOL WINAPI _CRT_INIT(HMODULE moduleHandle, DWORD reason, LPVOID reserved);

static DWORD WINAPI unload(HMODULE moduleHandle) noexcept
{
	Sleep(100);
	_CRT_INIT(moduleHandle, DLL_PROCESS_DETACH, nullptr);
	FreeLibraryAndExitThread(moduleHandle, 0);
}

BOOL APIENTRY DllEntryPoint(HMODULE moduleHandle, DWORD reason, LPVOID reserved)
{
	if (!_CRT_INIT(moduleHandle, reason, reserved))
		return FALSE;

	if (reason == DLL_PROCESS_ATTACH)
	{
		std::setlocale(LC_CTYPE, ".utf8");
		hooks = std::make_unique<Hooks>(moduleHandle);
		return TRUE;
	}

    return FALSE;
}
