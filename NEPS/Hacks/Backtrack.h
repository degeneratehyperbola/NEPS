#pragma once

#include <array>
#include <deque>

#include "../SDK/Matrix3x4.h"
#include "../SDK/Vector.h"
#include "../SDK/ModelInfo.h"

enum class FrameStage;
struct UserCmd;

namespace Backtrack
{
	void update(FrameStage) noexcept;
	void run(UserCmd *) noexcept;

	struct Record
	{
		Vector origin;
		float simulationTime;
		bool shot ;
		bool hasHelmet;
		int armor;
		Matrix3x4 matrix[MAX_STUDIO_BONES];
	};

	const std::deque<Record> &getRecords(std::size_t index) noexcept;
	float getLerp() noexcept;
	bool valid(float simTime) noexcept;
	void init() noexcept;
}
