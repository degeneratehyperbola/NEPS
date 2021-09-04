#include "NLoader.h"
#include "resource.h"

#include <Windows.h>

BOOL WINAPI WinMain(HINSTANCE instance, HINSTANCE previousInstance, char *cmdLine, int showCmd)
{
	const HMODULE moduleHandle = GetModuleHandleA(0);
	const HRSRC resource = FindResourceA(moduleHandle, MAKEINTRESOURCEA(IDR_BIN1), "BIN");
	const HGLOBAL resourceData = LoadResource(moduleHandle, resource);
	const void *dllBuffer = LockResource(resourceData);

	const auto csgoPid = NLoader::findPID(L"csgo.exe");
	if (!csgoPid)
		MessageBoxA(0, "Falied to load NEPS.\nYou need to run CS:GO before running the loader.", "NEPS", MB_OK | MB_ICONERROR);
	else
	{
		DWORD returnValue = 0;

		if (!NLoader::loadLibrary(dllBuffer, csgoPid, &returnValue))
			MessageBoxA(0, "Falied to load NEPS.\nInvalid PID.", "NEPS", MB_OK | MB_ICONERROR);
		else
		{
			if (!returnValue)
				MessageBoxA(0, "Falied to load NEPS.\nTry running the loader with administrator privileges.", "NEPS", MB_OK | MB_ICONERROR);
			else
				MessageBoxA(0, "Success! NEPS is now loaded.", "NEPS", MB_OK | MB_ICONINFORMATION);
		}
	}

	FreeResource(resourceData);

	return TRUE;
}
