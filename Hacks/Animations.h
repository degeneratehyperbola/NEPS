#pragma once

class Matrix3x4;
struct UserCmd;
struct Vector;

namespace Animations
{
	bool clientLerped(Matrix3x4 *out, const UserCmd *cmd, bool sendPacket) noexcept;
    void animSync(const UserCmd *cmd, bool sendPacket) noexcept;
}