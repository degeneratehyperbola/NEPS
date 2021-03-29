#pragma once

struct UserCmd;
struct Vector;

namespace AntiAim
{
	void run(UserCmd *, const Vector &, bool &) noexcept;
	void fakeUp(UserCmd *, const Vector &, bool &) noexcept;
}
