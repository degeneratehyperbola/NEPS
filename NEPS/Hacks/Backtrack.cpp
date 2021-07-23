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

static std::array<std::deque<Backtrack::Record>, 65> records;

struct Cvars {
    ConVar* updateRateVar;
    ConVar* maxUpdateRateVar;
    ConVar* interpVar;
    ConVar* interpRatioVar;
    ConVar* minInterpRatioVar;
    ConVar* maxInterpRatioVar;
    ConVar* maxUnlagVar;
};

static Cvars cvars;

void Backtrack::update(FrameStage stage) noexcept
{
	if (stage == FrameStage::RENDER_START)
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
			record.origin = entity->getAbsOrigin();
			record.simulationTime = entity->simulationTime();

			record.hasHelmet = entity->hasHelmet();
			record.armor = entity->armor();

			record.important = Helpers::animDataAuthenticity(entity);

			entity->setupBones(record.matrix, 256, BONE_USED_BY_ANYTHING, memory->globalVars->currenttime);

			records[i].push_front(record);

			while (records[i].size() > 3 && records[i].size() > static_cast<size_t>(Helpers::timeToTicks(static_cast<float>(config->backtrack.timeLimit) / 1000.0f)))
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

	if (!(cmd->buttons & UserCmd::IN_ATTACK))
		return;

	if (!localPlayer)
		return;

	auto localPlayerEyePosition = localPlayer->getEyePosition();
	const auto aimPunch = localPlayer->getAimPunch();

	Entity *bestTarget = interfaces->entityList->getEntityFromHandle(Aimbot::getTargetHandle());
	const Record *bestRecord = nullptr;
	auto bestFov = 255.0f;
	Vector bestTargetHeadOrigin = Vector{};

	if (bestTarget)
	{
		bestRecord = Aimbot::getTargetRecord();
	} else
	{
		for (int i = 1; i <= interfaces->engine->getMaxClients(); i++)
		{
			auto entity = interfaces->entityList->getEntity(i);
			if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive()
				|| !entity->isOtherEnemy(localPlayer.get()))
				continue;

			const auto &headOrigin = entity->getBonePosition(8);

			auto angle = Helpers::calculateRelativeAngle(localPlayerEyePosition, headOrigin, cmd->viewangles + (config->backtrack.recoilBasedFov ? aimPunch : Vector{}));
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

				auto angle = Helpers::calculateRelativeAngle(localPlayerEyePosition, record.origin, cmd->viewangles + (config->backtrack.recoilBasedFov ? aimPunch : Vector{}));
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
		const float remainder = std::fmodf(getLerp(), memory->globalVars->intervalPerTick);
		float fractionedTime = bestRecord->simulationTime;
		if (remainder > 0.0f)
			fractionedTime += memory->globalVars->intervalPerTick - remainder;

		memory->setAbsOrigin(bestTarget, bestRecord->origin);
		cmd->tickCount = Helpers::timeToTicks(fractionedTime + getLerp());
	}
}

const std::deque<Backtrack::Record> &Backtrack::getRecords(std::size_t index) noexcept
{
	return records[index];
}

float Backtrack::getLerp() noexcept
{
	auto ratio = std::clamp(cvars.interpRatioVar->getFloat(), cvars.minInterpRatioVar->getFloat(), cvars.maxInterpRatioVar->getFloat());
	return (std::max)(cvars.interpVar->getFloat(), (ratio / ((cvars.maxUpdateRateVar) ? cvars.maxUpdateRateVar->getFloat() : cvars.updateRateVar->getFloat())));
}

bool Backtrack::valid(float simTime) noexcept
{
	const auto network = interfaces->engine->getNetworkChannel();
	if (!network)
		return false;

	auto delta = std::clamp(network->getLatency(0) + network->getLatency(1) + getLerp(), 0.0f, cvars.maxUnlagVar->getFloat()) - (memory->globalVars->serverTime() - simTime);
	return std::abs(delta) <= 0.2f;
}

void Backtrack::init() noexcept
{
	cvars.updateRateVar = interfaces->cvar->findVar("cl_updaterate");
	cvars.maxUpdateRateVar = interfaces->cvar->findVar("sv_maxupdaterate");
	cvars.interpVar = interfaces->cvar->findVar("cl_interp");
	cvars.interpRatioVar = interfaces->cvar->findVar("cl_interp_ratio");
	cvars.minInterpRatioVar = interfaces->cvar->findVar("sv_client_min_interp_ratio");
	cvars.maxInterpRatioVar = interfaces->cvar->findVar("sv_client_max_interp_ratio");
	cvars.maxUnlagVar = interfaces->cvar->findVar("sv_maxunlag");
}
