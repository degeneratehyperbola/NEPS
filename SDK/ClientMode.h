#pragma once

#include <cstdint>

class HudChat
{
public:
	template <typename... Args>
	void printf(int filter, const char *fmt, Args... args) noexcept
	{
		(*reinterpret_cast<void(__cdecl ***)(void *, int, const char *, ...)>(this))[26](this, filter, fmt, args...);
	}
};

class ClientMode
{
public:
	auto getHudChat() noexcept
	{
		return *reinterpret_cast<HudChat **>(std::uintptr_t(this) + 28);
	}
};
