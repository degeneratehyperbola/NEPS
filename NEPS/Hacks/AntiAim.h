#pragma once

struct UserCmd;
struct Vector;
struct ImDrawList;

namespace AntiAim
{
	void run(UserCmd *cmd, const Vector & currentViewAngles, bool & sendPacket) noexcept;
	bool fakePitch(UserCmd *cmd) noexcept;
	void visualize(ImDrawList *drawList) noexcept;
}
