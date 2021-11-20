#pragma once

#include <memory>
#include <string>

//#define LEGACY_WATERMARK
//#define DEBUG_UI

struct ImFont;

class GUI
{
public:
	GUI() noexcept;
	void render() noexcept;
	bool open = false;
	const auto &getFont() noexcept { return font; };
	void updateColors() const noexcept;
private:
	void renderGuiStyle2() noexcept;
	void renderContextMenu() noexcept;
	void renderMenuBar() noexcept;
	void renderDebugWindow() noexcept;
	void renderAimbotWindow(bool contentOnly = false) noexcept;
	void renderAntiAimWindow(bool contentOnly = false) noexcept;
	void renderTriggerbotWindow(bool contentOnly = false) noexcept;
	void renderBacktrackWindow(bool contentOnly = false) noexcept;
	void renderGlowWindow(bool contentOnly = false) noexcept;
	void renderChamsWindow(bool contentOnly = false) noexcept;
	void renderESPWindow(bool contentOnly = false) noexcept;
	void renderVisualsWindow(bool contentOnly = false) noexcept;
	void renderSkinChangerWindow(bool contentOnly = false) noexcept;
	void renderSoundWindow(bool contentOnly = false) noexcept;
	void renderExploitsWindow(bool contentOnly = false) noexcept;
	void renderGriefingWindow(bool contentOnly = false) noexcept;
	void renderMovementWindow(bool contentOnly = false) noexcept;
	void renderMiscWindow(bool contentOnly = false) noexcept;
	void renderStyleWindow(bool contentOnly = false) noexcept;
	void renderConfigWindow(bool contentOnly = false) noexcept;

	struct
	{
		bool aimbot = false;
		bool triggerbot = false;
		bool backtrack = false;
		bool antiAim = false;
		bool chams = false;
		bool glow = false;
		bool streamProofESP = false;
		bool visuals = false;
		bool skinChanger = false;
		bool sound = false;
		bool griefing = false;
		bool exploits = false;
		bool movement = false;
		bool misc = false;
		bool style = false;
		bool config = false;
	} window;

	ImFont *font = nullptr;
};

inline std::unique_ptr<GUI> gui;
