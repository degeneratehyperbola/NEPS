#pragma once

#include "../Netvars.h"
#include "VirtualMethod.h"

struct Vector;

class IPlayerResource {
public:
    VIRTUAL_METHOD(bool, isAlive, 5, (int index), (this, index))
    VIRTUAL_METHOD(const char*, getPlayerName, 8, (int index), (this, index))
    VIRTUAL_METHOD(int, getPlayerHealth, 14, (int index), (this, index))
};

class PlayerResource {
public:
    auto getIPlayerResource() noexcept
    {
        return reinterpret_cast<IPlayerResource*>(uintptr_t(this) + 0x9D8);
    }

    NETVAR(bombsiteCenterA, "CCSPlayerResource", "m_bombsiteCenterA", Vector)
    NETVAR(bombsiteCenterB, "CCSPlayerResource", "m_bombsiteCenterB", Vector)
    NETVAR(bombOwner, "CCSPlayerResource", "m_iPlayerC4", int)
};
