#pragma once

class Entity;
class Matrix3x4;
struct UserCmd;

namespace Animations
{
    void releaseState() noexcept;
    void getDesyncedBones(Matrix3x4 *out) noexcept;
    bool desyncedAnimations(const UserCmd &cmd, bool sendPacket) noexcept;
    bool fixAnimation(const UserCmd &cmd, bool sendPacket) noexcept;
    void resolve(Entity *animatable) noexcept;
}
