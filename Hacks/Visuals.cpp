#include "../fnv.h"
#include "../Helpers.h"
#include "../GameData.h"
#include "Visuals.h"

#include "../SDK/Beam.h"
#include "../SDK/ConVar.h"
#include "../SDK/Entity.h"
#include "../SDK/FrameStage.h"
#include "../SDK/GameEvent.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/Input.h"
#include "../SDK/UserCmd.h"
#include "../SDK/Material.h"
#include "../SDK/MaterialSystem.h"
#include "../SDK/NetworkStringTable.h"
#include "../SDK/RenderContext.h"
#include "../SDK/Surface.h"
#include "../SDK/ModelInfo.h"
#include "../SDK/ViewSetup.h"

#include "../imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui_internal.h"

#include <array>

void Visuals::playerModel(FrameStage stage) noexcept
{
	if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
		return;

	static int originalIdx = 0;

	if (!localPlayer)
	{
		originalIdx = 0;
		return;
	}

	constexpr auto getModel = [](Team team) constexpr noexcept -> const char *
	{
		constexpr std::array models{
			//"models/player/custom_player/legacy/anime/astolfo/astolfo_v1.mdl", // proof that all cheaters are femboys
			"models/player/custom_player/legacy/ctm_fbi_variantb.mdl",
			"models/player/custom_player/legacy/ctm_fbi_variantf.mdl",
			"models/player/custom_player/legacy/ctm_fbi_variantg.mdl",
			"models/player/custom_player/legacy/ctm_fbi_varianth.mdl",
			"models/player/custom_player/legacy/ctm_sas_variantf.mdl",
			"models/player/custom_player/legacy/ctm_st6_variante.mdl",
			"models/player/custom_player/legacy/ctm_st6_variantg.mdl",
			"models/player/custom_player/legacy/ctm_st6_varianti.mdl",
			"models/player/custom_player/legacy/ctm_st6_variantk.mdl",
			"models/player/custom_player/legacy/ctm_st6_variantm.mdl",
			"models/player/custom_player/legacy/tm_balkan_variantf.mdl",
			"models/player/custom_player/legacy/tm_balkan_variantg.mdl",
			"models/player/custom_player/legacy/tm_balkan_varianth.mdl",
			"models/player/custom_player/legacy/tm_balkan_varianti.mdl",
			"models/player/custom_player/legacy/tm_balkan_variantj.mdl",
			"models/player/custom_player/legacy/tm_leet_variantf.mdl",
			"models/player/custom_player/legacy/tm_leet_variantg.mdl",
			"models/player/custom_player/legacy/tm_leet_varianth.mdl",
			"models/player/custom_player/legacy/tm_leet_varianti.mdl",
			"models/player/custom_player/legacy/tm_phoenix_variantf.mdl",
			"models/player/custom_player/legacy/tm_phoenix_variantg.mdl",
			"models/player/custom_player/legacy/tm_phoenix_varianth.mdl",

			"models/player/custom_player/legacy/tm_pirate.mdl",
			"models/player/custom_player/legacy/tm_pirate_varianta.mdl",
			"models/player/custom_player/legacy/tm_pirate_variantb.mdl",
			"models/player/custom_player/legacy/tm_pirate_variantc.mdl",
			"models/player/custom_player/legacy/tm_pirate_variantd.mdl",
			"models/player/custom_player/legacy/tm_anarchist.mdl",
			"models/player/custom_player/legacy/tm_anarchist_varianta.mdl",
			"models/player/custom_player/legacy/tm_anarchist_variantb.mdl",
			"models/player/custom_player/legacy/tm_anarchist_variantc.mdl",
			"models/player/custom_player/legacy/tm_anarchist_variantd.mdl",
			"models/player/custom_player/legacy/tm_balkan_varianta.mdl",
			"models/player/custom_player/legacy/tm_balkan_variantb.mdl",
			"models/player/custom_player/legacy/tm_balkan_variantc.mdl",
			"models/player/custom_player/legacy/tm_balkan_variantd.mdl",
			"models/player/custom_player/legacy/tm_balkan_variante.mdl",
			"models/player/custom_player/legacy/tm_jumpsuit_varianta.mdl",
			"models/player/custom_player/legacy/tm_jumpsuit_variantb.mdl",
			"models/player/custom_player/legacy/tm_jumpsuit_variantc.mdl"

			"models/player/custom_player/legacy/tm_phoenix_varianti.mdl",
			"models/player/custom_player/legacy/ctm_st6_variantj.mdl",
			"models/player/custom_player/legacy/ctm_st6_variantl.mdl",
			"models/player/custom_player/legacy/tm_balkan_variantk.mdl",
			"models/player/custom_player/legacy/tm_balkan_variantl.mdl",
			"models/player/custom_player/legacy/ctm_swat_variante.mdl",
			"models/player/custom_player/legacy/ctm_swat_variantf.mdl",
			"models/player/custom_player/legacy/ctm_swat_variantg.mdl",
			"models/player/custom_player/legacy/ctm_swat_varianth.mdl",
			"models/player/custom_player/legacy/ctm_swat_varianti.mdl",
			"models/player/custom_player/legacy/ctm_swat_variantj.mdl",
			"models/player/custom_player/legacy/tm_professional_varf.mdl",
			"models/player/custom_player/legacy/tm_professional_varf1.mdl",
			"models/player/custom_player/legacy/tm_professional_varf2.mdl",
			"models/player/custom_player/legacy/tm_professional_varf3.mdl",
			"models/player/custom_player/legacy/tm_professional_varf4.mdl",
			"models/player/custom_player/legacy/tm_professional_varg.mdl",
			"models/player/custom_player/legacy/tm_professional_varh.mdl",
			"models/player/custom_player/legacy/tm_professional_vari.mdl",
			"models/player/custom_player/legacy/tm_professional_varj.mdl"
		};

		switch (team)
		{
		case Team::TT: return static_cast<std::size_t>(config->visuals.playerModelT - 1) < models.size() ? models[config->visuals.playerModelT - 1] : nullptr;
		case Team::CT: return static_cast<std::size_t>(config->visuals.playerModelCT - 1) < models.size() ? models[config->visuals.playerModelCT - 1] : nullptr;
		default: return nullptr;
		}
	};

	if (const auto model = getModel(localPlayer->team()))
	{
		if (stage == FrameStage::RENDER_START)
		{
			originalIdx = localPlayer->modelIndex();
			if (const auto modelprecache = interfaces->networkStringTableContainer->findTable("modelprecache"))
			{
				modelprecache->addString(false, model);
				const auto viewmodelArmConfig = memory->getPlayerViewmodelArmConfigForPlayerModel(model);
				modelprecache->addString(false, viewmodelArmConfig[2]);
				modelprecache->addString(false, viewmodelArmConfig[3]);
			}
		}

		const auto idx = stage == FrameStage::RENDER_END && originalIdx ? originalIdx : interfaces->modelInfo->getModelIndex(model);

		localPlayer->setModelIndex(idx);

		if (const auto ragdoll = interfaces->entityList->getEntityFromHandle(localPlayer->ragdoll()))
			ragdoll->setModelIndex(idx);
	}
}

