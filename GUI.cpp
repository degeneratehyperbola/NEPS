#include <cwctype>
#include <fstream>
#include <functional>
#include <string>
#include <ShlObj.h>
#include <Windows.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_stdlib.h"

#include "imguiCustom.h"

#include "GUI.h"
#include "GameData.h"
#include "Config.h"
#include "Helpers.h"
#include "Hooks.h"
#include "Interfaces.h"
#include "Memory.h"
#include "Hacks/Misc.h"
#include "Hacks/Visuals.h"
#include "Hacks/SkinChanger.h"
#include "Hacks/Aimbot.h"
#include "SDK/Engine.h"
#include "SDK/Entity.h"
#include "SDK/EntityList.h"
#include "SDK/Input.h"
#include "SDK/InputSystem.h"
#include "SDK/ConVar.h"
#include "SDK/Cvar.h"

constexpr auto windowFlags = ImGuiWindowFlags_NoResize
| ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;

GUI::GUI() noexcept
{
	ImGui::StyleColorsClassic();

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
	
	if (PWSTR pathToFonts; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Fonts, 0, nullptr, &pathToFonts))) {
		const std::filesystem::path path{ pathToFonts };
		CoTaskMemFree(pathToFonts);

        ImFontConfig cfg;
        cfg.OversampleV = cfg.OversampleH = 8;
		cfg.PixelSnapH = false;

        fonts.msgothic = io.Fonts->AddFontFromFileTTF((path / "msgothic.ttc").string().c_str(), 14.0f, &cfg, Helpers::getFontGlyphRanges());
    }
}

void GUI::render() noexcept
{
    if (!config->style.menuStyle) {
        renderMenuBar();
        renderAimbotWindow();
        renderAntiAimWindow();
        renderTriggerbotWindow();
        renderBacktrackWindow();
        renderGlowWindow();
        renderChamsWindow();
        renderESPWindow();
        renderVisualsWindow();
        renderSkinChangerWindow();
        renderSoundWindow();
        renderStyleWindow();
        renderExploitsWindow();
        renderGriefingWindow();
        renderMovementWindow();
        renderMiscWindow();
        renderConfigWindow();
		debug();
    } else {
        renderGuiStyle2();
    }

	if (!ImGui::GetIO().WantCaptureMouse && ImGui::GetIO().MouseDown[1])
		ImGui::OpenPopup("##contxt_m");

	if (ImGui::BeginPopup("##contxt_m"))
	{
		if (ImGui::Selectable("Close all"))
			window = {};
		if (ImGui::Selectable("Open console"))
			interfaces->engine->clientCmdUnrestricted("toggleconsole");
		if (ImGui::Selectable("Fog UI"))
			interfaces->engine->clientCmdUnrestricted("fogui");
		if (ImGui::Selectable("Loaded textures"))
			interfaces->cvar->findVar("mat_texture_list")->setValue(true);
		if (ImGui::Selectable("Unload"))
			hooks->uninstall();
		ImGui::EndPopup();
	}
}

void GUI::updateColors() const noexcept
{
    switch (config->style.menuColors) {
    case 0: ImGui::StyleColorsDark(); break;
    case 1: ImGui::StyleColorsLight(); break;
    case 2: ImGui::StyleColorsClassic(); break;
    }
}

static void menuBarItem(const char* name, bool& enabled) noexcept
{
    if (ImGui::MenuItem(name)) {
        enabled = true;
        ImGui::SetWindowFocus(name);
	}
}

void GUI::renderMenuBar() noexcept
{
    if (ImGui::BeginMainMenuBar()) {
        menuBarItem("Config", window.config);
        menuBarItem("Style", window.style);
		ImGui::Separator();
        menuBarItem("Aimbot", window.aimbot);
        menuBarItem("Triggerbot", window.triggerbot);
        menuBarItem("Backtrack", window.backtrack);
        menuBarItem("Anti-aim", window.antiAim);
        menuBarItem("Chams", window.chams);
        menuBarItem("Glow", window.glow);
        menuBarItem("ESP", window.streamProofESP);
        menuBarItem("Visuals", window.visuals);
        menuBarItem("Skin changer", window.skinChanger);
        menuBarItem("Sound", window.sound);
        menuBarItem("Griefing", window.steamapi);
        menuBarItem("Exploits", window.exploits);
        menuBarItem("Movement", window.movement);
        menuBarItem("Misc", window.misc);
		ImGui::Separator();
		if (ImGui::BeginMenu("Context menu"))
		{
			if (ImGui::MenuItem("Close all"))
				window = {};
			if (ImGui::MenuItem("Open console"))
				interfaces->engine->clientCmdUnrestricted("toggleconsole");
			if (ImGui::MenuItem("Fog UI"))
				interfaces->engine->clientCmdUnrestricted("fogui");
			if (ImGui::MenuItem("Loaded textures"))
				interfaces->cvar->findVar("mat_texture_list")->setValue(true);
			if (ImGui::MenuItem("Unload"))
				hooks->uninstall();
			ImGui::EndMenu();
		}
        ImGui::EndMainMenuBar();
    }
}

void GUI::renderAimbotWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.aimbot)
            return;
        ImGui::SetNextWindowSize({ 430.0f, 0.0f });
        ImGui::Begin("Aimbot", &window.aimbot, windowFlags);
    }
    static int currentCategory{ 0 };
    ImGui::PushItemWidth(110.0f);
    ImGui::Combo("##category", &currentCategory, "All\0Pistols\0Heavy\0SMGs\0Rifles\0Zeus x27\0");
    ImGui::SameLine();
    static int currentWeapon{ 0 };

    switch (currentCategory) {
    case 0:
        currentWeapon = 0;
        ImGui::NewLine();
        break;
	case 5:
		currentWeapon = 39;
		ImGui::NewLine();
		break;
    case 1: {
        static int currentPistol{ 0 };
        static constexpr const char* pistols[]{ "All", "Glock-18", "P2000", "USP-S", "Dual Berettas", "P250", "Tec-9", "Five-SeveN", "CZ-75", "Desert Eagle", "R8 Revolver" };

        ImGui::Combo("##sub", &currentPistol, [](void* data, int idx, const char** out_text) {
            if (config->aimbot[idx ? idx : 35].bind.keyMode) {
                static std::string name;
                name = pistols[idx];
                *out_text = name.append(" *").c_str();
            } else {
                *out_text = pistols[idx];
            }
            return true;
            }, nullptr, IM_ARRAYSIZE(pistols));

        currentWeapon = currentPistol ? currentPistol : 35;
        break;
    }
    case 2: {
        static int currentHeavy{ 0 };
        static constexpr const char* heavies[]{ "All", "Nova", "XM1014", "Sawed-Off", "MAG-7", "M249", "Negev" };

        ImGui::Combo("##sub", &currentHeavy, [](void* data, int idx, const char** out_text) {
            if (config->aimbot[idx ? idx + 10 : 36].bind.keyMode) {
                static std::string name;
                name = heavies[idx];
                *out_text = name.append(" *").c_str();
            } else {
                *out_text = heavies[idx];
            }
            return true;
            }, nullptr, IM_ARRAYSIZE(heavies));

        currentWeapon = currentHeavy ? currentHeavy + 10 : 36;
        break;
    }
    case 3: {
        static int currentSmg{ 0 };
        static constexpr const char* smgs[]{ "All", "Mac-10", "MP9", "MP7", "MP5-SD", "UMP-45", "P90", "PP-Bizon" };

        ImGui::Combo("##sub", &currentSmg, [](void* data, int idx, const char** out_text) {
            if (config->aimbot[idx ? idx + 16 : 37].bind.keyMode) {
                static std::string name;
                name = smgs[idx];
                *out_text = name.append(" *").c_str();
            } else {
                *out_text = smgs[idx];
            }
            return true;
            }, nullptr, IM_ARRAYSIZE(smgs));

        currentWeapon = currentSmg ? currentSmg + 16 : 37;
        break;
    }
    case 4: {
        static int currentRifle{ 0 };
        static constexpr const char* rifles[]{ "All", "Galil AR", "Famas", "AK-47", "M4A4", "M4A1-S", "SSG-08", "SG-553", "AUG", "AWP", "G3SG1", "SCAR-20" };

        ImGui::Combo("##sub", &currentRifle, [](void* data, int idx, const char** out_text) {
            if (config->aimbot[idx ? idx + 23 : 38].bind.keyMode) {
                static std::string name;
                name = rifles[idx];
                *out_text = name.append(" *").c_str();
            } else {
                *out_text = rifles[idx];
            }
            return true;
            }, nullptr, IM_ARRAYSIZE(rifles));

        currentWeapon = currentRifle ? currentRifle + 23 : 38;
        break;
    }
    }
    ImGui::SameLine();
	ImGuiCustom::keyBind("Enabled", config->aimbot[currentWeapon].bind);

    ImGui::Separator();
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, 165.0f);

    ImGui::Checkbox("Aimlock", &config->aimbot[currentWeapon].aimlock);
    ImGui::Checkbox("Silent", &config->aimbot[currentWeapon].silent);
    ImGui::Checkbox("Friendly fire", &config->aimbot[currentWeapon].friendlyFire);
    ImGui::Checkbox("Visible only", &config->aimbot[currentWeapon].visibleOnly);
    ImGui::Checkbox("Scoped only", &config->aimbot[currentWeapon].scopedOnly);
    ImGui::Checkbox("Ignore flash", &config->aimbot[currentWeapon].ignoreFlash);
    ImGui::Checkbox("Ignore smoke", &config->aimbot[currentWeapon].ignoreSmoke);
    ImGui::Checkbox("Auto shot", &config->aimbot[currentWeapon].autoShot);
    ImGui::Checkbox("Auto scope", &config->aimbot[currentWeapon].autoScope);

	ImGui::SetNextItemWidth(85.0f);
    ImGui::Combo("Targeting", &config->aimbot[currentWeapon].targeting, "FOV\0Damage\0Hitchance\0Distance\0");

	const char *hitgroupNames[7] = {"Head", "Chest", "Stomach", "Left arm", "Right arm", "Left leg", "Right leg"};
	const char *selectedBones;
	if (config->aimbot[currentWeapon].hitgroup == (1 << 7) - 1)
		selectedBones = "All";
	else if (!config->aimbot[currentWeapon].hitgroup)
		selectedBones = "None";
	else
		selectedBones = "...";

	ImGui::SetNextItemWidth(85.0f);
	if (ImGui::BeginCombo("Hitgroup", selectedBones))
	{
		for (int i = 0; i < 7; i++)
		{
			bool selected = config->aimbot[currentWeapon].hitgroup & (1 << i);

			ImGui::PushID(i);
			ImGui::Selectable(hitgroupNames[i], &selected, ImGuiSelectableFlags_DontClosePopups);
			ImGui::PopID();

			if (selected)
				config->aimbot[currentWeapon].hitgroup |= (1 << i);
			else
				config->aimbot[currentWeapon].hitgroup &= ~(1 << i);
		}
		ImGui::EndPopup();
	}

	config->aimbot[currentWeapon].hitgroup = std::clamp(config->aimbot[currentWeapon].hitgroup, 0, (1 << 7) - 1);

    ImGui::NextColumn();
    ImGui::Checkbox("Multipoint", &config->aimbot[currentWeapon].multipoint);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("About 23%% less performant");

    ImGui::PushItemWidth(250.0f);
	ImGui::SliderFloat("##scale", &config->aimbot[currentWeapon].multipointScale, 0.5f, 1.0f, "Multipoint scale %.5f");
    ImGui::SliderFloat("##inaccuracy", &config->aimbot[currentWeapon].maxAimInaccuracy, 0.0f, 1.0f, "Max aim inaccuracy %.5f", ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat("##fov", &config->aimbot[currentWeapon].fov, 0.0f, 255.0f, "FOV %.2fdeg", ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat("##hitchance", &config->aimbot[currentWeapon].shotHitchance, 0.0f, 100.0f, "Hitchance %.0f%%");
	ImGui::SetNextItemWidth(95.0f);
    ImGui::InputFloat("Distance", &config->aimbot[currentWeapon].distance, 1.0f, 10.0f, "%.0fu");
    config->aimbot[currentWeapon].distance = max(config->aimbot[currentWeapon].distance, 0);

	ImGui::SetNextItemWidth(95.0f);
    ImGui::InputInt("Min damage", &config->aimbot[currentWeapon].minDamage);
    config->aimbot[currentWeapon].minDamage = max(config->aimbot[currentWeapon].minDamage, 0);
    ImGui::Checkbox("Health + min damage", &config->aimbot[currentWeapon].killshot);
	ImGui::SetNextItemWidth(100.0f);
	ImGui::Combo("Interpolation", &config->aimbot[currentWeapon].interpolation, "None\0Linear\0Quadratic\0");
	switch (config->aimbot[currentWeapon].interpolation)
	{
	case 0:
		break;
	case 1:
		ImGui::PushID("linear_speed");
		ImGui::SliderFloat("", &config->aimbot[currentWeapon].linearSpeed, 0.0f, 20.0f, "Speed %.4fdeg", ImGuiSliderFlags_Logarithmic);
		ImGui::PopID();
		break;
	case 2:
		ImGui::PushID("smoothness");
		ImGui::SliderFloat("", &config->aimbot[currentWeapon].smooth, 0.0f, 1.0f, "Smoothness %.4f");
		ImGui::PopID();
		break;
	}
    ImGui::Checkbox("Between shots", &config->aimbot[currentWeapon].betweenShots);

	ImGuiCustom::keyBind("Safety mode", config->aimbot[currentWeapon].safeOnly);
    ImGui::Checkbox("On shot", &config->aimbot[currentWeapon].onShot);
	ImGui::SameLine();
	ImGui::Checkbox("On move", &config->aimbot[currentWeapon].onMove);
    if (!contentOnly)
        ImGui::End();
}

void GUI::renderAntiAimWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.antiAim)
            return;
        ImGui::SetNextWindowSize({ 0.0f, 0.0f });
        ImGui::Begin("Anti-aim", &window.antiAim, windowFlags);
    }
    ImGui::Checkbox("##yaw", &config->antiAim.yaw);
    ImGui::SameLine();
	ImGui::SliderFloat("##yaw_sl", &config->antiAim.yawAngle, -180.0f, 180.0f, "Yaw %.2fdeg");
    ImGui::Checkbox("##pitch", &config->antiAim.pitch);
    ImGui::SameLine();
    ImGui::SliderFloat("##pitch_sl", &config->antiAim.pitchAngle, -89.0f, 89.0f, "Pitch %.2fdeg");
    ImGui::Checkbox("Desync", &config->antiAim.desync);
	if (config->antiAim.desync)
	{
		ImGui::Checkbox("Increase clamp", &config->antiAim.corrected);
		ImGui::Checkbox("Extended (experimental)", &config->antiAim.extended);
		ImGuiCustom::keyBind("Flip key", &config->antiAim.flipKey);
	}
	ImGuiCustom::keyBind("Fake duck", config->antiAim.fakeDuck);
	ImGui::SetNextItemWidth(90.0f);
	ImGui::InputInt("Choked packets", &config->antiAim.chokedPackets, 1, 5);
	config->antiAim.chokedPackets = std::clamp(config->antiAim.chokedPackets, 0, 64);
	ImGui::SameLine();
	ImGuiCustom::keyBind("##choke", config->antiAim.choke);
    if (!contentOnly)
        ImGui::End();
}

