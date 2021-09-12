#include "Helpers.hpp"
#include "../resource.h"

#include <Windows.h>
#include <TlHelp32.h>

void *Helpers::loadFromResource(std::size_t *size) noexcept
{
	HMODULE moduleHandle = GetModuleHandle(0);
	HRSRC resource = FindResource(moduleHandle, MAKEINTRESOURCE(IDR_BIN1), L"BIN");
	if (!resource) return nullptr;

	if (size)
		*size = SizeofResource(moduleHandle, resource);

	return LoadResource(moduleHandle, resource);
}

unsigned long Helpers::findPid(const std::wstring &processName) noexcept
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

bool Helpers::fileInUse(const std::filesystem::path &path) noexcept
{
	HANDLE file = CreateFileW(path.wstring().c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE)
		return true;

	CloseHandle(file);
	return false;
}

bool Helpers::loadLibreryRemote(void *process, const std::filesystem::path &path, std::uintptr_t *moduleHandle) noexcept
{
	if (!process)
		return false;

	if (!path.has_filename())
		return false;

	HMODULE kernel32 = GetModuleHandle(L"kernel32");
	if (!kernel32)
		return false;

	FARPROC loadLibrary = GetProcAddress(kernel32, "LoadLibraryW");
	if (!loadLibrary)
		return false;

	wchar_t pathString[MAX_PATH];
	wcscpy_s(pathString, path.wstring().c_str());

	void *pathImage = VirtualAllocEx(process, nullptr, sizeof(pathString), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!pathImage)
		return false;

	if (!WriteProcessMemory(process, pathImage, pathString, sizeof(pathString), nullptr))
	{
		VirtualFreeEx(process, pathImage, 0, MEM_RELEASE);
		return false;
	}

	HANDLE thread = CreateRemoteThread(process, 0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibrary), pathImage, 0, nullptr);
	if (!thread)
	{
		VirtualFreeEx(process, pathImage, 0, MEM_RELEASE);
		return false;
	}

	WaitForSingleObject(thread, INFINITE);

	if (moduleHandle)
		GetExitCodeThread(thread, reinterpret_cast<LPDWORD>(moduleHandle));

	CloseHandle(thread);

	return true;
}

bool Helpers::bypassCsgoInject(void *csgoProcess) noexcept
{
	const auto ntdll = LoadLibraryW(L"ntdll");
	if (!ntdll)
		return false;

	const auto ntOpenFile = GetProcAddress(ntdll, "NtOpenFile");
	if (!ntOpenFile)
		return false;

	std::uint8_t originalBytes[5];
	std::memcpy(originalBytes, ntOpenFile, sizeof(originalBytes));

	if (!WriteProcessMemory(csgoProcess, ntOpenFile, originalBytes, sizeof(originalBytes), nullptr))
		return false;

	return true;
}
