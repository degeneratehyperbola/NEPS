#pragma once

#include <vector>

#include "../Config.h"

class Entity;
struct ModelRenderInfo;
class Matrix3x4;
class Material;

class Chams
{
public:
	// Redirect entity models to draw with DrawModelExecute
	static void toSlowPipeline(Entity *) noexcept;

	Chams() noexcept;
	bool render(void *, void *, const ModelRenderInfo &, Matrix3x4 *) noexcept;
private:
	void renderPlayer(Entity *player) noexcept;
	void renderWeapons() noexcept;
	void renderProps() noexcept;
	void renderRagdolls() noexcept;
	void renderWorldWeapons() noexcept;
	void renderDefuser() noexcept;
	void renderC4() noexcept;
	void renderHands() noexcept;
	void renderSleeves() noexcept;

	Material *normal;
	Material *flat;
	Material *chrome;
	Material *glow;
	Material *pearlescent;
	Material *flatadditive;
	Material *animated;
	Material *glass;
	Material *crystal;
	Material *phong;
	Material *fresnel;

	constexpr auto dispatchMaterial(int id) const noexcept
	{
		switch (id)
		{
		default:
		case 0: return normal;
		case 1: return flat;
		case 2: return flatadditive;
		case 3: return animated;
		case 4: return glass;
		case 5: return chrome;
		case 6: return crystal;
		case 7: return phong;
		case 8: return fresnel;
		case 9: return glow;
		case 10: return pearlescent;
		}
	}

	bool appliedChams;
	void *ctx;
	void *state;
	const ModelRenderInfo *info;
	Matrix3x4 *customBoneToWorld;

	void applyChams(const std::array<Config::Chams::Material, 7> &chams, int health = 0, const Matrix3x4 *customMatrix = nullptr) noexcept;
};