void GUI::renderTriggerbotWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.triggerbot)
            return;
        ImGui::SetNextWindowSize({ 0.0f, 0.0f });
        ImGui::Begin("Triggerbot", &window.triggerbot, windowFlags);
    }
    static int currentCategory{ 0 };
    ImGui::PushItemWidth(110.0f);
    ImGui::Combo("##category", &currentCategory, "All\0Pistols\0Heavy\0SMGs\0Rifles\0Zeus x27\0");
    ImGui::SameLine();
    static int currentWeapon{ 0 };
    switch (currentCategory) {
    case 0:
        currentWeapon = 0;
        ImGui::NewLine();
        break;
    case 5:
        currentWeapon = 39;
        ImGui::NewLine();
        break;

    case 1: {
        static int currentPistol{ 0 };
        static constexpr const char* pistols[]{ "All", "Glock-18", "P2000", "USP-S", "Dual Berettas", "P250", "Tec-9", "Five-SeveN", "CZ-75", "Desert Eagle", "R8 Revolver" };

        ImGui::Combo("##sub", &currentPistol, [](void* data, int idx, const char** out_text) {
            if (config->triggerbot[idx ? idx : 35].bind.keyMode) {
                static std::string name;
                name = pistols[idx];
                *out_text = name.append(" *").c_str();
            } else {
                *out_text = pistols[idx];
            }
            return true;
            }, nullptr, IM_ARRAYSIZE(pistols));

        currentWeapon = currentPistol ? currentPistol : 35;
        break;
    }
    case 2: {
        static int currentHeavy{ 0 };
        static constexpr const char* heavies[]{ "All", "Nova", "XM1014", "Sawed-Off", "MAG-7", "M249", "Negev" };

        ImGui::Combo("##sub", &currentHeavy, [](void* data, int idx, const char** out_text) {
            if (config->triggerbot[idx ? idx + 10 : 36].bind.keyMode) {
                static std::string name;
                name = heavies[idx];
                *out_text = name.append(" *").c_str();
            } else {
                *out_text = heavies[idx];
            }
            return true;
            }, nullptr, IM_ARRAYSIZE(heavies));

        currentWeapon = currentHeavy ? currentHeavy + 10 : 36;
        break;
    }
    case 3: {
        static int currentSmg{ 0 };
        static constexpr const char* smgs[]{ "All", "Mac-10", "MP9", "MP7", "MP5-SD", "UMP-45", "P90", "PP-Bizon" };

        ImGui::Combo("##sub", &currentSmg, [](void* data, int idx, const char** out_text) {
            if (config->triggerbot[idx ? idx + 16 : 37].bind.keyMode) {
                static std::string name;
                name = smgs[idx];
                *out_text = name.append(" *").c_str();
            } else {
                *out_text = smgs[idx];
            }
            return true;
            }, nullptr, IM_ARRAYSIZE(smgs));

        currentWeapon = currentSmg ? currentSmg + 16 : 37;
        break;
    }
    case 4: {
        static int currentRifle{ 0 };
        static constexpr const char* rifles[]{ "All", "Galil AR", "Famas", "AK-47", "M4A4", "M4A1-S", "SSG-08", "SG-553", "AUG", "AWP", "G3SG1", "SCAR-20" };

        ImGui::Combo("##sub", &currentRifle, [](void* data, int idx, const char** out_text) {
            if (config->triggerbot[idx ? idx + 23 : 38].bind.keyMode) {
                static std::string name;
                name = rifles[idx];
                *out_text = name.append(" *").c_str();
            } else {
                *out_text = rifles[idx];
            }
            return true;
            }, nullptr, IM_ARRAYSIZE(rifles));

        currentWeapon = currentRifle ? currentRifle + 23 : 38;
        break;
    }
    }
	ImGui::SameLine();
	ImGuiCustom::keyBind("Enabled", config->triggerbot[currentWeapon].bind);
    ImGui::Separator();

	ImGui::Checkbox("Friendly fire", &config->triggerbot[currentWeapon].friendlyFire);
	ImGui::Checkbox("Visible only", &config->triggerbot[currentWeapon].visibleOnly);
    ImGui::Checkbox("Scoped only", &config->triggerbot[currentWeapon].scopedOnly);
    ImGui::Checkbox("Ignore flash", &config->triggerbot[currentWeapon].ignoreFlash);
    ImGui::Checkbox("Ignore smoke", &config->triggerbot[currentWeapon].ignoreSmoke);

	const char *hitgroupNames[7] = {"Head", "Chest", "Stomach", "Left arm", "Right arm", "Left leg", "Right leg"};
	const char *selectedBones;
	if (config->triggerbot[currentWeapon].hitgroup == (1 << 7) - 1)
		selectedBones = "All";
	else if (!config->triggerbot[currentWeapon].hitgroup)
		selectedBones = "None";
	else
		selectedBones = "...";

	ImGui::SetNextItemWidth(85.0f);
	if (ImGui::BeginCombo("Hitgroup", selectedBones))
	{
		for (int i = 0; i < 7; i++)
		{
			bool selected = config->triggerbot[currentWeapon].hitgroup & (1 << i);

			ImGui::PushID(i);
			ImGui::Selectable(hitgroupNames[i], &selected, ImGuiSelectableFlags_DontClosePopups);
			ImGui::PopID();

			if (selected)
				config->triggerbot[currentWeapon].hitgroup |= (1 << i);
			else
				config->triggerbot[currentWeapon].hitgroup &= ~(1 << i);
		}
		ImGui::EndPopup();
	}

	config->triggerbot[currentWeapon].hitgroup = std::clamp(config->triggerbot[currentWeapon].hitgroup, 0, (1 << 7) - 1);

    ImGui::PushItemWidth(300.0f);
    ImGui::SliderInt("##delay", &config->triggerbot[currentWeapon].shotDelay, 0, 300, "Shot delay %dms", ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat("##inaccuracy", &config->triggerbot[currentWeapon].maxShotInaccuracy, 0.0f, 1.0f, "Max inaccuracy %.5f", ImGuiSliderFlags_Logarithmic);
	ImGui::SliderFloat("##hitchance", &config->triggerbot[currentWeapon].hitchance, 0.0f, 100.0f, "Hitchance %.0f%%");
	ImGui::SetNextItemWidth(95.0f);
	ImGui::InputFloat("Distance", &config->triggerbot[currentWeapon].distance, 1.0f, 10.0f, "%.0fu");
	config->triggerbot[currentWeapon].distance = max(config->triggerbot[currentWeapon].distance, 0);
	ImGui::SetNextItemWidth(95.0f);
	ImGui::InputInt("Min damage", &config->triggerbot[currentWeapon].minDamage);
    config->triggerbot[currentWeapon].minDamage = std::clamp(config->triggerbot[currentWeapon].minDamage, 0, 250);
    ImGui::Checkbox("Health + min damage", &config->triggerbot[currentWeapon].killshot);
    ImGui::SliderFloat("##burst", &config->triggerbot[currentWeapon].burstTime, 0.0f, 1.0f, "Burst time %.3fs");

    if (!contentOnly)
        ImGui::End();
}

