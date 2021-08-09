#include <array>

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

void Animations::copyLerpedBones(Matrix3x4 *out) noexcept
{
	if (out) std::copy(desyncedBones.begin(), desyncedBones.end(), out);
}

bool Animations::desyncedAnimations(const UserCmd &cmd, bool sendPacket) noexcept
{
	assert(desyncedState);

	bool matrixUpdated = false;

	if (!localPlayer) return matrixUpdated;

	if (!memory->input->isCameraInThirdPerson) return matrixUpdated;

	if (static auto spawnTime = localPlayer->spawnTime(); !interfaces->engine->isInGame() || spawnTime != localPlayer->spawnTime())
	{
		memory->createState(desyncedState, localPlayer.get());
		spawnTime = localPlayer->spawnTime();
	}

	if (sendPacket)
	{
		const auto backupPoseParam = localPlayer->poseParam();
		const auto backupAbsYaw = localPlayer->getAbsAngle().y;

		memory->updateState(desyncedState, NULL, NULL, cmd.viewangles.y, cmd.viewangles.x, NULL);
		memory->invalidateBoneCache(localPlayer.get());
		memory->setAbsAngle(localPlayer.get(), Vector{0.0f, desyncedState->feetYaw, 0.0f});

		localPlayer->animationLayers()[Entity::ANIMATION_LAYER_LEAN].weight = FLT_EPSILON;

		matrixUpdated = localPlayer->setupBones(desyncedBones.data(), MAX_STUDIO_BONES, BONE_USED_BY_ANYTHING, memory->globalVars->currenttime);

		if (const auto &origin = localPlayer->getRenderOrigin(); matrixUpdated)
			for (int i = 0; i < MAX_STUDIO_BONES; i++)
			{
				desyncedBones[i].setOrigin(desyncedBones[i].origin() - origin);
			}

		localPlayer->poseParam() = backupPoseParam;
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

	static auto backupPoseParam = localPlayer->poseParam();
	static auto backupAbsYaw = state->feetYaw;

	localPlayer->animationLayers()[Entity::ANIMATION_LAYER_LEAN].weight = FLT_EPSILON;
	state->duckAmount = std::clamp(state->duckAmount, 0.0f, 1.0f);

	memory->updateState(state, NULL, NULL, cmd.viewangles.y, cmd.viewangles.x, NULL);

	matrixUpdated = localPlayer->setupBones(nullptr, MAX_STUDIO_BONES, BONE_USED_BY_ANYTHING, memory->globalVars->currenttime);

	if (sendPacket)
	{
		backupPoseParam = localPlayer->poseParam();
		backupAbsYaw = state->feetYaw;
	}

	memory->setAbsAngle(localPlayer.get(), Vector{0.0f, backupAbsYaw, 0.0f});
	localPlayer->poseParam() = backupPoseParam;

	return matrixUpdated;
}

struct ResolverData
{
	std::array<AnimLayer, MAX_ANIM_LAYERS> previousLayers;
	float previousFeetYaw = 0.0f;
	float nextLbyUpdate = 0.0f;
};

static std::array<ResolverData, 65> playerResolverData;

void Animations::resolveLBY(Entity *animatable) noexcept
{
	if (Helpers::animDataAuthenticity(animatable))
		return;

	auto state = animatable->getAnimState();
	if (!state)
		return;

	auto &resolverData = playerResolverData[animatable->index()];
	const auto lbyUpdate = Helpers::lbyUpdate(animatable, resolverData.nextLbyUpdate);
	const auto layers = animatable->animationLayers();

	state->feetYaw = resolverData.previousFeetYaw;
	animatable->updateClientSideAnimation();
	resolverData.previousFeetYaw = state->feetYaw;

	const auto backupFeetYaw = state->feetYaw;
	std::array<float, 3U> layerMovePlaybackRates;

	state->feetYaw = animatable->eyeAngles().y - 60.0f;
	animatable->updateClientSideAnimation();
	animatable->setupBones(nullptr, MAX_STUDIO_BONES, BONE_USED_BY_HITBOX, animatable->simulationTime());
	layerMovePlaybackRates[0] = layers[Entity::ANIMATION_LAYER_MOVEMENT_MOVE].playbackRate;

	state->feetYaw = animatable->eyeAngles().y;
	animatable->updateClientSideAnimation();
	animatable->setupBones(nullptr, MAX_STUDIO_BONES, BONE_USED_BY_HITBOX, animatable->simulationTime());
	layerMovePlaybackRates[1] = layers[Entity::ANIMATION_LAYER_MOVEMENT_MOVE].playbackRate;
	
	state->feetYaw = animatable->eyeAngles().y + 60.0f;
	animatable->updateClientSideAnimation();
	animatable->setupBones(nullptr, MAX_STUDIO_BONES, BONE_USED_BY_HITBOX, animatable->simulationTime());
	layerMovePlaybackRates[2] = layers[Entity::ANIMATION_LAYER_MOVEMENT_MOVE].playbackRate;

	state->feetYaw = backupFeetYaw;

	signed char side = 0;
	if ((animatable->velocity().length2D() < 0.1f || state->timeSinceStartedMoving < 0.22f) && !layers[Entity::ANIMATION_LAYER_ADJUST].weight && !layers[Entity::ANIMATION_LAYER_ADJUST].cycle && !layers[Entity::ANIMATION_LAYER_MOVEMENT_MOVE].weight)
	{
		const auto delta = Helpers::angleDiffDeg(animatable->eyeAngles().y, state->feetYaw);
		side = delta <= 0.0f ? 1 : -1;
	} else if (!static_cast<int>(layers[Entity::ANIMATION_LAYER_LEAN].weight * 1000.0f) && static_cast<int>(layers[Entity::ANIMATION_LAYER_MOVEMENT_MOVE].weight * 1000.0f) == static_cast<int>(resolverData.previousLayers[Entity::ANIMATION_LAYER_MOVEMENT_MOVE].weight * 1000.0f))
	{
		const auto negative = std::fabsf(layers[Entity::ANIMATION_LAYER_MOVEMENT_MOVE].playbackRate - layerMovePlaybackRates[0]);
		const auto zero = std::fabsf(layers[Entity::ANIMATION_LAYER_MOVEMENT_MOVE].playbackRate - layerMovePlaybackRates[1]);
		const auto positive = std::fabsf(layers[Entity::ANIMATION_LAYER_MOVEMENT_MOVE].playbackRate - layerMovePlaybackRates[2]);

		if (zero < positive || negative <= positive || positive * 1000.f)
		{
			if (zero >= negative && positive > negative && !(negative * 1000.f))
				side = 1;
		} else
			side = -1;
	} else
	{
		std::srand(static_cast<unsigned int>(memory->globalVars->serverTime()));
		side = (std::rand() & 1) * 2 - 1;
	}

	std::copy(layers, layers + animatable->getAnimationLayerCount(), resolverData.previousLayers.begin());

	state->feetYaw = Helpers::normalizeDeg(animatable->eyeAngles().y + 60.0f * side);

	state->duckAmount = std::clamp(state->duckAmount, 0.0f, 1.0f);
	animatable->updateClientSideAnimation();
	animatable->setupBones(nullptr, MAX_STUDIO_BONES, BONE_USED_BY_ANYTHING, animatable->simulationTime());
}
