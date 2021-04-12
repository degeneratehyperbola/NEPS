#pragma once

struct UserCmd;
struct Vector;
class Entity;
namespace Backtrack { struct Record; }

namespace Aimbot
{
	void run(UserCmd *cmd) noexcept;
	void choseTarget(UserCmd *cmd) noexcept;
	Vector getTargetPoint() noexcept;
	Entity *getTarget() noexcept;
	const Backtrack::Record *getTargetRecord() noexcept;
}