void GUI::renderBacktrackWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.backtrack)
            return;
        ImGui::SetNextWindowSize({ 0.0f, 0.0f });
        ImGui::Begin("Backtrack", &window.backtrack, windowFlags);
    }
    ImGui::Checkbox("Enabled", &config->backtrack.enabled);
    ImGui::Checkbox("Ignore smoke", &config->backtrack.ignoreSmoke);
    ImGui::Checkbox("Recoil based fov", &config->backtrack.recoilBasedFov);
    ImGui::PushItemWidth(220.0f);
    ImGui::SliderInt("Time limit", &config->backtrack.timeLimit, 1, 200, "%d ms");
    ImGui::PopItemWidth();
    if (!contentOnly)
        ImGui::End();
}

void GUI::renderGlowWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.glow)
            return;
        ImGui::SetNextWindowSize({ 320.0f, 0.0f });
        ImGui::Begin("Glow", &window.glow, windowFlags);
    }
    static int currentCategory{ 0 };
    ImGui::PushItemWidth(110.0f);
    ImGui::PushID(0);
    ImGui::Combo("", &currentCategory, "Allies\0Enemies\0Planting\0Defusing\0Local player\0Weapons\0C4\0Planted C4\0Chickens\0Defuse kits\0Projectiles\0Hostages\0Ragdolls\0");
    ImGui::PopID();
    static int currentItem{ 0 };
    if (currentCategory <= 3) {
        ImGui::SameLine();
        static int currentType{ 0 };
        ImGui::PushID(1);
        ImGui::Combo("", &currentType, "All\0Visible\0Occluded\0");
        ImGui::PopID();
        currentItem = currentCategory * 3 + currentType;
    } else {
        currentItem = currentCategory + 8;
    }

    ImGui::SameLine();
    ImGui::Checkbox("Enabled", &config->glow[currentItem].enabled);
    ImGui::Separator();
    ImGui::Columns(2, nullptr, false); 
    ImGui::Checkbox("Health based", &config->glow[currentItem].healthBased);

    ImGuiCustom::colorPicker("Color", config->glow[currentItem].color, &config->glow[currentItem].rainbow, &config->glow[currentItem].rainbowSpeed);

    ImGui::NextColumn();
    ImGui::SetNextItemWidth(100.0f);
    ImGui::Combo("Style", &config->glow[currentItem].style, "Default\0Rim3d\0Edge\0Edge Pulse\0");
	if (!config->glow[currentItem].style)
		ImGui::Checkbox("Full bloom", &config->glow[currentItem].full);
   
    if (!contentOnly)
        ImGui::End();
}

void GUI::renderChamsWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.chams)
            return;
        ImGui::SetNextWindowSize({ 0.0f, 0.0f });
        ImGui::Begin("Chams", &window.chams, windowFlags);
    }
    static int currentCategory{ 0 };
    ImGui::PushItemWidth(110.0f);

    static int material = 1;

    if (ImGui::Combo("##category", &currentCategory, "Allies\0Enemies\0Planting\0Defusing\0Backtrack\0Local player\0Desync\0Viewmodel\0Sleeves\0Hands\0Weapons\0C4\0Defuse kits\0Ragdolls\0Props\0"))
        material = 1;

    ImGui::SameLine();

    if (material <= 1)
        ImGuiCustom::arrowButtonDisabled("##left", ImGuiDir_Left);
    else if (ImGui::ArrowButton("##left", ImGuiDir_Left))
        --material;

    ImGui::SameLine();
    ImGui::Text("%d", material);

    constexpr std::array categories{ "Allies", "Enemies", "Planting", "Defusing", "Backtrack", "Local player", "Desync", "Weapons", "Sleeves", "Hands", "World weapons", "C4", "Defusers", "Ragdolls", "Props" };

    ImGui::SameLine();

    if (material >= int(config->chams[categories[currentCategory]].materials.size()))
        ImGuiCustom::arrowButtonDisabled("##right", ImGuiDir_Right);
    else if (ImGui::ArrowButton("##right", ImGuiDir_Right))
        ++material;

    ImGui::SameLine();

    auto& chams{ config->chams[categories[currentCategory]].materials[material - 1] };

    ImGui::Checkbox("Enabled", &chams.enabled);
    ImGui::Separator();
    ImGui::Checkbox("Health based", &chams.healthBased);
    ImGui::Checkbox("Blinking", &chams.blinking);
    ImGui::Combo("Material", &chams.material, "Diffuse\0Flat\0Flat additive\0Animated\0Glass\0Chrome\0Crystal\0Phong\0Fresnel\0Glow\0Pearlescent\0");
    ImGui::Checkbox("Wireframe", &chams.wireframe);
    ImGui::Checkbox("Cover", &chams.cover);
    ImGui::Checkbox("Ignore-Z", &chams.ignorez);
    ImGuiCustom::colorPicker("Color", chams.color, &chams.rainbow, &chams.rainbowSpeed);

	if (categories[currentCategory] == "Backtrack")
		ImGui::Checkbox("Trailing backtrack", &config->chams["Backtrack"].trailBacktrack);

    if (!contentOnly) {
        ImGui::End();
    }
}

