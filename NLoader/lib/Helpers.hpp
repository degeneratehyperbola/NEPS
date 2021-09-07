#pragma once

#include <cstddef>
#include <string>
#include <filesystem>

namespace Helpers
{
	void *loadFromResource(std::size_t *size = nullptr) noexcept;
	unsigned long findPid(const std::wstring &processName) noexcept;
	bool fileInUse(const std::filesystem::path &) noexcept;
	bool loadLibreryRemote(void *process, const std::filesystem::path &, std::uintptr_t *moduleHandle = nullptr) noexcept;
	bool bypassCsgoInject(void *csgoProcess) noexcept;
}