void Visuals::colorWorld() noexcept
{
	if (!config->visuals.world.enabled && !config->visuals.sky.enabled && !config->visuals.props.enabled)
		return;

	for (short h = interfaces->materialSystem->firstMaterial(); h != interfaces->materialSystem->invalidMaterial(); h = interfaces->materialSystem->nextMaterial(h))
	{
		const auto mat = interfaces->materialSystem->getMaterial(h);

		if (!mat || mat->isErrorMaterial() || mat->getReferenceCount() < 1)
			continue;

		if (config->visuals.world.enabled && std::strstr(mat->getTextureGroupName(), "World"))
		{
			if (config->visuals.world.rainbow)
				mat->colorModulate(Helpers::rainbowColor(config->visuals.world.rainbowSpeed));
			else
				mat->colorModulate(config->visuals.world.color[0], config->visuals.world.color[1], config->visuals.world.color[2]);
			mat->alphaModulate(config->visuals.world.color[3]);
			mat->setMaterialVarFlag(MaterialVarFlag::NO_DRAW, config->visuals.world.color[3] == 0.0f);
		} else if (config->visuals.props.enabled && std::strstr(mat->getTextureGroupName(), "StaticProp"))
		{
			if (config->visuals.props.rainbow)
				mat->colorModulate(Helpers::rainbowColor(config->visuals.props.rainbowSpeed));
			else
				mat->colorModulate(config->visuals.props.color[0], config->visuals.props.color[1], config->visuals.props.color[2]);
			mat->alphaModulate(config->visuals.props.color[3]);
			mat->setMaterialVarFlag(MaterialVarFlag::NO_DRAW, config->visuals.props.color[3] == 0.0f);
		} else if (config->visuals.sky.enabled && std::strstr(mat->getTextureGroupName(), "SkyBox"))
		{
			if (config->visuals.sky.rainbow)
				mat->colorModulate(Helpers::rainbowColor(config->visuals.sky.rainbowSpeed));
			else
				mat->colorModulate(config->visuals.sky.color);
		}
	}
}

