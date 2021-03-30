#pragma once

struct UserCmd;
struct Vector;
class Entity;

namespace Aimbot
{
    void run(UserCmd *cmd) noexcept;
    void choseTarget(UserCmd *cmd) noexcept;
	Vector getTargetPoint() noexcept;
	Entity *getTarget() noexcept;
}
