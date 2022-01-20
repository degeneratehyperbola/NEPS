#include "../Interfaces.h"
#include "../Memory.h"

#include "../SDK/Engine.h"
#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/GameMovement.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/MoveHelper.h"
#include "../SDK/Prediction.h"

#include "EnginePrediction.h"

static int localPlayerFlags;
float m_oldCurrenttime;
float m_oldFrametime;

void EnginePrediction::run(UserCmd* cmd) noexcept
{
    if (!localPlayer)
        return;
    
    localPlayerFlags = localPlayer->flags();

    *memory->predictionRandomSeed = 0;

    const auto oldCurrenttime = memory->globalVars->currentTime;
    const auto oldFrametime = memory->globalVars->frameTime;

    memory->globalVars->currentTime = memory->globalVars->serverTime();
    memory->globalVars->frameTime = memory->globalVars->intervalPerTick;

    memory->moveHelper->setHost(localPlayer.get());
    interfaces->prediction->setupMove(localPlayer.get(), cmd, memory->moveHelper, memory->moveData);
    interfaces->gameMovement->processMovement(localPlayer.get(), memory->moveData);
    interfaces->prediction->finishMove(localPlayer.get(), cmd, memory->moveData);
    memory->moveHelper->setHost(nullptr);

    *memory->predictionRandomSeed = -1;

    memory->globalVars->currentTime = oldCurrenttime;
    memory->globalVars->frameTime = oldFrametime;
}

void EnginePrediction::start(UserCmd* cmd) noexcept
{
	if (!localPlayer)
		return;

	*memory->predictionRandomSeed = 0;

	m_oldCurrenttime = memory->globalVars->currentTime;
	m_oldFrametime = memory->globalVars->frameTime;

	memory->globalVars->currentTime = memory->globalVars->serverTime();
	memory->globalVars->frameTime = memory->globalVars->intervalPerTick;

	memory->moveHelper->setHost(localPlayer.get());
	interfaces->prediction->setupMove(localPlayer.get(), cmd, memory->moveHelper, memory->moveData);
	interfaces->gameMovement->processMovement(localPlayer.get(), memory->moveData);
	interfaces->prediction->finishMove(localPlayer.get(), cmd, memory->moveData);
}

void EnginePrediction::end(UserCmd* cmd) noexcept
{
	if (!localPlayer)
		return;

	memory->moveHelper->setHost(nullptr);

	*memory->predictionRandomSeed = -1;

	memory->globalVars->currentTime = m_oldCurrenttime;
	memory->globalVars->frameTime = m_oldFrametime;
}

int EnginePrediction::getFlags() noexcept
{
    return localPlayerFlags;
}
