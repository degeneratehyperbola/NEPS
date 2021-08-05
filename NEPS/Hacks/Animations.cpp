#include <array>

#include "Animations.h"
#include "Memory.h"

#include "../SDK/Entity.h"
#include "../SDK/Input.h"
#include "../SDK/UserCmd.h"

static AnimState *lerpedState = new AnimState{};
static std::array<Matrix3x4, MAX_STUDIO_BONES> lerpedBones;

void Animations::releaseState() noexcept
{
	if (lerpedState)
		delete lerpedState;
}

void Animations::copyLerpedBones(Matrix3x4 *out) noexcept
{
	if (out) std::copy(lerpedBones.begin(), lerpedBones.end(), out);
}

bool Animations::animDesynced(const UserCmd &cmd, bool sendPacket) noexcept
{
	assert(lerpedState);

	bool matrixUpdated = false;

	if (!localPlayer) return matrixUpdated;

	if (!memory->input->isCameraInThirdPerson) return matrixUpdated;

	if (static auto spawnTime = localPlayer->spawnTime(); !interfaces->engine->isInGame() || spawnTime != localPlayer->spawnTime())
	{
		memory->createState(lerpedState, localPlayer.get());
		spawnTime = localPlayer->spawnTime();
	}

	const auto layers = localPlayer->animationLayers();

	static std::array<AnimLayer, MAX_ANIM_LAYERS> lerpedLayers;

	if (sendPacket)
	{
		std::copy(layers, layers + localPlayer->getAnimationLayerCount(), lerpedLayers.begin());

		const auto backupPoseParam = localPlayer->poseParam();
		const auto backupAbsYaw = localPlayer->getAbsAngle().y;

		*(int *)(localPlayer.get() + 0xA68) = 0;

		memory->updateState(lerpedState, NULL, NULL, cmd.viewangles.y, cmd.viewangles.x, NULL);
		memory->invalidateBoneCache(localPlayer.get());
		memory->setAbsAngle(localPlayer.get(), Vector{0.0f, lerpedState->feetYaw, 0.0f});

		std::copy(lerpedLayers.begin(), lerpedLayers.end(), layers);
		layers[Entity::ANIMATION_LAYER_LEAN].weight = FLT_EPSILON;

		matrixUpdated = localPlayer->setupBones(lerpedBones.data(), MAX_STUDIO_BONES, BONE_USED_BY_ANYTHING, memory->globalVars->currenttime);

		if (const auto &origin = localPlayer->getRenderOrigin(); matrixUpdated)
			for (int i = 0; i < MAX_STUDIO_BONES; i++)
			{
				lerpedBones[i].setOrigin(lerpedBones[i].origin() - origin);
			}

		localPlayer->poseParam() = backupPoseParam;
		memory->setAbsAngle(localPlayer.get(), Vector{0.0f, backupAbsYaw, 0.0f});
	}

	return matrixUpdated;
}

bool Animations::animSynced(const UserCmd &cmd, bool sendPacket) noexcept
{
	bool matrixUpdated = false;

	if (!localPlayer) return matrixUpdated;

	if (!memory->input->isCameraInThirdPerson || !config->misc.fixAnimation)
		return matrixUpdated;

	const auto layers = localPlayer->animationLayers();
	auto state = localPlayer->getAnimState();

	static auto backupPoseParam = localPlayer->poseParam();
	static auto backupAbsYaw = state->feetYaw;

	if (state->lastClientSideAnimationUpdateFramecount == memory->globalVars->framecount)
		state->lastClientSideAnimationUpdateFramecount -= 1;

	static std::array<AnimLayer, MAX_ANIM_LAYERS> networkedLayers;

	layers[Entity::ANIMATION_LAYER_LEAN].weight = FLT_EPSILON;
	std::copy(layers, layers + localPlayer->getAnimationLayerCount(), networkedLayers.begin());

	localPlayer->clientAnimations() = true;
	memory->updateState(state, NULL, NULL, cmd.viewangles.y, cmd.viewangles.x, NULL);
	localPlayer->clientAnimations() = false;

	matrixUpdated = localPlayer->setupBones(nullptr, MAX_STUDIO_BONES, BONE_USED_BY_ANYTHING, memory->globalVars->currenttime);

	if (sendPacket)
	{
		backupPoseParam = localPlayer->poseParam();
		backupAbsYaw = state->feetYaw;
	}

	state->duckAmount = std::clamp(state->duckAmount, 0.0f, 1.0f);
	state->feetYawRate = 0.0f;
	memory->setAbsAngle(localPlayer.get(), Vector{0.0f, backupAbsYaw, 0.0f});
	std::copy(networkedLayers.begin(), networkedLayers.end(), layers);
	localPlayer->poseParam() = backupPoseParam;
	localPlayer->clientAnimations() = true;

	return matrixUpdated;
}

struct ResolverData
{
	std::array<AnimLayer, MAX_ANIM_LAYERS> previousLayers;
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

	const auto layers = animatable->animationLayers();
	auto &resolverData = playerResolverData[animatable->index()];
	const auto lbyUpdate = Helpers::lbyUpdate(animatable, resolverData.nextLbyUpdate);

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
	}

	std::copy(layers, layers + animatable->getAnimationLayerCount(), resolverData.previousLayers.data());

	animatable->clientAnimations() = false;

	state->feetYaw = animatable->eyeAngles().y - animatable->getMaxDesyncAngle() * side;

	state->duckAmount = std::clamp(state->duckAmount, 0.0f, 1.0f);
	state->feetYawRate = 0.0f;

	animatable->updateClientSideAnimation();
	animatable->setupBones(nullptr, MAX_STUDIO_BONES, BONE_USED_BY_ANYTHING, animatable->simulationTime());
	animatable->clientAnimations() = true;
}
