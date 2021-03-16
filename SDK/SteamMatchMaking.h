#pragma once

#include "VirtualMethod.h"
#include "SteamID.h"

class SteamMatchMaking
{
	VIRTUAL_METHOD(bool, sendLobbyChatMsg, 26, (SteamID lobby, const void *msgBody, int size), (this, lobby, msgBody, size))
};
