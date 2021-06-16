#pragma once

#include "../Netvars.h"
#include "VirtualMethod.h"

struct Vector;

class PlayerResourceInterface
{
public:
	VIRTUAL_METHOD(bool, isAlive, 5, (int index), (this, index))
	VIRTUAL_METHOD(const char *, getPlayerName, 8, (int index), (this, index))
	VIRTUAL_METHOD(int, getPlayerHealth, 14, (int index), (this, index))
};

class PlayerResource
{
public:
	auto getPlayerResourceInterface() noexcept
	{
		return reinterpret_cast<PlayerResourceInterface *>(uintptr_t(this) + 0x9D8);
	}

	NETVAR(bombsiteCenterA, "CCSPlayerResource", "m_bombsiteCenterA", Vector)
	NETVAR(bombsiteCenterB, "CCSPlayerResource", "m_bombsiteCenterB", Vector)

	NETVAR(bombOwner, "CCSPlayerResource", "m_iPlayerC4", int) // Entity index of a C4 carrier or 0
	NETVAR(vip, "CCSPlayerResource", "m_iPlayerVIP", int) // Entity index of a VIP or 0

	NETVAR(competitiveRanking, "CCSPlayerResource", "m_iCompetitiveRanking", int [65])
	NETVAR(competitiveWins, "CCSPlayerResource", "m_iCompetitiveWins", int [65])
	NETVAR(teammateColor, "CCSPlayerResource", "m_iCompTeammateColor", int [65])
	NETVAR(activeCoinRank, "CCSPlayerResource", "m_nActiveCoinRank", int [65])
	NETVAR(musicId, "CCSPlayerResource", "m_nMusicID", int [65])
	NETVAR(level, "CCSPlayerResource", "m_nPersonaDataPublicLevel", int [65])
	NETVAR(commendsLeader, "CCSPlayerResource", "m_nPersonaDataPublicCommendsLeader", int [65])
	NETVAR(commendsTeacher, "CCSPlayerResource", "m_nPersonaDataPublicCommendsTeacher", int [65])
	NETVAR(commendsFriendly, "CCSPlayerResource", "m_nPersonaDataPublicCommendsFriendly", int [65])
};
