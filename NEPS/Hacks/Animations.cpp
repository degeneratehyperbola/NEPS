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

	static std::array<AnimLayer, MAX_ANIM_OVERLAYS> lerpedLayers;

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
		layers[12].weight = FLT_EPSILON;

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

	static std::array<AnimLayer, MAX_ANIM_OVERLAYS> networkedLayers;

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

void Animations::resolveLBY(Entity *animatable, int misses) noexcept
{
	if (!misses || !animatable || !animatable->isPlayer())
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

	const auto layers = animatable->animationLayers();

	std::array<float, 3U> records;
	const auto backupFeetYaw = state->feetYaw;

	state->feetYaw = animatable->eyeAngles().y;
	animatable->setupBones(nullptr, MAX_STUDIO_BONES, BONE_USED_BY_ANYTHING, memory->globalVars->currenttime);
	records[0] = layers[6].playbackRate;

	state->feetYaw = animatable->eyeAngles().y + 60.0f;
	animatable->setupBones(nullptr, MAX_STUDIO_BONES, BONE_USED_BY_ANYTHING, memory->globalVars->currenttime);
	records[1] = layers[6].playbackRate;

	state->feetYaw = animatable->eyeAngles().y - 60.0f;
	animatable->setupBones(nullptr, MAX_STUDIO_BONES, BONE_USED_BY_ANYTHING, memory->globalVars->currenttime);
	records[2] = layers[6].playbackRate;

	state->feetYaw = backupFeetYaw;

	signed char side = 0;
	if (animatable->velocity().length2D() < 0.1f)
	{
		const auto delta = Helpers::angleDiffDeg(state->feetYaw, animatable->eyeAngles().y);

		if (layers[3].weight == 0.0f && layers[3].cycle == 0.0f)
			side = delta <= 0.0f ? 1 : -1;
	}
	else if (!static_cast<int>(layers[12].weight * 1000) && static_cast<int>(layers[12].weight * 1000) == static_cast<int>(layers[6].weight * 1000))
	{
		const auto a = std::fabsf(layers[6].playbackRate - records[0]);
		const auto b = std::fabsf(layers[6].playbackRate - records[1]);
		const auto c = std::fabsf(layers[6].playbackRate - records[2]);

		if (a < c || b <= c || static_cast<int>(c * 1000))
		{
			if (a >= b && c > b && !static_cast<int>(b * 1000))
				side = 1;
		} else side = -1;
	}

	state->feetYaw = animatable->eyeAngles().y - animatable->getMaxDesyncAngle() * side;

	state->duckAmount = std::clamp(state->duckAmount, 0.0f, 1.0f);
	state->feetYawRate = 0.0f;

	animatable->setupBones(nullptr, MAX_STUDIO_BONES, BONE_USED_BY_ANYTHING, memory->globalVars->currenttime);
	animatable->clientAnimations() = true;
	animatable->effectFlags() = backupEffects;
}
