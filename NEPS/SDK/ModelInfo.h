#pragma once

#include <cstdint>

#include "Pad.h"
#include "Vector.h"
#include "VirtualMethod.h"

struct StudioBbox
{
	int bone;
	int group;
	Vector bbMin;
	Vector bbMax;
	int hitboxNameIndex;
	Vector offsetOrientation;
	float capsuleRadius;
	int	unused[4];
};

enum Hitbox
{
	Hitbox_Head,
	Hitbox_Neck,
	Hitbox_Pelvis,
	Hitbox_Belly,
	Hitbox_Thorax,
	Hitbox_LowerChest,
	Hitbox_UpperChest,
	Hitbox_RightThigh,
	Hitbox_LeftThigh,
	Hitbox_RightCalf,
	Hitbox_LeftCalf,
	Hitbox_RightFoot,
	Hitbox_LeftFoot,
	Hitbox_RightHand,
	Hitbox_LeftHand,
	Hitbox_RightUpperArm,
	Hitbox_RightForearm,
	Hitbox_LeftUpperArm,
	Hitbox_LeftForearm,
	Hitbox_Max
};

struct StudioHitboxSet
{
	int nameIndex;
	int numHitboxes;
	int hitboxIndex;

	const char *getName() noexcept
	{
		return nameIndex ? reinterpret_cast<const char *>(std::uintptr_t(this) + nameIndex) : nullptr;
	}

	StudioBbox *getHitbox(int i) noexcept
	{
		return i >= 0 && i < numHitboxes ? reinterpret_cast<StudioBbox *>(std::uintptr_t(this) + hitboxIndex) + i : nullptr;
	}
};

#define MAX_STUDIO_BONES 256
#define BONE_USED_BY_HITBOX 0x100
#define BONE_USED_BY_ANYTHING 0x7FF00

struct StudioBone
{
	int nameIndex;
	int	parent;
	PAD(152)
	int flags;
	PAD(52)

	const char *getName() const noexcept
	{
		return nameIndex ? reinterpret_cast<const char *>(std::uintptr_t(this) + nameIndex) : nullptr;
	}
};

struct StudioHdr
{
	int id;
	int version;
	int checksum;
	char name[64];
	int length;
	Vector eyePosition;
	Vector illumPosition;
	Vector hullMin;
	Vector hullMax;
	Vector bbMin;
	Vector bbMax;
	int flags;
	int numBones;
	int boneIndex;
	int numBoneControllers;
	int boneControllerIndex;
	int numHitboxSets;
	int hitboxSetIndex;

	const StudioBone *getBone(int i) const noexcept
	{
		return i >= 0 && i < numBones ? reinterpret_cast<StudioBone *>(std::uintptr_t(this) + boneIndex) + i : nullptr;
	}

	StudioHitboxSet *getHitboxSet(int i) noexcept
	{
		return i >= 0 && i < numHitboxSets ? reinterpret_cast<StudioHitboxSet *>(std::uintptr_t(this) + hitboxSetIndex) + i : nullptr;
	}
};

struct Model;

class ModelInfo
{
public:
	VIRTUAL_METHOD(int, getModelIndex, 2, (const char *name), (this, name))
	VIRTUAL_METHOD(StudioHdr *, getStudioModel, 32, (const Model *model), (this, model))
};
