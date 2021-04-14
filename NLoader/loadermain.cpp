#include <iostream>

#include <Windows.h>
#include <TlHelp32.h>

#ifdef REQ_NET
#include "curl/curl.h"
#endif // REQ_NET

#include "resource.h"
#include "version.hpp"

using namespace std;

typedef HMODULE(__stdcall *PLOADLIBRARY)(LPCSTR);
typedef FARPROC(__stdcall *PGETPROCADDRESS)(HMODULE, LPCSTR);

typedef BOOL(__stdcall *DLLMAIN)(HMODULE, DWORD, LPVOID);

struct LOADERDATA
{
	LPVOID ImageBase;

	PIMAGE_NT_HEADERS NtHeaders;
	PIMAGE_BASE_RELOCATION BaseReloc;
	PIMAGE_IMPORT_DESCRIPTOR ImportDirectory;

	PLOADLIBRARY fnLoadLibraryA;
	PGETPROCADDRESS fnGetProcAddress;
};

static DWORD FindPID(wstring processName)
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

#ifdef REQ_NET
static BOOL RequestBinary(LPVOID out)
{
	auto curl = curl_easy_init();

	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, "");
		curl_easy_cleanup(curl);
	}
}
#endif // REQ_NET

BOOL __stdcall LibraryLoader(LPVOID memory)
{
	LOADERDATA *LoaderParams = (LOADERDATA *)memory;

	// Call TLS callbacks
	#ifdef CALL_TLS
	IMAGE_DATA_DIRECTORY pIDD = LoaderParams->NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];

	if (pIDD.VirtualAddress)
	{
		PIMAGE_TLS_DIRECTORY pITD = (PIMAGE_TLS_DIRECTORY)((LPBYTE)LoaderParams->ImageBase + pIDD.VirtualAddress);
		if (pITD)
		{
			PIMAGE_TLS_CALLBACK *pITC = (PIMAGE_TLS_CALLBACK *)pITD->AddressOfCallBacks;
			if (pITC)
			{
				while (*pITC)
				{
					(*pITC)((LPVOID)LoaderParams->ImageBase, DLL_PROCESS_ATTACH, NULL);
					pITC++;
				}
			}
		}
	}
	#endif // CALL_TLS

	// ???1
	PIMAGE_BASE_RELOCATION pIBR = LoaderParams->BaseReloc;

	// Calculate the delta
	DWORD delta = (DWORD)((LPBYTE)LoaderParams->ImageBase - LoaderParams->NtHeaders->OptionalHeader.ImageBase);

	// ???1
	while (pIBR->VirtualAddress)
	{
		if (pIBR->SizeOfBlock >= sizeof(IMAGE_BASE_RELOCATION))
		{
			int count = (pIBR->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
			PWORD list = (PWORD)(pIBR + 1);

			for (int i = 0; i < count; i++)
			{
				if (list[i])
				{
					PDWORD ptr = (PDWORD)((LPBYTE)LoaderParams->ImageBase + (pIBR->VirtualAddress + (list[i] & 0xFFF)));
					*ptr += delta;
				}
			}
		}

		pIBR = (PIMAGE_BASE_RELOCATION)((LPBYTE)pIBR + pIBR->SizeOfBlock);
	}

	// Resolve DLL imports
	PIMAGE_IMPORT_DESCRIPTOR pIID = LoaderParams->ImportDirectory;

	while (pIID->Characteristics)
	{
		PIMAGE_THUNK_DATA OrigFirstThunk = (PIMAGE_THUNK_DATA)((LPBYTE)LoaderParams->ImageBase + pIID->OriginalFirstThunk);
		PIMAGE_THUNK_DATA FirstThunk = (PIMAGE_THUNK_DATA)((LPBYTE)LoaderParams->ImageBase + pIID->FirstThunk);

		HMODULE hModule = LoaderParams->fnLoadLibraryA((LPCSTR)LoaderParams->ImageBase + pIID->Name);

		if (!hModule)
			return FALSE;

		while (OrigFirstThunk->u1.AddressOfData)
		{
			if (OrigFirstThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)
			{
				// Import by ordinal
				DWORD Function = (DWORD)LoaderParams->fnGetProcAddress(hModule,
					(LPCSTR)(OrigFirstThunk->u1.Ordinal & 0xFFFF));

				if (!Function)
					return FALSE;

				FirstThunk->u1.Function = Function;
			} else
			{
				// Import by name
				PIMAGE_IMPORT_BY_NAME pIBN = (PIMAGE_IMPORT_BY_NAME)((LPBYTE)LoaderParams->ImageBase + OrigFirstThunk->u1.AddressOfData);
				DWORD Function = (DWORD)LoaderParams->fnGetProcAddress(hModule, (LPCSTR)pIBN->Name);
				if (!Function)
					return FALSE;

				FirstThunk->u1.Function = Function;
			}
			OrigFirstThunk++;
			FirstThunk++;
		}
		pIID++;
	}

	// Call the entry point if it exists
	if (LoaderParams->NtHeaders->OptionalHeader.AddressOfEntryPoint)
	{
		DLLMAIN EntryPoint = (DLLMAIN)((LPBYTE)LoaderParams->ImageBase + LoaderParams->NtHeaders->OptionalHeader.AddressOfEntryPoint);

		// Call the entry point with exclusive parameters
		return EntryPoint((HMODULE)LoaderParams->ImageBase, DLL_PROCESS_ATTACH | SIGNATURE, NULL);
	}
	return FALSE;
}

