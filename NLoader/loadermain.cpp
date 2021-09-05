#include "lib/Helpers.hpp"
#include "resource.h"

#include <Windows.h>
#include <ctime>
#include <filesystem>

BOOL RemoteLoadStreamLibraryW(ULONG, PBYTE, LPCWSTR), RemoteLoadStreamLibraryA(ULONG, PBYTE, LPCSTR);

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previousInstance, LPSTR cmdLine, int showCmd)
{
	std::size_t size;
	const auto resource = NLoader::loadFromResource(&size);
	const void *dllData = LockResource(resource);

	const auto csgoPid = NLoader::findPid(L"csgo.exe");
	if (!csgoPid)
		MessageBoxA(0, "Falied to load NEPS.\nYou need to run CS:GO before running the loader.", "NEPS", MB_OK | MB_ICONERROR);
	else
	{
		wchar_t tempPath[MAX_PATH];
		std::size_t charCount = GetTempPath(MAX_PATH, tempPath);

		for (const auto &entry : std::filesystem::directory_iterator(tempPath))
		{
			if (entry.is_regular_file() && entry.path().filename().wstring().ends_with(L".neps.tempalloc"))
				std::filesystem::remove(entry.path());
		}

		wchar_t tempFile[MAX_PATH];
		std::srand(static_cast<int>(std::time(0)));
		std::swprintf(tempFile, MAX_PATH, L"%ls\\%i.neps.tempalloc", tempPath, std::rand());

		if (!RemoteLoadStreamLibraryW(csgoPid, (PBYTE)dllData, tempFile))
			MessageBoxA(0, "Falied to load NEPS.\nUnknown error occured while loading library.", "NEPS", MB_OK | MB_ICONERROR);
		else
			MessageBoxA(0, "Success! NEPS is now loaded.", "NEPS", MB_OK | MB_ICONINFORMATION);
	}

	FreeResource(resource);

	return TRUE;
}
