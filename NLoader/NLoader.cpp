#include "NLoader.h"
#include "Version.hpp"

#include <Windows.h>
#include <TlHelp32.h>

typedef HMODULE(__stdcall *LoadLibraryFn)(LPCSTR);
typedef FARPROC(__stdcall *GetProcAddressFn)(HMODULE, LPCSTR);
typedef BOOL(__stdcall *DllMainFn)(HMODULE, DWORD, LPVOID);

struct LoaderParams
{
	void *imageBase;

	PIMAGE_NT_HEADERS ntHeaders;
	PIMAGE_BASE_RELOCATION imageBaseRelocation;
	PIMAGE_IMPORT_DESCRIPTOR imports;

	LoadLibraryFn fnLoadLibraryA;
	GetProcAddressFn fnGetProcAddress;
};

int __stdcall manualLoader(LoaderParams *params) noexcept
{
	auto imageBaseRelocation = params->imageBaseRelocation;
	const unsigned long delta = (unsigned long)((std::byte *)params->imageBase - params->ntHeaders->OptionalHeader.ImageBase);

	while (imageBaseRelocation->VirtualAddress)
	{
		if (imageBaseRelocation->SizeOfBlock >= sizeof(IMAGE_BASE_RELOCATION))
		{
			const std::size_t count = (imageBaseRelocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(unsigned short);
			unsigned short *list = (unsigned short *)(imageBaseRelocation + 1);

			for (std::size_t i = 0; i < count; ++i)
			{
				if (list[i])
				{
					unsigned long *ptr = (unsigned long *)((std::byte *)params->imageBase + (imageBaseRelocation->VirtualAddress + (list[i] & 0xFFF)));
					*ptr += delta;
				}
			}
		}

		imageBaseRelocation = decltype(imageBaseRelocation)((std::byte *)imageBaseRelocation + imageBaseRelocation->SizeOfBlock);
	}

	auto importDescriptor = params->imports;

	while (importDescriptor->Characteristics)
	{
		PIMAGE_THUNK_DATA originalFirstThunk = (PIMAGE_THUNK_DATA)((std::byte *)params->imageBase + importDescriptor->OriginalFirstThunk);
		PIMAGE_THUNK_DATA firstThunk = (PIMAGE_THUNK_DATA)((std::byte *)params->imageBase + importDescriptor->FirstThunk);

		HMODULE moduleHandle = params->fnLoadLibraryA((const char *)params->imageBase + importDescriptor->Name);

		if (!moduleHandle)
			return 0;

		while (originalFirstThunk->u1.AddressOfData)
		{
			if (originalFirstThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)
			{
				std::uintptr_t function = (std::uintptr_t)params->fnGetProcAddress(moduleHandle,
					(const char *)(originalFirstThunk->u1.Ordinal & 0xFFFF));

				if (!function)
					return 0;

				firstThunk->u1.Function = function;
			} else
			{
				PIMAGE_IMPORT_BY_NAME importByName = (PIMAGE_IMPORT_BY_NAME)((std::byte *)params->imageBase + originalFirstThunk->u1.AddressOfData);
				std::uintptr_t function = (std::uintptr_t)params->fnGetProcAddress(moduleHandle, (LPCSTR)importByName->Name);
				if (!function)
					return 0;

				firstThunk->u1.Function = function;
			}
			originalFirstThunk++;
			firstThunk++;
		}
		importDescriptor++;
	}

	if (params->ntHeaders->OptionalHeader.AddressOfEntryPoint)
	{
		const DllMainFn entryPoint = (DllMainFn)((std::byte *)params->imageBase + params->ntHeaders->OptionalHeader.AddressOfEntryPoint);

		return entryPoint((HMODULE)params->imageBase, DLL_PROCESS_ATTACH | (SIGNATURE & ~SIGNATURE_MASK), 0);
	}
	return 0;
}

int __stdcall stub() noexcept
{
	return 0;
}

int __stdcall NLoader::loadLibrary(const void *dllBuffer, unsigned long pid, unsigned long *dllMainReturn) noexcept
{
	if (!pid)
		return 0;

	LoaderParams params;

	const PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)dllBuffer;
	const PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((std::byte *)dllBuffer + dosHeader->e_lfanew);

	const HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, 0, pid);
	if (!process)
		return 0;

	void *executableImage = VirtualAllocEx(process, 0, ntHeaders->OptionalHeader.SizeOfImage,
		MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	WriteProcessMemory(process, executableImage, dllBuffer,
		ntHeaders->OptionalHeader.SizeOfHeaders, 0);

	const PIMAGE_SECTION_HEADER sectionHeader = (PIMAGE_SECTION_HEADER)(ntHeaders + 1);
	for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++)
	{
		WriteProcessMemory(process, (void *)((std::byte *)executableImage + sectionHeader[i].VirtualAddress),
			(void *)((std::byte *)dllBuffer + sectionHeader[i].PointerToRawData), sectionHeader[i].SizeOfRawData, 0);
	}

	void *loaderImage = VirtualAllocEx(process, 0, 4096, MEM_COMMIT | MEM_RESERVE,
		PAGE_EXECUTE_READWRITE);

	params.imageBase = executableImage;
	params.ntHeaders = (PIMAGE_NT_HEADERS)((std::byte *)executableImage + dosHeader->e_lfanew);

	params.imageBaseRelocation = (PIMAGE_BASE_RELOCATION)((std::byte *)executableImage
		+ ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
	params.imports = (PIMAGE_IMPORT_DESCRIPTOR)((std::byte *)executableImage
		+ ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	params.fnLoadLibraryA = LoadLibraryA;
	params.fnGetProcAddress = GetProcAddress;

	WriteProcessMemory(process, loaderImage, &params, sizeof(LoaderParams), 0);
	WriteProcessMemory(process, (void *)((LoaderParams *)loaderImage + 1), manualLoader, (std::uintptr_t)stub - (std::uintptr_t)manualLoader, 0);

	const HANDLE thread = CreateRemoteThread(process, 0, 0, (LPTHREAD_START_ROUTINE)((LoaderParams *)loaderImage + 1), loaderImage, 0, 0);
	WaitForSingleObject(thread, INFINITE);
	if (dllMainReturn)
		GetExitCodeThread(thread, dllMainReturn);

	VirtualFreeEx(process, loaderImage, 0, MEM_RELEASE);

	return 1;
}

unsigned long NLoader::findPID(const std::wstring &processName) noexcept
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
