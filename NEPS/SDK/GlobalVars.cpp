#include "GlobalVars.h"
#include "UserCmd.h"
#include "Entity.h"

float GlobalVars::serverTime(UserCmd *cmd) const noexcept
{
	static int tick;
	static UserCmd *previousCmd;

	if (cmd)
	{
		if (localPlayer && (!previousCmd || previousCmd->hasBeenPredicted))
			tick = localPlayer->tickBase();
		else
			tick++;
		previousCmd = cmd;
	}
	return tick * intervalPerTick;
}
