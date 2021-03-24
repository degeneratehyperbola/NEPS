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
private:
	void updateColors() const noexcept;
	void renderMenuBar() noexcept;
	void debug() noexcept;
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
	void renderStyleWindow(bool contentOnly = false) noexcept;
	void renderExploitsWindow(bool contentOnly = false) noexcept;
	void renderGriefingWindow(bool contentOnly = false) noexcept;
	void renderMovementWindow(bool contentOnly = false) noexcept;
	void renderMiscWindow(bool contentOnly = false) noexcept;
	void renderConfigWindow(bool contentOnly = false) noexcept;
	void renderGuiStyle2() noexcept;

	struct
	{
		bool aimbot = false;
		bool antiAim = false;
		bool triggerbot = false;
		bool backtrack = false;
		bool glow = false;
		bool chams = false;
		bool streamProofESP = false;
		bool visuals = false;
		bool skinChanger = false;
		bool sound = false;
		bool style = false;
		bool exploits = false;
		bool steamapi = false;
		bool movement = false;
		bool misc = false;
		bool config = false;
	} window;

	ImFont *font = nullptr;
};

inline std::unique_ptr<GUI> gui;