DWORD __stdcall Stub()
{
	return 0;
}

BOOL APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
	// Find CS:GO
	DWORD ProcessID = FindPID(L"csgo.exe");

	if (!ProcessID)
	{
		MessageBoxA(NULL, "Falied to load NEPS.\nYou need to run CS:GO before running the loader.", "NEPS", MB_OK | MB_ICONERROR);
		return FALSE;
	}

	LOADERDATA LoaderParams;

	HMODULE hModule = GetModuleHandleA(NULL);
	HRSRC hResource = FindResourceA(hModule, MAKEINTRESOURCEA(IDR_BIN1), "BIN");
	PVOID Buffer = LoadResource(hModule, hResource);

	// Target DLL's DOS Header
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)Buffer;
	// Target DLL's NT Headers
	PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((LPBYTE)Buffer + pDosHeader->e_lfanew);

	// Opening target process.
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessID);
	// Allocating memory for the DLL
	PVOID ExecutableImage = VirtualAllocEx(hProcess, NULL, pNtHeaders->OptionalHeader.SizeOfImage,
		MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	// Copy the headers to target process
	WriteProcessMemory(hProcess, ExecutableImage, Buffer,
		pNtHeaders->OptionalHeader.SizeOfHeaders, NULL);

	// Target DLL's Section Header
	PIMAGE_SECTION_HEADER pSectHeader = (PIMAGE_SECTION_HEADER)(pNtHeaders + 1);
	// Copying sections of the dll to the target process
	for (int i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++)
	{
		WriteProcessMemory(hProcess, (PVOID)((LPBYTE)ExecutableImage + pSectHeader[i].VirtualAddress),
			(PVOID)((LPBYTE)Buffer + pSectHeader[i].PointerToRawData), pSectHeader[i].SizeOfRawData, NULL);
	}

	// Allocate memory for the loader code.
	PVOID LoaderMemory = VirtualAllocEx(hProcess, NULL, 4096, MEM_COMMIT | MEM_RESERVE,
		PAGE_EXECUTE_READWRITE);

	LoaderParams.ImageBase = ExecutableImage;
	LoaderParams.NtHeaders = (PIMAGE_NT_HEADERS)((LPBYTE)ExecutableImage + pDosHeader->e_lfanew);

	LoaderParams.BaseReloc = (PIMAGE_BASE_RELOCATION)((LPBYTE)ExecutableImage
		+ pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
	LoaderParams.ImportDirectory = (PIMAGE_IMPORT_DESCRIPTOR)((LPBYTE)ExecutableImage
		+ pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	LoaderParams.fnLoadLibraryA = LoadLibraryA;
	LoaderParams.fnGetProcAddress = GetProcAddress;

	// Write the loader information to target process
	WriteProcessMemory(hProcess, LoaderMemory, &LoaderParams, sizeof(LOADERDATA),
		NULL);
	// Write the loader code to target process
	WriteProcessMemory(hProcess, (PVOID)((LOADERDATA *)LoaderMemory + 1), LibraryLoader,
		(DWORD)Stub - (DWORD)LibraryLoader, NULL);
	// Create a remote thread to execute the loader code
	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)((LOADERDATA *)LoaderMemory + 1),
		LoaderMemory, 0, NULL);

	// Wait for the loader to finish executing
	WaitForSingleObject(hThread, INFINITE);

	// Get loader's return value to determine DllMain's return value
	DWORD dwReturnValue = 0;
	GetExitCodeThread(hThread, &dwReturnValue);

	// Inform user about errors
	if (!dwReturnValue) MessageBoxA(NULL, "Falied to load NEPS.\nTry running the loader with administrator privileges.", "NEPS", MB_OK | MB_ICONERROR);
	else MessageBoxA(NULL, "Success! NEPS is now loaded.", "NEPS", MB_OK | MB_ICONINFORMATION);

	// Free the allocated loader code
	VirtualFreeEx(hProcess, LoaderMemory, 0, MEM_RELEASE);
	FreeResource(Buffer);

	return TRUE;
}
