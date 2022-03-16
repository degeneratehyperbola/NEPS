#include "Players.h"

#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../GameData.h"

#include "../SDK/Entity.h"
#include "../SDK/EngineTrace.h"
#include "../SDK/PhysicsSurfaceProps.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/UserCmd.h"
#include "../SDK/WeaponData.h"
#include "../SDK/WeaponId.h"
#include "../SDK/StudioRender.h"

#include "../lib/Helpers.hpp"

void Players::updatePlayerList() noexcept {
	if (!config->players.enabled) return;
	if (!localPlayer) return;
    for (int i = 0; i < players.size(); i++) {
        auto player = &players.at(i);

        if (!player)
            continue;

        auto entity = interfaces->entityList->getEntity(i);
        if (!entity || !entity->isPlayer() || entity == localPlayer.get()) {
            player->invalid = true;
            player->flagged = false;
            continue;
        }
        else {
            player->invalid = false;
            player->name = entity->getPlayerName();
        }
    }
}