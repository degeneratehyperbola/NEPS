#include "Backtrack.h"
#include "Aimbot.h"

#include "../Config.h"

#include "../SDK/Cvar.h"
#include "../SDK/ConVar.h"
#include "../SDK/Entity.h"
#include "../SDK/FrameStage.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/UserCmd.h"

#include "../lib/Helpers.hpp"

static std::array<std::deque<Record>, 65> records;

void Backtrack::update(FrameStage stage) noexcept
{
	int timeLimit = config->backtrack.timeLimit;
	if (timeLimit <= 0 || timeLimit >= 201) { config->backtrack.fakeLatency = true; }
	else { config->backtrack.fakeLatency = false; }

	if (stage == FrameStage::RenderStart)
	{
		if (!config->backtrack.enabled || !localPlayer || !localPlayer->isAlive())
		{
			for (auto &record : records)
				record.clear();
			return;
		}

		for (int i = 1; i <= interfaces->engine->getMaxClients(); i++)
		{
			auto entity = interfaces->entityList->getEntity(i);
			if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive() || !entity->isOtherEnemy(localPlayer.get()))
			{
				records[i].clear();
				continue;
			}

			if (!records[i].empty() && records[i].front().simulationTime == entity->simulationTime())
				continue;

			Record record;
			record.ownerIdx = entity->index();
			record.origin = entity->getAbsOrigin();
			record.simulationTime = entity->simulationTime();

			record.hasHelmet = entity->hasHelmet();
			record.armor = entity->armor();

			record.important = Helpers::animDataAuthenticity(entity);

			std::copy(entity->boneCache().memory, entity->boneCache().memory + entity->boneCache().size, record.bones);

			records[i].push_front(record);

			while (records[i].size() > 3 && records[i].size() > static_cast<size_t>(Helpers::timeToTicks(static_cast<float>(config->backtrack.timeLimit) / 1000.f)))
				records[i].pop_back();

			if (auto invalid = std::find_if(std::cbegin(records[i]), std::cend(records[i]), [](const Record &rec) { return !valid(rec.simulationTime); }); invalid != std::cend(records[i]))
				records[i].erase(invalid, std::cend(records[i]));
		}
	}
}

void Backtrack::run(UserCmd *cmd) noexcept
{
	if (!config->backtrack.enabled)
		return;

	if (!(cmd->buttons & UserCmd::Button_Attack))
		return;

	if (!localPlayer)
		return;

	Entity *bestTarget = interfaces->entityList->getEntityFromHandle(Aimbot::getTargetHandle());
	const Record *bestRecord = nullptr;

	if (bestTarget)
		bestRecord = Aimbot::getTargetRecord();
	else
	{
		auto localPlayerEyePosition = localPlayer->getEyePosition();
		const auto aimPunch = localPlayer->getAimPunch();

		auto bestFov = 255.0f;
		Vector bestTargetHeadOrigin;
		for (int i = 1; i <= interfaces->engine->getMaxClients(); i++)
		{
			auto entity = interfaces->entityList->getEntity(i);
			if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive()
				|| !entity->isOtherEnemy(localPlayer.get()))
				continue;

			const auto &headOrigin = entity->getBonePosition(8);

			auto angle = Helpers::calculateRelativeAngle(localPlayerEyePosition, headOrigin, cmd->viewangles + (config->backtrack.recoilBasedFov ? aimPunch : Vector{ }));
			auto fov = std::hypotf(angle.x, angle.y);
			if (fov < bestFov)
			{
				bestFov = fov;
				bestTarget = entity;
				bestTargetHeadOrigin = headOrigin;
			}
		}

		if (bestTarget)
		{
			if (records[bestTarget->index()].size() <= 3 || (!config->backtrack.ignoreSmoke && memory->lineGoesThroughSmoke(localPlayer->getEyePosition(), bestTargetHeadOrigin, 1)))
				return;

			bestFov = 255.0f;

			for (const auto &record : records[bestTarget->index()])
			{
				if (!valid(record.simulationTime))
					continue;

				auto angle = Helpers::calculateRelativeAngle(localPlayerEyePosition, record.origin, cmd->viewangles + (config->backtrack.recoilBasedFov ? aimPunch : Vector{ }));
				auto fov = std::hypotf(angle.x, angle.y);
				if (fov < bestFov)
				{
					bestFov = fov;
					bestRecord = &record;
				}
			}
		}
	}

	if (bestRecord && bestTarget)
	{
		memory->setAbsOrigin(bestTarget, bestRecord->origin);
		cmd->tickCount = Helpers::timeToTicks(bestRecord->simulationTime + getLerp());
	}
}

const std::deque<Record> &Backtrack::getRecords(std::size_t index) noexcept
{
	return records[index];
}

float Backtrack::getLerp() noexcept
{
	static auto updateRateVar = interfaces->cvar->findVar("cl_updaterate");
	static auto maxUpdateRateVar = interfaces->cvar->findVar("sv_maxupdaterate");
	static auto interpVar = interfaces->cvar->findVar("cl_interp");
	static auto interpRatioVar = interfaces->cvar->findVar("cl_interp_ratio");
	static auto minInterpRatioVar = interfaces->cvar->findVar("sv_client_min_interp_ratio");
	static auto maxInterpRatioVar = interfaces->cvar->findVar("sv_client_max_interp_ratio");

	auto ratio = std::clamp(interpRatioVar->getFloat(), minInterpRatioVar->getFloat(), maxInterpRatioVar->getFloat());
	return std::max(interpVar->getFloat(), (ratio / (maxUpdateRateVar ? maxUpdateRateVar->getFloat() : updateRateVar->getFloat())));
}

bool Backtrack::valid(float simTime) noexcept
{
	const auto networkChannel = interfaces->engine->getNetworkChannel();
	if (!networkChannel)
		return false;

	static auto maxUnlagVar = interfaces->cvar->findVar("sv_maxunlag");

	auto delta = std::clamp(networkChannel->getLatency(0) + networkChannel->getLatency(1) + getLerp(), 0.0f, maxUnlagVar->getFloat()) - (memory->globalVars->serverTime() - simTime);
	return std::abs(delta) <= 0.2f;
}