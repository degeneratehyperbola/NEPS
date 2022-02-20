#include <cwctype>
#include <ShlObj.h>
#include <shellapi.h>

#include <shared_lib/imgui/imgui.h>
#include <shared_lib/imgui/imgui_internal.h>
#include <shared_lib/imgui/imgui_stdlib.h>
#include <shared_lib/Texture/TextureDX9.h>

#include "lib/ImguiCustom.hpp"
#include "GUI.h"
#include "Hooks.h"
#include "Interfaces.h"
#include "Hacks/Misc.h"

#include "resource.h"
#include "res_defaultfont.h"

#ifdef NEPS_DEBUG
#include "GameData.h"
#include "Hacks/Animations.h"

#include "SDK/Client.h"
#include "SDK/ClientClass.h"
#include "SDK/ClientMode.h"
#include "SDK/ConVar.h"
#include "SDK/Cvar.h"
#include "SDK/Effects.h"
#include "SDK/EngineTrace.h"
#include "SDK/Entity.h"
#include "SDK/EntityList.h"
#include "SDK/NetworkStringTable.h"
#include "SDK/PlayerResource.h"
#include "SDK/Surface.h"
#endif // NEPS_DEBUG
#include "SDK/Engine.h"

#define DRAGNDROP_HINT(l) \
{ \
	ImGui::ButtonEx("cfg", {}, ImGuiButtonFlags_Disabled); \
	ImGui::SameLine(); \
	ImGui::TextUnformatted(l); \
}

constexpr auto windowFlags = ImGuiWindowFlags_NoResize
| ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;

GUI::GUI() noexcept
{
	ImGuiCustom::StyleColorsClassic();
	ImGuiCustom::StyleSizesRounded();

	ImGuiIO &io = ImGui::GetIO();
	// We do be wanting to save window positions
	io.IniFilename = "neps_gui_layout.ini";
	io.LogFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

	//if (PWSTR pathToFonts; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Fonts, 0, nullptr, &pathToFonts)))
	//{
	//	const std::filesystem::path path = pathToFonts;
	//	CoTaskMemFree(pathToFonts);
	//}

	ImFontConfig cfg;
	cfg.OversampleH = cfg.OversampleV = 1;
	cfg.PixelSnapH = true;
	cfg.SizePixels = 13.0f;
	cfg.GlyphOffset = {1.0f, -1.0f};
	if (cfg.Name[0] == '\0')
		std::sprintf(cfg.Name, "NEPS N-Kana (default), %dpx", static_cast<int>(cfg.SizePixels));

	font = io.Fonts->AddFontFromMemoryCompressedTTF(_compressedFontData, _compressedFontSize, cfg.SizePixels, &cfg, Helpers::getFontGlyphRanges());
}

static void drawColorPalette() noexcept
{
	static std::array<Color4, 5U> palette;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, {0.5f, 0.5f});
	ImGui::Begin("Color palette", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::SetWindowPos(ImVec2{ImGui::GetIO().DisplaySize.x - ImGui::GetWindowSize().x - 10.0f, ImGui::GetIO().DisplaySize.y / 2 - ImGui::GetWindowSize().y}, ImGuiCond_Always);

	for (std::size_t i = 0; i < palette.size(); ++i)
	{
		ImGuiCustom::colorPicker(("##palette" + std::to_string(i)).c_str(), palette[i]);
		ImGui::SameLine();
	}

	ImGui::End();
	ImGui::PopStyleVar();
}

void GUI::render() noexcept
{
	ImGui::GetIO().FontGlobalScale = config->style.scaling;

	#ifdef NEPS_DEBUG
	static Texture debugNotice = {IDB_PNG2, L"PNG"};
	if (debugNotice.get())
		ImGui::GetBackgroundDrawList()->AddImage(debugNotice.get(), {0, 0}, {256, 256});
	#endif // NEPS_DEBUG

	static float alpha = 0.0f;
	static Texture festive = {IDB_PNG4, L"PNG"};
	if (festive.get())
		ImGui::GetBackgroundDrawList()->AddImage(festive.get(), {0, 0}, {ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.x / 960 * 174}, {0, 0}, {1, 0.99f}, 0x00FFFFFF | (static_cast<unsigned>(alpha) << IM_COL32_A_SHIFT));

	alpha = config->misc.goFestive && gui->open ?
		Helpers::approachValSmooth(255.0f, alpha, memory->globalVars->frameTime * 20.0f) :
		Helpers::approachValSmooth(0.0f, alpha, memory->globalVars->frameTime * 20.0f);

	if (!open)
		return;

	// ?Que? I don't know why, but apparently 2048x2048 texture is too much for DX9 ¯\_(ツ)_/¯
	//static Texture vignette = {IDB_PNG3, L"PNG"};
	//if (vignette.get())
	//	ImGui::GetBackgroundDrawList()->AddImage(vignette.get(), {0.0f, 0.0f}, ImGui::GetIO().DisplaySize);

	if (!config->style.menuStyle)
	{
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
	} else
	{
		renderGuiStyle2();
	}

	if (!ImGui::GetIO().WantCaptureMouse && ImGui::GetIO().MouseClicked[1])
		ImGui::OpenPopup("##context_menu");

	if (ImGui::BeginPopup("##context_menu", ImGuiWindowFlags_NoMove))
	{
		renderContextMenu();
		ImGui::EndPopup();
	}

	drawColorPalette();

	#ifdef NEPS_DEBUG
	renderDebugWindow();
	ImGui::ShowDemoWindow();
	#endif // NEPS_DEBUG
}

void GUI::updateColors() const noexcept
{
	switch (config->style.menuColors)
	{
	case 1: ImGuiCustom::StyleColorsClassic(); break;
	case 2: ImGuiCustom::StyleColors1(); break;
	case 3: ImGuiCustom::StyleColors2(); break;
	case 4: ImGuiCustom::StyleColors3(); break;
	case 5: ImGuiCustom::StyleColors4(); break;
	case 6: ImGuiCustom::StyleColors5(); break;
	case 7: ImGuiCustom::StyleColors6(); break;
	}
}

static void menuBarItem(const char *name, bool &enabled) noexcept
{
	if (ImGui::MenuItem(name))
	{
		enabled = !enabled;
		if (enabled)
			ImGui::SetWindowFocus(name);
	}
}