void Visuals::modifySmoke(FrameStage stage) noexcept
{
	if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
		return;

	constexpr std::array smokeMaterials = {
		"particle/vistasmokev1/vistasmokev1_emods",
		"particle/vistasmokev1/vistasmokev1_emods_impactdust",
		"particle/vistasmokev1/vistasmokev1_fire",
		"particle/vistasmokev1/vistasmokev1_smokegrenade"
	};

	for (const auto mat : smokeMaterials)
	{
		const auto material = interfaces->materialSystem->findMaterial(mat);
		material->setMaterialVarFlag(MaterialVarFlag::NO_DRAW, stage == FrameStage::RENDER_START && config->visuals.noSmoke);
		material->setMaterialVarFlag(MaterialVarFlag::WIREFRAME, stage == FrameStage::RENDER_START && config->visuals.wireframeSmoke);
	}
}

void Visuals::thirdperson() noexcept
{
	if (!localPlayer || !config->visuals.thirdPerson.keyMode) return;

	static Helpers::KeyBindState thirdPerson;

	if (localPlayer->isAlive())
		memory->input->isCameraInThirdPerson = thirdPerson[config->visuals.thirdPerson];
	else if (localPlayer->getObserverTarget() && (localPlayer->observerMode() == ObsMode::InEye || localPlayer->observerMode() == ObsMode::Chase))
	{ 
		memory->input->isCameraInThirdPerson = false;
		localPlayer->observerMode() = thirdPerson[config->visuals.thirdPerson] ? ObsMode::InEye : ObsMode::Chase;
	}
}

void Visuals::removeVisualRecoil(FrameStage stage) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    static Vector aimPunch;
    static Vector viewPunch;

    if (stage == FrameStage::RENDER_START) {
        aimPunch = localPlayer->aimPunchAngle();
        viewPunch = localPlayer->viewPunchAngle();

        if (config->visuals.noAimPunch)
            localPlayer->aimPunchAngle() = Vector{ };

        if (config->visuals.noViewPunch)
            localPlayer->viewPunchAngle() = Vector{ };

    } else if (stage == FrameStage::RENDER_END) {
        localPlayer->aimPunchAngle() = aimPunch;
        localPlayer->viewPunchAngle() = viewPunch;
    }
}

void Visuals::removeBlur(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    static auto blur = interfaces->materialSystem->findMaterial("dev/scope_bluroverlay");
    blur->setMaterialVarFlag(MaterialVarFlag::NO_DRAW, stage == FrameStage::RENDER_START && config->visuals.noBlur);
}

