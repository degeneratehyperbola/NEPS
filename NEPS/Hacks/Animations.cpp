#include <array>

#include "Aimbot.h"
#include "Animations.h"
#include "Backtrack.h"
#include "Memory.h"

#include "../GameData.h"

#include "../SDK/Entity.h"
#include "../SDK/Input.h"
#include "../SDK/UserCmd.h"

static AnimState *desyncedState = new AnimState{};
static std::array<Matrix3x4, MAX_STUDIO_BONES> desyncedBones;

void Animations::releaseState() noexcept
{
	if (desyncedState)
		delete desyncedState;

	if (localPlayer)
		localPlayer->clientAnimations() = true;
}

void Animations::getDesyncedBones(Matrix3x4 *out) noexcept
{
	if (out) std::copy(desyncedBones.begin(), desyncedBones.end(), out);
}

void Animations::desyncedAnimations(const UserCmd &cmd, bool sendPacket) noexcept
{
	assert(desyncedState);

	auto &poseParams = localPlayer->poseParams();
	const auto layers = localPlayer->animLayers();
	if (!desyncedState || !layers)
		return;

	if (!localPlayer || !localPlayer->isAlive()) return;

	if (!memory->input->isCameraInThirdPerson) return;

	if (static auto spawnTime = 0.0f; !interfaces->engine->isInGame() || spawnTime != localPlayer->spawnTime())
	{
		memory->createState(desyncedState, localPlayer.get());
		spawnTime = localPlayer->spawnTime();
	}

	if (sendPacket)
	{
		const auto backupYaw = localPlayer->getAbsAngle().y;
		std::array<float, PoseParam_Count> backupPoseParams;
		std::array<AnimLayer, AnimLayer_Count> backupLayers;

		std::copy(layers, layers + localPlayer->getAnimLayerCount(), backupLayers.begin());
		std::copy(poseParams.begin(), poseParams.end(), backupPoseParams.begin());

		desyncedState->update(cmd.viewangles);
		memory->setAbsAngle(localPlayer.get(), {0.0f, desyncedState->goalFeetYaw, 0.0f});

		memory->invalidateBoneCache(localPlayer.get());
		const bool updated = localPlayer->setupBones(desyncedBones.data(), MAX_STUDIO_BONES, BONE_USED_BY_ANYTHING, memory->globalVars->currentTime);

		if (const auto &origin = localPlayer->getRenderOrigin(); updated)
			for (auto &m : desyncedBones)
				m.setOrigin(m.origin() - origin);

		std::copy(backupLayers.begin(), backupLayers.end(), layers);
		std::copy(backupPoseParams.begin(), backupPoseParams.end(), poseParams.begin());
		memory->setAbsAngle(localPlayer.get(), {0.0f, backupYaw, 0.0f});
	}
}

void Animations::fixAnimation(const UserCmd &cmd, bool sendPacket) noexcept
{
	if (!localPlayer) return;

	if (!config->misc.fixAnimation || !localPlayer->isAlive() || !memory->input->isCameraInThirdPerson)
	{
		localPlayer->clientAnimations() = true;
		localPlayer->updateClientSideAnimation();
		localPlayer->clientAnimations() = false;
		return;
	}

	auto &poseParams = localPlayer->poseParams();
	auto state = localPlayer->animState();
	const auto layers = localPlayer->animLayers();
	if (!state || !layers)
		return;

	static auto networkedYaw = state->goalFeetYaw;
	static std::array<float, PoseParam_Count> networkedPoseParams;
	static std::array<AnimLayer, AnimLayer_Count> networkedLayers;

	static int previousTick = 0;
	if (previousTick != memory->globalVars->tickCount)
	{
		previousTick = memory->globalVars->tickCount;
		std::copy(layers, layers + localPlayer->getAnimLayerCount(), networkedLayers.begin());
		localPlayer->clientAnimations() = true;
		state->update(cmd.viewangles);
		localPlayer->updateClientSideAnimation();
		localPlayer->clientAnimations() = false;

		if (sendPacket)
		{
			std::copy(poseParams.begin(), poseParams.end(), networkedPoseParams.begin());
			networkedYaw = state->goalFeetYaw;
		}
	}
	
	memory->setAbsAngle(localPlayer.get(), {0.0f, networkedYaw, 0.0f});
	std::copy(networkedLayers.begin(), networkedLayers.end(), layers);
	std::copy(networkedPoseParams.begin(), networkedPoseParams.end(), poseParams.begin());

	memory->invalidateBoneCache(localPlayer.get());
	localPlayer->setupBones(nullptr, MAX_STUDIO_BONES, BONE_USED_BY_ANYTHING, memory->globalVars->currentTime);
}

void Animations::resolve(Entity *animatable) noexcept
{
	auto state = animatable->animState();
	if (!state)
		return;

	constexpr auto authentic = [](Entity *animatable, const PlayerData *playerData) noexcept
	{
		if (animatable->moveType() == MoveType::Ladder) return true;
		if (animatable->moveType() == MoveType::Noclip) return true;
		if (animatable->isBot()) return true;
		const float simulationTime = animatable->simulationTime();
		const auto remoteActiveWeapon = animatable->getActiveWeapon();
		if (remoteActiveWeapon && Helpers::timeToTicks(remoteActiveWeapon->lastShotTime()) == Helpers::timeToTicks(simulationTime)) return true;

		if (playerData->chokedPackets < -3) return true;
		if (playerData->lbyUpdate) return true;

		return false;
	};

	GameData::Lock lock;
	if (auto playerData = GameData::playerByHandle(animatable->handle()))
	{
		playerData->updateLby(animatable);

		const float maxDesync = std::fminf(std::fabsf(animatable->getMaxDesyncAngle()), 58.0f);
		const float lowDesync = std::fminf(35.0f, maxDesync);
		if (!authentic(animatable, playerData))
		{
			const float lbyDelta = Helpers::angleDiffDeg(animatable->eyeAngles().y, state->goalFeetYaw);

			const std::array<float, 3U> positions = {-maxDesync, 0.0f, maxDesync};
			std::vector<float> distances;
			for (const auto &position : positions)
				distances.emplace_back(std::fabsf(position - lbyDelta));

			const auto current = std::distance(distances.begin(), std::min_element(distances.begin(), distances.end()));

			state->goalFeetYaw = Helpers::normalizeDeg(animatable->eyeAngles().y + positions[(current + Aimbot::getMisses() + 1) % positions.size()]);
		}
	}

	memory->invalidateBoneCache(animatable);
	animatable->setupBones(nullptr, MAX_STUDIO_BONES, BONE_USED_BY_ANYTHING, memory->globalVars->currentTime);
}
