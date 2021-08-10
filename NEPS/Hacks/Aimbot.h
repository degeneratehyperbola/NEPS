#pragma once

struct UserCmd;
struct Vector;
class Entity;
class GameEvent;
struct Record;

namespace Aimbot
{
	int getTargetHandle() noexcept;
	const Record *getTargetRecord() noexcept;
	void missCounter(GameEvent *event) noexcept;
	void resetMissCounter() noexcept;
    int getMisses() noexcept;
    void predictPeek(UserCmd *cmd) noexcept;
    void run(UserCmd *cmd) noexcept;
}