void Visuals::removeGrass(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

	constexpr auto getGrassMaterialName = []() noexcept -> const char *
	{
		switch (fnv::hashRuntime(interfaces->engine->getLevelName()))
		{
		case fnv::hash("dz_blacksite"): return "detail/detailsprites_survival";
		case fnv::hash("dz_sirocco"): return "detail/dust_massive_detail_sprites";
		case fnv::hash("coop_autumn"): return "detail/autumn_detail_sprites";
		case fnv::hash("dz_frostbite"): return "ski/detail/detailsprites_overgrown_ski";
			// dz_junglety has been removed in 7/23/2020 patch
			// case fnv::hash("dz_junglety"): return "detail/tropical_grass";
		default: return nullptr;
		}
	};

    if (const auto grassMaterialName = getGrassMaterialName())
        interfaces->materialSystem->findMaterial(grassMaterialName)->setMaterialVarFlag(MaterialVarFlag::NO_DRAW, stage == FrameStage::RENDER_START && config->visuals.noGrass);
}

#define DRAW_SCREEN_EFFECT(material) \
{ \
    const auto drawFunction = memory->drawScreenEffectMaterial; \
    int w, h; \
    interfaces->surface->getScreenSize(w, h); \
    __asm { \
        __asm push h \
        __asm push w \
        __asm push 0 \
        __asm xor edx, edx \
        __asm mov ecx, material \
        __asm call drawFunction \
        __asm add esp, 12 \
    } \
}

void Visuals::applyScreenEffects() noexcept
{
    if (!config->visuals.screenEffect)
        return;

    const auto material = interfaces->materialSystem->findMaterial([] {
        constexpr std::array effects{
            "effects/dronecam",
            "effects/underwater_overlay",
            "effects/healthboost",
            "effects/dangerzone_screen"
        };

        if (config->visuals.screenEffect <= 2 || static_cast<std::size_t>(config->visuals.screenEffect - 2) >= effects.size())
            return effects[0];
        return effects[config->visuals.screenEffect - 2];
    }());

    if (config->visuals.screenEffect == 1)
        material->findVar("$c0_x")->setValue(0.0f);
    else if (config->visuals.screenEffect == 2)
        material->findVar("$c0_x")->setValue(0.1f);
    else if (config->visuals.screenEffect >= 4)
        material->findVar("$c0_x")->setValue(1.0f);

    DRAW_SCREEN_EFFECT(material)
}

void Visuals::hitEffect(GameEvent* event) noexcept
{
    if (config->visuals.hitEffect && localPlayer) {
        static float lastHitTime = 0.0f;

        if (event && interfaces->engine->getPlayerForUserID(event->getInt("attacker")) == localPlayer->index()) {
            lastHitTime = memory->globalVars->realtime;
            return;
        }

		const auto timeSinceHit = memory->globalVars->realtime - lastHitTime;

		if (timeSinceHit > config->visuals.hitEffectTime)
			return;

        constexpr auto getEffectMaterial = [] {
            static constexpr const char* effects[]{
            "effects/dronecam",
            "effects/underwater_overlay",
            "effects/healthboost",
            "effects/dangerzone_screen"
            };

            if (config->visuals.hitEffect <= 2)
                return effects[0];
            return effects[config->visuals.hitEffect - 2];
        };

           
        auto material = interfaces->materialSystem->findMaterial(getEffectMaterial());
        if (config->visuals.hitEffect == 1)
            material->findVar("$c0_x")->setValue(0.0f);
        else if (config->visuals.hitEffect == 2)
            material->findVar("$c0_x")->setValue(1.0f - timeSinceHit / config->visuals.hitEffectTime * 0.1f);
        else if (config->visuals.hitEffect >= 4)
            material->findVar("$c0_x")->setValue(1.0f - timeSinceHit / config->visuals.hitEffectTime);

        DRAW_SCREEN_EFFECT(material)
    }
}

