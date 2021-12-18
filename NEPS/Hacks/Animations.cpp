#include <array>

#include "Aimbot.h"
#include "Animations.h"
#include "Backtrack.h"
#include "Memory.h"

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

bool Animations::desyncedAnimations(const UserCmd &cmd, bool sendPacket) noexcept
{
	assert(desyncedState);

	bool matrixUpdated = false;

	if (!desyncedState)
		return matrixUpdated;

	if (!localPlayer || !localPlayer->isAlive()) return matrixUpdated;

	if (!memory->input->isCameraInThirdPerson) return matrixUpdated;

	if (static auto spawnTime = localPlayer->spawnTime(); !interfaces->engine->isInGame() || spawnTime != localPlayer->spawnTime())
	{
		memory->createState(desyncedState, localPlayer.get());
		spawnTime = localPlayer->spawnTime();
	}

	if (sendPacket)
	{
		const auto backupPoseParams = localPlayer->poseParams();
		const auto backupAbsYaw = localPlayer->getAbsAngle().y;

		memory->updateState(desyncedState, nullptr, 0.0f, cmd.viewangles.y, cmd.viewangles.x, nullptr);
		memory->invalidateBoneCache(localPlayer.get());
		memory->setAbsAngle(localPlayer.get(), Vector{0.0f, desyncedState->goalFeetYaw, 0.0f});

		localPlayer->animationLayers()[AnimLayer_Lean].weight = FLT_EPSILON;
		
		matrixUpdated = localPlayer->setupBones(desyncedBones.data(), MAX_STUDIO_BONES, BONE_USED_BY_ANYTHING, memory->globalVars->currenttime);

		if (const auto &origin = localPlayer->getRenderOrigin(); matrixUpdated)
		{
			for (int i = 0; i < MAX_STUDIO_BONES; i++)
				desyncedBones[i].setOrigin(desyncedBones[i].origin() - origin);
		}

		localPlayer->poseParams() = backupPoseParams;
		memory->setAbsAngle(localPlayer.get(), Vector{0.0f, backupAbsYaw, 0.0f});
	}

	return matrixUpdated;
}

bool Animations::fixAnimation(const UserCmd &cmd, bool sendPacket) noexcept
{
	bool matrixUpdated = false;

	if (!localPlayer) return matrixUpdated;

	if (!config->misc.fixAnimation || !localPlayer->isAlive() || !memory->input->isCameraInThirdPerson)
	{
		localPlayer->clientAnimations() = true;
		localPlayer->updateClientSideAnimation();
		localPlayer->clientAnimations() = false;
		return matrixUpdated;
	}

	auto state = localPlayer->animState();
	if (!state)
		return matrixUpdated;

	localPlayer->clientAnimations() = false;

	static auto backupPoseParams = localPlayer->poseParams();
	static auto backupAbsYaw = state->goalFeetYaw;

	static int previousTick = 0;
	if (previousTick != memory->globalVars->tickCount)
	{
		previousTick = memory->globalVars->tickCount;

		localPlayer->clientAnimations() = true;
		memory->updateState(state, nullptr, 0.0f, cmd.viewangles.y, cmd.viewangles.x, nullptr);
		localPlayer->updateClientSideAnimation();
		localPlayer->clientAnimations() = false;

		if (sendPacket)
		{
			backupPoseParams = localPlayer->poseParams();
			backupAbsYaw = state->goalFeetYaw;
		}
	}

	localPlayer->animationLayers()[AnimLayer_Lean].weight = FLT_EPSILON;

	memory->invalidateBoneCache(localPlayer.get());
	matrixUpdated = localPlayer->setupBones(nullptr, MAX_STUDIO_BONES, BONE_USED_BY_ANYTHING, memory->globalVars->currenttime);

	state->duckAmount = std::clamp(state->duckAmount, 0.0f, 1.0f);
	state->moveWeight = 0.0f;

	memory->setAbsAngle(localPlayer.get(), Vector{0.0f, backupAbsYaw, 0.0f});
	localPlayer->poseParams() = backupPoseParams;

	return matrixUpdated;
}

struct ResolverData
{
	std::array<AnimLayer, AnimLayer_Count> previousLayers;
	float feetYaw;
	float nextLbyUpdate;
	int misses;
	int previousTick;
};

static std::array<ResolverData, 65> playerResolverData;

void Animations::resolve(Entity *animatable) noexcept
{
	auto state = animatable->animState();
	if (!state)
		return;

	auto &resolverData = playerResolverData[animatable->index()];
	const bool lbyUpdate = Helpers::lbyUpdate(animatable, resolverData.nextLbyUpdate);
	const auto layers = animatable->animationLayers();

	if (animatable->handle() == Aimbot::getTargetHandle())
		resolverData.misses = Aimbot::getMisses();

	animatable->clientAnimations() = true;

	const auto simulationTick = Helpers::timeToTicks(animatable->simulationTime());
	if (resolverData.previousTick != simulationTick)
	{
		resolverData.previousTick = simulationTick;

		const float maxDesync = std::fminf(std::fabsf(animatable->getMaxDesyncAngle()), 58.0f);
		const float lowDesync = std::fminf(35.0f, maxDesync);
		if (!Helpers::animDataAuthenticity(animatable) && !lbyUpdate)
		{
			animatable->updateClientSideAnimation();

			const float lbyDelta = Helpers::angleDiffDeg(animatable->eyeAngles().y, state->goalFeetYaw);

			const std::array<float, 3U> positions = {-maxDesync, 0.0f, maxDesync};
			std::vector<float> distances;
			for (const auto &position : positions)
				distances.emplace_back(std::fabsf(position - lbyDelta));

			const auto current = std::distance(distances.begin(), std::min_element(distances.begin(), distances.end()));

			resolverData.feetYaw = Helpers::normalizeDeg(animatable->eyeAngles().y + positions[(current + resolverData.misses + 1) % positions.size()]);
		} else resolverData.feetYaw = state->goalFeetYaw;
	}

	layers[AnimLayer_Lean].weight = FLT_EPSILON;

	state->duckAmount = std::clamp(state->duckAmount, 0.0f, 1.0f);
	state->landingDuckAdditiveAmount = std::clamp(state->landingDuckAdditiveAmount, 0.0f, 1.0f);
	state->feetCycle = layers[AnimLayer_MovementMove].cycle;
	state->moveWeight = layers[AnimLayer_MovementMove].weight;
	state->goalFeetYaw = resolverData.feetYaw;

	std::copy(layers, layers + animatable->getAnimationLayerCount(), resolverData.previousLayers.begin());

	memory->invalidateBoneCache(animatable);
	animatable->setupBones(nullptr, MAX_STUDIO_BONES, BONE_USED_BY_ANYTHING, memory->globalVars->currenttime);
}
