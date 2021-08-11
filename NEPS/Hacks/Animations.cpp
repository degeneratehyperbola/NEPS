#include <array>

#include "Aimbot.h"
#include "Animations.h"
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
}

void Animations::copyDesyncedBones(Matrix3x4 *out) noexcept
{
	if (out) std::copy(desyncedBones.begin(), desyncedBones.end(), out);
}

bool Animations::desyncedAnimations(const UserCmd &cmd, bool sendPacket) noexcept
{
	assert(desyncedState);

	bool matrixUpdated = false;

	if (!desyncedState)
		return matrixUpdated;

	if (!localPlayer) return matrixUpdated;

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

		memory->updateState(desyncedState, NULL, NULL, cmd.viewangles.y, cmd.viewangles.x, NULL);
		memory->invalidateBoneCache(localPlayer.get());
		memory->setAbsAngle(localPlayer.get(), Vector{0.0f, desyncedState->feetYaw, 0.0f});

		localPlayer->animationLayers()[AnimLayer_Lean].weight = FLT_EPSILON;
		
		matrixUpdated = localPlayer->setupBones(desyncedBones.data(), MAX_STUDIO_BONES, BONE_USED_BY_ANYTHING, memory->globalVars->currenttime);

		if (const auto &origin = localPlayer->getRenderOrigin(); matrixUpdated)
			for (int i = 0; i < MAX_STUDIO_BONES; i++)
			{
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

	if (!memory->input->isCameraInThirdPerson || !config->misc.fixAnimation)
		return matrixUpdated;

	auto state = localPlayer->getAnimState();
	if (!state)
		return matrixUpdated;

	static auto backupPoseParams = localPlayer->poseParams();
	static auto backupAbsYaw = state->feetYaw;

	localPlayer->animationLayers()[AnimLayer_Lean].weight = FLT_EPSILON;
	state->duckAmount = std::clamp(state->duckAmount, 0.0f, 1.0f);

	memory->updateState(state, NULL, NULL, cmd.viewangles.y, cmd.viewangles.x, NULL);

	matrixUpdated = localPlayer->setupBones(nullptr, MAX_STUDIO_BONES, BONE_USED_BY_ANYTHING, memory->globalVars->currenttime);
	
	if (sendPacket)
	{
		backupPoseParams = localPlayer->poseParams();
		backupAbsYaw = state->feetYaw;
	}

	memory->setAbsAngle(localPlayer.get(), Vector{0.0f, backupAbsYaw, 0.0f});
	localPlayer->poseParams() = backupPoseParams;

	return matrixUpdated;
}

struct ResolverData
{
	std::array<AnimLayer, AnimLayer_Count> previousLayers;
	float previousFeetYaw;
	float nextLbyUpdate;
	int misses;
};

static std::array<ResolverData, 65> playerResolverData;

void Animations::resolve(Entity *animatable) noexcept
{
	if (Helpers::animDataAuthenticity(animatable))
		return;

	auto state = animatable->getAnimState();
	if (!state)
		return;

	auto &resolverData = playerResolverData[animatable->index()];
	const auto simulationTime = animatable->simulationTime();
	const auto lbyUpdate = Helpers::lbyUpdate(animatable, resolverData.nextLbyUpdate);
	const auto layers = animatable->animationLayers();

	if (animatable->handle() == Aimbot::getTargetHandle())
		resolverData.misses = Aimbot::getMisses();

	state->feetYaw = resolverData.previousFeetYaw;
	animatable->updateClientSideAnimation();
	resolverData.previousFeetYaw = state->feetYaw;

	const auto backupFeetYaw = state->feetYaw;
	std::array<float, 3U> layerMovePlaybackRates;

	state->feetYaw = animatable->eyeAngles().y - 60.0f;
	animatable->updateClientSideAnimation();
	animatable->setupBones(nullptr, MAX_STUDIO_BONES, BONE_USED_BY_HITBOX, simulationTime);
	layerMovePlaybackRates[0] = layers[AnimLayer_MovementMove].playbackRate;

	state->feetYaw = animatable->eyeAngles().y;
	animatable->updateClientSideAnimation();
	animatable->setupBones(nullptr, MAX_STUDIO_BONES, BONE_USED_BY_HITBOX, simulationTime);
	layerMovePlaybackRates[1] = layers[AnimLayer_MovementMove].playbackRate;
	
	state->feetYaw = animatable->eyeAngles().y + 60.0f;
	animatable->updateClientSideAnimation();
	animatable->setupBones(nullptr, MAX_STUDIO_BONES, BONE_USED_BY_HITBOX, simulationTime);
	layerMovePlaybackRates[2] = layers[AnimLayer_MovementMove].playbackRate;

	state->feetYaw = backupFeetYaw;

	signed char side = 0;
	if ((animatable->velocity().length2D() < 0.1f || state->timeSinceStartedMoving < 0.22f) && !layers[AnimLayer_Adjust].weight && !layers[AnimLayer_Adjust].cycle && !layers[AnimLayer_MovementMove].weight)
	{
		const auto delta = Helpers::angleDiffDeg(animatable->eyeAngles().y, state->feetYaw);
		side = delta <= 0.0f ? 1 : -1;
	} else if (!static_cast<int>(layers[AnimLayer_Lean].weight * 1000.0f) && static_cast<int>(layers[AnimLayer_MovementMove].weight * 1000.0f) == static_cast<int>(resolverData.previousLayers[AnimLayer_MovementMove].weight * 1000.0f))
	{
		const auto negative = std::fabsf(layers[AnimLayer_MovementMove].playbackRate - layerMovePlaybackRates[0]);
		const auto zero = std::fabsf(layers[AnimLayer_MovementMove].playbackRate - layerMovePlaybackRates[1]);
		const auto positive = std::fabsf(layers[AnimLayer_MovementMove].playbackRate - layerMovePlaybackRates[2]);

		if (zero < positive || negative <= positive || positive * 1000.f)
		{
			if (zero >= negative && positive > negative && !(negative * 1000.f))
				side = 1;
		} else
			side = -1;
	} else if (animatable->velocity().length2D() < 0.1f || state->timeSinceStartedMoving < 0.22f)
	{
		const auto delta = Helpers::angleDiffDeg(animatable->eyeAngles().y, state->feetYaw);
		if (delta > 35.0f)
			side = -1;
		else if (delta < -35.0f)
			side = 1;
	}
	else
		side = resolverData.misses % 3 - 1;

	std::copy(layers, layers + animatable->getAnimationLayerCount(), resolverData.previousLayers.begin());

	state->feetYaw = Helpers::normalizeDeg(animatable->eyeAngles().y + 60.0f * side);

	state->duckAmount = std::clamp(state->duckAmount, 0.0f, 1.0f);
	animatable->updateClientSideAnimation();
	animatable->setupBones(nullptr, MAX_STUDIO_BONES, BONE_USED_BY_ANYTHING, simulationTime);
}