void Visuals::killEffect(GameEvent *event) noexcept
{
	if (config->visuals.killEffect && localPlayer)
	{
		static float lastKillTime = 0.0f;

		if (event && interfaces->engine->getPlayerForUserID(event->getInt("attacker")) == localPlayer->index())
		{
			lastKillTime = memory->globalVars->realtime;
			return;
		}

		const auto timeSinceKill = memory->globalVars->realtime - lastKillTime;

		if (timeSinceKill > config->visuals.killEffectTime)
			return;

		constexpr auto getEffectMaterial = []
		{
			static constexpr const char *effects[]{
			"effects/dronecam",
			"effects/underwater_overlay",
			"effects/healthboost",
			"effects/dangerzone_screen"
			};

			if (config->visuals.killEffect <= 2)
				return effects[0];
			return effects[config->visuals.killEffect - 2];
		};


		auto material = interfaces->materialSystem->findMaterial(getEffectMaterial());
		if (config->visuals.killEffect == 1)
			material->findVar("$c0_x")->setValue(0.0f);
		else if (config->visuals.killEffect == 2)
			material->findVar("$c0_x")->setValue((1.0f - timeSinceKill / config->visuals.killEffectTime) * 0.1f);
		else if (config->visuals.killEffect >= 4)
			material->findVar("$c0_x")->setValue(1.0f - timeSinceKill / config->visuals.killEffectTime);

		DRAW_SCREEN_EFFECT(material)
	}
}

void Visuals::hitMarker(GameEvent *event, ImDrawList *drawList) noexcept
{
	if (config->visuals.hitMarker == 0)
		return;

	static float lastHitTime = 0.0f;
	static int dmg = 0;

	if (event)
	{
		if (localPlayer && event->getInt("attacker") == localPlayer->getUserId())
		{
			lastHitTime = memory->globalVars->realtime;
			dmg = event->getInt("dmg_health");
		}
		return;
	}

	const auto timeSinceHit = memory->globalVars->realtime - lastHitTime;

	if (timeSinceHit > config->visuals.hitMarkerTime)
		return;

	switch (config->visuals.hitMarker)
	{
	case 1:
	{
		const auto &mid = ImGui::GetIO().DisplaySize / 2.0f;
		const auto color = Helpers::calculateColor(1.0f, 1.0f, 1.0f, 1.0f - timeSinceHit / config->visuals.hitMarkerTime);
		drawList->AddLine({mid.x - 10, mid.y - 10}, {mid.x - 4, mid.y - 4}, color);
		drawList->AddLine({mid.x + 10.5f, mid.y - 10.5f}, {mid.x + 4.5f, mid.y - 4.5f}, color);
		drawList->AddLine({mid.x + 10.5f, mid.y + 10.5f}, {mid.x + 4.5f, mid.y + 4.5f}, color);
		drawList->AddLine({mid.x - 10, mid.y + 10}, {mid.x - 4, mid.y + 4}, color);
		break;
	}
	case 2:
	{
		const auto &mid = ImGui::GetIO().DisplaySize / 2.0f;
		const auto color = Helpers::calculateColor(1.0f, 1.0f, 1.0f, 1.0f - timeSinceHit / config->visuals.hitMarkerTime);
		drawList->AddCircle(mid, 17.0f, color);
		break;
	}
	}
}

void Visuals::disablePostProcessing(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    *memory->disablePostProcessing = stage == FrameStage::RENDER_START && config->visuals.disablePostProcessing;
}

void Visuals::reduceFlashEffect() noexcept
{
    if (localPlayer)
        localPlayer->flashMaxAlpha() = 255.0f - config->visuals.flashReduction * 2.55f;
}

bool Visuals::removeHands(const char* modelName) noexcept
{
    return config->visuals.noHands && std::strstr(modelName, "arms") && !std::strstr(modelName, "sleeve");
}

bool Visuals::removeSleeves(const char* modelName) noexcept
{
    return config->visuals.noSleeves && std::strstr(modelName, "sleeve");
}

