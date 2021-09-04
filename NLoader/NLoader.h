#pragma once

#include <string>

namespace NLoader
{
	int __stdcall loadLibrary(const void *dllBuffer, unsigned long pid, unsigned long *dllMainReturn) noexcept;
	unsigned long findPID(const std::wstring &processName) noexcept;
}
