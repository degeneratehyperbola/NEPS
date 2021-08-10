#pragma once

struct UserCmd;
struct Vector;
struct ImDrawList;

namespace AntiAim
{
	void run(UserCmd *, const Vector &, bool &) noexcept;
	bool fakePitch(UserCmd *) noexcept;
	void visualize(ImDrawList *) noexcept;
}