bool Visuals::removeWeapons(const char* modelName) noexcept
{
    return config->visuals.noWeapons && std::strstr(modelName, "models/weapons/v_")
        && !std::strstr(modelName, "arms") && !std::strstr(modelName, "tablet")
        && !std::strstr(modelName, "parachute") && !std::strstr(modelName, "fists");
}

void Visuals::removeFire(FrameStage stage) noexcept
{
	if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
		return;

	constexpr std::array fireMaterials = {
		"decals/molotovscorch",
		"particle/fire_explosion_1/fire_explosion_1",
		"particle/fire_explosion_1/fire_explosion_1_oriented",
		"particle/fire_burning_character/fire_molotov_crop",
		"particle/fire_burning_character/fire_env_fire_depthblend",
		"particle/fire_burning_character/fire_env_fire",
		"particle/vistasmokev1/vistasmokev1_nearcull_nodepth"
	};

	for (const auto mat : fireMaterials)
	{
		interfaces->materialSystem->findMaterial(mat)->setMaterialVarFlag(MaterialVarFlag::NO_DRAW, stage == FrameStage::RENDER_START && config->visuals.noFire);
	}
}

void Visuals::skybox(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    if (const auto& skyboxes = Helpers::skyboxList; stage == FrameStage::RENDER_START && config->visuals.skybox > 0 && static_cast<std::size_t>(config->visuals.skybox) < skyboxes.size()) {
        memory->loadSky(skyboxes[config->visuals.skybox]);
    } else {
        static const auto sv_skyname = interfaces->cvar->findVar("sv_skyname");
        memory->loadSky(sv_skyname->string);
    }
}

void Visuals::bulletBeams(GameEvent *event)
{
	if (!config->visuals.selfBeams.enabled && !config->visuals.allyBeams.enabled && !config->visuals.enemyBeams.enabled)
		return;

	if (!localPlayer)
		return;

	if (!interfaces->engine->isInGame())
		return;

	auto player = interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserID(event->getInt("userid")));

	if (!player) return;

	Vector pos;
	pos.x = event->getFloat("x");
	pos.y = event->getFloat("y");
	pos.z = event->getFloat("z");

	constexpr auto getBeamSprite = [](int beam) constexpr noexcept
	{
		switch (beam)
		{
		case 0:
			return "sprites/physbeam.vmt";
		case 1:
			return "sprites/white.vmt";
		case 2:
			return "sprites/purplelaser1.vmt";
		case 3:
			return "sprites/laserbeam.vmt";
		}
		return "";
	};

	int sprite = 0;
	float col[4] = {};
	float width = 0.0f;
	float life = 0.0f;
	float noise = 0.0f;
	bool noiseOnce = true;
	bool railgun = false;
	if (localPlayer->isOtherEnemy(player))
	{
		if (!config->visuals.enemyBeams.enabled) return;
		sprite = config->visuals.enemyBeams.sprite;
		std::copy(config->visuals.enemyBeams.col.begin(), config->visuals.enemyBeams.col.end(), col);
		width = config->visuals.enemyBeams.width;
		life = config->visuals.enemyBeams.life;
		noise = config->visuals.enemyBeams.noise;
		noiseOnce = config->visuals.enemyBeams.noiseOnce;
		railgun = config->visuals.enemyBeams.railgun;
	}
	else if (player != localPlayer.get())
	{
		if (!config->visuals.allyBeams.enabled) return;
		sprite = config->visuals.allyBeams.sprite;
		std::copy(config->visuals.allyBeams.col.begin(), config->visuals.allyBeams.col.end(), col);
		width = config->visuals.allyBeams.width;
		life = config->visuals.allyBeams.life;
		noise = config->visuals.allyBeams.noise;
		noiseOnce = config->visuals.allyBeams.noiseOnce;
		railgun = config->visuals.allyBeams.railgun;
	}
	else
	{
		if (!config->visuals.selfBeams.enabled) return;
		sprite = config->visuals.selfBeams.sprite;
		std::copy(config->visuals.selfBeams.col.begin(), config->visuals.selfBeams.col.end(), col);
		width = config->visuals.selfBeams.width;
		life = config->visuals.selfBeams.life;
		noise = config->visuals.selfBeams.noise;
		noiseOnce = config->visuals.selfBeams.noiseOnce;
		railgun = config->visuals.selfBeams.railgun;
	}

	if (life <= 0.0f || width == 0.0f || col[3] == 0.0f || col[0] == 0.0f && col[1] == 0.0f && col[2] == 0.0f)
		return;

	if (const auto modelprecache = interfaces->networkStringTableContainer->findTable("modelprecache"))
		modelprecache->addString(false, getBeamSprite(sprite));

	const float distance = player->getEyePosition().distTo(pos);

	BeamInfo info;
	info.type = TE_BEAMPOINTS;
	info.modelName = getBeamSprite(sprite);
	info.haloScale = 0.0f;
	info.life = life;
	info.width = width;
	info.endWidth = width;
	info.fadeLength = 100.0f;
	info.amplitude = railgun ? noise * 0.02f : noise * 200.0f / distance;
	info.speed = 0.2f;
	info.startFrame = 0;
	info.frameRate = 60.0f;
	info.red = col[0] * 255.0f;
	info.green = col[1] * 255.0f;
	info.blue = col[2] * 255.0f;
	info.brightness = col[3] * 255.0f;
	info.segments = noise > 0.0f ? static_cast<int>(distance) / 15 : -1;
	info.renderable = true;
	info.end = pos;
	info.start = player->getEyePosition();
	info.flags = FBEAM_SHADEIN;
	if (railgun || noiseOnce || noise == 0.0f)
		info.flags |= FBEAM_ONLYNOISEONCE;
	if (railgun)
		info.flags |= FBEAM_SINENOISE;

	if (const auto beam = memory->viewRenderBeams->createBeamPoints(info))
	{
		beam->flags &= ~FBEAM_FOREVER;
		beam->die = memory->globalVars->currenttime + life;
	}
}

