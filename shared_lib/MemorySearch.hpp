#pragma once

#include <array>
#include <cassert>
#include <limits>
#include <Psapi.h>
#include <string>
#include <string_view>
#include <Windows.h>

namespace MemorySearch
{
	std::pair<void *, std::size_t> getModuleInformation(const char *name) noexcept
	{
		if (HMODULE handle = GetModuleHandleA(name))
		{
			if (MODULEINFO moduleInfo; GetModuleInformation(GetCurrentProcess(), handle, &moduleInfo, sizeof(moduleInfo)))
				return std::make_pair(moduleInfo.lpBaseOfDll, moduleInfo.SizeOfImage);
		}
		return {};
	}

	[[nodiscard]] auto generateBadCharTable(std::string_view pattern) noexcept
	{
		assert(!pattern.empty());

		std::array<std::size_t, (std::numeric_limits<std::uint8_t>::max)() + 1> table;

		auto lastWildcard = pattern.rfind('?');
		if (lastWildcard == std::string_view::npos)
			lastWildcard = 0;

		const auto defaultShift = (std::max)(std::size_t(1), pattern.length() - 1 - lastWildcard);
		table.fill(defaultShift);

		for (auto i = lastWildcard; i < pattern.length() - 1; ++i)
			table[static_cast<std::uint8_t>(pattern[i])] = pattern.length() - 1 - i;

		return table;
	}

	std::uintptr_t findPattern(const char *moduleName, std::string pattern, bool reportNotFound = true) noexcept
	{
		static auto id = 0;
		++id;

		const auto [moduleBase, moduleSize] = getModuleInformation(moduleName);

		if (moduleBase && moduleSize)
		{
			const auto lastIdx = pattern.length() - 1;
			const auto badCharTable = generateBadCharTable(pattern);

			auto start = static_cast<const char *>(moduleBase);
			const auto end = start + moduleSize - pattern.length();

			while (start <= end)
			{
				int i = lastIdx;
				while (i >= 0 && (pattern[i] == '?' || start[i] == pattern[i]))
					--i;

				if (i < 0)
					return reinterpret_cast<std::uintptr_t>(start);

				start += badCharTable[static_cast<std::uint8_t>(start[lastIdx])];
			}
		}

		#ifdef _WIN32
		if (reportNotFound)
			MessageBoxA(NULL, ("Failed to find pattern #" + std::to_string(id) + '!').c_str(), "NEPS", MB_OK | MB_ICONWARNING);
		#endif

		return 0;
	}
}