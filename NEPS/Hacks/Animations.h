#pragma once

class Matrix3x4;
class Entity;
struct UserCmd;
struct Vector;

namespace Animations
{
	bool clientLerped(Matrix3x4 *out, const UserCmd *cmd, bool sendPacket) noexcept;
    bool animSync(const UserCmd *cmd, bool sendPacket) noexcept;
    void resolve(Entity *animatable) noexcept;
}