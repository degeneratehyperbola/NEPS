#include <array>

#include "Animations.h"

#include "../SDK/AnimState.h"
#include "../SDK/ConVar.h"
#include "../SDK/Entity.h"
#include "../SDK/Input.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/Matrix3x4.h"
#include "../SDK/ModelInfo.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/UserCmd.h"
#include "../SDK/Vector.h"

#include "../Config.h"
#include "../GameData.h"
#include "../Interfaces.h"
#include "../Memory.h"

static AnimState *lerpedState = new AnimState;
static std::array<Matrix3x4, MAX_STUDIO_BONES> lerpedBones;

void Animations::releaseState() noexcept
{
	if (lerpedState)
		delete lerpedState;
}

bool Animations::clientLerped(const UserCmd &cmd, bool sendPacket) noexcept
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
		std::copy(localPlayer->animOverlays(), localPlayer->animOverlays() + localPlayer->getAnimationLayerCount(), lerpedLayers.begin());

		const auto backupPoseParam = localPlayer->poseParam();
		const auto backupAbsYaw = localPlayer->getAbsAngle().y;

		*(int *)(localPlayer.get() + 0xA68) = 0;

		memory->updateState(lerpedState, NULL, NULL, cmd.viewangles.y, cmd.viewangles.x, NULL);
		memory->invalidateBoneCache(localPlayer.get());
		memory->setAbsAngle(localPlayer.get(), Vector{0.0f, lerpedState->feetYaw, 0.0f});

		std::copy(lerpedLayers.begin(), lerpedLayers.end(), localPlayer->animOverlays());
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

bool Animations::animSync(const UserCmd &cmd, bool sendPacket) noexcept
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

	std::copy(localPlayer->animOverlays(), localPlayer->animOverlays() + localPlayer->getAnimationLayerCount(), networkedLayers.begin());

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
	std::copy(networkedLayers.begin(), networkedLayers.end(), localPlayer->animOverlays());
	localPlayer->poseParam() = backupPoseParam;
	localPlayer->clientAnimations() = true;

	return matrixUpdated;
}

void Animations::resolve(Entity *animatable) noexcept
{
	if (!animatable || !animatable->isPlayer())
		return;

	if (Helpers::animDataAuthenticity(animatable))
		return;

	auto state = animatable->getAnimState();
	if (!state)
		return;

	const auto backupEffects = animatable->effectFlags();
	animatable->effectFlags() |= 8;

	memory->invalidateBoneCache(animatable);
	memory->updateState(state, nullptr, animatable->thirdPersonAngles().x, animatable->thirdPersonAngles().y, 0.0f, nullptr);
	
	// Return random desync position out of 3 possible
	// This hereby gives us a 33% chance to resolve target correctly, unless difference between those positions is much more than the width of the head (when target is using some bizarre extended anti-aim with dual berettas and pitch = 0)
	const auto delta = Helpers::angleDiffDeg(state->feetYaw, state->eyeYaw);
	constexpr std::array<float, 3> positions = {-60.0f, 0.0f, 60.0f};
	size_t current = 0;
	for (size_t i = 1; i < positions.size(); ++i)
	{
		if (Helpers::equals(delta, positions[i], 30.0f))
			current = i;
	}

	state->feetYaw = state->eyeYaw + positions[(current + 1 + std::rand() % (positions.size() - 1)) % positions.size()];
	state->duckAmount = std::clamp(state->duckAmount, 0.0f, 1.0f);
	state->feetYawRate = 0.0f;

	animatable->effectFlags() = backupEffects;
}

void Animations::copyLerpedBones(Matrix3x4 *out) noexcept
{
	if (out) std::copy(lerpedBones.begin(), lerpedBones.end(), out);
}
