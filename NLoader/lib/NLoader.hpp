#pragma once

#include <cstddef>
#include <string>

namespace NLoader
{
	void *loadFromResource(std::size_t *size = nullptr) noexcept;
	unsigned long findPid(const std::wstring &processName) noexcept;
}
