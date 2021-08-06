#pragma once

class Entity;
class Matrix3x4;
struct UserCmd;

namespace Animations
{
    void releaseState() noexcept;
    void copyLerpedBones(Matrix3x4 *out) noexcept;
    bool desyncedAnimations(const UserCmd &cmd, bool sendPacket) noexcept;
    bool animationFix(const UserCmd &cmd, bool sendPacket) noexcept;
    void resolveLBY(Entity *animatable) noexcept;
}
