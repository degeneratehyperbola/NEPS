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

	static std::array<AnimLayer, MAX_ANIM_OVERLAYS> lerpedLayers;

	if (sendPacket)
	{
		std::copy(localPlayer->animationLayers(), localPlayer->animationLayers() + localPlayer->getAnimationLayerCount(), lerpedLayers.begin());

		const auto backupPoseParam = localPlayer->poseParam();
		const auto backupAbsYaw = localPlayer->getAbsAngle().y;

		*(int *)(localPlayer.get() + 0xA68) = 0;

		memory->updateState(lerpedState, NULL, NULL, cmd.viewangles.y, cmd.viewangles.x, NULL);
		memory->invalidateBoneCache(localPlayer.get());
		memory->setAbsAngle(localPlayer.get(), Vector{0.0f, lerpedState->feetYaw, 0.0f});

		std::copy(lerpedLayers.begin(), lerpedLayers.end(), localPlayer->animationLayers());
		localPlayer->getAnimationLayer(12)->weight = FLT_EPSILON;

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

	auto state = localPlayer->getAnimState();

	static auto backupPoseParam = localPlayer->poseParam();
	static auto backupAbsYaw = state->feetYaw;

	if (state->lastClientSideAnimationUpdateFramecount == memory->globalVars->framecount)
		state->lastClientSideAnimationUpdateFramecount -= 1;

	static std::array<AnimLayer, MAX_ANIM_OVERLAYS> networkedLayers;

	std::copy(localPlayer->animationLayers(), localPlayer->animationLayers() + localPlayer->getAnimationLayerCount(), networkedLayers.begin());

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
	std::copy(networkedLayers.begin(), networkedLayers.end(), localPlayer->animationLayers());
	localPlayer->poseParam() = backupPoseParam;
	localPlayer->clientAnimations() = true;

	return matrixUpdated;
}

void Animations::resolveLBY(Entity *animatable, int seed) noexcept
{
	if (!seed || !animatable || !animatable->isPlayer())
		return;

	if (Helpers::animDataAuthenticity(animatable))
		return;

	auto state = animatable->getAnimState();
	if (!state)
		return;

	if (state->lastClientSideAnimationUpdateFramecount == memory->globalVars->framecount)
		state->lastClientSideAnimationUpdateFramecount -= 1;

	const auto backupEffects = animatable->effectFlags();
	animatable->effectFlags() |= 8;
	
	animatable->clientAnimations() = true;
	memory->updateState(state, nullptr, animatable->eyeAngles().x, animatable->eyeAngles().y, 0.0f, nullptr);
	animatable->clientAnimations() = false;
	
	// Return random desync position out of 3 possible
	// This hereby gives us a 33% chance to resolve target correctly, unless difference between those positions is much more than the width of the head (when target is using some bizarre extended anti-aim with dual berettas and pitch = 0)
	std::srand(seed);

	const auto delta = Helpers::angleDiffDeg(state->feetYaw, state->eyeYaw);
	const std::array<float, 3> positions = {animatable->getMaxDesyncAngle(), 0.0f, -animatable->getMaxDesyncAngle()};

	state->duckAmount = std::clamp(state->duckAmount, 0.0f, 1.0f);
	state->feetYawRate = 0.0f;
	state->feetYaw = state->eyeYaw + positions[rand() % positions.size()];

	animatable->clientAnimations() = true;
	animatable->effectFlags() = backupEffects;
}
