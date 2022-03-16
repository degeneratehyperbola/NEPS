#pragma once
#include <vector>
#include <array>
#include <string>

namespace Players
{
	struct PlayerData {
		bool invalid = true;
		bool flagged = false;
		std::string name;
	};

	void updatePlayerList() noexcept;
	void spectatorFilter() noexcept;

	inline bool noSpectators = false;
	inline std::array<PlayerData, 65> players{};
	inline PlayerData invalidData{};
}