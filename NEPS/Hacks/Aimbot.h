#pragma once

struct UserCmd;
struct Vector;
class Entity;
class GameEvent;
struct Record;

namespace Aimbot
{
	void missCounter(GameEvent *event) noexcept;
	void resetMissCounter() noexcept;
    void run(UserCmd *cmd) noexcept;
	int getTargetHandle() noexcept;
	const Record *getTargetRecord() noexcept;
}
