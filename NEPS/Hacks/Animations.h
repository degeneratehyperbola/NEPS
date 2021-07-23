#pragma once

class Entity;
class Matrix3x4;
struct UserCmd;

namespace Animations
{
    void releaseState() noexcept;
    void copyLerpedBones(Matrix3x4 *out) noexcept;
    bool clientLerped(const UserCmd &cmd, bool sendPacket) noexcept;
    bool animSync(const UserCmd &cmd, bool sendPacket) noexcept;
    void resolveLBY(Entity *animatable, int seed) noexcept;
}