void GUI::renderESPWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.streamProofESP)
            return;
        ImGui::SetNextWindowSize({ 0.0f, 0.0f });
        ImGui::Begin("ESP", &window.streamProofESP, windowFlags);
    }

    static std::size_t currentCategory;
    static auto currentItem = "All";

    constexpr auto getConfigShared = [](std::size_t category, const char* item) noexcept -> Shared& {
        switch (category) {
        case 0: default: return config->esp.enemies[item];
        case 1: return config->esp.allies[item];
        case 2: return config->esp.weapons[item];
        case 3: return config->esp.projectiles[item];
        case 4: return config->esp.lootCrates[item];
        case 5: return config->esp.otherEntities[item];
        }
    };

    constexpr auto getConfigPlayer = [](std::size_t category, const char* item) noexcept -> Player& {
        switch (category) {
        case 0: default: return config->esp.enemies[item];
        case 1: return config->esp.allies[item];
        }
    };

    if (ImGui::ListBoxHeader("##list", { 170.0f, 250.0f })) {
        constexpr std::array categories{ "Enemies", "Allies", "Weapons", "Projectiles", "Loot Crates", "Other Entities" };

        for (std::size_t i = 0; i < categories.size(); ++i) {
            if (ImGui::Selectable(categories[i], currentCategory == i && std::string_view{ currentItem } == "All")) {
                currentCategory = i;
                currentItem = "All";
            }

            if (ImGui::BeginDragDropSource()) {
                switch (i) {
                case 0: case 1: ImGui::SetDragDropPayload("Player", &getConfigPlayer(i, "All"), sizeof(Player), ImGuiCond_Once); break;
                case 2: ImGui::SetDragDropPayload("Weapon", &config->esp.weapons["All"], sizeof(Weapon), ImGuiCond_Once); break;
                case 3: ImGui::SetDragDropPayload("Projectile", &config->esp.projectiles["All"], sizeof(Projectile), ImGuiCond_Once); break;
                default: ImGui::SetDragDropPayload("Entity", &getConfigShared(i, "All"), sizeof(Shared), ImGuiCond_Once); break;
                }
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Player")) {
                    const auto& data = *(Player*)payload->Data;

                    switch (i) {
                    case 0: case 1: getConfigPlayer(i, "All") = data; break;
                    case 2: config->esp.weapons["All"] = data; break;
                    case 3: config->esp.projectiles["All"] = data; break;
                    default: getConfigShared(i, "All") = data; break;
                    }
                }

                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Weapon")) {
                    const auto& data = *(Weapon*)payload->Data;

                    switch (i) {
                    case 0: case 1: getConfigPlayer(i, "All") = data; break;
                    case 2: config->esp.weapons["All"] = data; break;
                    case 3: config->esp.projectiles["All"] = data; break;
                    default: getConfigShared(i, "All") = data; break;
                    }
                }

                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Projectile")) {
                    const auto& data = *(Projectile*)payload->Data;

                    switch (i) {
                    case 0: case 1: getConfigPlayer(i, "All") = data; break;
                    case 2: config->esp.weapons["All"] = data; break;
                    case 3: config->esp.projectiles["All"] = data; break;
                    default: getConfigShared(i, "All") = data; break;
                    }
                }

                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity")) {
                    const auto& data = *(Shared*)payload->Data;

                    switch (i) {
                    case 0: case 1: getConfigPlayer(i, "All") = data; break;
                    case 2: config->esp.weapons["All"] = data; break;
                    case 3: config->esp.projectiles["All"] = data; break;
                    default: getConfigShared(i, "All") = data; break;
                    }
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::PushID(i);
            ImGui::Indent();

            const auto items = [](std::size_t category) noexcept -> std::vector<const char*> {
                switch (category) {
                case 0:
                case 1: return { "Visible", "Occluded" };
                case 2: return { "Pistols", "SMGs", "Rifles", "Sniper Rifles", "Shotguns", "Machineguns", "Grenades", "Melee", "Other" };
                case 3: return { "Flashbang", "HE Grenade", "Breach Charge", "Bump Mine", "Decoy Grenade", "Molotov", "TA Grenade", "Smoke Grenade", "Snowball" };
                case 4: return { "Pistol Case", "Light Case", "Heavy Case", "Explosive Case", "Tools Case", "Cash Dufflebag" };
				case 5: return { "Defuse Kit", "Chicken", "Planted C4", "Hostage", "Sentry", "Cash", "Ammo Box", "Radar Jammer", "Snowball Pile", "Collectable Coin" };
                default: return { };
                }
            }(i);

            const auto categoryEnabled = getConfigShared(i, "All").enabled;

            for (std::size_t j = 0; j < items.size(); ++j) {
                static bool selectedSubItem;
                if (!categoryEnabled || getConfigShared(i, items[j]).enabled) {
                    if (ImGui::Selectable(items[j], currentCategory == i && !selectedSubItem && std::string_view{ currentItem } == items[j])) {
                        currentCategory = i;
                        currentItem = items[j];
                        selectedSubItem = false;
                    }

                    if (ImGui::BeginDragDropSource()) {
                        switch (i) {
                        case 0: case 1: ImGui::SetDragDropPayload("Player", &getConfigPlayer(i, items[j]), sizeof(Player), ImGuiCond_Once); break;
                        case 2: ImGui::SetDragDropPayload("Weapon", &config->esp.weapons[items[j]], sizeof(Weapon), ImGuiCond_Once); break;
                        case 3: ImGui::SetDragDropPayload("Projectile", &config->esp.projectiles[items[j]], sizeof(Projectile), ImGuiCond_Once); break;
                        default: ImGui::SetDragDropPayload("Entity", &getConfigShared(i, items[j]), sizeof(Shared), ImGuiCond_Once); break;
                        }
                        ImGui::EndDragDropSource();
                    }

                    if (ImGui::BeginDragDropTarget()) {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Player")) {
                            const auto& data = *(Player*)payload->Data;

                            switch (i) {
                            case 0: case 1: getConfigPlayer(i, items[j]) = data; break;
                            case 2: config->esp.weapons[items[j]] = data; break;
                            case 3: config->esp.projectiles[items[j]] = data; break;
                            default: getConfigShared(i, items[j]) = data; break;
                            }
                        }

                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Weapon")) {
                            const auto& data = *(Weapon*)payload->Data;

                            switch (i) {
                            case 0: case 1: getConfigPlayer(i, items[j]) = data; break;
                            case 2: config->esp.weapons[items[j]] = data; break;
                            case 3: config->esp.projectiles[items[j]] = data; break;
                            default: getConfigShared(i, items[j]) = data; break;
                            }
                        }

                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Projectile")) {
                            const auto& data = *(Projectile*)payload->Data;

                            switch (i) {
                            case 0: case 1: getConfigPlayer(i, items[j]) = data; break;
                            case 2: config->esp.weapons[items[j]] = data; break;
                            case 3: config->esp.projectiles[items[j]] = data; break;
                            default: getConfigShared(i, items[j]) = data; break;
                            }
                        }

                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity")) {
                            const auto& data = *(Shared*)payload->Data;

                            switch (i) {
                            case 0: case 1: getConfigPlayer(i, items[j]) = data; break;
                            case 2: config->esp.weapons[items[j]] = data; break;
                            case 3: config->esp.projectiles[items[j]] = data; break;
                            default: getConfigShared(i, items[j]) = data; break;
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }
                }

                if (i != 2)
                    continue;

                ImGui::Indent();

                const auto subItems = [](std::size_t item) noexcept -> std::vector<const char*> {
                    switch (item) {
                    case 0: return { "Glock-18", "P2000", "USP-S", "Dual Berettas", "P250", "Tec-9", "Five-SeveN", "CZ75-Auto", "Desert Eagle", "R8 Revolver" };
                    case 1: return { "MAC-10", "MP9", "MP7", "MP5-SD", "UMP-45", "P90", "PP-Bizon" };
                    case 2: return { "Galil AR", "FAMAS", "AK-47", "M4A4", "M4A1-S", "SG 553", "AUG" };
                    case 3: return { "SSG 08", "AWP", "G3SG1", "SCAR-20" };
                    case 4: return { "Nova", "XM1014", "Sawed-Off", "MAG-7" };
                    case 5: return { "M249", "Negev" };
                    case 6: return { "Flashbang", "HE Grenade", "Smoke Grenade", "Molotov", "Decoy Grenade", "Incendiary", "TA Grenade", "Fire Bomb", "Diversion", "Frag Grenade", "Snowball" };
                    case 7: return { "Axe", "Hammer", "Wrench" };
                    case 8: return { "C4", "Healthshot", "Bump Mine", "Zone Repulsor", "Shield" };
                    default: return { };
                    }
                }(j);

                const auto itemEnabled = getConfigShared(i, items[j]).enabled;

                for (const auto subItem : subItems) {
                    auto& subItemConfig = config->esp.weapons[subItem];
                    if ((categoryEnabled || itemEnabled) && !subItemConfig.enabled)
                        continue;

                    if (ImGui::Selectable(subItem, currentCategory == i && selectedSubItem && std::string_view{ currentItem } == subItem)) {
                        currentCategory = i;
                        currentItem = subItem;
                        selectedSubItem = true;
                    }

                    if (ImGui::BeginDragDropSource()) {
                        ImGui::SetDragDropPayload("Weapon", &subItemConfig, sizeof(Weapon), ImGuiCond_Once);
                        ImGui::EndDragDropSource();
                    }

                    if (ImGui::BeginDragDropTarget()) {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Player")) {
                            const auto& data = *(Player*)payload->Data;
                            subItemConfig = data;
                        }

                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Weapon")) {
                            const auto& data = *(Weapon*)payload->Data;
                            subItemConfig = data;
                        }

                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Projectile")) {
                            const auto& data = *(Projectile*)payload->Data;
                            subItemConfig = data;
                        }

                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity")) {
                            const auto& data = *(Shared*)payload->Data;
                            subItemConfig = data;
                        }
                        ImGui::EndDragDropTarget();
                    }
                }

                ImGui::Unindent();
            }
            ImGui::Unindent();
            ImGui::PopID();
        }
        ImGui::ListBoxFooter();
    }

    ImGui::SameLine();

    if (ImGui::BeginChild("##child", { 400.0f, 0.0f })) {
        auto& sharedConfig = getConfigShared(currentCategory, currentItem);

        ImGui::Checkbox("Enabled", &sharedConfig.enabled);
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 260.0f);
        ImGui::SetNextItemWidth(220.0f);
        if (ImGui::BeginCombo("Font", config->getSystemFonts()[sharedConfig.font.index].c_str())) {
            for (size_t i = 0; i < config->getSystemFonts().size(); i++) {
                bool isSelected = config->getSystemFonts()[i] == sharedConfig.font.name;
                if (ImGui::Selectable(config->getSystemFonts()[i].c_str(), isSelected, 0, { 250.0f, 0.0f })) {
                    sharedConfig.font.index = i;
                    sharedConfig.font.name = config->getSystemFonts()[i];
                    config->scheduleFontLoad(sharedConfig.font.name);
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();

        constexpr auto spacing = 210.0f;
        ImGuiCustom::colorPicker("Tracer", sharedConfig.snapline);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(90.0f);
        ImGui::Combo("##1", &sharedConfig.snapline.type, "Bottom\0Top\0Crosshair\0");
        ImGui::SameLine(spacing);
        ImGuiCustom::colorPicker("Box", sharedConfig.box);
        ImGui::SameLine();

        if (ImGui::ArrowButton("espbox", ImGuiDir_Right))
            ImGui::OpenPopup("##box");

        if (ImGui::BeginPopup("##box")) {
            ImGui::SetNextItemWidth(95.0f);
            ImGui::Combo("Type", &sharedConfig.box.type, "2D\0" "2D corners\0" "3D\0" "3D corners\0");
            ImGui::SetNextItemWidth(275.0f);
            ImGui::SliderFloat3("Scale", sharedConfig.box.scale.data(), 0.0f, 0.50f, "%.2f");
            ImGuiCustom::colorPicker("Fill / outline", sharedConfig.box.fill);
            ImGui::EndPopup();
        }

        ImGuiCustom::colorPicker("Name", sharedConfig.name);
        ImGui::SameLine(spacing);

        if (currentCategory < 2) {
            auto& playerConfig = getConfigPlayer(currentCategory, currentItem);

            ImGuiCustom::colorPicker("Weapon", playerConfig.weapon);
            ImGuiCustom::colorPicker("Flash duration", playerConfig.flashDuration);
            ImGui::SameLine(spacing);
            ImGuiCustom::colorPicker("Skeleton", playerConfig.skeleton);
            ImGui::Checkbox("Audible", &playerConfig.audibleOnly);
            ImGui::SameLine(spacing);
            ImGui::Checkbox("Spotted", &playerConfig.spottedOnly);

            ImGuiCustom::colorPicker("Head box", playerConfig.headBox);
            ImGui::SameLine();

            if (ImGui::ArrowButton("headbox", ImGuiDir_Right))
                ImGui::OpenPopup("##head_box");

            if (ImGui::BeginPopup("##head_box")) {
                ImGui::SetNextItemWidth(95.0f);
                ImGui::Combo("Type", &playerConfig.headBox.type, "2D\0" "Corner 2D\0" "3D\0" "Corner 3D\0");
                ImGui::SetNextItemWidth(275.0f);
                ImGui::SliderFloat3("Scale", playerConfig.headBox.scale.data(), 0.0f, 0.50f, "%.2f");
                ImGuiCustom::colorPicker("Fill / outline", playerConfig.headBox.fill);
                ImGui::EndPopup();
            }
        
            ImGui::SameLine(spacing);
            ImGuiCustom::colorPicker("Health bar", playerConfig.healthBar);
			ImGuiCustom::colorPicker("Flags", playerConfig.flags);
            ImGui::SameLine(spacing);
        } else if (currentCategory == 2) {
            auto& weaponConfig = config->esp.weapons[currentItem];
            ImGuiCustom::colorPicker("Ammo", weaponConfig.ammo);
        } else if (currentCategory == 3) {
            auto& trails = config->esp.projectiles[currentItem].trails;

            ImGui::Checkbox("Trails", &trails.enabled);

            if (ImGui::ArrowButton("esptrails", ImGuiDir_Right))
                ImGui::OpenPopup("##trails");

            if (ImGui::BeginPopup("##trails")) {
                constexpr auto trailPicker = [](const char* name, Trail& trail) noexcept {
                    ImGui::PushID(name);
                    ImGuiCustom::colorPicker(name, trail);
                    ImGui::SameLine(150.0f);
                    ImGui::SetNextItemWidth(95.0f);
                    ImGui::Combo("", &trail.type, "Line\0Circles\0Filled Circles\0");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(95.0f);
                    ImGui::InputFloat("Time", &trail.time, 0.1f, 0.5f, "%.1fs");
                    trail.time = std::clamp(trail.time, 1.0f, 60.0f);
                    ImGui::PopID();
                };

                trailPicker("Local player", trails.localPlayer);
                trailPicker("Allies", trails.allies);
                trailPicker("Enemies", trails.enemies);
                ImGui::EndPopup();
            }
        }

		ImGui::SetNextItemWidth(95.0f);
        ImGui::InputFloat("Text cull", &sharedConfig.textCullDistance, 1.0f, 10.0f, "%.0fu");
    }

    ImGui::EndChild();

    if (!contentOnly)
        ImGui::End();
}

void GUI::renderVisualsWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.visuals)
            return;
        ImGui::SetNextWindowSize({ 560.0f, 0.0f });
        ImGui::Begin("Visuals", &window.visuals, windowFlags);
    }
    ImGui::Columns(2, nullptr, false);
    ImGui::Checkbox("Disable post-processing", &config->visuals.disablePostProcessing);
    ImGui::Checkbox("Inverse ragdoll gravity", &config->visuals.inverseRagdollGravity);
    ImGui::Checkbox("No fog", &config->visuals.noFog);
    ImGui::Checkbox("No 3d sky", &config->visuals.no3dSky);
    //ImGui::Checkbox("No land bob", &config->visuals.noLandBob);
    ImGui::Checkbox("No aim punch", &config->visuals.noAimPunch);
    ImGui::Checkbox("No view punch", &config->visuals.noViewPunch);
    ImGui::Checkbox("No hands", &config->visuals.noHands);
    ImGui::Checkbox("No sleeves", &config->visuals.noSleeves);
    ImGui::Checkbox("No weapons", &config->visuals.noWeapons);
    ImGui::Checkbox("No smoke", &config->visuals.noSmoke);
    ImGui::Checkbox("No blur", &config->visuals.noBlur);
    ImGui::Checkbox("No scope overlay", &config->visuals.noScopeOverlay);
    ImGui::Checkbox("No grass", &config->visuals.noGrass);
    ImGui::Checkbox("No shadows", &config->visuals.noShadows);
    ImGui::Checkbox("Wireframe smoke", &config->visuals.wireframeSmoke);
	ImGui::Checkbox("Model names", &config->visuals.modelNames);

	ImGuiCustom::colorPicker("Molotov radius", config->visuals.molotovHull);
	ImGuiCustom::colorPicker("Player bounds", config->visuals.playerBounds);
	ImGuiCustom::colorPicker("Player velocity", config->visuals.playerVel);

	ImGuiCustom::colorPicker("Own beams", config->visuals.self.col, nullptr, nullptr, &config->visuals.self.enabled);
	ImGui::SameLine();
	if (ImGui::ArrowButton("bself", ImGuiDir_Right))
		ImGui::OpenPopup("##bself");

	if (ImGui::BeginPopup("##bself"))
	{
		ImGui::SetNextItemWidth(100.0f);
		ImGui::Combo("Sprite", &config->visuals.self.sprite, "Phys beam\0Solid\0Laser\0Laser beam\0");
		ImGui::PushItemWidth(255.0f);
		ImGui::SliderFloat("##bwidth", &config->visuals.self.width, 0.0f, 5.0f, "Thickness %.3f");
		ImGui::SliderFloat("##blife", &config->visuals.self.life, 0.0f, 10.0f, "Duration %.3fs");
		ImGui::PopItemWidth();
		ImGui::Checkbox("Railgun / noise", &config->visuals.self.railgun);
		ImGui::SetNextItemWidth(255.0f);
		if (config->visuals.self.railgun)
			ImGui::SliderFloat("##bnoise", &config->visuals.self.noise, 0.0f, 10.0f, "Radius %.3f");
		else
			ImGui::SliderFloat("##bnoise", &config->visuals.self.noise, 0.0f, 10.0f, "Noise %.3f");
		if (!config->visuals.self.railgun)
			ImGui::Checkbox("Do noise once", &config->visuals.self.noiseOnce);
		ImGui::EndPopup();
	}
	
	ImGuiCustom::colorPicker("Ally beams", config->visuals.ally.col, nullptr, nullptr, &config->visuals.ally.enabled);
	ImGui::SameLine();
	if (ImGui::ArrowButton("bally", ImGuiDir_Right))
		ImGui::OpenPopup("##bally");

	if (ImGui::BeginPopup("##bally"))
	{
		ImGui::SetNextItemWidth(100.0f);
		ImGui::Combo("Sprite", &config->visuals.ally.sprite, "Phys beam\0Solid\0Laser\0Laser beam\0");
		ImGui::PushItemWidth(255.0f);
		ImGui::SliderFloat("##bwidth", &config->visuals.ally.width, 0.0f, 5.0f, "Thickness %.3f");
		ImGui::SliderFloat("##blife", &config->visuals.ally.life, 0.0f, 10.0f, "Duration %.3fs");
		ImGui::PopItemWidth();
		ImGui::Checkbox("Railgun / noise", &config->visuals.ally.railgun);
		ImGui::SetNextItemWidth(255.0f);
		if (config->visuals.ally.railgun)
			ImGui::SliderFloat("##bnoise", &config->visuals.ally.noise, 0.0f, 10.0f, "Radius %.3f");
		else
			ImGui::SliderFloat("##bnoise", &config->visuals.ally.noise, 0.0f, 10.0f, "Noise %.3f");
		if (!config->visuals.ally.railgun)
			ImGui::Checkbox("Do noise once", &config->visuals.ally.noiseOnce);
		ImGui::EndPopup();
	}

	ImGuiCustom::colorPicker("Enemy beams", config->visuals.enemy.col, nullptr, nullptr, &config->visuals.enemy.enabled);
	ImGui::SameLine();
	if (ImGui::ArrowButton("benemy", ImGuiDir_Right))
		ImGui::OpenPopup("##benemy");

	if (ImGui::BeginPopup("##benemy"))
	{
		ImGui::SetNextItemWidth(100.0f);
		ImGui::Combo("Sprite", &config->visuals.enemy.sprite, "Phys beam\0Solid\0Laser\0Laser beam\0");
		ImGui::PushItemWidth(255.0f);
		ImGui::SliderFloat("##bwidth", &config->visuals.enemy.width, 0.0f, 5.0f, "Thickness %.3f");
		ImGui::SliderFloat("##blife", &config->visuals.enemy.life, 0.0f, 10.0f, "Duration %.3fs");
		ImGui::PopItemWidth();
		ImGui::Checkbox("Railgun / noise", &config->visuals.enemy.railgun);
		ImGui::SetNextItemWidth(255.0f);
		if (config->visuals.enemy.railgun)
			ImGui::SliderFloat("##bnoise", &config->visuals.enemy.noise, 0.0f, 10.0f, "Radius %.3f");
		else
			ImGui::SliderFloat("##bnoise", &config->visuals.enemy.noise, 0.0f, 10.0f, "Noise %.3f");
		if (!config->visuals.enemy.railgun)
			ImGui::Checkbox("Do noise once", &config->visuals.enemy.noiseOnce);
		ImGui::EndPopup();
	}

	ImGui::PushItemWidth(100.0f);
	ImGui::Combo("Bullet impacts", &config->visuals.bulletImpacts, "None\0All\0Client\0Server\0");
	ImGui::Combo("Accuracy tracers", &config->visuals.accuracyTracers, "None\0Hover\0Contact\0");
	ImGui::PopItemWidth();

    ImGui::NextColumn();

	ImGuiCustom::keyBind("Zoom", config->visuals.zoom);
    ImGui::PushItemWidth(255.0f);
    ImGui::SliderInt("##zoom", &config->visuals.zoomFac, 0, 99, "Zoom factor %d%%");
	ImGuiCustom::keyBind("Thirdperson", config->visuals.thirdPerson);
    ImGui::SliderInt("##tpdist", &config->visuals.thirdpersonDistance, 0, 500, "Thirdperson distance %du");
    ImGui::SliderInt("##fovmod", &config->visuals.fov, 1, 170, "FOV %ddeg");
	ImGui::Checkbox("Keep FOV", &config->visuals.forceFov);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Keep FOV when scoped");
    ImGui::SliderInt("##far_z", &config->visuals.farZ, 0, 2500, "Far Z %d");
    ImGui::SliderInt("##flash_red", &config->visuals.flashReduction, 0, 100, "Flash reduction %d%%");
    ImGui::SliderFloat("##brightness", &config->visuals.brightness, 0.0f, 1.0f, "Brightness %.2f");
    ImGui::PopItemWidth();

    ImGui::Combo("Skybox", &config->visuals.skybox, Helpers::skyboxList.data(), Helpers::skyboxList.size());

    ImGuiCustom::colorPicker("World color", config->visuals.world);
    ImGuiCustom::colorPicker("Props color", config->visuals.props);
    ImGuiCustom::colorPicker("Sky color", config->visuals.sky);
	//ImGuiCustom::colorPicker("Console color", config->visuals.consoleCol);
		;
    ImGui::PushItemWidth(255.0f);
	ImGui::Checkbox("Opposite hand knife", &config->visuals.oppositeHandKnife);
    ImGui::Checkbox("Deagle spinner", &config->visuals.deagleSpinner);
	ImGui::SetNextItemWidth(130.0f);
    ImGui::Combo("Screen effect", &config->visuals.screenEffect, "None\0Drone cam\0Noisy drone\0Underwater\0Healthboost\0Dangerzone\0");
	ImGui::SetNextItemWidth(130.0f);
    ImGui::Combo("Hit effect", &config->visuals.hitEffect, "None\0Drone cam\0Noisy drone\0Underwater\0Healthboost\0Dangerzone\0");
    ImGui::SliderFloat("##heff_time", &config->visuals.hitEffectTime, 0.1f, 1.5f, "Hit effect time %.2fs");
	ImGui::SetNextItemWidth(130.0f);
    ImGui::Combo("Kill effect", &config->visuals.killEffect, "None\0Drone cam\0Noisy drone\0Underwater\0Healthboost\0Dangerzone\0");
    ImGui::SliderFloat("##keff_time", &config->visuals.killEffectTime, 0.1f, 1.5f, "Kill effect time %.2fs");
	ImGui::SetNextItemWidth(130.0f);
    ImGui::Combo("Hit marker", &config->visuals.hitMarker, "None\0Cross\0Circle");
    ImGui::SliderFloat("##hmark_time", &config->visuals.hitMarkerTime, 0.1f, 1.5f, "Hit marker time %.2fs");
	ImGui::SliderFloat("##aspect_ratio", &config->visuals.aspectratio, 0.0f, 5.0f, "Aspect ratio %.2f");
    ImGui::PopItemWidth();

	ImGui::Checkbox("Post processing", &config->visuals.colorCorrection.enabled);
	ImGui::SameLine();
    if (ImGui::ArrowButton("post", ImGuiDir_Right))
        ImGui::OpenPopup("##post");

    if (ImGui::BeginPopup("##post")) {
        ImGui::VSliderFloat("##1", { 40.0f, 160.0f }, &config->visuals.colorCorrection.blue, 0.0f, 1.0f, "Blue\n%.3f"); ImGui::SameLine();
        ImGui::VSliderFloat("##2", { 40.0f, 160.0f }, &config->visuals.colorCorrection.red, 0.0f, 1.0f, "Red\n%.3f"); ImGui::SameLine();
        ImGui::VSliderFloat("##3", { 40.0f, 160.0f }, &config->visuals.colorCorrection.mono, 0.0f, 1.0f, "Mono\n%.3f"); ImGui::SameLine();
        ImGui::VSliderFloat("##4", { 40.0f, 160.0f }, &config->visuals.colorCorrection.saturation, 0.0f, 1.0f, "Sat\n%.3f"); ImGui::SameLine();
        ImGui::VSliderFloat("##5", { 40.0f, 160.0f }, &config->visuals.colorCorrection.ghost, 0.0f, 1.0f, "Ghost\n%.3f"); ImGui::SameLine();
        ImGui::VSliderFloat("##6", { 40.0f, 160.0f }, &config->visuals.colorCorrection.green, 0.0f, 1.0f, "Green\n%.3f"); ImGui::SameLine();
        ImGui::VSliderFloat("##7", { 40.0f, 160.0f }, &config->visuals.colorCorrection.yellow, 0.0f, 1.0f, "Yellow\n%.3f"); ImGui::SameLine();
        ImGui::EndPopup();
    }

	ImGui::Checkbox("Viewmodel", &config->visuals.viewmodel.enabled);
	ImGui::SameLine();
	if (ImGui::ArrowButton("viewmodel", ImGuiDir_Right))
		ImGui::OpenPopup("##viewmodel");

	if (ImGui::BeginPopup("##viewmodel"))
	{
		ImGui::PushItemWidth(290.0f);
		ImGui::SliderFloat("##x", &config->visuals.viewmodel.x, -20.0f, 20.0f, "X: %.4f");
		ImGui::SliderFloat("##y", &config->visuals.viewmodel.y, -20.0f, 20.0f, "Y: %.4f");
		ImGui::SliderFloat("##z", &config->visuals.viewmodel.z, -20.0f, 20.0f, "Z: %.4f");
		ImGui::SliderInt("##vmfov", &config->visuals.viewmodel.fov, -60, 60, "Viewmodel FOV: %d");
		ImGui::SliderFloat("##roll", &config->visuals.viewmodel.roll, -90.0f, 90.0f, "Viewmodel roll: %.2f");
		ImGui::PopItemWidth();
		ImGui::EndPopup();
	}

    if (!contentOnly)
        ImGui::End();
}

