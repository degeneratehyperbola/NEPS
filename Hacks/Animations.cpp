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
#include "../SDK/MemAlloc.h"

#include "../Config.h"
#include "../GameData.h"
#include "../Interfaces.h"
#include "../Memory.h"

bool Animations::clientLerped(Matrix3x4 *out, const UserCmd *cmd, bool sendPacket) noexcept
{
	bool matrixUpdated = false;

	if (!localPlayer) return matrixUpdated;

	if (!memory->input->isCameraInThirdPerson) return matrixUpdated;

	static AnimState *lerpedState;

	static bool init = false;
	if (!init)
	{
		lerpedState = static_cast<AnimState *>(memory->memalloc->alloc(sizeof(AnimState)));
		if (lerpedState)
		{
			memory->createState(lerpedState, localPlayer.get());
			init = true;
		}
	}

	static auto spawnTime = localPlayer->spawnTime();
	if (!lerpedState || !interfaces->engine->isInGame() || spawnTime != localPlayer->spawnTime())
	{
		spawnTime = localPlayer->spawnTime();
		init = false;
		return matrixUpdated;
	}

	static std::array<AnimLayer, MAX_ANIM_OVERLAYS> lerpedLayers;

	if (sendPacket)
	{
		std::copy(localPlayer->animOverlays(), localPlayer->animOverlays() + localPlayer->getAnimationLayerCount(), lerpedLayers.begin());

		const auto bPoseParam = localPlayer->poseParam();
		const auto bAbsYaw = localPlayer->getAbsAngle().y;

		*(int *)(localPlayer.get() + 0xA68) = 0;

		memory->updateState(lerpedState, NULL, NULL, cmd->viewangles.y, cmd->viewangles.x, NULL);
		memory->invalidateBoneCache(localPlayer.get());
		memory->setAbsAngle(localPlayer.get(), Vector{0.0f, lerpedState->feetYaw, 0.0f});

		std::copy(lerpedLayers.begin(), lerpedLayers.end(), localPlayer->animOverlays());
		localPlayer->getAnimationLayer(12)->weight = FLT_EPSILON;

		matrixUpdated = localPlayer->setupBones(out, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, 0.0f);

		const auto &origin = localPlayer->getRenderOrigin();
		if (matrixUpdated)
			for (int i = 0; i < MAXSTUDIOBONES; i++)
			{
				out[i].setOrigin(out[i].origin() - origin);
			}

		localPlayer->poseParam() = bPoseParam;
		memory->setAbsAngle(localPlayer.get(), Vector{0.0f, bAbsYaw, 0.0f});
	}

	return matrixUpdated;
}

void Animations::animSync(const UserCmd *cmd, bool sendPacket) noexcept
{
	if (!localPlayer) return;

	if (!memory->input->isCameraInThirdPerson || !config->misc.fixAnimation)
		return;

	static auto bPoseParam = localPlayer->poseParam();
	static auto bAbsYaw = localPlayer->getAnimState()->feetYaw;

	if (localPlayer->getAnimState()->lastClientSideAnimationUpdateFramecount == memory->globalVars->framecount)
		localPlayer->getAnimState()->lastClientSideAnimationUpdateFramecount -= 1;

	static std::array<AnimLayer, MAX_ANIM_OVERLAYS> networkedLayers;

	std::copy(localPlayer->animOverlays(), localPlayer->animOverlays() + localPlayer->getAnimationLayerCount(), networkedLayers.begin());

	localPlayer->clientAnimations() = true;
	memory->updateState(localPlayer->getAnimState(), NULL, NULL, cmd->viewangles.y, cmd->viewangles.x, NULL);
	localPlayer->clientAnimations() = false;

	localPlayer->setupBones(nullptr, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, 0.0f);

	if (sendPacket)
	{
		bPoseParam = localPlayer->poseParam();
		bAbsYaw = localPlayer->getAnimState()->feetYaw;
	}

	localPlayer->getAnimState()->duckAmount = std::clamp(localPlayer->getAnimState()->duckAmount, 0.0f, 1.0f);
	localPlayer->getAnimState()->feetYawRate = 0.0f;
	memory->setAbsAngle(localPlayer.get(), Vector{0.0f, bAbsYaw, 0.0f});
	std::copy(networkedLayers.begin(), networkedLayers.end(), localPlayer->animOverlays());
	localPlayer->poseParam() = bPoseParam;
	localPlayer->clientAnimations() = true;
}
