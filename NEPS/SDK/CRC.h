#pragma once

#include <cstdint>

class SetupCRC
{
	uint32_t value = 0xFFFFFFFF;

public:
	constexpr operator uint32_t() noexcept
	{
		return value ^ 0xFFFFFFFF;
	}

	void process(const void *buffer, unsigned size) noexcept;
};