void GUI::renderSkinChangerWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.skinChanger)
            return;
        ImGui::SetNextWindowSize({ 700.0f, 0.0f });
        ImGui::Begin("Skin changer", &window.skinChanger, windowFlags);
    }

    static auto itemIndex = 0;

    ImGui::PushItemWidth(110.0f);
    ImGui::Combo("##1", &itemIndex, [](void* data, int idx, const char** out_text) {
        *out_text = game_data::weapon_names[idx].name;
        return true;
        }, nullptr, game_data::weapon_names.size(), 5);
    ImGui::PopItemWidth();

    auto& selected_entry = config->skinChanger[itemIndex];
    selected_entry.itemIdIndex = itemIndex;

    {
        ImGui::SameLine();
        ImGui::Checkbox("Enabled", &selected_entry.enabled);
        ImGui::Separator();
        ImGui::Columns(2, nullptr, false);
        ImGui::InputInt("Seed", &selected_entry.seed);
        ImGui::InputInt("StatTrak", &selected_entry.stat_trak);
        selected_entry.stat_trak = (std::max)(selected_entry.stat_trak, -1);
        ImGui::SliderFloat("Wear", &selected_entry.wear, FLT_MIN, 1.f, "%.10f", ImGuiSliderFlags_Logarithmic);

        static std::string filter;
        ImGui::PushID("Search");
        ImGui::InputTextWithHint("", "Search", &filter);
        ImGui::PopID();

        if (ImGui::ListBoxHeader("Paint kit")) {
            const auto& kits = itemIndex == 1 ? SkinChanger::getGloveKits() : SkinChanger::getSkinKits();

            std::wstring filterWide(filter.length(), L'\0');
            const auto newLen = mbstowcs(filterWide.data(), filter.c_str(), filter.length());
            if (newLen != static_cast<std::size_t>(-1))
                filterWide.resize(newLen);
			std::transform(filterWide.begin(), filterWide.end(), filterWide.begin(), [](wchar_t w) { return std::towupper(w); });

            for (std::size_t i = 0; i < kits.size(); ++i) {
                if (filter.empty() || wcsstr(kits[i].nameUpperCase.c_str(), filterWide.c_str())) {
                    ImGui::PushID(i);
                    if (ImGui::Selectable(kits[i].name.c_str(), i == selected_entry.paint_kit_vector_index))
                        selected_entry.paint_kit_vector_index = i;
                    ImGui::PopID();
                }
            }
            ImGui::ListBoxFooter();
        }

        ImGui::Combo("Quality", &selected_entry.entity_quality_vector_index, [](void* data, int idx, const char** out_text) {
            *out_text = SkinChanger::getQualities()[idx].name.c_str();
            return true;
            }, nullptr, SkinChanger::getQualities().size(), 5);

        if (itemIndex == 0) {
            ImGui::Combo("Knife", &selected_entry.definition_override_vector_index, [](void* data, int idx, const char** out_text) {
                *out_text = game_data::knife_names[idx].name;
                return true;
                }, nullptr, game_data::knife_names.size(), 5);
        } else if (itemIndex == 1) {
            ImGui::Combo("Glove", &selected_entry.definition_override_vector_index, [](void* data, int idx, const char** out_text) {
                *out_text = game_data::glove_names[idx].name;
                return true;
                }, nullptr, game_data::glove_names.size(), 5);
        } else {
            static auto unused_value = 0;
            selected_entry.definition_override_vector_index = 0;
            ImGui::Combo("Unavailable", &unused_value, "For knives or gloves\0");
        }

        ImGui::InputText("Name tag", selected_entry.custom_name, 32);

		constexpr auto playerModels = "Default\0Special Agent Ava | FBI\0Operator | FBI SWAT\0Markus Delrow | FBI HRT\0Michael Syfers | FBI Sniper\0B Squadron Officer | SAS\0Seal Team 6 Soldier | NSWC SEAL\0Buckshot | NSWC SEAL\0Lt. Commander Ricksaw | NSWC SEAL\0Third Commando Company | KSK\0'Two Times' McCoy | USAF TACP\0Dragomir | Sabre\0Rezan The Ready | Sabre\0'The Doctor' Romanov | Sabre\0Maximus | Sabre\0Blackwolf | Sabre\0The Elite Mr. Muhlik | Elite Crew\0Ground Rebel | Elite Crew\0Osiris | Elite Crew\0Prof. Shahmat | Elite Crew\0Enforcer | Phoenix\0Slingshot | Phoenix\0Soldier | Phoenix\0Pirate\0Pirate Variant A\0Pirate Variant B\0Pirate Variant C\0Pirate Variant D\0Anarchist\0Anarchist Variant A\0Anarchist Variant B\0Anarchist Variant C\0Anarchist Variant D\0Balkan Variant A\0Balkan Variant B\0Balkan Variant C\0Balkan Variant D\0Balkan Variant E\0Jumpsuit Variant A\0Jumpsuit Variant B\0Jumpsuit Variant C\0Street Soldier | Phoenix\0'Blueberries' Buckshot | NSWC SEAL\0'Two Times' McCoy | TACP Cavalry\0Rezan the Redshirt | Sabre\0Dragomir | Sabre Footsoldier\0Cmdr. Mae 'Dead Cold' Jamison | SWAT\0 1st Lieutenant Farlow | SWAT\0John 'Van Healen' Kask | SWAT\0Bio-Haz Specialist | SWAT\0Sergeant Bombson | SWAT\0Chem-Haz Specialist | SWAT\0Sir Bloody Miami Darryl | The Professionals\0Sir Bloody Silent Darryl | The Professionals\0Sir Bloody Skullhead Darryl | The Professionals\0Sir Bloody Darryl Royale | The Professionals\0Sir Bloody Loudmouth Darryl | The Professionals\0Safecracker Voltzmann | The Professionals\0Little Kev | The Professionals\0Number K | The Professionals\0Getaway Sally | The Professionals\0";
		ImGui::Combo("T player model", &config->visuals.playerModelT, playerModels);
		ImGui::Combo("CT player model", &config->visuals.playerModelCT, playerModels);
    }

    ImGui::NextColumn();

    {
        ImGui::PushID("sticker");

        static auto selectedStickerSlot = 0;

        ImGui::PushItemWidth(-1);

        if (ImGui::ListBoxHeader("", 5)) {
            for (int i = 0; i < 5; ++i) {
                ImGui::PushID(i);

                const auto kit_vector_index = config->skinChanger[itemIndex].stickers[i].kit_vector_index;
                const std::string text = '#' + std::to_string(i + 1) + "  " + SkinChanger::getStickerKits()[kit_vector_index].name;

                if (ImGui::Selectable(text.c_str(), i == selectedStickerSlot))
                    selectedStickerSlot = i;

                ImGui::PopID();
            }
            ImGui::ListBoxFooter();
        }

        ImGui::PopItemWidth();

        auto& selected_sticker = selected_entry.stickers[selectedStickerSlot];

        static std::string filter;
        ImGui::PushID("Search");
        ImGui::InputTextWithHint("", "Search", &filter);
        ImGui::PopID();

        if (ImGui::ListBoxHeader("Sticker")) {
            const auto& kits = SkinChanger::getStickerKits();

            std::wstring filterWide(filter.length(), L'\0');
            const auto newLen = mbstowcs(filterWide.data(), filter.c_str(), filter.length());
            if (newLen != static_cast<std::size_t>(-1))
                filterWide.resize(newLen);
			std::transform(filterWide.begin(), filterWide.end(), filterWide.begin(), [](wchar_t w) { return std::towupper(w); });

            for (std::size_t i = 0; i < kits.size(); ++i) {
                if (filter.empty() || wcsstr(kits[i].nameUpperCase.c_str(), filterWide.c_str())) {
                    ImGui::PushID(i);
                    if (ImGui::Selectable(kits[i].name.c_str(), i == selected_sticker.kit_vector_index))
                        selected_sticker.kit_vector_index = i;
                    ImGui::PopID();
                }
            }
            ImGui::ListBoxFooter();
        }

        ImGui::SliderFloat("Wear", &selected_sticker.wear, FLT_MIN, 1.0f, "%.10f", ImGuiSliderFlags_Logarithmic);
        ImGui::SliderFloat("Scale", &selected_sticker.scale, 0.1f, 5.0f);
        ImGui::SliderFloat("Rotation", &selected_sticker.rotation, 0.0f, 360.0f);

        ImGui::PopID();
    }
    selected_entry.update();

    ImGui::Columns(1);

    ImGui::Separator();

    if (ImGui::Button("Update", { 130.0f, 30.0f }))
        SkinChanger::scheduleHudUpdate();

    if (!contentOnly)
        ImGui::End();
}

