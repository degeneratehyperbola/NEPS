#pragma once

class Matrix3x4;
struct UserCmd;
struct Vector;

namespace Animations
{
	bool clientLerped(Matrix3x4 *out, UserCmd *cmd, bool &sendPacket, Vector *headPos = nullptr, float *feetYawDelta = nullptr) noexcept;
    void animSync(UserCmd *cmd, bool &sendPacket, Vector *headPos = nullptr) noexcept;
}