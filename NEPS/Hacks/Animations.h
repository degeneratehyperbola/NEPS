#pragma once

class Entity;
class Matrix3x4;
struct UserCmd;

namespace Animations
{
    void releaseState() noexcept;
    void getDesyncedBoneMatrices(Matrix3x4 *out) noexcept;
    void localComputeDesync(const UserCmd &cmd, bool sendPacket) noexcept;
    void localAnimationFix(const UserCmd &cmd, bool sendPacket) noexcept;
    void resolveAnimations(Entity *animatable) noexcept;
}
