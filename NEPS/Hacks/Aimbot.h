#pragma once

struct UserCmd;
struct Vector;
class Entity;
class GameEvent;
namespace Backtrack { struct Record; }

namespace Aimbot
{
	void missCounter(GameEvent *event) noexcept;
	void resetMissCounter() noexcept;
    void run(UserCmd *cmd) noexcept;
	int getTargetHandle() noexcept;
	const Backtrack::Record *getTargetRecord() noexcept;
}
