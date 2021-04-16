#pragma once

class Entity;
class Matrix3x4;
struct UserCmd;
struct Vector;

namespace Animations
{
	bool clientLerped(const UserCmd &cmd, bool sendPacket) noexcept;
    bool animSync(const UserCmd &cmd, bool sendPacket) noexcept;
    void resolve(Entity *animatable) noexcept;
    void copyLerpedBones(Matrix3x4 *out) noexcept;
}