void GUI::renderSoundWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.sound)
            return;
        ImGui::SetNextWindowSize({0.0f, 0.0f});
        ImGui::Begin("Sound", &window.sound, windowFlags);
    }

    ImGui::PushItemWidth(110.0f);
	ImGui::Combo("Hit Sound", &config->sound.hitSound, "None\0Metal\0Gamesense\0Bell\0Glass\0Custom\0");
	if (config->sound.hitSound)
	{
		ImGui::SetNextItemWidth(200.0f);
		ImGui::SliderFloat("##hitvol", &config->sound.hitSoundVol, 0.0f, 1.0f, "Volume %.3f");
	}
	if (config->sound.hitSound == 5)
	{
		ImGui::PushID("hitfile");
		ImGui::InputText("Filename", &config->sound.customHitSound);
		ImGui::PopID();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Audio file must be put in csgo/sound/ directory");
	}
	ImGui::Combo("Kill sound", &config->sound.killSound, "None\0Metal\0Gamesense\0Bell\0Glass\0Custom\0");
	if (config->sound.killSound)
	{
		ImGui::SetNextItemWidth(200.0f);
		ImGui::SliderFloat("##killvol", &config->sound.killSoundVol, 0.0f, 1.0f, "Volume %.3f");
	}
	if (config->sound.killSound == 5)
	{
		ImGui::PushID("killfile");
		ImGui::InputText("Filename", &config->sound.customKillSound);
		ImGui::PopID();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Audio file must be put in csgo/sound/ directory");
	}
    ImGui::PopItemWidth();

    ImGui::PushItemWidth(200.0f);
    ImGui::SliderInt("##chicken", &config->sound.chickenVolume, 0, 200, "Chicken volume %d%%");

	ImGui::Separator();

    static int currentCategory{ 0 };
	ImGui::SetNextItemWidth(110.0f);
    ImGui::Combo("##whose", &currentCategory, "Local player\0Allies\0Enemies\0");
    ImGui::SliderInt("##master", &config->sound.players[currentCategory].masterVolume, 0, 200, "Master volume %d%%");
    ImGui::SliderInt("##headshot", &config->sound.players[currentCategory].headshotVolume, 0, 200, "Headshot volume %d%%");
    ImGui::SliderInt("##weapon", &config->sound.players[currentCategory].weaponVolume, 0, 200, "Weapon volume %d%%");
    ImGui::SliderInt("##footstep", &config->sound.players[currentCategory].footstepVolume, 0, 200, "Footstep volume %d%%");
    ImGui::PopItemWidth();

    if (!contentOnly)
        ImGui::End();
}

