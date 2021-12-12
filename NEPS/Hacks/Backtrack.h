#pragma once

#include <array>
#include <deque>

#include "../SDK/Matrix3x4.h"
#include "../SDK/Vector.h"
#include "../SDK/ModelInfo.h"
#include "../SDK/NetworkChannel.h"

enum class FrameStage;
struct UserCmd;

struct Record
{
	int ownerIdx;
	Vector origin;
	float simulationTime;
	bool important;
	bool hasHelmet;
	int armor;
	Matrix3x4 matrix[MAX_STUDIO_BONES];
};

namespace Backtrack
{
	void update(FrameStage) noexcept;
	void run(UserCmd *) noexcept;

	const std::deque<Record> &getRecords(std::size_t index) noexcept;
	float getLerp() noexcept;
	bool valid(float simTime) noexcept;
}
