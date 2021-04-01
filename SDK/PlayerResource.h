#pragma once

#include "../Netvars.h"
#include "VirtualMethod.h"

#define MAX_PLAYERS 64

struct Vector;

class IPlayerResource
{
public:
	VIRTUAL_METHOD(bool, isAlive, 5, (int index), (this, index))
	VIRTUAL_METHOD(const char *, getPlayerName, 8, (int index), (this, index))
	VIRTUAL_METHOD(int, getPlayerHealth, 14, (int index), (this, index))
};

class PlayerResource
{
public:
	auto getIPlayerResource() noexcept
	{
		return reinterpret_cast<IPlayerResource *>(uintptr_t(this) + 0x9D8);
	}

	NETVAR(bombsiteCenterA, "CCSPlayerResource", "m_bombsiteCenterA", Vector)
	NETVAR(bombsiteCenterB, "CCSPlayerResource", "m_bombsiteCenterB", Vector)

	NETVAR(bombOwner, "CCSPlayerResource", "m_iPlayerC4", int) // Entity index of a C4 carrier or 0
	NETVAR(vip, "CCSPlayerResource", "m_iPlayerVIP", int) // Entity index of a VIP or 0

	NETVAR(competitiveRanking, "CCSPlayerResource", "m_iCompetitiveRanking", int [MAX_PLAYERS + 1])
	NETVAR(competitiveWins, "CCSPlayerResource", "m_iCompetitiveWins", int [MAX_PLAYERS + 1])
	NETVAR(teammateColor, "CCSPlayerResource", "m_iCompTeammateColor", int [MAX_PLAYERS + 1])
	NETVAR(coinFlags, "CCSPlayerResource", "m_nActiveCoinRank", int [MAX_PLAYERS + 1])
	NETVAR(musicId, "CCSPlayerResource", "m_nMusicID", int [MAX_PLAYERS + 1])
	NETVAR(level, "CCSPlayerResource", "m_nPersonaDataPublicLevel", int [MAX_PLAYERS + 1])
	NETVAR(commendsLeader, "CCSPlayerResource", "m_nPersonaDataPublicCommendsLeader", int [MAX_PLAYERS + 1])
	NETVAR(commendsTeacher, "CCSPlayerResource", "m_nPersonaDataPublicCommendsTeacher", int [MAX_PLAYERS + 1])
	NETVAR(commendsFriendly, "CCSPlayerResource", "m_nPersonaDataPublicCommendsFriendly", int [MAX_PLAYERS + 1])
};