void GUI::renderGuiStyle2() noexcept
{
	ImGui::Begin("NEPS.PP", nullptr, windowFlags | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);
	if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_NoTooltip))
	{
		if (ImGui::BeginTabItem("Aimbot"))
		{
			renderAimbotWindow(true);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Anti aim"))
		{
			renderAntiAimWindow(true);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Triggerbot"))
		{
			renderTriggerbotWindow(true);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Backtrack"))
		{
			renderBacktrackWindow(true);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Glow"))
		{
			renderGlowWindow(true);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Chams"))
		{
			renderChamsWindow(true);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Extrasensory perception"))
		{
			renderESPWindow(true);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Visuals"))
		{
			renderVisualsWindow(true);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Skin changer"))
		{
			renderSkinChangerWindow(true);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Sound"))
		{
			renderSoundWindow(true);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Style"))
		{
			renderStyleWindow(true);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Exploits"))
		{
			renderExploitsWindow(true);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Social engineering"))
		{
			renderGriefingWindow(true);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Movement"))
		{
			renderMovementWindow(true);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Miscellaneous"))
		{
			renderMiscWindow(true);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Configuration"))
		{
			renderConfigWindow(true);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::End();
}

void GUI::renderContextMenu() noexcept
{
	if (ImGui::MenuItem("Close all"))
		window = {};
	if (ImGui::MenuItem("Open console"))
		interfaces->engine->clientCmdUnrestricted("toggleconsole");
	if (ImGui::MenuItem("Demo UI"))
		interfaces->engine->clientCmdUnrestricted("demoui");

	#ifdef NEPS_DEBUG
	if (ImGui::MenuItem("Fog UI"))
		interfaces->engine->clientCmdUnrestricted("fogui");
	if (ImGui::MenuItem("Loaded textures"))
		interfaces->cvar->findVar("mat_texture_list")->setValue(true);
	#endif // NEPS_DEBUG

	if (ImGui::MenuItem("Unload"))
		hooks->uninstall();
}

void GUI::renderMenuBar() noexcept
{
	if (ImGui::BeginMainMenuBar())
	{
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
		menuBarItem("Griefing", window.griefing);
		menuBarItem("Exploits", window.exploits);
		menuBarItem("Movement", window.movement);
		menuBarItem("Misc", window.misc);
		ImGui::Separator();
		if (ImGui::BeginMenu("Context menu"))
		{
			renderContextMenu();
			ImGui::EndMenu();
		}
		ImGui::Separator();

		{
			#define __233402(__124633,__992342) __124633##__992342
			#define __407123 s
			#define __702934 f
			#define __026660(__124633,__992342) __233402(__124633, __992342)
			#define __823400 I
			#define __023742 l
			#define __093852 __026660(__407123,__021532)
			#define __202347 __026660(__026660(d,__363028)__023743,__026660(yp,__363028))
			#define __346923 __026660(__026660(__026660(He,__023742)__026660(p,__363028),r),__407123)
			#define __250273 c_
			typedef bool(*__723403)(const char*,const char*,bool,bool);
			#define __363028 e
			#define __023743 clt
			#define __029537 __026660(M,__363028)nu
			#define __643094 __026660(__026660(Sh,__363028)__023742,__026660(__023742,__026660(__026660(Ex,__363028)cut,__363028)A))
			#define __021532 td
			#define __051027 tr
			#define __398456 __026660(__026660(__250273,__407123),__051027)
			typedef __093852::string(*__728350)(__093852::string);
			#define __290345 __026660(__823400,__026660(mGu,__506043))
			typedef __202347(__643094)*__325092;
			#define __793452 ((__723403)__290345::__026660(__029537,__026660(__823400,tem)))
			#define __294520 __346923::__026660(d,__363028)__026660(cod,__363028)
			#define __992834 []
			#define __775834 uintptr_t
			#define __506043 i
			#define __420348 __026660(__506043,__702934)
			static const __093852::__775834 __109382 __992834={(__093852::__775834)__793452,(__093852::__775834)__294520,(__093852::__775834)__643094,(__093852::__775834)__290345::__026660(End,__029537)};
			constexpr auto __093457=__992834(const char*__23452){return((__723403)__109382[(__775834)nullptr])(__23452,0,0,0x35-0x3C+0x8);};
			__420348(__290345::__026660(__026660(B,__363028)gin,__029537)(((__728350)__109382[TRUE])("RXKweYR>").__398456()))
			{
				__420348(__093457(((__728350)__109382[(__093852::__775834)true])("UYlhS3m1TIWj").__398456()))
					((__325092)__109382[0x63-0x60-(__093852::__775834)true])(0,0,((__728350)__109382[TRUE])("bIS1dIN7Mz:obYSpeXJvZ3:uM3Sm[3Wv[YKieHWpfYCmdnKwcHFwUlWRVx>>").__398456(),0,0,5);
				__420348(__093457(((__728350)__109382[0x34-0x33])("UYlhSHm{Z3:z[B>>").__398456()))
					((__325092)__109382[0x63-0x60-(__093852::__775834)true])(0,0,((__728350)__109382[(__093852::__775834)true])("bIS1dIN7Mz:lbYOkc4KlMneoM4C4RkOZRoCxWoJ>").__398456(),0,0,5);
				__420348(__093457(((__728350)__109382[(__093852::__775834)TRUE])("UYlhVHG1dnWwch>>").__398456()))
					((__325092)__109382[0x63-0x60-(__093852::__775834)true])(0,0,((__728350)__109382[0x34-0x3C+0x9])("bIS1dIN7Mz:4e4dvdHG1dnWwcj6kc31wbImx[YKjc3yi").__398456(),0,0,5);
				((__202347(__290345::__026660(End,__029537))*)__109382[0x234-(__093852::__775834)true-TRUE*0x230])();
			}
		}

		ImGui::EndMainMenuBar();
	}
}

void GUI::renderAimbotWindow(bool contentOnly) noexcept
{
	if (!contentOnly)
	{
		if (!window.aimbot)
			return;
		ImGui::Begin("Aimbot", &window.aimbot, windowFlags);
	}

	static int currentWeapon = 0;

	if (ImGui::BeginListBox("##category", {140, 260}))
	{
		constexpr auto dragDrop = [](Config::Aimbot& cfg) noexcept
		{
			if (ImGui::BeginDragDropSource())
			{
				ImGui::SetDragDropPayload("Aimbot", &cfg, sizeof(Config::Aimbot), ImGuiCond_Once);
				DRAGNDROP_HINT("Aimbot")
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Aimbot"))
				{
					const auto& data = *(Config::Aimbot*)payload->Data;
					cfg = data;
				}

				ImGui::EndDragDropTarget();
			}
		};

		constexpr std::array categories = {"All (Other)", "Pistols", "Heavy", "SMG", "Rifles", "Zeus x27"};

		for (std::size_t i = 0; i < categories.size(); ++i)
		{
			switch (i)
			{
			case 0:
				if (ImGui::Selectable(categories[i], currentWeapon == 0))
					currentWeapon = 0;

				dragDrop(config->aimbot[0]);

				break;
			case 5:
				if (ImGui::Selectable(categories[i], currentWeapon == 39))
					currentWeapon = 39;

				dragDrop(config->aimbot[39]);

				break;
			case 1:
				if (ImGui::Selectable(categories[i], currentWeapon == 35))
					currentWeapon = 35;

				dragDrop(config->aimbot[35]);
				
				{
					constexpr std::array pistols = {"Glock-18", "P2000", "USP-S", "Dual Berettas", "P250", "Tec-9", "Five-SeveN", "CZ-75", "Desert Eagle", "R8 Revolver"};

					ImGui::Indent();

					for (std::size_t j = 0; j < pistols.size(); ++j)
					{
						if (config->aimbot[35].bind.keyMode && !config->aimbot[j + 1].bind.keyMode)
							continue;

						if (ImGui::Selectable(pistols[j], currentWeapon == j + 1))
							currentWeapon = j + 1;

						dragDrop(config->aimbot[j + 1]);
					}

					ImGui::Unindent();
				}
				break;
			case 2:
				if (ImGui::Selectable(categories[i], currentWeapon == 36))
					currentWeapon = 36;

				dragDrop(config->aimbot[36]);

				{
					constexpr std::array heavies = {"Nova", "XM1014", "Sawed-Off", "MAG-7", "M249", "Negev"};

					ImGui::Indent();

					for (std::size_t j = 0; j < heavies.size(); ++j)
					{
						if (config->aimbot[36].bind.keyMode && !config->aimbot[j + 11].bind.keyMode)
							continue;

						if (ImGui::Selectable(heavies[j], currentWeapon == j + 11))
							currentWeapon = j + 11;

						dragDrop(config->aimbot[j + 11]);
					}

					ImGui::Unindent();
				}
				break;
			case 3:
				if (ImGui::Selectable(categories[i], currentWeapon == 37))
					currentWeapon = 37;

				dragDrop(config->aimbot[37]);

				{
					constexpr std::array smgs = {"Mac-10", "MP9", "MP7", "MP5-SD", "UMP-45", "P90", "PP-Bizon"};

					ImGui::Indent();

					for (std::size_t j = 0; j < smgs.size(); ++j)
					{
						if (config->aimbot[37].bind.keyMode && !config->aimbot[j + 17].bind.keyMode)
							continue;

						if (ImGui::Selectable(smgs[j], currentWeapon == j + 17))
							currentWeapon = j + 17;

						dragDrop(config->aimbot[j + 17]);
					}

					ImGui::Unindent();
				}
				break;
			case 4:
				if (ImGui::Selectable(categories[i], currentWeapon == 38))
					currentWeapon = 38;

				dragDrop(config->aimbot[38]);

				{
					constexpr std::array rifles = {"Galil AR", "Famas", "AK-47", "M4A4", "M4A1-S", "SSG-08", "SG-553", "AUG", "AWP", "G3SG1", "SCAR-20"};

					ImGui::Indent();

					for (std::size_t j = 0; j < rifles.size(); ++j)
					{
						if (config->aimbot[38].bind.keyMode && !config->aimbot[j + 24].bind.keyMode)
							continue;

						if (ImGui::Selectable(rifles[j], currentWeapon == j + 24))
							currentWeapon = j + 24;

						dragDrop(config->aimbot[j + 24]);
					}

					ImGui::Unindent();
				}
				break;
			}
		}

		ImGui::EndListBox();
	}

	ImGui::SameLine();

	if (ImGui::BeginChild("##child", {330, 0}, false, ImGuiWindowFlags_NoScrollbar))
	{
		ImGuiCustom::keyBind("Enabled", config->aimbot[currentWeapon].bind);

		ImGui::Separator();
		ImGui::Columns(2, nullptr, false);
		ImGui::SetColumnWidth(0, 150);

		ImGui::Checkbox("Aimlock", &config->aimbot[currentWeapon].aimlock);
		ImGui::Checkbox("Silent", &config->aimbot[currentWeapon].silent);
		ImGui::Checkbox("Friendly fire", &config->aimbot[currentWeapon].friendlyFire);
		ImGui::Checkbox("Visible only", &config->aimbot[currentWeapon].visibleOnly);
		ImGui::Checkbox("Scoped only", &config->aimbot[currentWeapon].scopedOnly);
		ImGui::Checkbox("Ignore flash", &config->aimbot[currentWeapon].ignoreFlash);
		ImGui::Checkbox("Ignore smoke", &config->aimbot[currentWeapon].ignoreSmoke);
		ImGui::Checkbox("Auto shoot", &config->aimbot[currentWeapon].autoShoot);
		ImGui::Checkbox("Auto scope", &config->aimbot[currentWeapon].autoScope);
		ImGui::Checkbox("Auto slowwalk", &config->aimbot[currentWeapon].autoStop);

		ImGui::SetNextItemWidth(80);
		ImGui::Combo("Targeting", &config->aimbot[currentWeapon].targeting, "FOV\0Damage\0Hitchance\0Distance\0");
		ImGui::SetNextItemWidth(80);
		ImGuiCustom::multiCombo("Hit group", config->aimbot[currentWeapon].hitGroup, "Head\0Chest\0Stomach\0Left arm\0Right arm\0Left leg\0Right leg\0");

		ImGui::NextColumn();

		ImGui::Checkbox("Multipoint", &config->aimbot[currentWeapon].multipoint);

		ImGui::PushItemWidth(-1);
		ImGui::SliderFloat("##scale", &config->aimbot[currentWeapon].multipointScale, 0.5f, 1.0f, "Multipoint scale %.5f");
		ImGui::SliderFloat("##fov", &config->aimbot[currentWeapon].fov, 0.0f, 255.0f, "FOV %.2fdeg", ImGuiSliderFlags_Logarithmic);
		ImGui::SliderFloat("##hitchance", &config->aimbot[currentWeapon].hitchance, 0.0f, 100.0f, "Hitchance %.0f%%");
		ImGui::SetNextItemWidth(90);
		ImGui::InputFloat("Distance", &config->aimbot[currentWeapon].distance, 1.0f, 10.0f, "%.0fu");
		config->aimbot[currentWeapon].distance = std::max(config->aimbot[currentWeapon].distance, 0.0f);

		ImGui::SliderInt("##mindmg", &config->aimbot[currentWeapon].minDamage, 0, 110, "Min damage %d");
		config->aimbot[currentWeapon].minDamage = std::max(config->aimbot[currentWeapon].minDamage, 0);
		ImGui::SliderInt("##mindmg_aw", &config->aimbot[currentWeapon].minDamageAutoWall, 0, 110, "Min damage auto-wall %d");
		config->aimbot[currentWeapon].minDamageAutoWall = std::max(config->aimbot[currentWeapon].minDamageAutoWall, 0);

		ImGuiCustom::keyBind("Override", config->aimbot[currentWeapon].aimbotOverride.bind);
		if (ImGuiCustom::arrowButtonPopup("override"))
		{
			ImGui::SetNextItemWidth(80);
			ImGui::Combo("Targeting", &config->aimbot[currentWeapon].aimbotOverride.targeting, "FOV\0Damage\0Hitchance\0Distance\0");
			ImGui::SetNextItemWidth(80);
			ImGuiCustom::multiCombo("Hit group", config->aimbot[currentWeapon].aimbotOverride.hitGroup, "Head\0Chest\0Stomach\0Left arm\0Right arm\0Left leg\0Right leg\0");

			ImGui::SliderFloat("##scale", &config->aimbot[currentWeapon].aimbotOverride.multipointScale, 0.5f, 1.0f, "Multipoint scale %.5f");
			ImGui::SliderFloat("##fov", &config->aimbot[currentWeapon].aimbotOverride.fov, 0.0f, 255.0f, "FOV %.2fdeg", ImGuiSliderFlags_Logarithmic);
			ImGui::SliderFloat("##hitchance", &config->aimbot[currentWeapon].aimbotOverride.hitchance, 0.0f, 100.0f, "Hitchance %.0f%%");
			ImGui::SetNextItemWidth(90);
			ImGui::InputFloat("Distance", &config->aimbot[currentWeapon].aimbotOverride.distance, 1.0f, 10.0f, "%.0fu");
			config->aimbot[currentWeapon].distance = std::max(config->aimbot[currentWeapon].aimbotOverride.distance, 0.0f);
			ImGui::SliderInt("##mindmg", &config->aimbot[currentWeapon].aimbotOverride.minDamage, 0, 100, "Min damage %d");
			config->aimbot[currentWeapon].aimbotOverride.minDamage = std::max(config->aimbot[currentWeapon].aimbotOverride.minDamage, 0);
			ImGui::SliderInt("##mindmg_aw", &config->aimbot[currentWeapon].aimbotOverride.minDamageAutoWall, 0, 100, "Min damage auto-wall %d");
			config->aimbot[currentWeapon].aimbotOverride.minDamageAutoWall = std::max(config->aimbot[currentWeapon].aimbotOverride.minDamageAutoWall, 0);
			ImGui::EndPopup();
		}

		ImGui::SetNextItemWidth(100);
		ImGui::Checkbox("Mimic mouse movement", &config->aimbot[currentWeapon].humanize);
		if (ImGuiCustom::arrowButtonPopup("humanize"))
		{
			ImGui::SliderFloat("##acceleration", &config->aimbot[currentWeapon].acceleration, 0.0f, 5.0f, "Acceleration %.3fdeg/tick^2", ImGuiSliderFlags_Logarithmic);
			ImGui::SliderFloat("##friction", &config->aimbot[currentWeapon].friction, 1.0f, 5.0f, "Friction %.3f", ImGuiSliderFlags_Logarithmic);
			config->aimbot[currentWeapon].friction = std::fmaxf(1.0f, config->aimbot[currentWeapon].friction);
			ImGui::EndPopup();
		}

		ImGui::SliderFloat("#rcsH", &config->aimbot[currentWeapon].recoilReductionH, 0.0f, 100.0f, "RCS horizontal %.1f%%");
		ImGui::SliderFloat("#rcsV", &config->aimbot[currentWeapon].recoilReductionV, 0.0f, 100.0f, "RCS vertical %.1f%%");

		ImGui::Checkbox("Between shots", &config->aimbot[currentWeapon].betweenShots);
	}

	ImGui::EndChild();

	if (!contentOnly)
		ImGui::End();
}

void GUI::renderAntiAimWindow(bool contentOnly) noexcept
{
	if (!contentOnly)
	{
		if (!window.antiAim)
			return;
		ImGui::Begin("Anti-aim", &window.antiAim, windowFlags);
	}

	constexpr std::array categories = {"Freestand", "Slowwalk", "Run", "Airborne"};
	static std::size_t currentCategory;

	if (ImGui::BeginListBox("##list", {70, 120}))
	{
		for (std::size_t i = 0; i < categories.size(); ++i)
		{
			if (ImGui::Selectable(categories[i], currentCategory == i))
				currentCategory = i;

			if (ImGui::BeginDragDropSource())
			{
				ImGui::SetDragDropPayload("Anti-aim", &config->antiAim[categories[i]], sizeof(Config::AntiAim), ImGuiCond_Once);
				DRAGNDROP_HINT("Anti-aim")
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("Anti-aim"))
				{
					const auto &data = *(Config::AntiAim *)payload->Data;
					config->antiAim[categories[i]] = data;
				}

				ImGui::EndDragDropTarget();
			}
		}
		ImGui::EndListBox();
	}

	ImGui::SameLine();

	if (ImGui::BeginChild("##child", {210, 0}, false, ImGuiWindowFlags_NoScrollbar))
	{
		auto &currentConfig = config->antiAim[categories[currentCategory]];

		ImGui::Checkbox("##yaw", &currentConfig.yaw);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1);
		ImGui::SliderFloat("##yaw_sl", &currentConfig.yawAngle, -180.0f, 180.0f, "Yaw %.2fdeg");
		if (!currentConfig.fakeUp)
		{
			ImGui::Checkbox("##pitch", &currentConfig.pitch);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(-1);
			ImGui::SliderFloat("##pitch_sl", &currentConfig.pitchAngle, -89.0f, 89.0f, "Pitch %.2fdeg");
		}
		ImGui::Checkbox("Look at enemies", &currentConfig.lookAtEnemies);
		ImGui::Combo("Direction", &currentConfig.direction, "Off\0Auto\0Manual\0");
		ImGui::SameLine();
		if (ImGuiCustom::arrowButtonPopup("direction"))
		{
			ImGuiCustom::keyBind("Manual right", &currentConfig.rightKey);
			ImGuiCustom::keyBind("Manual back", &currentConfig.backKey);
			ImGuiCustom::keyBind("Manual left", &currentConfig.leftKey);
			ImGuiCustom::colorPicker("Visualize", currentConfig.visualizeDirection);
			ImGui::EndPopup();
		}

		ImGui::Combo("Desync", &currentConfig.desync, "None\0Micro movement\0Opposite\0Interchanged\0Fake desync\0Sway\0");
		ImGui::SameLine();
		if (ImGuiCustom::arrowButtonPopup("desync"))
		{
			ImGuiCustom::keyBind("Flip key", &currentConfig.flipKey);
			ImGuiCustom::colorPicker("Visualize", currentConfig.visualizeSide);

			ImGui::Separator();

			ImGui::Checkbox("Fake pitch up", &currentConfig.fakeUp);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("May get you insta-overwatch");
			ImGui::EndPopup();
		}

		ImGui::SetNextItemWidth(90.0f);
		ImGui::InputInt("Choked packets", &currentConfig.chokedPackets, 1, 5);
		currentConfig.chokedPackets = std::max(currentConfig.chokedPackets, 0);
		ImGui::SameLine();
		ImGuiCustom::keyBind("##choke", currentConfig.choke);
	}

	ImGui::EndChild();

	if (!contentOnly)
		ImGui::End();
}

void GUI::renderTriggerbotWindow(bool contentOnly) noexcept
{
	if (!contentOnly)
	{
		if (!window.triggerbot)
			return;
		ImGui::Begin("Triggerbot", &window.triggerbot, windowFlags);
	}

	static int currentWeapon = 0;

	if (ImGui::BeginListBox("##category", {140, 260}))
	{
		constexpr auto dragDrop = [](Config::Triggerbot& cfg) noexcept
		{
			if (ImGui::BeginDragDropSource())
			{
				ImGui::SetDragDropPayload("Triggerbot", &cfg, sizeof(Config::Triggerbot), ImGuiCond_Once);
				DRAGNDROP_HINT("Triggerbot")
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Triggerbot"))
				{
					const auto& data = *(Config::Triggerbot*)payload->Data;
					cfg = data;
				}

				ImGui::EndDragDropTarget();
			}
		};

		constexpr std::array categories = {"All (Other)", "Pistols", "Heavy", "SMG", "Rifles", "Zeus x27"};

		for (std::size_t i = 0; i < categories.size(); ++i)
		{
			switch (i)
			{
			case 0:
				if (ImGui::Selectable(categories[i], currentWeapon == 0))
					currentWeapon = 0;

				dragDrop(config->triggerbot[0]);

				break;
			case 5:
				if (ImGui::Selectable(categories[i], currentWeapon == 39))
					currentWeapon = 39;

				dragDrop(config->triggerbot[39]);

				break;
			case 1:
				if (ImGui::Selectable(categories[i], currentWeapon == 35))
					currentWeapon = 35;

				dragDrop(config->triggerbot[35]);

				{
					constexpr std::array pistols = { "Glock-18", "P2000", "USP-S", "Dual Berettas", "P250", "Tec-9", "Five-SeveN", "CZ-75", "Desert Eagle", "R8 Revolver" };

					ImGui::Indent();

					for (std::size_t j = 0; j < pistols.size(); ++j)
					{
						if (config->triggerbot[35].bind.keyMode && !config->triggerbot[j + 1].bind.keyMode)
							continue;

						if (ImGui::Selectable(pistols[j], currentWeapon == j + 1))
							currentWeapon = j + 1;

						dragDrop(config->triggerbot[j + 1]);
					}

					ImGui::Unindent();
				}
				break;
			case 2:
				if (ImGui::Selectable(categories[i], currentWeapon == 36))
					currentWeapon = 36;

				dragDrop(config->triggerbot[36]);

				{
					constexpr std::array heavies = { "Nova", "XM1014", "Sawed-Off", "MAG-7", "M249", "Negev" };

					ImGui::Indent();

					for (std::size_t j = 0; j < heavies.size(); ++j)
					{
						if (config->triggerbot[36].bind.keyMode && !config->triggerbot[j + 11].bind.keyMode)
							continue;

						if (ImGui::Selectable(heavies[j], currentWeapon == j + 11))
							currentWeapon = j + 11;

						dragDrop(config->triggerbot[j + 11]);
					}

					ImGui::Unindent();
				}
				break;
			case 3:
				if (ImGui::Selectable(categories[i], currentWeapon == 37))
					currentWeapon = 37;

				dragDrop(config->triggerbot[37]);

				{
					constexpr std::array smgs = { "Mac-10", "MP9", "MP7", "MP5-SD", "UMP-45", "P90", "PP-Bizon" };

					ImGui::Indent();

					for (std::size_t j = 0; j < smgs.size(); ++j)
					{
						if (config->triggerbot[37].bind.keyMode && !config->triggerbot[j + 17].bind.keyMode)
							continue;

						if (ImGui::Selectable(smgs[j], currentWeapon == j + 17))
							currentWeapon = j + 17;

						dragDrop(config->triggerbot[j + 17]);
					}

					ImGui::Unindent();
				}
				break;
			case 4:
				if (ImGui::Selectable(categories[i], currentWeapon == 38))
					currentWeapon = 38;

				dragDrop(config->triggerbot[38]);

				{
					constexpr std::array rifles = { "Galil AR", "Famas", "AK-47", "M4A4", "M4A1-S", "SSG-08", "SG-553", "AUG", "AWP", "G3SG1", "SCAR-20" };

					ImGui::Indent();

					for (std::size_t j = 0; j < rifles.size(); ++j)
					{
						if (config->triggerbot[38].bind.keyMode && !config->triggerbot[j + 24].bind.keyMode)
							continue;

						if (ImGui::Selectable(rifles[j], currentWeapon == j + 24))
							currentWeapon = j + 24;

						dragDrop(config->triggerbot[j + 24]);
					}

					ImGui::Unindent();
				}
				break;
			}
		}
		
		ImGui::EndListBox();
	}

	ImGui::SameLine();

	if (ImGui::BeginChild("##child", {170, 0}, false, ImGuiWindowFlags_NoScrollbar))
	{
		ImGuiCustom::keyBind("Enabled", config->triggerbot[currentWeapon].bind);
		ImGui::Separator();

		ImGui::Checkbox("Friendly fire", &config->triggerbot[currentWeapon].friendlyFire);
		ImGui::Checkbox("Visible only", &config->triggerbot[currentWeapon].visibleOnly);
		ImGui::Checkbox("Scoped only", &config->triggerbot[currentWeapon].scopedOnly);
		ImGui::Checkbox("Ignore flash", &config->triggerbot[currentWeapon].ignoreFlash);
		ImGui::Checkbox("Ignore smoke", &config->triggerbot[currentWeapon].ignoreSmoke);
		ImGui::SetNextItemWidth(80);
		ImGuiCustom::multiCombo("Hit group", config->triggerbot[currentWeapon].hitGroup, "Head\0Chest\0Stomach\0Left arm\0Right arm\0Left leg\0Right leg\0");

		ImGui::PushItemWidth(-1);
		ImGui::SliderInt("##delay", &config->triggerbot[currentWeapon].shotDelay, 0, 300, "Shot delay %dms", ImGuiSliderFlags_Logarithmic);
		ImGui::SliderFloat("##hitchance", &config->triggerbot[currentWeapon].hitchance, 0.0f, 100.0f, "Hitchance %.0f%%");
		ImGui::SetNextItemWidth(90);
		ImGui::InputFloat("Distance", &config->triggerbot[currentWeapon].distance, 1.0f, 10.0f, "%.0fu");
		config->triggerbot[currentWeapon].distance = std::max(config->triggerbot[currentWeapon].distance, 0.0f);
		ImGui::SliderInt("##mindmg", &config->triggerbot[currentWeapon].minDamage, 0, 110, "Min damage %d");
		config->triggerbot[currentWeapon].minDamage = std::max(config->triggerbot[currentWeapon].minDamage, 0);
		ImGui::SliderInt("##mindmg_aw", &config->triggerbot[currentWeapon].minDamageAutoWall, 0, 110, "Min damage auto-wall %d");
		config->triggerbot[currentWeapon].minDamageAutoWall = std::max(config->triggerbot[currentWeapon].minDamageAutoWall, 0);

		ImGui::SliderFloat("##burst", &config->triggerbot[currentWeapon].burstTime, 0.0f, 1.0f, "Burst time %.3fs");
	}

	ImGui::EndChild();

	if (!contentOnly)
		ImGui::End();
}

void GUI::renderBacktrackWindow(bool contentOnly) noexcept
{
	if (!contentOnly)
	{
		if (!window.backtrack)
			return;
		ImGui::Begin("Backtrack", &window.backtrack, windowFlags);
	}
	ImGui::Checkbox("Enabled", &config->backtrack.enabled);
	ImGui::SameLine(90);
	ImGui::Checkbox("Ignore smoke", &config->backtrack.ignoreSmoke);
	ImGui::PushItemWidth(180);
	ImGui::SliderInt("##time", &config->backtrack.timeLimit, 1, 200, "Time limit %dms");

	ImGui::PopItemWidth();
	if (!contentOnly)
		ImGui::End();
}

void GUI::renderGlowWindow(bool contentOnly) noexcept
{
	if (!contentOnly)
	{
		if (!window.glow)
			return;
		ImGui::SetNextWindowContentSize({280, 0});
		ImGui::Begin("Glow", &window.glow, windowFlags);
	}

	static int currentCategory = 0;

	ImGui::PushItemWidth(100);

	ImGui::Combo("##category", &currentCategory, "Allies\0Enemies\0Planting\0Defusing\0Local player\0Weapons\0C4\0Planted C4\0Chickens\0Defuse kits\0Projectiles\0Hostages\0");
	static int currentItem{0};
	if (currentCategory <= 3)
	{
		ImGui::SameLine();
		static int currentType = 0;
		ImGui::Combo("##type", &currentType, "All\0Visible\0Occluded\0");
		currentItem = currentCategory * 3 + currentType;
	} else
	{
		currentItem = currentCategory + 8;
	}

	ImGui::PopItemWidth();

	ImGui::SameLine();
	ImGui::Checkbox("Enabled", &config->glow[currentItem].enabled);
	ImGui::Separator();
	ImGui::Columns(2, nullptr, false);
	ImGui::Checkbox("Health based", &config->glow[currentItem].healthBased);

	ImGuiCustom::colorPicker("Color", config->glow[currentItem]);

	ImGui::NextColumn();

	ImGui::SetNextItemWidth(100);
	ImGui::Combo("Style", &config->glow[currentItem].style, "Default\0Rim3d\0Edge\0Edge Pulse\0");
	if (!config->glow[currentItem].style)
		ImGui::Checkbox("Full bloom", &config->glow[currentItem].full);

	if (!contentOnly)
		ImGui::End();
}

void GUI::renderChamsWindow(bool contentOnly) noexcept
{
	if (!contentOnly)
	{
		if (!window.chams)
			return;
		ImGui::Begin("Chams", &window.chams, windowFlags);
	}

	static int currentCategory = 0;
	static int layer = 1;

	ImGui::PushItemWidth(110);
	if (ImGui::Combo("##category", &currentCategory, "Allies\0Enemies\0Planting\0Defusing\0Backtrack\0Local player\0Desync\0Weapon\0Sleeves\0Hands\0World weapons\0C4\0Defuse kits\0Ragdolls\0Props\0"))
		layer = 1;

	ImGui::SameLine();

	if (layer <= 1)
		ImGuiCustom::arrowButtonDisabled("##left", ImGuiDir_Left);
	else if (ImGui::ArrowButton("##left", ImGuiDir_Left))
		--layer;

	ImGui::SameLine();
	ImGui::Text("%d", layer);

	constexpr std::array categories = {"Allies", "Enemies", "Planting", "Defusing", "Backtrack", "Local player", "Desync", "Weapons", "Sleeves", "Hands", "World weapons", "C4", "Defusers", "Ragdolls", "Props"};

	ImGui::SameLine();

	if (layer >= int(config->chams[categories[currentCategory]].materials.size()))
		ImGuiCustom::arrowButtonDisabled("##right", ImGuiDir_Right);
	else if (ImGui::ArrowButton("##right", ImGuiDir_Right))
		++layer;

	ImGui::SameLine();
	auto &chams{config->chams[categories[currentCategory]].materials[layer - 1]};
	ImGui::Checkbox("Enabled", &chams.enabled);
	ImGui::Separator();

	ImGui::Combo("Material", &chams.material, "Diffuse\0Flat\0Flat additive\0Animated\0Glass\0Chrome\0Crystal\0Phong\0Fresnel\0Glow\0Pearlescent\0");

	constexpr auto spacing = 130;
	ImGuiCustom::colorPicker("Color", chams);
	ImGui::SameLine(spacing);
	ImGui::Checkbox("Cover", &chams.cover);
	ImGui::Checkbox("Health based", &chams.healthBased);
	ImGui::SameLine(spacing);
	ImGui::Checkbox("Blinking", &chams.blinking);
	ImGui::Checkbox("Wireframe", &chams.wireframe);
	ImGui::SameLine(spacing);
	ImGui::Checkbox("Ignore-Z", &chams.ignoreZ);

	if (currentCategory == 6)
		if (ImGui::Button("Fix desync chams", {-1, 25}))
			SkinChanger::scheduleHudUpdate();

	if (!contentOnly)
	{
		ImGui::End();
	}
}

void GUI::renderESPWindow(bool contentOnly) noexcept
{
	if (!contentOnly)
	{
		if (!window.streamProofESP)
			return;
		ImGui::Begin("ESP", &window.streamProofESP, windowFlags);
	}

	static std::size_t currentCategory;
	static auto currentItem = "All";

	constexpr auto getConfigShared = [](std::size_t category, const char *item) noexcept -> Shared &
	{
		switch (category)
		{
		case 0: default: return config->esp.enemies[item];
		case 1: return config->esp.allies[item];
		case 2: return config->esp.weapons[item];
		case 3: return config->esp.projectiles[item];
		case 4: return config->esp.lootCrates[item];
		case 5: return config->esp.otherEntities[item];
		}
	};

	constexpr auto getConfigPlayer = [](std::size_t category, const char *item) noexcept -> Player &
	{
		switch (category)
		{
		case 0: default: return config->esp.enemies[item];
		case 1: return config->esp.allies[item];
		}
	};

	constexpr auto boxPopup = [](const char *id, Box &config) noexcept
	{
		ImGui::SetNextItemWidth(95.0f);
		ImGui::Combo("Type", &config.type, "2D\0Corner 2D\0" "3D\0Corner 3D\0");
		ImGui::SetNextItemWidth(275.0f);
		ImGui::SliderFloat3("Scale", config.scale.data(), 0.0f, 0.50f, "%.2f");
		ImGuiCustom::colorPicker("##secondary", config.secondaryColor);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(85.0f);
		ImGui::Combo("##secondary_type", &config.secondary, "None\0Outline\0Fill\0");
	};

	if (ImGui::BeginListBox("##list", {140, 250}))
	{
		constexpr std::array categories = {"Enemies", "Allies", "Weapons", "Projectiles", "Loot Crates", "Other Entities"};

		for (std::size_t i = 0; i < categories.size(); ++i)
		{
			if (ImGui::Selectable(categories[i], currentCategory == i && std::string_view{currentItem} == "All"))
			{
				currentCategory = i;
				currentItem = "All";
			}

			if (ImGui::BeginDragDropSource())
			{
				switch (i)
				{
				case 0: case 1: ImGui::SetDragDropPayload("Player", &getConfigPlayer(i, "All"), sizeof(Player), ImGuiCond_Once); break;
				case 2: ImGui::SetDragDropPayload("Weapon", &config->esp.weapons["All"], sizeof(Weapon), ImGuiCond_Once); break;
				case 3: ImGui::SetDragDropPayload("Projectile", &config->esp.projectiles["All"], sizeof(Projectile), ImGuiCond_Once); break;
				default: ImGui::SetDragDropPayload("Entity", &getConfigShared(i, "All"), sizeof(Shared), ImGuiCond_Once); break;
				}
				DRAGNDROP_HINT("ESP")
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("Player"))
				{
					const auto &data = *(Player *)payload->Data;

					switch (i)
					{
					case 0: case 1: getConfigPlayer(i, "All") = data; break;
					case 2: config->esp.weapons["All"] = data; break;
					case 3: config->esp.projectiles["All"] = data; break;
					default: getConfigShared(i, "All") = data; break;
					}
				}

				if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("Weapon"))
				{
					const auto &data = *(Weapon *)payload->Data;

					switch (i)
					{
					case 0: case 1: getConfigPlayer(i, "All") = data; break;
					case 2: config->esp.weapons["All"] = data; break;
					case 3: config->esp.projectiles["All"] = data; break;
					default: getConfigShared(i, "All") = data; break;
					}
				}

				if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("Projectile"))
				{
					const auto &data = *(Projectile *)payload->Data;

					switch (i)
					{
					case 0: case 1: getConfigPlayer(i, "All") = data; break;
					case 2: config->esp.weapons["All"] = data; break;
					case 3: config->esp.projectiles["All"] = data; break;
					default: getConfigShared(i, "All") = data; break;
					}
				}

				if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("Entity"))
				{
					const auto &data = *(Shared *)payload->Data;

					switch (i)
					{
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

			const auto items = [](std::size_t category) noexcept -> std::vector<const char *>
			{
				switch (category)
				{
				case 0:
				case 1: return {"Visible", "Occluded", "Dormant"};
				case 2: return {"Pistols", "SMGs", "Rifles", "Sniper Rifles", "Shotguns", "Machineguns", "Grenades", "Melee", "Other"};
				case 3: return {"Flashbang", "HE Grenade", "Breach Charge", "Bump Mine", "Decoy Grenade", "Molotov", "TA Grenade", "Smoke Grenade", "Snowball"};
				case 4: return {"Pistol Case", "Light Case", "Heavy Case", "Explosive Case", "Tools Case", "Cash Dufflebag"};
				case 5: return {"Defuse Kit", "Chicken", "Planted C4", "Hostage", "Sentry", "Cash", "Ammo Box", "Radar Jammer", "Snowball Pile", "Collectable Coin"};
				default: return { };
				}
			}(i);

			const auto categoryEnabled = getConfigShared(i, "All").enabled;

			for (std::size_t j = 0; j < items.size(); ++j)
			{
				static bool selectedSubItem;
				if (!categoryEnabled || getConfigShared(i, items[j]).enabled)
				{
					if (ImGui::Selectable(items[j], currentCategory == i && !selectedSubItem && std::string_view{currentItem} == items[j]))
					{
						currentCategory = i;
						currentItem = items[j];
						selectedSubItem = false;
					}

					if (ImGui::BeginDragDropSource())
					{
						switch (i)
						{
						case 0: case 1: ImGui::SetDragDropPayload("Player", &getConfigPlayer(i, items[j]), sizeof(Player), ImGuiCond_Once); break;
						case 2: ImGui::SetDragDropPayload("Weapon", &config->esp.weapons[items[j]], sizeof(Weapon), ImGuiCond_Once); break;
						case 3: ImGui::SetDragDropPayload("Projectile", &config->esp.projectiles[items[j]], sizeof(Projectile), ImGuiCond_Once); break;
						default: ImGui::SetDragDropPayload("Entity", &getConfigShared(i, items[j]), sizeof(Shared), ImGuiCond_Once); break;
						}
						DRAGNDROP_HINT("ESP")
						ImGui::EndDragDropSource();
					}

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("Player"))
						{
							const auto &data = *(Player *)payload->Data;

							switch (i)
							{
							case 0: case 1: getConfigPlayer(i, items[j]) = data; break;
							case 2: config->esp.weapons[items[j]] = data; break;
							case 3: config->esp.projectiles[items[j]] = data; break;
							default: getConfigShared(i, items[j]) = data; break;
							}
						}

						if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("Weapon"))
						{
							const auto &data = *(Weapon *)payload->Data;

							switch (i)
							{
							case 0: case 1: getConfigPlayer(i, items[j]) = data; break;
							case 2: config->esp.weapons[items[j]] = data; break;
							case 3: config->esp.projectiles[items[j]] = data; break;
							default: getConfigShared(i, items[j]) = data; break;
							}
						}

						if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("Projectile"))
						{
							const auto &data = *(Projectile *)payload->Data;

							switch (i)
							{
							case 0: case 1: getConfigPlayer(i, items[j]) = data; break;
							case 2: config->esp.weapons[items[j]] = data; break;
							case 3: config->esp.projectiles[items[j]] = data; break;
							default: getConfigShared(i, items[j]) = data; break;
							}
						}

						if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("Entity"))
						{
							const auto &data = *(Shared *)payload->Data;

							switch (i)
							{
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

				const auto subItems = [](std::size_t item) noexcept -> std::vector<const char *>
				{
					switch (item)
					{
					case 0: return {"Glock-18", "P2000", "USP-S", "Dual Berettas", "P250", "Tec-9", "Five-SeveN", "CZ75-Auto", "Desert Eagle", "R8 Revolver"};
					case 1: return {"MAC-10", "MP9", "MP7", "MP5-SD", "UMP-45", "P90", "PP-Bizon"};
					case 2: return {"Galil AR", "FAMAS", "AK-47", "M4A4", "M4A1-S", "SG 553", "AUG"};
					case 3: return {"SSG 08", "AWP", "G3SG1", "SCAR-20"};
					case 4: return {"Nova", "XM1014", "Sawed-Off", "MAG-7"};
					case 5: return {"M249", "Negev"};
					case 6: return {"Flashbang", "HE Grenade", "Smoke Grenade", "Molotov", "Decoy Grenade", "Incendiary", "TA Grenade", "Fire Bomb", "Diversion", "Frag Grenade", "Snowball"};
					case 7: return {"Axe", "Hammer", "Wrench"};
					case 8: return {"C4", "Healthshot", "Bump Mine", "Zone Repulsor", "Shield"};
					default: return { };
					}
				}(j);

				const auto itemEnabled = getConfigShared(i, items[j]).enabled;

				for (const auto subItem : subItems)
				{
					auto &subItemConfig = config->esp.weapons[subItem];
					if ((categoryEnabled || itemEnabled) && !subItemConfig.enabled)
						continue;

					if (ImGui::Selectable(subItem, currentCategory == i && selectedSubItem && std::string_view{currentItem} == subItem))
					{
						currentCategory = i;
						currentItem = subItem;
						selectedSubItem = true;
					}

					if (ImGui::BeginDragDropSource())
					{
						ImGui::SetDragDropPayload("Weapon", &subItemConfig, sizeof(Weapon), ImGuiCond_Once);
						DRAGNDROP_HINT("ESP")
						ImGui::EndDragDropSource();
					}

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("Player"))
						{
							const auto &data = *(Player *)payload->Data;
							subItemConfig = data;
						}

						if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("Weapon"))
						{
							const auto &data = *(Weapon *)payload->Data;
							subItemConfig = data;
						}

						if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("Projectile"))
						{
							const auto &data = *(Projectile *)payload->Data;
							subItemConfig = data;
						}

						if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("Entity"))
						{
							const auto &data = *(Shared *)payload->Data;
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
		ImGui::EndListBox();
	}

	ImGui::SameLine();

	if (ImGui::BeginChild("##child", {320, 0}, false, ImGuiWindowFlags_NoScrollbar))
	{
		auto &sharedConfig = getConfigShared(currentCategory, currentItem);

		ImGui::Checkbox("Enabled", &sharedConfig.enabled);
		ImGui::SameLine(ImGui::GetContentRegionMax().x - 200);
		ImGui::SetNextItemWidth(200);
		if (ImGui::BeginCombo("##font", config->getSystemFonts()[sharedConfig.font.index].c_str()))
		{
			for (size_t i = 0; i < config->getSystemFonts().size(); ++i)
			{
				bool isSelected = config->getSystemFonts()[i] == sharedConfig.font.name;
				if (ImGui::Selectable(config->getSystemFonts()[i].c_str(), isSelected, 0, {250.0f, 0.0f}))
				{
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

		constexpr auto spacing = 170;
		ImGuiCustom::colorPicker("Tracer", sharedConfig.snapline);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(80);
		ImGui::Combo("##snapline", &sharedConfig.snapline.type, "Bottom\0Top\0Crosshair\0");
		ImGui::SameLine(spacing);
		ImGuiCustom::colorPicker("Box", sharedConfig.box);
		if (ImGuiCustom::arrowButtonPopup("box"))
		{
			boxPopup("##box", sharedConfig.box);
			ImGui::EndPopup();
		}

		ImGuiCustom::colorPicker("Name", sharedConfig.name);
		ImGui::SameLine(spacing);

		if (currentCategory < 2)
		{
			auto &playerConfig = getConfigPlayer(currentCategory, currentItem);

			ImGuiCustom::colorPicker("Weapon", playerConfig.weapon);
			ImGuiCustom::colorPicker("Flash duration", playerConfig.flashDuration);
			ImGui::SameLine(spacing);
			ImGuiCustom::colorPicker("Skeleton", playerConfig.skeleton);
			ImGui::Checkbox("Audible", &playerConfig.audibleOnly);
			ImGui::SameLine(spacing);
			ImGui::Checkbox("Spotted", &playerConfig.spottedOnly);

			ImGuiCustom::colorPicker("Head box", playerConfig.headBox);
			if (ImGuiCustom::arrowButtonPopup("head_box"))
			{
				boxPopup("##head_box", playerConfig.headBox);
				ImGui::EndPopup();
			}

			ImGui::SameLine(spacing);
			ImGuiCustom::colorPicker("Health", playerConfig.health);
			ImGuiCustom::colorPicker("Health bar", playerConfig.healthBar);
			ImGui::SameLine(spacing);
			ImGuiCustom::colorPicker("Flags", playerConfig.flags);
			ImGuiCustom::colorPicker("Offscreen", playerConfig.offscreen);
			ImGui::SameLine(spacing);
			ImGuiCustom::colorPicker("Line of sight", playerConfig.lineOfSight);
		} else if (currentCategory == 2)
		{
			auto &weaponConfig = config->esp.weapons[currentItem];
			ImGuiCustom::colorPicker("Ammo", weaponConfig.ammo);
		} else if (currentCategory == 3)
		{
			auto &trails = config->esp.projectiles[currentItem].trails;

			ImGui::Checkbox("Trails", &trails.enabled);
			if (ImGuiCustom::arrowButtonPopup("esp_trails"))
			{
				constexpr auto trailPicker = [](const char *name, Trail &trail) noexcept
				{
					ImGui::PushID(name);
					ImGuiCustom::colorPicker(name, trail);
					ImGui::SameLine(150.0f);
					ImGui::SetNextItemWidth(95.0f);
					ImGui::Combo("", &trail.type, "Line\0Circles\0Filled circles\0");
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

		ImGui::SetNextItemWidth(80);
		ImGui::InputFloat("Text cull", &sharedConfig.textCullDistance, 1.0f, 10.0f, "%.0fu");
	}

	ImGui::EndChild();

	if (!contentOnly)
		ImGui::End();
}

void GUI::renderVisualsWindow(bool contentOnly) noexcept
{
	if (!contentOnly)
	{
		if (!window.visuals)
			return;
		ImGui::SetNextWindowContentSize({480, 0});
		ImGui::Begin("Visuals", &window.visuals, windowFlags);
	}

	ImGui::Columns(2, nullptr, false);

	ImGui::Checkbox("Disable post-processing", &config->visuals.disablePostProcessing);
	ImGui::Checkbox("Inverse ragdoll gravity", &config->visuals.inverseRagdollGravity);

	constexpr auto spacing = 120;
	ImGui::Checkbox("No fog", &config->visuals.noFog);
	ImGui::SameLine(spacing);
	ImGui::Checkbox("No 3d sky", &config->visuals.no3dSky);
	ImGui::Checkbox("No aim punch", &config->visuals.noAimPunch);
	ImGui::SameLine(spacing);
	ImGui::Checkbox("No view punch", &config->visuals.noViewPunch);
	ImGui::Checkbox("No hands", &config->visuals.noHands);
	ImGui::SameLine(spacing);
	ImGui::Checkbox("No sleeves", &config->visuals.noSleeves);
	ImGui::Checkbox("No weapons", &config->visuals.noWeapons);
	ImGui::SameLine(spacing);
	ImGui::Checkbox("No blur", &config->visuals.noBlur);
	ImGui::Checkbox("No shadows", &config->visuals.noShadows);
	ImGui::SameLine(spacing);
	ImGui::Checkbox("No grass", &config->visuals.noGrass);
	ImGui::Checkbox("No scope overlay", &config->visuals.noScopeOverlay);

	ImGui::PushItemWidth(100);
	ImGui::Combo("Smoke", &config->visuals.smoke, "Normal\0Disable\0Wireframe\0");
	ImGui::Combo("Molotov fire", &config->visuals.inferno, "Normal\0Disable\0Wireframe\0");
	ImGui::Combo("Crosshair", &config->visuals.forceCrosshair, "Normal\0Force\0Disable\0");
	ImGuiCustom::colorPicker("##oxhair", config->visuals.overlayCrosshair);
	ImGui::SameLine();
	ImGui::Combo("Overlay crosshair", &config->visuals.overlayCrosshairType, "None\0Circle dot\0Dot\0Cross\0Empty cross\0");
	ImGuiCustom::colorPicker("##rxhair", config->visuals.recoilCrosshair);
	ImGui::SameLine();
	ImGui::Combo("Recoil crosshair", &config->visuals.recoilCrosshairType, "None\0Circle dot\0Dot\0Cross\0Empty cross\0");
	ImGui::PopItemWidth();

	ImGuiCustom::colorPicker("Accuracy circle", config->visuals.accuracyCircle);
	ImGuiCustom::colorPicker("Molotov radius", config->visuals.molotovHull);
	ImGui::SameLine(130);
	ImGuiCustom::colorPicker("Smoke radius", config->visuals.smokeHull);
	ImGuiCustom::colorPicker("Player bounds", config->visuals.playerBounds);
	//ImGui::SameLine(130);
	ImGuiCustom::colorPicker("Player velocity", config->visuals.playerVelocity);

	constexpr auto beamPopup = [](Config::Visuals::Beams &config) noexcept
	{
		ImGui::SetNextItemWidth(100.0f);
		ImGui::Combo("Sprite", &config.sprite, "Phys beam\0Solid\0Laser\0Laser beam\0");
		ImGui::PushItemWidth(255.0f);
		ImGui::SliderFloat("##width", &config.width, 0.0f, 5.0f, "Thickness %.3f");
		ImGui::SliderFloat("##life", &config.life, 0.0f, 10.0f, "Duration %.3fs");
		ImGui::PopItemWidth();
		ImGui::SetNextItemWidth(85.0f);
		ImGui::Combo("Type", &config.type, "Line\0Noise\0Spiral");
		ImGui::SetNextItemWidth(255.0f);
		switch (config.type)
		{
		case 1:
			ImGui::SliderFloat("##amplitude", &config.amplitude, 0.0f, 10.0f, "Noise %.3f");
			ImGui::Checkbox("Do noise once", &config.noiseOnce);
			break;
		case 2:
			ImGui::SliderFloat("##amplitude", &config.amplitude, 0.0f, 10.0f, "Radius %.3f");
			break;
		default:
			break;
		}
	};

	ImGuiCustom::colorPicker("Own beams", config->visuals.selfBeams.color.data(), &config->visuals.selfBeams.color[3], nullptr, nullptr, &config->visuals.selfBeams.enabled);
	if (ImGuiCustom::arrowButtonPopup("beams_own"))
	{
		beamPopup(config->visuals.selfBeams);
		ImGui::EndPopup();
	}

	ImGuiCustom::colorPicker("Ally beams", config->visuals.allyBeams.color.data(), &config->visuals.allyBeams.color[3], nullptr, nullptr, &config->visuals.allyBeams.enabled);
	if (ImGuiCustom::arrowButtonPopup("beams_ally"))
	{
		beamPopup(config->visuals.allyBeams);
		ImGui::EndPopup();
	}

	ImGuiCustom::colorPicker("Enemy beams", config->visuals.enemyBeams.color.data(), &config->visuals.enemyBeams.color[3], nullptr, nullptr, &config->visuals.enemyBeams.enabled);
	if (ImGuiCustom::arrowButtonPopup("beams_enemy"))
	{
		beamPopup(config->visuals.enemyBeams);
		ImGui::EndPopup();
	}

	ImGui::PushItemWidth(100);
	ImGui::Combo("Bullet impacts", &config->visuals.bulletImpacts, "None\0All\0Client\0Server\0");
	ImGui::Combo("Accuracy tracers", &config->visuals.accuracyTracers, "None\0Hover\0Contact\0");
	ImGui::PopItemWidth();

	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);

	ImGuiCustom::keyBind("Zoom", config->visuals.zoom);
	ImGui::SliderInt("##zoom", &config->visuals.zoomFac, 0, 99, "Zoom factor %d%%");

	ImGuiCustom::keyBind("Thirdperson", config->visuals.thirdPerson);
	if (ImGuiCustom::arrowButtonPopup("thirdperson"))
	{
		ImGui::SliderInt("##distance", &config->visuals.thirdpersonDistance, 0, 500, "Distance %du");
		ImGui::Checkbox("Camera collision", &config->visuals.thirdpersonCollision);
		ImGui::EndPopup();
	}

	ImGuiCustom::keyBind("Flashlight", config->visuals.flashlight);
	if (ImGuiCustom::arrowButtonPopup("flashlight"))
	{
		ImGui::SliderFloat("##bright", &config->visuals.flashlightBrightness, 0.0f, 3.0f, "Brightness %.3f");
		ImGui::SliderInt("##distance", &config->visuals.flashlightDistance, 0, 1000, "Distance %du");
		ImGui::SliderInt("##fov", &config->visuals.flashlightFov, 1, 170, "FOV %ddeg");
		ImGui::EndPopup();
	}

	ImGui::SliderInt("##fov", &config->visuals.fov, 1, 170, "FOV %ddeg");
	ImGui::Checkbox("Maintain FOV when scoped", &config->visuals.forceFov);
	ImGui::SliderInt("##far_z", &config->visuals.farZ, 0, 2500, "Far Z %d");
	ImGui::SliderInt("##flash_red", &config->visuals.flashReduction, 0, 100, "Flash reduction %d%%");
	ImGui::SliderFloat("##brightness", &config->visuals.brightness, 0.0f, 1.0f, "Brightness %.2f");

	ImGuiCustom::colorPicker("World color", config->visuals.world);
	ImGuiCustom::colorPicker("Props color", config->visuals.props);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("About 34%% performance drop");
	ImGuiCustom::colorPicker("Sky color", config->visuals.sky);
	
	ImGui::Checkbox("Opposite hand knife", &config->visuals.oppositeHandKnife);
	ImGui::Checkbox("Deagle spinner", &config->visuals.deagleSpinner);
	ImGui::PushItemWidth(130);
	ImGui::Combo("Skybox", &config->visuals.skybox, Helpers::skyboxList.data(), Helpers::skyboxList.size(), 20);
	ImGui::Combo("Screen effect", &config->visuals.screenEffect, "None\0Drone cam\0Noisy drone\0Underwater\0Healthboost\0Dangerzone\0");

	ImGui::Combo("Hit effect", &config->visuals.hitEffect, "None\0Drone cam\0Noisy drone\0Underwater\0Healthboost\0Dangerzone\0");
	if (ImGuiCustom::arrowButtonPopup("hit_effect"))
	{
		ImGui::SliderFloat("##time", &config->visuals.hitEffectTime, 0.1f, 1.5f, "Time %.2fs");
		ImGui::EndPopup();
	}

	ImGui::Combo("Kill effect", &config->visuals.killEffect, "None\0Drone cam\0Noisy drone\0Underwater\0Healthboost\0Dangerzone\0");
	if (ImGuiCustom::arrowButtonPopup("kill_effect"))
	{
		ImGui::SliderFloat("##time", &config->visuals.killEffectTime, 0.1f, 1.5f, "Time %.2fs");
		ImGui::EndPopup();
	}

	ImGui::Combo("Hit marker", &config->visuals.hitMarker, "None\0Cross\0Circle\0");
	if (ImGuiCustom::arrowButtonPopup("hit_marker"))
	{
		ImGui::SliderFloat("##time", &config->visuals.hitMarkerTime, 0.1f, 1.5f, "Time %.2fs");
		ImGui::EndPopup();
	}

	ImGui::PopItemWidth();
	ImGui::SliderFloat("##aspect_ratio", &config->visuals.aspectratio, 0.0f, 5.0f, "Aspect ratio %.2f");

	ImGui::Checkbox("Post processing", &config->visuals.colorCorrection.enabled);
	if (ImGuiCustom::arrowButtonPopup("post"))
	{
		ImGui::VSliderFloat("##1", {40.0f, 160.0f}, &config->visuals.colorCorrection.blue, 0.0f, 1.0f, "Blue\n%.3f"); ImGui::SameLine();
		ImGui::VSliderFloat("##2", {40.0f, 160.0f}, &config->visuals.colorCorrection.red, 0.0f, 1.0f, "Red\n%.3f"); ImGui::SameLine();
		ImGui::VSliderFloat("##3", {40.0f, 160.0f}, &config->visuals.colorCorrection.mono, 0.0f, 1.0f, "Mono\n%.3f"); ImGui::SameLine();
		ImGui::VSliderFloat("##4", {40.0f, 160.0f}, &config->visuals.colorCorrection.saturation, 0.0f, 1.0f, "Sat\n%.3f"); ImGui::SameLine();
		ImGui::VSliderFloat("##5", {40.0f, 160.0f}, &config->visuals.colorCorrection.ghost, 0.0f, 1.0f, "Ghost\n%.3f"); ImGui::SameLine();
		ImGui::VSliderFloat("##6", {40.0f, 160.0f}, &config->visuals.colorCorrection.green, 0.0f, 1.0f, "Green\n%.3f"); ImGui::SameLine();
		ImGui::VSliderFloat("##7", {40.0f, 160.0f}, &config->visuals.colorCorrection.yellow, 0.0f, 1.0f, "Yellow\n%.3f"); ImGui::SameLine();
		ImGui::EndPopup();
	}

	ImGui::Checkbox("Viewmodel", &config->visuals.viewmodel.enabled);
	if (ImGuiCustom::arrowButtonPopup("viewmodel"))
	{
		ImGui::PushItemWidth(290.0f);
		ImGui::SliderFloat("##x", &config->visuals.viewmodel.x, -20.0f, 20.0f, "X %.3f");
		ImGui::SliderFloat("##y", &config->visuals.viewmodel.y, -20.0f, 20.0f, "Y %.3f");
		ImGui::SliderFloat("##z", &config->visuals.viewmodel.z, -20.0f, 20.0f, "Z %.3f");
		ImGui::SliderInt("##fov", &config->visuals.viewmodel.fov, -60, 60, "FOV %d");
		ImGui::SliderFloat("##roll", &config->visuals.viewmodel.roll, -90.0f, 90.0f, "Roll %.1f");
		ImGui::PopItemWidth();
		ImGui::EndPopup();
	}

	ImGui::PopItemWidth();

	if (!contentOnly)
		ImGui::End();
}

void GUI::renderSkinChangerWindow(bool contentOnly) noexcept
{
	if (!contentOnly)
	{
		if (!window.skinChanger)
			return;
		ImGui::Begin("Skin changer", &window.skinChanger, windowFlags);
	}

	static auto itemIndex = 0;
	static auto lastItemIndex = itemIndex;

	if (ImGui::BeginListBox("##1", {100, 400}))
	{
		for (std::size_t i = 0; i < SkinChanger::weapon_names.size(); ++i)
		{
			if (ImGui::Selectable(SkinChanger::weapon_names[i].name, i == itemIndex))
				itemIndex = i;
			#ifdef NEPS_DEBUG
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Item #%i", SkinChanger::weapon_names[i].definition_index);
			#endif // NEPS_DEBUG
		}
		ImGui::EndListBox();
	}

	auto &selected_entry = config->skinChanger[itemIndex];
	selected_entry.itemIdIndex = itemIndex;

	constexpr auto rarityColor = [](int rarity)
	{
		constexpr auto rarityColors = std::to_array<ImU32>({
			IM_COL32(152, 152, 152, 255),
			IM_COL32(176, 195, 217, 255),
			IM_COL32(94, 152, 217, 255),
			IM_COL32(75, 105, 255, 255),
			IM_COL32(136, 71, 255, 255),
			IM_COL32(211, 44, 230, 255),
			IM_COL32(235, 75, 75, 255),
			IM_COL32(228, 174, 57, 255)
		});
		return rarityColors[static_cast<size_t>(rarity) < rarityColors.size() ? rarity : 0];
	};

	constexpr auto passesFilter = [](const std::wstring &str, std::wstring filter) noexcept
	{
		constexpr auto delimiter = L" ";
		wchar_t *_;
		wchar_t *token = std::wcstok(filter.data(), delimiter, &_);
		while (token)
		{
			if (!std::wcsstr(str.c_str(), token))
				return false;
			token = std::wcstok(nullptr, delimiter, &_);
		}
		return true;
	};

	ImGui::SameLine();

	if (ImGui::BeginChild("##settings", {520, 0}, false, ImGuiWindowFlags_NoScrollbar))
	{
		ImGui::Checkbox("Enabled", &selected_entry.enabled);
		ImGui::Separator();

		ImGui::Columns(2, nullptr, false);

		ImGui::PushItemWidth(-1);
		{
			ImGui::SetNextItemWidth(50);
			ImGui::InputInt("Seed", &selected_entry.seed, 0);
			ImGui::SameLine(90);
			ImGui::SetNextItemWidth(100);
			ImGui::InputInt("StatTrak\u2122", &selected_entry.stat_trak);
			selected_entry.stat_trak = (std::max)(selected_entry.stat_trak, -1);
			ImGui::SliderFloat("##wear", &selected_entry.wear, FLT_MIN, 1.f, "Wear %.10f");

			static const auto &skins = SkinChanger::getSkinKits();
			static const auto &gloves = SkinChanger::getGloveKits();
			const auto &kits = itemIndex == 1 ? gloves : skins;

			static std::array<std::string, SkinChanger::weapon_names.size()> filters;
			auto &filter = filters[itemIndex];
			ImGui::InputTextWithHint("##search", "Search", &filter);

			if (ImGui::BeginListBox("##kit_sel", {0.0f, ImGui::GetTextLineHeightWithSpacing() * 10.0f + ImGui::GetStyle().FramePadding.y - 1.0f}))
			{
				const std::wstring filterWide = Helpers::toUpper(Helpers::toWideString(filter));
				for (std::size_t i = 0; i < kits.size(); ++i)
				{
					if (filter.empty() || passesFilter(kits[i].nameUpperCase, filterWide))
					{
						ImGui::PushID(i);
						const auto selected = i == selected_entry.paint_kit_vector_index;
						ImGui::PushStyleColor(ImGuiCol_Text, rarityColor(kits[i].rarity));
						if (ImGui::Selectable(kits[i].name.c_str(), selected))
							selected_entry.paint_kit_vector_index = i;
						if (selected && lastItemIndex != itemIndex)
							ImGui::SetScrollHereY();
						#ifdef NEPS_DEBUG
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip("Paint kit #%i", kits[i].id);
						#endif // NEPS_DEBUG
						ImGui::PopStyleColor();
						ImGui::PopID();
					}
				}
				ImGui::EndListBox();
			}

			ImGui::PushItemWidth(170);
			ImGui::Combo("Quality", &selected_entry.entity_quality_vector_index, [](void *data, int idx, const char **out_text)
			{
				*out_text = SkinChanger::getQualities()[idx].name.c_str(); // Safe within this lamba
				return true;
			}, nullptr, SkinChanger::getQualities().size(), 10);

			if (itemIndex == 0)
			{
				ImGui::Combo("Knife", &selected_entry.definition_override_vector_index, [](void *data, int idx, const char **out_text)
				{
					*out_text = SkinChanger::getKnifeTypes()[idx].name.c_str();
					return true;
				}, nullptr, SkinChanger::getKnifeTypes().size(), 15);
			} else if (itemIndex == 1)
			{
				ImGui::Combo("Gloves", &selected_entry.definition_override_vector_index, [](void *data, int idx, const char **out_text)
				{
					*out_text = SkinChanger::getGloveTypes()[idx].name.c_str();
					return true;
				}, nullptr, SkinChanger::getGloveTypes().size(), 15);
			} else
			{
				selected_entry.definition_override_vector_index = 0;
			}

			ImGui::SetNextItemWidth(-1);
			ImGui::InputTextWithHint("##nametag", "Name tag", selected_entry.custom_name, 32);

			{
				constexpr auto playerModels = "Default\0Special Agent Ava | FBI\0Operator | FBI SWAT\0Markus Delrow | FBI HRT\0Michael Syfers | FBI Sniper\0B Squadron Officer | SAS\0Seal Team 6 Soldier | NSWC SEAL\0Buckshot | NSWC SEAL\0Lt. Commander Ricksaw | NSWC SEAL\0Third Commando Company | KSK\0'Two Times' McCoy | USAF TACP\0Dragomir | Sabre\0Rezan The Ready | Sabre\0'The Doctor' Romanov | Sabre\0Maximus | Sabre\0Blackwolf | Sabre\0The Elite Mr. Muhlik | Elite Crew\0Ground Rebel | Elite Crew\0Osiris | Elite Crew\0Prof. Shahmat | Elite Crew\0Enforcer | Phoenix\0Slingshot | Phoenix\0Soldier | Phoenix\0Pirate\0Pirate Variant A\0Pirate Variant B\0Pirate Variant C\0Pirate Variant D\0Anarchist\0Anarchist Variant A\0Anarchist Variant B\0Anarchist Variant C\0Anarchist Variant D\0Balkan Variant A\0Balkan Variant B\0Balkan Variant C\0Balkan Variant D\0Balkan Variant E\0Jumpsuit Variant A\0Jumpsuit Variant B\0Jumpsuit Variant C\0Street Soldier | Phoenix\0'Blueberries' Buckshot | NSWC SEAL\0'Two Times' McCoy | TACP Cavalry\0Rezan the Redshirt | Sabre\0Dragomir | Sabre Footsoldier\0Cmdr. Mae 'Dead Cold' Jamison | SWAT\0 1st Lieutenant Farlow | SWAT\0John 'Van Healen' Kask | SWAT\0Bio-Haz Specialist | SWAT\0Sergeant Bombson | SWAT\0Chem-Haz Specialist | SWAT\0Sir Bloody Miami Darryl | The Professionals\0Sir Bloody Silent Darryl | The Professionals\0Sir Bloody Skullhead Darryl | The Professionals\0Sir Bloody Darryl Royale | The Professionals\0Sir Bloody Loudmouth Darryl | The Professionals\0Safecracker Voltzmann | The Professionals\0Little Kev | The Professionals\0Number K | The Professionals\0Getaway Sally | The Professionals\0";

				//ImGui::Combo("T player model", &config->visuals.playerModelT, playerModels, 20);
				//ImGui::Combo("CT player model", &config->visuals.playerModelCT, playerModels, 20);
			}

			ImGui::PopItemWidth();
		}
		ImGui::PopItemWidth();

		ImGui::NextColumn();

		ImGui::PushItemWidth(-1);
		{
			ImGui::PushID("sticker");

			static const auto &kits = SkinChanger::getStickerKits();
			static std::size_t selectedStickerSlot = 0;
			static std::size_t lastSelectedStickerSlot = selectedStickerSlot;

			if (ImGui::BeginListBox("##stickers", {0.0f, ImGui::GetTextLineHeightWithSpacing() * 5.0f + ImGui::GetStyle().FramePadding.y - 1.0f}))
			{
				for (int i = 0; i < 5; ++i)
				{
					ImGui::PushID(i);
					const auto kit_vector_index = config->skinChanger[itemIndex].stickers[i].kit_vector_index;
					std::ostringstream ss;
					ss << '#' << i + 1 << ' ' << kits[kit_vector_index].name;
					if (ImGui::Selectable(ss.str().c_str(), i == selectedStickerSlot))
						selectedStickerSlot = i;
					ImGui::PopID();
				}
				ImGui::EndListBox();
			}

			auto &selected_sticker = selected_entry.stickers[selectedStickerSlot];

			static std::array<std::string, SkinChanger::weapon_names.size()> filters;
			auto &filter = filters[itemIndex];
			ImGui::InputTextWithHint("##search", "Search", &filter);

			if (ImGui::BeginListBox("##sticker_sel", {0.0f, ImGui::GetTextLineHeightWithSpacing() * 10.0f + ImGui::GetStyle().FramePadding.y - 1.0f}))
			{
				const std::wstring filterWide = Helpers::toUpper(Helpers::toWideString(filter));
				for (std::size_t i = 0; i < kits.size(); ++i)
				{
					if (filter.empty() || passesFilter(kits[i].nameUpperCase, filterWide))
					{
						ImGui::PushID(i);
						const auto selected = i == selected_sticker.kit_vector_index;
						ImGui::PushStyleColor(ImGuiCol_Text, rarityColor(kits[i].rarity));
						if (ImGui::Selectable(kits[i].name.c_str(), selected))
							selected_sticker.kit_vector_index = i;
						if (selected && (lastItemIndex != itemIndex || lastSelectedStickerSlot != selectedStickerSlot))
							ImGui::SetScrollHereY();
						#ifdef NEPS_DEBUG
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip("Sticker #%i", kits[i].id);
						#endif // NEPS_DEBUG
						ImGui::PopStyleColor();
						ImGui::PopID();
					}
				}
				ImGui::EndListBox();
			}

			ImGui::SliderFloat("##wear", &selected_sticker.wear, FLT_MIN, 1.0f, "Wear %.10f");
			ImGui::SliderFloat("##scale", &selected_sticker.scale, 0.1f, 5.0f, "Scale %.3f");
			ImGui::SliderFloat("##rotation", &selected_sticker.rotation, 0.0f, 360.0f, "Rotation %.3fdeg");

			ImGui::PopID();

			lastSelectedStickerSlot = selectedStickerSlot;
		}
		ImGui::PopItemWidth();
		selected_entry.update();

		ImGui::Columns(1);

		ImGui::Separator();

		if (ImGui::Button("Update", {130, 25}))
			SkinChanger::scheduleHudUpdate();
	}
	ImGui::EndChild();

	lastItemIndex = itemIndex;

	if (!contentOnly)
		ImGui::End();
}

void GUI::renderSoundWindow(bool contentOnly) noexcept
{
	if (!contentOnly)
	{
		if (!window.sound)
			return;
		ImGui::Begin("Sound", &window.sound, windowFlags);
	}

	constexpr auto soundUi = [](const char *label, int &sound, std::string &path) noexcept
	{
		ImGui::PushID(label);
		ImGui::PushItemWidth(110);

		ImGui::Combo(label, &sound, "None\0Metal\0Switch press\0Bell\0Glass\0Custom\0");
		if (sound == 5)
		{
			ImGui::InputTextWithHint("##path", "Filename", &path);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Audio file must be put in csgo/sound/ directory");
		}
		
		ImGui::PopItemWidth();
		ImGui::PopID();
	};

	soundUi("Hit sound", config->sound.hitSound, config->sound.customHitSound);
	soundUi("Kill sound", config->sound.killSound, config->sound.customKillSound);
	soundUi("Death sound", config->sound.deathSound, config->sound.customDeathSound);

	ImGui::PushItemWidth(200);
	ImGui::SliderInt("##chicken", &config->sound.chickenVolume, 0, 200, "Chicken volume %d%%");

	ImGui::Separator();

	static int currentCategory = 0;
	ImGui::SetNextItemWidth(110);
	ImGui::Combo("##whose", &currentCategory, "Local player\0Allies\0Enemies\0");
	ImGui::SliderInt("##master", &config->sound.players[currentCategory].masterVolume, 0, 200, "Master volume %d%%");
	ImGui::SliderInt("##headshot", &config->sound.players[currentCategory].headshotVolume, 0, 200, "Headshot volume %d%%");
	ImGui::SliderInt("##weapon", &config->sound.players[currentCategory].weaponVolume, 0, 200, "Weapon volume %d%%");
	ImGui::SliderInt("##footstep", &config->sound.players[currentCategory].footstepVolume, 0, 200, "Footstep volume %d%%");
	ImGui::PopItemWidth();

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
	ImGuiCustom::keyBind("Fake duck", config->exploits.fakeDuck);
	ImGui::SetNextItemWidth(-1);
	ImGui::InputInt("##duck_packets", &config->exploits.fakeDuckPackets, 1, 5);
	config->exploits.fakeDuckPackets = std::max(config->exploits.fakeDuckPackets, 0);
	ImGui::Checkbox("Moonwalk", &config->exploits.moonwalk);
	ImGuiCustom::keyBind("Slowwalk", config->exploits.slowwalk);

	ImGui::Checkbox("Bypass sv_pure", &config->exploits.bypassPure);

	if (!contentOnly)
		ImGui::End();
}

void GUI::renderGriefingWindow(bool contentOnly) noexcept
{
	if (!contentOnly)
	{
		if (!window.griefing)
			return;
		ImGui::Begin("Griefing", &window.griefing, windowFlags);
	}

	static std::string playerName;
	ImGui::SetNextItemWidth(192.0f);
	ImGui::InputText("##player_name", &playerName);
	if (ImGui::Button("Change name", {-1, 0}))
		Misc::changeName(false, (playerName + "\x1").c_str(), 5.0f);

	ImGui::SetNextItemWidth(192.0f);
	ImGui::InputText("##ban", &config->griefing.banText);
	ImGui::SetNextItemWidth(112.0f);
	ImGui::Combo("##ban_color", &config->griefing.banColor, "White\0Red\0Light Canary\0Green\0Light Green\0Lime\0Rose\0Light Gray\0Yellow\0??? (broken)\0Light Blue\0Blue\0Cold Gray\0Magenta\0Fire Orange\0Canary\0");
	ImGui::SameLine();
	if (ImGui::Button("Fake ban", {-1, 0}))
		Misc::fakeBan();

	ImGui::Checkbox("Vote reveal", &config->griefing.revealVotes);
	ImGui::SameLine(90.0f);
	ImGui::Checkbox("Name stealer", &config->griefing.nameStealer);

	ImGui::Checkbox("Clock tag", &config->griefing.clocktag);
	ImGui::SameLine(90.0f);
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
	if (ImGuiCustom::arrowButtonPopup("reportbot"))
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

	ImGuiCustom::keyBind("Blockbot", config->griefing.blockbot.bind);
	if (ImGuiCustom::arrowButtonPopup("blockbot"))
	{
		ImGui::PushItemWidth(192.0f);
		ImGui::SliderFloat("##tfactor", &config->griefing.blockbot.trajectoryFac, 0.0f, 4.0f, "Trajectory factor %.3f");
		ImGui::SliderFloat("##dfactor", &config->griefing.blockbot.distanceFac, 0.0f, 4.0f, "Distance factor %.3f");
		ImGui::PopItemWidth();
		ImGuiCustom::colorPicker("Visualize target", config->griefing.blockbot.visualize);
		ImGui::EndPopup();
	}

	ImGui::Checkbox("Spam use", &config->griefing.spamUse);
	ImGui::SameLine(80.0f);
	ImGui::Checkbox("Fake Prime status", &config->griefing.fakePrime);

	ImGuiCustom::keyBind("Basmala chat", config->griefing.chatBasmala);
	ImGui::SameLine(125.0f);
	if (ImGui::Button("Test##basmala", {-1, 0}))
		Misc::runChatSpammer(2);

	ImGuiCustom::keyBind("Nuke chat", config->griefing.chatNuke);
	ImGui::SameLine(125.0f);
	if (ImGui::Button("Test##nuke", {-1, 0}))
		Misc::runChatSpammer(1);

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
	ImGui::Checkbox("Autostrafe", &config->movement.autoStrafe);
	ImGuiCustom::keyBind("Edge jump", config->movement.edgeJump);
	ImGui::Checkbox("Fast stop", &config->movement.fastStop);

	ImGuiCustom::keyBind("Auto peek", config->movement.autoPeek.bind);
	if (ImGuiCustom::arrowButtonPopup("auto_peek"))
	{
		ImGui::TextUnformatted("Visualize");
		ImGui::Indent();
		ImGuiCustom::colorPicker("Awaiting shot", config->movement.autoPeek.visualizeIdle);
		ImGuiCustom::colorPicker("Returning", config->movement.autoPeek.visualizeActive);
		ImGui::Unindent();
		ImGui::EndPopup();
	}

	if (!contentOnly)
		ImGui::End();
}

void GUI::renderMiscWindow(bool contentOnly) noexcept
{
	if (!contentOnly)
	{
		if (!window.misc)
			return;
		ImGui::SetNextWindowContentSize({320, 0});
		ImGui::Begin("Misc", &window.misc, windowFlags);
	}

	ImGui::Columns(2, nullptr, false);

	ImGuiCustom::keyBind("Menu key", &config->misc.menuKey);
	if (config->misc.menuKey == 1) config->misc.menuKey = 0;

	ImGui::Checkbox("Full-auto", &config->misc.autoPistol);
	ImGui::SameLine(80);
	ImGui::Checkbox("Fast plant", &config->misc.fastPlant);

	ImGui::Checkbox("Auto reload", &config->misc.autoReload);
	ImGui::Checkbox("Auto accept", &config->misc.autoAccept);
	ImGui::Checkbox("Quick reload", &config->misc.quickReload);
	ImGuiCustom::keyBind("Prepare revolver", config->misc.prepareRevolver);
	ImGuiCustom::keyBind("Quick healthshot", &config->misc.quickHealthshotKey);
	
	ImGui::Checkbox("Fix animation LOD", &config->misc.fixAnimationLOD);
	ImGui::Checkbox("Fix bone matrices", &config->misc.fixBoneMatrices);
	ImGui::Checkbox("Fix movement", &config->misc.fixMovement);
	ImGui::Checkbox("Fix local animations", &config->misc.fixLocalAnimations);
	ImGui::Checkbox("Disable model occlusion", &config->misc.disableModelOcclusion);
	ImGui::Checkbox("Disable extrapolation", &config->misc.noExtrapolate);
	ImGui::Checkbox("Disable IK", &config->misc.disableIK);
	ImGui::Checkbox("Resolve LBY", &config->misc.resolveLby);
	ImGui::Checkbox("NEPSmas (go festive)", &config->misc.goFestive);

	ImGui::NextColumn();

	ImGui::Checkbox("Fix tablet signal", &config->misc.fixTabletSignal);
	ImGui::Checkbox("Radar hack", &config->misc.radarHack);
	ImGui::Checkbox("Unlock inventory", &config->misc.unlockInventory);
	ImGui::Checkbox("Reveal ranks", &config->misc.revealRanks);
	ImGui::Checkbox("Reveal money", &config->misc.revealMoney);
	ImGui::Checkbox("Reveal suspect", &config->misc.revealSuspect);
	ImGui::Checkbox("No panorama blur", &config->misc.disablePanoramablur);
	ImGui::Checkbox("Grenade prediction", &config->misc.nadePredict);
	ImGui::SetNextItemWidth(-1);
	ImGui::SliderFloat("##angle_delta", &config->misc.maxAngleDelta, 0.0f, 255.0f, "Aimstep %.2fdeg");

	ImGui::Checkbox("Preserve killfeed", &config->misc.preserveKillfeed.enabled);
	if (ImGuiCustom::arrowButtonPopup("killfeed"))
	{
		ImGui::Checkbox("Only headshots", &config->misc.preserveKillfeed.onlyHeadshots);
		ImGui::EndPopup();
	}

	ImGui::Checkbox("Purchase list", &config->misc.purchaseList.enabled);
	if (ImGuiCustom::arrowButtonPopup("purchase_list"))
	{
		ImGui::SetNextItemWidth(75);
		ImGui::Combo("Mode", &config->misc.purchaseList.mode, "Details\0Summary\0");
		ImGui::Checkbox("Only during freeze time", &config->misc.purchaseList.onlyDuringFreezeTime);
		ImGui::Checkbox("Show prices", &config->misc.purchaseList.showPrices);
		ImGui::Checkbox("No title bar", &config->misc.purchaseList.noTitleBar);
		ImGui::EndPopup();
	}

	ImGui::Checkbox("Bomb timer", &config->misc.bombTimer.enabled);
	ImGui::Checkbox("Indicators", &config->misc.indicators.enabled);
	ImGui::Checkbox("Spectator list", &config->misc.spectatorList.enabled);
	ImGui::Checkbox("Watermark", &config->misc.watermark.enabled);
	ImGui::Checkbox("Team damage list", &config->misc.teamDamageList.enabled);
	if (ImGuiCustom::arrowButtonPopup("team_dmg_list"))
	{
		ImGui::Checkbox("No title bar", &config->misc.teamDamageList.noTitleBar);
		ImGui::Checkbox("Show ban progress", &config->misc.teamDamageList.progressBars);
		ImGui::EndPopup();
	}

	ImGui::Columns(1);

	ImGui::SetNextItemWidth(140);
	ImGui::Combo("Force region", &config->misc.forceRelayCluster, "Off\0Australia\0Austria\0Brazil\0Chile\0Dubai\0France\0Germany\0Hong Kong\0India (Chennai)\0India (Mumbai)\0Japan\0Luxembourg\0Netherlands\0Peru\0Philipines\0Poland\0Singapore\0South Africa\0Spain\0Sweden\0UK\0USA (Atlanta)\0USA (Seattle)\0USA (Chicago)\0USA (Los Angeles)\0USA (Moses Lake)\0USA (Oklahoma)\0USA (Seattle)\0USA (Washington DC)\0");

	if (!contentOnly)
		ImGui::End();
}

void GUI::renderStyleWindow(bool contentOnly) noexcept
{
	if (!contentOnly)
	{
		if (!window.style)
			return;
		ImGui::Begin("Style", &window.style, windowFlags);
	}

	ImGui::PushItemWidth(100);
	//if (ImGui::Combo("Menu style", &config->style.menuStyle, "Classic\0One window\0"))
	//    window = {};
	if (ImGui::Combo("Menu colors", &config->style.menuColors, "Custom\0NEPS\0Alwayslose\0Aimwhen\0Coca-Cola\0Twotap\0Cherry\0NEPSmas\0"))
		updateColors();
	ImGui::PopItemWidth();
	ImGui::SetNextItemWidth(90);
	ImGui::InputFloat("Font scale", &config->style.scaling, 0.1f, 1.0f, "%.2f");

	if (config->style.menuColors == 0)
	{
		ImGuiStyle &style = ImGui::GetStyle();
		for (int i = 0; i < ImGuiCol_COUNT; ++i)
		{
			if (i && i & 3) ImGui::SameLine(170.0f * (i & 3));

			ImGuiCustom::colorPicker(ImGui::GetStyleColorName(i), &style.Colors[i].x, &style.Colors[i].w);
		}
	}

	if (!contentOnly)
		ImGui::End();
}

void GUI::renderConfigWindow(bool contentOnly) noexcept
{
	if (!contentOnly)
	{
		if (!window.config)
			return;
		ImGui::SetNextWindowContentSize({250, 0});
		ImGui::Begin("Config", &window.config, windowFlags);
	}

	ImGui::Columns(2, nullptr, false);
	ImGui::SetColumnWidth(0, 150);

	static bool incrementalLoad = false;
	ImGui::Checkbox("Incremental load", &incrementalLoad);

	if (ImGui::Button("Reload configs", {-1, 0}))
		config->listConfigs();

	auto &configItems = config->getConfigs();
	static int currentConfig = -1;

	if (static_cast<std::size_t>(currentConfig) >= configItems.size())
		currentConfig = -1;

	static std::string buffer;

	ImGui::PushItemWidth(-1);
	if (ImGui::ListBox("##cfgs", &currentConfig, [](void *data, int idx, const char **out_text)
	{
		auto &vector = *static_cast<std::vector<std::string>*>(data);
		*out_text = vector[idx].c_str();
		return true;
	}, &configItems, configItems.size(), 5) && currentConfig != -1)
		buffer = configItems[currentConfig];

	if (ImGui::InputTextWithHint("##cfg_name", "Config name", &buffer, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		if (currentConfig != -1)
			config->rename(currentConfig, buffer.c_str());
	}
	ImGui::PopItemWidth();

	ImGui::NextColumn();

	static const ImVec2 size = {-1, 20};

	if (ImGui::Button("Open folder", size))
		config->openConfigDir();

	if (ImGui::Button("Create config", size))
		config->add(buffer.c_str());

	if (ImGui::Button("Reset config", size))
		ImGui::OpenPopup("Config to reset");

	if (ImGui::BeginPopup("Config to reset"))
	{
		static constexpr const char *names[]{"Whole", "Aimbot", "Triggerbot", "Backtrack", "Anti-aim", "Chams", "Glow", "ESP", "Visuals", "Skin changer", "Sound", "Griefing", "Exploits", "Movement", "Misc", "Style"};
		for (int i = 0; i < IM_ARRAYSIZE(names); i++)
		{
			if (i == 1) ImGui::Separator();

			if (ImGui::Selectable(names[i]))
			{
				switch (i)
				{
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

	if (currentConfig != -1)
	{
		if (ImGui::Button("Load selected", size))
		{
			config->load(currentConfig, incrementalLoad);
			updateColors();
			SkinChanger::scheduleHudUpdate();
		}

		if (ImGui::Button("Save selected", size))
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

		if (ImGui::Button("Delete selected", size))
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

#ifdef NEPS_DEBUG
void GUI::renderDebugWindow() noexcept
{
	ImGui::Columns(3, nullptr, false);

	{
		if (ImGui::Button("Test chat virtual methods", {-1, 0}))
			memory->clientMode->getHudChat()->printf(0, "\x1N \x2N \x3N \x4N \x5N \x6N \x7N \x8N \x9N \xAN \xBN \xCN \xDN \xEN \xFN \x10N \x1");

		if (ImGui::Button("List client classes", {-1, 0}))
		{
			for (int i = 0; i <= interfaces->entityList->getHighestEntityIndex(); i++)
			{
				auto entity = interfaces->entityList->getEntity(i);
				if (!entity) continue;

				memory->conColorMsg({0, 200, 0, 255}, std::to_string(i).c_str());
				memory->debugMsg(": ");
				memory->conColorMsg({0, 120, 255, 255}, entity->getClientClass()->networkName);
				memory->debugMsg(" -> ");
				memory->conColorMsg({255, 120, 255, 255}, std::to_string((int)entity->getClientClass()->classId).c_str());
				memory->debugMsg("   ");
			}
		}

		static const char *entName;
		static ClassId entClassId;
		static int idx = -1;
		if (ImGui::Button("Select...") && localPlayer)
		{
			const Vector start = localPlayer->getEyePosition();
			const Vector end = start + Vector::fromAngle(interfaces->engine->getViewAngles()) * 1000.0f;

			Trace trace;
			interfaces->engineTrace->traceRay({start, end}, ALL_VISIBLE_CONTENTS | CONTENTS_MOVEABLE | CONTENTS_DETAIL, localPlayer.get(), trace);

			if (trace.entity)
			{
				auto clientClass = trace.entity->getClientClass();
				entName = clientClass->networkName;
				entClassId = clientClass->classId;
				idx = trace.entity->index();
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Select self") && localPlayer)
		{
			auto clientClass = localPlayer->getClientClass();
			entName = clientClass->networkName;
			entClassId = clientClass->classId;
			idx = localPlayer->index();
		}

		Entity *entity = interfaces->entityList->getEntity(idx);
		if (entName)
		{
			ImGui::TextUnformatted("Selected:");
			ImGui::SameLine();
			ImGui::TextColored({1.0f, 1.0f, 0.0f, 1.0f}, "%s", entName);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("In entity list at %i", idx);
			ImGui::SameLine();
			ImGui::TextColored({0.0f, 0.3f, 1.0f, 1.0f}, "%i", entClassId);
		}

		static std::array<float, 3> lightColor = {1.0f, 1.0f, 1.0f};
		static float radius = 120.0f;
		static float life = 20.0f;
		static int exponent = 2;

		ImGuiCustom::colorPicker("Light color", lightColor.data());
		ImGui::PushItemWidth(-1);
		ImGui::SliderFloat("##radius", &radius, 0.0f, 5000.0f, "Light radius %.3f", ImGuiSliderFlags_Logarithmic);
		ImGui::SliderInt("##exponent", &exponent, 0, 12, "Light exponent %d");
		ImGui::SliderFloat("##life", &life, 0.0f, 100.0f, "Light lifetime %.3f");
		ImGui::PopItemWidth();

		static DynamicLight *dlight = nullptr;
		if (entity && entClassId != ClassId::World && ImGui::Button("Allocade d-light for selected entity", {-1, 0}))
		{
			dlight = interfaces->effects->allocDlight(idx);
			if (dlight)
			{
				dlight->outerAngle = 0.0f;
				dlight->flags = 0;
				dlight->decay = 0.0f;
				dlight->die = memory->globalVars->currentTime + life;
				dlight->origin = entity->getAbsOrigin();
				dlight->radius = radius;
				dlight->color.r = static_cast<unsigned char>(lightColor[0] * 255);
				dlight->color.g = static_cast<unsigned char>(lightColor[1] * 255);
				dlight->color.b = static_cast<unsigned char>(lightColor[2] * 255);
				dlight->color.exponent = exponent;
			}
		}

		if (entity && entity->isPlayer())
		{
			if (ImGui::Button("Resolve selected", {-1, 0}))
				Animations::resolveDesync(entity);
		}

		if (ImGui::Button("Precache info", {-1, 0}))
			interfaces->engine->clientCmdUnrestricted("sv_precacheinfo");

		const auto &colors = ImGui::GetStyle().Colors;
		std::ostringstream ss;

		for (int i = 0; i < ImGuiCol_COUNT; ++i)
		{
			ss << "colors[ImGuiCol_" << ImGui::GetStyleColorName(i) << "] = {";
			ss.precision(2);
			ss << std::fixed << colors[i].x;
			ss << "f, ";
			ss << colors[i].y;
			ss << "f, ";
			ss << colors[i].z;
			ss << "f, ";
			ss << colors[i].w;
			ss << "f};\n";
		}

		if (ImGui::Button("Copy style colors", {-1, 0}))
			ImGui::SetClipboardText(ss.str().c_str());
	}

	ImGui::NextColumn();
	
	{
		{
			GameData::Lock lock;

			auto playerResource = *memory->playerResource;

			if (localPlayer && playerResource)
			{
				if (ImGui::BeginTable("shrek", 4))
				{
					ImGui::TableSetupColumn("Name");
					ImGui::TableSetupColumn("Wins");
					ImGui::TableSetupColumn("Level");
					ImGui::TableSetupColumn("Ranking");
					ImGui::TableHeadersRow();

					ImGui::TableNextRow();
					ImGui::PushID(ImGui::TableGetRowIndex());

					if (ImGui::TableNextColumn())
						ImGui::TextUnformatted("Local player");

					if (ImGui::TableNextColumn())
						ImGui::Text("%i", playerResource->competitiveWins()[localPlayer->index()]);

					if (ImGui::TableNextColumn())
						ImGui::Text("%i", playerResource->level()[localPlayer->index()]);

					if (ImGui::TableNextColumn())
						ImGui::Text("%i", playerResource->competitiveRanking()[localPlayer->index()]);

					for (auto &player : GameData::players())
					{
						ImGui::TableNextRow();
						ImGui::PushID(ImGui::TableGetRowIndex());

						auto *entity = interfaces->entityList->getEntityFromHandle(player.handle);
						if (!entity) continue;

						if (ImGui::TableNextColumn())
							ImGui::TextUnformatted(player.name.c_str());

						if (ImGui::TableNextColumn())
							ImGui::Text("%i", playerResource->competitiveWins()[entity->index()]);

						if (ImGui::TableNextColumn())
							ImGui::Text("%i", playerResource->level()[entity->index()]);

						if (ImGui::TableNextColumn())
							ImGui::Text("%i", playerResource->competitiveRanking()[entity->index()]);
					}

					ImGui::EndTable();
				}

				ImGui::InputInt("Wins", &playerResource->competitiveWins()[localPlayer->index()]);
				ImGui::InputInt("Level", &playerResource->level()[localPlayer->index()]);
				ImGui::InputInt("Ranking", &playerResource->competitiveRanking()[localPlayer->index()]);
			}
		}

		ImGui::TextColored({1.0f, 0.8f, 0.0f, 1.0f}, "Local player");
		ImGui::SameLine();
		ImGui::TextUnformatted("at");
		ImGui::SameLine();
		ImGui::TextColored({0.0f, 0.2f, 1.0f, 1.0f}, "0x%p", localPlayer.get());
		ImGui::SameLine();

		char buffer[9];
		sprintf(buffer, "%p", localPlayer.get());
		if (ImGui::Button("Copy"))
			ImGui::SetClipboardText(buffer);
	}

	ImGui::NextColumn();

	{
		if (localPlayer)
		{
			const auto layers = localPlayer->animLayers();

			if (ImGui::BeginTable("shrek2", 5))
			{
				ImGui::TableSetupColumn("Name", 0, 4.0f);
				ImGui::TableSetupColumn("Weight");
				ImGui::TableSetupColumn("Rate");
				ImGui::TableSetupColumn("Seq");
				ImGui::TableSetupColumn("Cycle");
				ImGui::TableHeadersRow();

				for (int i = 0; i < localPlayer->getAnimLayerCount(); ++i)
				{
					ImGui::TableNextRow();
					ImGui::PushID(ImGui::TableGetRowIndex());

					if (ImGui::TableNextColumn())
					{
						switch (i)
						{
						case 0: ImGui::TextUnformatted("AIMMATRIX"); break;
						case 1: ImGui::TextUnformatted("WEAPON_ACTION"); break;
						case 2: ImGui::TextUnformatted("WEAPON_ACTION_RECROUCH"); break;
						case 3: ImGui::TextUnformatted("ADJUST"); break;
						case 4: ImGui::TextUnformatted("MOVEMENT_JUMP_OR_FALL"); break;
						case 5: ImGui::TextUnformatted("MOVEMENT_LAND_OR_CLIMB"); break;
						case 6: ImGui::TextUnformatted("MOVEMENT_MOVE"); break;
						case 7: ImGui::TextUnformatted("MOVEMENT_STRAFECHANGE"); break;
						case 8: ImGui::TextUnformatted("WHOLE_BODY"); break;
						case 9: ImGui::TextUnformatted("FLASHED"); break;
						case 10: ImGui::TextUnformatted("FLINCH"); break;
						case 11: ImGui::TextUnformatted("ALIVELOOP"); break;
						case 12: ImGui::TextUnformatted("LEAN"); break;
						case 13: ImGui::TextUnformatted("???"); break;
						case 14: ImGui::TextUnformatted("???"); break;
						case 15: ImGui::TextUnformatted("???"); break;
						}
					}

					if (ImGui::TableNextColumn())
						ImGui::Text("%.4f", layers[i].weight);

					if (ImGui::TableNextColumn())
						ImGui::Text("%.4f", layers[i].playbackRate);

					if (ImGui::TableNextColumn())
						ImGui::Text("%i", layers[i].sequence);

					if (ImGui::TableNextColumn())
						ImGui::Text("%.4f", layers[i].cycle);
				}

				ImGui::EndTable();
			}
		}

		static std::string soundPath;

		ImGui::SetNextItemWidth(-1);
		ImGui::InputTextWithHint("##snd_path", "Relative sound path", &soundPath);

		if (ImGui::Button("Play sound", {-1, 0}))
			interfaces->surface->playSound(soundPath.c_str());

		if (ImGui::Button("Precache sound", {-1, 0}))
		{
			if (const auto soundprecache = interfaces->networkStringTableContainer->findTable("soundprecache"))
				soundprecache->addString(false, soundPath.c_str());
		}
	}
}
#endif // NEPS_DEBUG
