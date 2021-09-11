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

		memory->updateState(desyncedState, NULL, NULL, cmd.viewangles.y, cmd.viewangles.x, NULL);
		memory->invalidateBoneCache(localPlayer.get());
		memory->setAbsAngle(localPlayer.get(), Vector{0.0f, desyncedState->feetYaw, 0.0f});

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

	auto state = localPlayer->getAnimState();
	if (!state)
		return matrixUpdated;

	localPlayer->clientAnimations() = false;

	state->lastClientSideAnimationUpdateFramecount = std::min(state->lastClientSideAnimationUpdateFramecount, memory->globalVars->framecount - 1);

	static auto backupPoseParams = localPlayer->poseParams();
	static auto backupAbsYaw = state->feetYaw;

	static int previousTick = 0;
	if (previousTick != memory->globalVars->tickCount)
	{
		previousTick = memory->globalVars->tickCount;

		localPlayer->clientAnimations() = true;
		memory->updateState(state, NULL, NULL, cmd.viewangles.y, cmd.viewangles.x, NULL);
		localPlayer->updateClientSideAnimation();
		localPlayer->clientAnimations() = false;

		if (sendPacket)
		{
			backupPoseParams = localPlayer->poseParams();
			backupAbsYaw = state->feetYaw;
		}
	}

	localPlayer->animationLayers()[AnimLayer_Lean].weight = FLT_EPSILON;

	matrixUpdated = localPlayer->setupBones(nullptr, MAX_STUDIO_BONES, BONE_USED_BY_ANYTHING, memory->globalVars->currenttime);

	state->duckAmount = std::clamp(state->duckAmount, 0.0f, 1.0f);
	state->feetYawRate = 0.0f;

	memory->setAbsAngle(localPlayer.get(), Vector{0.0f, backupAbsYaw, 0.0f});
	localPlayer->poseParams() = backupPoseParams;

	return matrixUpdated;
}

struct ResolverData
{
	std::array<AnimLayer, AnimLayer_Count> previousLayers;
	float desyncAmount;
	signed char desyncSide;
	float feetYaw;
	float previousFeetYaw;
	float nextLbyUpdate;
	int misses;
	int previousTick;
};

static std::array<ResolverData, 65> playerResolverData;

void Animations::resolve(Entity *animatable) noexcept
{
	auto state = animatable->getAnimState();
	if (!state)
		return;

	auto &resolverData = playerResolverData[animatable->index()];
	const bool lbyUpdate = Helpers::lbyUpdate(animatable, resolverData.nextLbyUpdate);
	const auto layers = animatable->animationLayers();

	if (animatable->handle() == Aimbot::getTargetHandle())
		resolverData.misses = Aimbot::getMisses();

	state->lastClientSideAnimationUpdateFramecount = std::min(state->lastClientSideAnimationUpdateFramecount, memory->globalVars->framecount - 1);

	animatable->clientAnimations() = true;

	state->feetYaw = resolverData.previousFeetYaw;
	animatable->updateClientSideAnimation();
	resolverData.previousFeetYaw = state->feetYaw;

	const auto simulationTick = Helpers::timeToTicks(animatable->simulationTime());
	if (resolverData.previousTick != simulationTick)
	{
		resolverData.previousTick = simulationTick;

		const float maxDesync = std::fminf(std::fabsf(animatable->getMaxDesyncAngle()), 58.0f);
		if (!Helpers::animDataAuthenticity(animatable) && !lbyUpdate)
		{
			const float lbyDelta = Helpers::angleDiffDeg(animatable->eyeAngles().y, resolverData.previousFeetYaw);
			const float lbyTargetDelta = Helpers::angleDiffDeg(animatable->eyeAngles().y, animatable->lbyTarget());
			const bool notMove = animatable->velocity().length2D() < 0.1f && std::fabsf(animatable->velocity().z) < 100.0f;

			switch (resolverData.misses % 6)
			{
			case 3:
			case 4:
			case 5:
				resolverData.desyncAmount = std::fminf(35.0f, maxDesync);
				break;
			default:
				resolverData.desyncAmount = maxDesync;
				break;
			}

			switch (resolverData.misses % 3)
			{
			case 1:
				resolverData.desyncSide = -resolverData.desyncSide;
				break;
			case 2:
				resolverData.desyncSide = 0;
				break;
			default:
				resolverData.desyncSide = lbyTargetDelta > 0.0f ? -1 : 1;
				break;
			}

			resolverData.feetYaw = Helpers::normalizeDeg(animatable->eyeAngles().y + resolverData.desyncAmount * resolverData.desyncSide);
		}
	}

	layers[AnimLayer_Lean].weight = FLT_EPSILON;

	state->duckAmount = std::clamp(state->duckAmount, 0.0f, 1.0f);
	state->landingDuckAdditiveAmount = std::clamp(state->landingDuckAdditiveAmount, 0.0f, 1.0f);
	state->feetCycle = layers[AnimLayer_MovementMove].cycle;
	state->feetYawRate = layers[AnimLayer_MovementMove].weight;
	state->feetYaw = resolverData.feetYaw;

	std::copy(layers, layers + animatable->getAnimationLayerCount(), resolverData.previousLayers.begin());

	animatable->updateClientSideAnimation();
	animatable->setupBones(nullptr, MAX_STUDIO_BONES, BONE_USED_BY_ANYTHING, memory->globalVars->currenttime);

	animatable->clientAnimations() = false;
}
