#pragma once

class matrix3x4;
struct UserCmd;
struct Vector;

namespace Animations
{
	bool clientLerped(matrix3x4 *out, UserCmd *cmd, bool &sendPacket, Vector *headPos = nullptr) noexcept;
    void animSync(UserCmd *cmd, bool &sendPacket, Vector *headPos = nullptr) noexcept;
}