void Visuals::drawMolotovHull(ImDrawList *drawList) noexcept
{
	if (!config->visuals.molotovHull.enabled)
		return;

	const auto color = Helpers::calculateColor(config->visuals.molotovHull);

	GameData::Lock lock;

	static const auto flameCircumference = []
	{
		std::array<Vector, 64> points;
		for (std::size_t i = 0; i < points.size(); ++i)
		{
			constexpr auto flameRadius = 60.0f; // https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/game/server/cstrike15/Effects/inferno.cpp#L889
			points[i] = Vector{flameRadius * std::cos(Helpers::degreesToRadians(i * (360.0f / points.size()))),
				flameRadius * std::sin(Helpers::degreesToRadians(i * (360.0f / points.size()))),
				0.0f};
		}
		return points;
	}();

	for (const auto &molotov : GameData::infernos())
	{
		for (const auto &pos : molotov.points)
		{
			std::array<ImVec2, flameCircumference.size()> screenPoints;
			std::size_t count = 0;

			for (const auto &point : flameCircumference)
			{
				if (Helpers::worldToScreen(pos + point, screenPoints[count]))
					++count;
			}

			if (count < 1)
				continue;

			std::swap(screenPoints[0], *std::min_element(screenPoints.begin(), screenPoints.begin() + count, [](const auto &a, const auto &b) { return a.y < b.y || (a.y == b.y && a.x < b.x); }));

			constexpr auto orientation = [](const ImVec2 &a, const ImVec2 &b, const ImVec2 &c)
			{
				return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
			};

			std::sort(screenPoints.begin() + 1, screenPoints.begin() + count, [&](const auto &a, const auto &b) { return orientation(screenPoints[0], a, b) > 0.0f; });

			drawList->AddConvexPolyFilled(screenPoints.data(), count, color);
			drawList->AddPolyline(screenPoints.data(), count, color | IM_COL32_A_MASK, true, config->visuals.molotovHull.thickness);
		}

		
	}
}
