#include "../Config.h"
#include "Glow.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../SDK/Entity.h"
#include "../SDK/ClientClass.h"
#include "../SDK/GlowObjectManager.h"
#include "../SDK/GlobalVars.h"
#include "../lib/Helpers.hpp"

static std::vector<std::pair<int, int>> customGlowEntities;

void Glow::render() noexcept
{
    if (!localPlayer)
		return;

	const auto &glow = config->glow;

	Glow::clearCustomObjects();

	const auto highestEntityIndex = interfaces->entityList->getHighestEntityIndex();
	for (int i = interfaces->engine->getMaxClients() + 1; i <= highestEntityIndex; ++i)
	{
		const auto entity = interfaces->entityList->getEntity(i);
		if (!entity || entity->isDormant())
			continue;

		switch (entity->getClientClass()->classId)
		{
		case ClassId::EconEntity:
		case ClassId::GrenadeProjectile:
		case ClassId::BreachChargeProjectile:
		case ClassId::BumpMineProjectile:
		case ClassId::DecoyProjectile:
		case ClassId::MolotovProjectile:
		case ClassId::SensorGrenadeProjectile:
		case ClassId::SmokeGrenadeProjectile:
		case ClassId::SnowballProjectile:
		case ClassId::Hostage:
		//case ClassId::Ragdoll:
			if (!memory->glowObjectManager->hasGlowEffect(entity))
			{
				if (auto index{memory->glowObjectManager->registerGlowObject(entity)}; index != -1)
					customGlowEntities.emplace_back(i, index);
			}
		}
	}

	for (int i = 0; i < memory->glowObjectManager->glowObjectDefinitions.size; i++)
	{
		GlowObjectDefinition &glowobject = memory->glowObjectManager->glowObjectDefinitions[i];

		auto entity = glowobject.entity;

		if (glowobject.isUnused() || !entity || entity->isDormant())
			continue;

		auto applyGlow = [&glowobject](decltype(glow[0]) &glow, int health = 0) noexcept
		{
			if (glow.enabled)
			{
				glowobject.renderWhenOccluded = true;
				glowobject.renderWhenUnoccluded = false;
				glowobject.glowAlpha = glow.color[3];
				glowobject.glowStyle = glow.style;
				glowobject.glowAlphaMax = 0.6f;
				if (glow.healthBased && health)
				{
					const auto &&[r, g, b] = Helpers::hsvToRgb(std::lerp(0.0f, 1.0f / 3.0f, std::clamp(health / 100.0f, 0.0f, 1.0f)), 1.0f, 1.0f);
					glowobject.glowColor = {r, g, b};
				}
				else if (glow.rainbow)
				{
					const auto &&[r, g, b] = Helpers::rainbowColor(glow.rainbowSpeed);
					glowobject.glowColor = {r, g, b};
				} else
					glowobject.glowColor = {glow.color[0], glow.color[1], glow.color[2]};
				glowobject.fullBloomRender = glow.full;
			}
		};

		auto applyPlayerGlow = [applyGlow](decltype(glow[0]) &glowAll, decltype(glow[0]) &glowVisible, decltype(glow[0]) &glowOccluded, Entity *entity) noexcept
		{
			if (glowAll.enabled)
				applyGlow(glowAll, entity->health());
			else if (glowVisible.enabled && entity->visibleTo(localPlayer.get()))
				applyGlow(glowVisible, entity->health());
			else if (glowOccluded.enabled && !entity->visibleTo(localPlayer.get()))
				applyGlow(glowOccluded, entity->health());
		};

		switch (entity->getClientClass()->classId)
		{
		case ClassId::Player:
			if (!entity->isAlive())
				break;
			if (auto activeWeapon{entity->getActiveWeapon()}; activeWeapon && activeWeapon->getClientClass()->classId == ClassId::C4 && activeWeapon->c4StartedArming())
				applyPlayerGlow(glow[6], glow[7], glow[8], entity);
			else if (entity->isDefusing())
				applyPlayerGlow(glow[9], glow[10], glow[11], entity);
			else if (entity == localPlayer.get())
				applyGlow(glow[12], entity->health());
			else if (entity->isOtherEnemy(localPlayer.get()))
				applyPlayerGlow(glow[3], glow[4], glow[5], entity);
			else
				applyPlayerGlow(glow[0], glow[1], glow[2], entity);
			break;
		case ClassId::C4: applyGlow(glow[14]); break;
		case ClassId::PlantedC4: applyGlow(glow[15]); break;
		case ClassId::Chicken: applyGlow(glow[16]); break;
		case ClassId::EconEntity: applyGlow(glow[17]); break;

		case ClassId::GrenadeProjectile:
		case ClassId::BreachChargeProjectile:
		case ClassId::BumpMineProjectile:
		case ClassId::DecoyProjectile:
		case ClassId::MolotovProjectile:
		case ClassId::SensorGrenadeProjectile:
		case ClassId::SmokeGrenadeProjectile:
		case ClassId::SnowballProjectile:
			applyGlow(glow[18]); break;

		case ClassId::Hostage: applyGlow(glow[19]); break;
		//case ClassId::Ragdoll: applyGlow(glow[20]); break;
		default:
			if (entity->isWeapon())
			{
				applyGlow(glow[13]);
				if (!glow[13].enabled) glowobject.renderWhenOccluded = false;
			}
		}
	}
}

void Glow::clearCustomObjects() noexcept
{
	for (const auto &[entityIndex, glowObjectIndex] : customGlowEntities)
		memory->glowObjectManager->unregisterGlowObject(glowObjectIndex);

	customGlowEntities.clear();
}
