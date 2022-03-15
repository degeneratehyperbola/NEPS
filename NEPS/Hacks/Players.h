#pragma once
#include <vector>
#include <array>

namespace Players
{
	struct PlayerData {
		bool invalid = true;
		bool flagged = false;
	};

	void updatePlayerList() noexcept;

	inline std::array<PlayerData, 65> players{};
	inline PlayerData invalidData{};
}