void GUI::renderStyleWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.style)
            return;
        ImGui::SetNextWindowSize({ 0.0f, 0.0f });
        ImGui::Begin("Style", &window.style, windowFlags);
    }

    ImGui::PushItemWidth(150.0f);
    //if (ImGui::Combo("Menu style", &config->style.menuStyle, "Classic\0One window\0"))
    //    window = { };
    if (ImGui::Combo("Menu colors", &config->style.menuColors, "Frontier\0Eastern Sun\0NEPS\0Custom\0"))
        updateColors();
    ImGui::PopItemWidth();

    if (config->style.menuColors == 3) {
        ImGuiStyle& style = ImGui::GetStyle();
        for (int i = 0; i < ImGuiCol_COUNT; i++) {
            if (i && i & 3) ImGui::SameLine(200.0f * (i & 3));

            ImGuiCustom::colorPicker(ImGui::GetStyleColorName(i), (std::array<float, 4>&)style.Colors[i]);
        }
    }

    if (!contentOnly)
        ImGui::End();
}

void GUI::renderExploitsWindow(bool contentOnly) noexcept
{
	if (!contentOnly)
	{
		if (!window.exploits)
			return;
		ImGui::Begin("Exploits", &window.exploits, windowFlags);
	}

	ImGui::Checkbox("Anti AFK kick", &config->exploits.antiAfkKick);
	ImGui::Checkbox("Fast duck", &config->exploits.fastDuck);
	ImGui::Checkbox("Moonewalk", &config->exploits.moonwalk);
	ImGuiCustom::keyBind("Slowwalk", config->exploits.slowwalk);

	ImGui::Checkbox("Bypass sv_pure", &config->exploits.bypassPure);
	ImGui::PushItemWidth(90.0f);
	if (ImGui::Button("Spoof sv_cheats"))
	{
		const static auto cheatoVar = interfaces->cvar->findVar("sv_cheats");
		cheatoVar->onChangeCallbacks.size = 0;
		cheatoVar->setValue(true);
	}

	//ImGuiCustom::keyBind("Doubletap", config->exploits.doubletap);

	if (!contentOnly)
		ImGui::End();
}

void GUI::renderGriefingWindow(bool contentOnly) noexcept
{
	if (!contentOnly)
	{
		if (!window.steamapi)
			return;
		ImGui::Begin("Griefing", &window.steamapi, windowFlags);
	}

	static std::string playerInfoName;
	ImGui::SetNextItemWidth(192.0f);
	ImGui::InputText("##player_name", &playerInfoName);
	if (ImGui::Button("Change name"))
		Misc::changeName(false, playerInfoName.c_str() + '\x1', 0.0f);

	ImGui::SetNextItemWidth(192.0f);
	ImGui::InputText("##ban", &config->griefing.banText);
	ImGui::SetNextItemWidth(112.0f);
	ImGui::Combo("##ban_color", &config->griefing.banColor, "White\0Red\0Purple\0Green\0Light green\0Turquoise\0Light red\0Gray\0Yellow\0Gray 2\0Light blue\0Gray/Purple\0Blue\0Pink\0Dark orange\0Orange\0");
	ImGui::SameLine();
	if (ImGui::Button("Fake ban", ImVec2{75.0f, 0.0f}))
		Misc::fakeBan(true);

	ImGui::Checkbox("Fake prime", &config->griefing.fakePrime);
	ImGui::Checkbox("Name stealer", &config->griefing.nameStealer);
	ImGui::Checkbox("Clock tag", &config->griefing.clocktag);

	ImGui::Checkbox("Custom clantag", &config->griefing.customClanTag);
	if (config->griefing.customClanTag)
	{
		ImGui::SetNextItemWidth(192.0f);
		ImGui::InputText("##clantag", config->griefing.clanTag, sizeof(config->griefing.clanTag));
		ImGui::SetNextItemWidth(100.0f);
		ImGui::Combo("Animation", &config->griefing.animatedClanTag, "None\0Scroll\0");
	}

	ImGui::Checkbox("Kill message", &config->griefing.killMessage);
	if (config->griefing.killMessage)
	{
		ImGui::SetNextItemWidth(192.0f);
		ImGui::InputText("##killmsg", &config->griefing.killMessageString);
	}

	ImGui::Checkbox("Reportbot", &config->griefing.reportbot.enabled);
	ImGui::SameLine();

	if (ImGui::ArrowButton("reportbot", ImGuiDir_Right))
		ImGui::OpenPopup("##reportbot");

	if (ImGui::BeginPopup("##reportbot"))
	{
		ImGui::PushItemWidth(80.0f);
		ImGui::Combo("Target", &config->griefing.reportbot.target, "Enemies\0Allies\0All\0");
		ImGui::InputInt("Delay (s)", &config->griefing.reportbot.delay);
		config->griefing.reportbot.delay = (std::max)(config->griefing.reportbot.delay, 1);
		ImGui::InputInt("Rounds", &config->griefing.reportbot.rounds);
		config->griefing.reportbot.rounds = (std::max)(config->griefing.reportbot.rounds, 1);
		ImGui::PopItemWidth();
		ImGui::Checkbox("Abusive communications", &config->griefing.reportbot.textAbuse);
		ImGui::Checkbox("Griefing", &config->griefing.reportbot.griefing);
		ImGui::Checkbox("Wall hacking", &config->griefing.reportbot.wallhack);
		ImGui::Checkbox("Aim hacking", &config->griefing.reportbot.aimbot);
		ImGui::Checkbox("Other hacking", &config->griefing.reportbot.other);
		if (ImGui::Button("Reset"))
			Misc::resetReportbot();
		ImGui::EndPopup();
	}

	ImGuiCustom::keyBind("Blockbot", config->griefing.bb);
	ImGui::PushItemWidth(192.0f);
	ImGui::SliderFloat("##tfactor", &config->griefing.bbTrajFac, 0.0f, 4.0f, "Trajectory factor %.3fu");
	ImGui::SliderFloat("##dfactor", &config->griefing.bbDistFac, 0.0f, 4.0f, "Distance factor %.3fu");
	ImGui::PopItemWidth();
	ImGuiCustom::keyBind("Blockbot target", config->griefing.bbTar);
	ImGui::SameLine();
	ImGuiCustom::colorPicker("##bbinfo", config->griefing.bbCol);

	if (!contentOnly)
		ImGui::End();
}

void GUI::renderMovementWindow(bool contentOnly) noexcept
{
	if (!contentOnly)
	{
		if (!window.movement)
			return;
		ImGui::Begin("Movement", &window.movement, windowFlags);
	}

	ImGui::Checkbox("Bunnyhop", &config->movement.bunnyHop);
	if (config->movement.bunnyHop)
	{
		ImGui::Checkbox("Autostrafe", &config->movement.autoStrafe);
		ImGui::SliderFloat("##steer", &config->movement.steerSpeed, 0.0f, 30.0f, "Autostrafe steer %.3f");
	}
	ImGuiCustom::keyBind("Edge jump", config->movement.edgeJump);
	ImGui::Checkbox("Fast stop", &config->movement.fastStop);
	if (!contentOnly)
		ImGui::End();
}

