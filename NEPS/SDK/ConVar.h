#pragma once

#include <type_traits>

#include "UtlVector.h"
#include "Pad.h"
#include "VirtualMethod.h"

enum CvarFlag
{
	FCVAR_DEVELOPMENTONLY = 2,
	FCVAR_HIDDEN = 16
};

struct ConCommandBase
{
public:
	VIRTUAL_METHOD(bool, isFlagSet, 2, (int flag), (this, flag))
	VIRTUAL_METHOD(void, removeFlags, 4, (int flags), (this, flags))
	VIRTUAL_METHOD(int, getFlags, 5, (), (this))
	VIRTUAL_METHOD(ConCommandBase*, getNext, 9, (), (this))
};

struct ConVar : public ConCommandBase
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
