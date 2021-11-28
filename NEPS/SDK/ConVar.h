#pragma once

#include <type_traits>

#include "UtlVector.h"
#include "Pad.h"
#include "VirtualMethod.h"

enum
{
	FCVAR_NONE = 0,
	FCVAR_UNREGISTERED = (1 << 0),
	FCVAR_DEVELOPMENTONLY = (1 << 1),
	FCVAR_GAMEDLL = (1 << 2),
	FCVAR_CLIENTDLL = (1 << 3),
	FCVAR_HIDDEN = (1 << 4),
	FCVAR_PROTECTED = (1 << 5),
	FCVAR_SPONLY = (1 << 6),
	FCVAR_ARCHIVE = (1 << 7),
	FCVAR_NOTIFY = (1 << 8),
	FCVAR_USERINFO = (1 << 9),
	FCVAR_CHEAT = (1 << 14),
	FCVAR_PRINTABLEONLY = (1 << 10),
	FCVAR_UNLOGGED = (1 << 11),
	FCVAR_NEVER_AS_STRING = (1 << 12),
	FCVAR_REPLICATED = (1 << 13),
	FCVAR_DEMO = (1 << 16),
	FCVAR_DONTRECORD = (1 << 17),
	FCVAR_NOT_CONNECTED = (1 << 22),
	FCVAR_ARCHIVE_XBOX = (1 << 24),
	FCVAR_SERVER_CAN_EXECUTE = (1 << 28),
	FCVAR_SERVER_CANNOT_QUERY = (1 << 29),
	FCVAR_CLIENTCMD_CAN_EXECUTE = (1 << 30)
};

struct ConCommandBase
{
public:
	VIRTUAL_METHOD(bool, isFlagSet, 2, (int flag), (this, flag))
	VIRTUAL_METHOD(void, removeFlags, 4, (int flags), (this, flags))
	VIRTUAL_METHOD(int, getFlags, 5, (), (this))
	VIRTUAL_METHOD(ConCommandBase *, getNext, 9, (), (this))
};

struct ConVar
{
	VIRTUAL_METHOD(float, getFloat, 12, (), (this))
	VIRTUAL_METHOD(int, getInt, 13, (), (this))
	VIRTUAL_METHOD(void, setValue, 14, (const char *value), (this, value))
	VIRTUAL_METHOD(void, setValue, 15, (float value), (this, value))
	VIRTUAL_METHOD(void, setValue, 16, (int value), (this, value))

	PAD(24)
	std::add_pointer_t<void __cdecl()> changeCallback;
	ConVar *parent;
	const char *defaultValue;
	char *string;
	PAD(28)
	UtlVector<void(__cdecl *)()> onChangeCallbacks;
};