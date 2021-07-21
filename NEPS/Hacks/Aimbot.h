#pragma once

struct UserCmd;
struct Vector;
class Entity;
namespace Backtrack { struct Record; }

namespace Aimbot
{
	void run(UserCmd *cmd) noexcept;
	int getTargetHandle() noexcept;
	const Backtrack::Record *getTargetRecord() noexcept;
}