void GUI::renderMiscWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.misc)
            return;
        ImGui::SetNextWindowSize({ 420.0f, 0.0f });
        ImGui::Begin("Misc", &window.misc, windowFlags);
    }
    ImGui::Columns(2, nullptr, false);
	ImGuiCustom::keyBind("Menu key", &config->misc.menuKey);
	if (config->misc.menuKey == 1) config->misc.menuKey = 0;
    ImGui::Checkbox("Full-auto", &config->misc.autoPistol);
    ImGui::Checkbox("Auto reload", &config->misc.autoReload);
    ImGui::Checkbox("Auto accept", &config->misc.autoAccept);

	#ifdef LEGACY_WATERMARK
	ImGui::PushID("spec");
    ImGuiCustom::colorPicker("Spectator list", config->misc.spectatorList);
	ImGui::PopID();
	ImGui::SameLine();
	ImGui::PushID("specBg");
	ImGuiCustom::colorPicker("Background", config->misc.specBg.color, &config->misc.specBg.rainbow, &config->misc.specBg.rainbowSpeed);
	ImGui::PopID();
	ImGui::PushID("water");
    ImGuiCustom::colorPicker("Watermark", config->misc.watermark);
	ImGui::PopID();
	ImGui::SameLine();
	ImGui::PushID("waterBg");
	ImGuiCustom::colorPicker("Background", config->misc.bg.color, &config->misc.bg.rainbow, &config->misc.bg.rainbowSpeed);
	ImGui::PopID();
	#else
	ImGui::Checkbox("Spectator list", &config->misc.spectatorList.enabled);
	ImGui::Checkbox("Watermark", &config->misc.watermark.enabled);
	#endif // LEGACY_WATERMARK

	ImGuiCustom::colorPicker("Crosshair overlay", config->misc.noscopeCrosshair);
	ImGuiCustom::colorPicker("Recoil crosshair", config->misc.recoilCrosshair);
	ImGuiCustom::colorPicker("Offscreen Enemies", config->misc.offscreenEnemies);
	ImGui::Checkbox("Grenade prediction", &config->misc.nadePredict);
	ImGuiCustom::colorPicker("Bomb timer", config->misc.bombTimer);
    ImGui::Checkbox("Fix animation LOD", &config->misc.fixAnimationLOD);
    ImGui::Checkbox("Fix bone matrix", &config->misc.fixBoneMatrix);
    ImGui::Checkbox("Fix movement", &config->misc.fixMovement);
    ImGui::Checkbox("Sync client animations", &config->misc.fixAnimation);
    ImGui::Checkbox("Disable model occlusion", &config->misc.disableModelOcclusion);

    ImGui::NextColumn();

	ImGui::Checkbox("Fast plant", &config->misc.fastPlant);
	ImGui::Checkbox("Quick reload", &config->misc.quickReload);
	ImGui::Checkbox("Fix tablet signal", &config->misc.fixTabletSignal);
	ImGui::Checkbox("Radar hack", &config->misc.radarHack);
	ImGui::Checkbox("Reveal ranks", &config->misc.revealRanks);
	ImGui::Checkbox("Reveal money", &config->misc.revealMoney);
	ImGui::Checkbox("Reveal suspect", &config->misc.revealSuspect);
    ImGui::Checkbox("Disable HUD blur", &config->misc.disablePanoramablur);
	ImGuiCustom::keyBind("Prepare revolver", config->misc.prepareRevolver);
	ImGuiCustom::keyBind("Quick healthshot", &config->misc.quickHealthshotKey);
    ImGui::SetNextItemWidth(190.0f);
	ImGui::SliderFloat("##angle_delta", &config->misc.maxAngleDelta, 0.0f, 255.0f, "Aimstep %.2f");
	ImGui::Checkbox("Preserve killfeed", &config->misc.preserveKillfeed.enabled);
    ImGui::SameLine();

    if (ImGui::ArrowButton("killfeed", ImGuiDir_Right))
        ImGui::OpenPopup("##killfeed");

    if (ImGui::BeginPopup("##killfeed")) {
        ImGui::Checkbox("Only headshots", &config->misc.preserveKillfeed.onlyHeadshots);
        ImGui::EndPopup();
    }

    ImGui::Checkbox("Purchase list", &config->misc.purchaseList.enabled);
    ImGui::SameLine();

    if (ImGui::ArrowButton("purchases", ImGuiDir_Right))
        ImGui::OpenPopup("##purchases");

    if (ImGui::BeginPopup("##purchases")) {
        ImGui::SetNextItemWidth(75.0f);
        ImGui::Combo("Mode", &config->misc.purchaseList.mode, "Details\0Summary\0");
        ImGui::Checkbox("Only during freeze time", &config->misc.purchaseList.onlyDuringFreezeTime);
        ImGui::Checkbox("Show prices", &config->misc.purchaseList.showPrices);
        ImGui::Checkbox("No title bar", &config->misc.purchaseList.noTitleBar);
        ImGui::EndPopup();
    }

    ImGui::Checkbox("Spam use", &config->misc.spamUse);
    ImGui::Checkbox("Indicators", &config->misc.indicators);

    if (!contentOnly)
        ImGui::End();
}

void GUI::renderConfigWindow(bool contentOnly) noexcept
{
    if (!contentOnly) {
        if (!window.config)
            return;
        ImGui::SetNextWindowSize({ 290.0f, 0.0f });
        ImGui::Begin("Config", &window.config, windowFlags);
    }

    ImGui::Columns(2, nullptr, false);
	ImGui::SetColumnWidth(0, 166.0f);
	ImGui::SetColumnWidth(1, 134.0f);

    static bool incrementalLoad = false;
    ImGui::Checkbox("Incremental Load", &incrementalLoad);

    ImGui::PushItemWidth(160.0f);

    if (ImGui::Button("Reload configs", { 160.0f, 25.0f }))
        config->listConfigs();

    auto& configItems = config->getConfigs();
    static int currentConfig = -1;

    if (static_cast<std::size_t>(currentConfig) >= configItems.size())
        currentConfig = -1;

    static std::string buffer;

    if (ImGui::ListBox("", &currentConfig, [](void* data, int idx, const char** out_text) {
        auto& vector = *static_cast<std::vector<std::string>*>(data);
        *out_text = vector[idx].c_str();
        return true;
        }, &configItems, configItems.size(), 5) && currentConfig != -1)
            buffer = configItems[currentConfig];

        ImGui::PushID(0);
        if (ImGui::InputTextWithHint("", "Config name", &buffer, ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (currentConfig != -1)
                config->rename(currentConfig, buffer.c_str());
        }
        ImGui::PopID();
        ImGui::NextColumn();

		ImGui::PopItemWidth();

		if (ImGui::Button("Open folder", {110.0f, 25.0f}))
			config->openConfigDir();

        if (ImGui::Button("Create config", { 110.0f, 25.0f }))
            config->add(buffer.c_str());

        if (ImGui::Button("Reset config", { 110.0f, 25.0f }))
            ImGui::OpenPopup("Config to reset");

        if (ImGui::BeginPopup("Config to reset")) {
            static constexpr const char* names[]{ "Whole", "Aimbot", "Triggerbot", "Backtrack", "Anti-aim", "Chams", "Glow", "ESP", "Visuals", "Skin changer", "Sound", "Players", "Exploits", "Movement", "Misc", "Style" };
            for (int i = 0; i < IM_ARRAYSIZE(names); i++) {
                if (i == 1) ImGui::Separator();

                if (ImGui::Selectable(names[i])) {
                    switch (i) {
                    case 0: config->reset(); updateColors(); SkinChanger::scheduleHudUpdate(); break;
                    case 1: config->aimbot = { }; break;
                    case 2: config->triggerbot = { }; break;
                    case 3: config->backtrack = { }; break;
                    case 4: config->antiAim = { }; break;
                    case 5: config->chams = { }; break;
                    case 6: config->glow = { }; break;
                    case 7: config->esp = { }; break;
                    case 8: config->visuals = { }; break;
                    case 9: config->skinChanger = { }; SkinChanger::scheduleHudUpdate(); break;
                    case 10: config->sound = { }; break;
					case 11: config->griefing = { }; break;
					case 12: config->exploits = { }; break;
					case 13: config->movement = { }; break;
                    case 14: config->misc = { }; break;
                    case 15: config->style = { }; updateColors(); break;
                    }
                }
            }
            ImGui::EndPopup();
        }
        if (currentConfig != -1) {
            if (ImGui::Button("Load selected", {110.0f, 25.0f })) {
                config->load(currentConfig, incrementalLoad);
                updateColors();
                SkinChanger::scheduleHudUpdate();
            }

			if (ImGui::Button("Save selected", {110.0f, 25.0f}))
				ImGui::OpenPopup("##reallySave");
			if (ImGui::BeginPopup("##reallySave"))
			{
				ImGui::TextUnformatted("Are you sure?");
				if (ImGui::Button("No", {45.0f, 0.0f}))
					ImGui::CloseCurrentPopup();
				ImGui::SameLine();
				if (ImGui::Button("Yes", {45.0f, 0.0f}))
				{
					config->save(currentConfig);
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

            if (ImGui::Button("Delete selected", { 110.0f, 25.0f }))
				ImGui::OpenPopup("##reallyDelete");
			if (ImGui::BeginPopup("##reallyDelete"))
			{
				ImGui::TextUnformatted("Are you sure?");
				if (ImGui::Button("No", {45.0f, 0.0f}))
					ImGui::CloseCurrentPopup();
				ImGui::SameLine();
				if (ImGui::Button("Yes", {45.0f, 0.0f}))
				{
					config->remove(currentConfig);
					currentConfig = -1;
					buffer.clear();
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
        }
        if (!contentOnly)
            ImGui::End();
}

void GUI::renderGuiStyle2() noexcept
{
    ImGui::Begin("NEPS.PP", nullptr, windowFlags | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);
    if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_NoTooltip)) {
        if (ImGui::BeginTabItem("Aimbot")) {
            renderAimbotWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Anti aim")) {
            renderAntiAimWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Triggerbot")) {
            renderTriggerbotWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Backtrack")) {
            renderBacktrackWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Glow")) {
            renderGlowWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Chams")) {
            renderChamsWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Extrasensory perception")) {
            renderESPWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Visuals")) {
            renderVisualsWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Skin changer")) {
            renderSkinChangerWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Sound")) {
            renderSoundWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Style")) {
            renderStyleWindow(true);
            ImGui::EndTabItem();
        }
		if (ImGui::BeginTabItem("Exploits")) {
            renderExploitsWindow(true);
            ImGui::EndTabItem();
        }
		if (ImGui::BeginTabItem("Social engineering")) {
            renderGriefingWindow(true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Movement")) {
            renderMovementWindow(true);
            ImGui::EndTabItem();
        }
		if (ImGui::BeginTabItem("Miscellaneous"))
		{
			renderMiscWindow(true);
			ImGui::EndTabItem();
		}
        if (ImGui::BeginTabItem("Configuration")) {
            renderConfigWindow(true);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

void GUI::debug() noexcept
{
	#define DEBUG_UI
	#ifdef DEBUG_UI

	static bool egg = false;

	if (ImGui::Button("Gay button"))
		egg = !egg;

	if (!egg) return;

	static auto &colors = ImGui::GetStyle().Colors;
	char buffer[128 * ImGuiCol_COUNT] = "";

	for (int i = 0; i < ImGuiCol_COUNT; i++)
	{
		strcat(buffer, "colors[ImGuiCol_");
		strcat(buffer, ImGui::GetStyleColorName(i));
		strcat(buffer, "] = {");
		strcat(buffer, std::to_string(colors[i].x).c_str());
		strcat(buffer, "f, ");
		strcat(buffer, std::to_string(colors[i].y).c_str());
		strcat(buffer, "f, ");
		strcat(buffer, std::to_string(colors[i].z).c_str());
		strcat(buffer, "f, ");
		strcat(buffer, std::to_string(colors[i].w).c_str());
		strcat(buffer, "f};\n");
	}

	ImGui::InputTextMultiline("", buffer, 128 * ImGuiCol_COUNT, {400.0f, 500.0f}, ImGuiInputTextFlags_ReadOnly);

	#endif // DEBUG_UI
}
