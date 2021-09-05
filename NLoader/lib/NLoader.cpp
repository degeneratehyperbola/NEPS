#include "NLoader.hpp"
#include "../resource.h"

#include <Windows.h>
#include <TlHelp32.h>

void *NLoader::loadFromResource(std::size_t *size) noexcept
{
	const HMODULE moduleHandle = GetModuleHandle(0);
	const HRSRC resource = FindResource(moduleHandle, MAKEINTRESOURCE(IDR_BIN1), L"BIN");
	if (!resource) return nullptr;

	if (size)
		*size = SizeofResource(moduleHandle, resource);

	return LoadResource(moduleHandle, resource);
}

unsigned long NLoader::findPid(const std::wstring &processName) noexcept
{
	PROCESSENTRY32 processInfo;
	processInfo.dwSize = sizeof(processInfo);

	HANDLE processSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (processSnapshot == INVALID_HANDLE_VALUE)
		return 0;

	Process32First(processSnapshot, &processInfo);
	if (!processName.compare(processInfo.szExeFile))
	{
		CloseHandle(processSnapshot);
		return processInfo.th32ProcessID;
	}

	while (Process32Next(processSnapshot, &processInfo))
	{
		if (!processName.compare(processInfo.szExeFile))
		{
			CloseHandle(processSnapshot);
			return processInfo.th32ProcessID;
		}
	}

	CloseHandle(processSnapshot);
	return 0;
}
