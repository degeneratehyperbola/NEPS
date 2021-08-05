#pragma once

class Entity;
class Matrix3x4;
struct UserCmd;

namespace Animations
{
    void releaseState() noexcept;
    void copyLerpedBones(Matrix3x4 *out) noexcept;
    bool animDesynced(const UserCmd &cmd, bool sendPacket) noexcept;
    bool animSynced(const UserCmd &cmd, bool sendPacket) noexcept;
    void resolveLBY(Entity *animatable) noexcept;
}