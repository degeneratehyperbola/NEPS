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
	void renderContextMenu() noexcept;
	void renderMenuBar() noexcept;
	void renderAimbotWindow() noexcept;
	void renderAntiAimWindow() noexcept;
	void renderTriggerbotWindow() noexcept;
	void renderBacktrackWindow() noexcept;
	void renderGlowWindow() noexcept;
	void renderChamsWindow() noexcept;
	void renderESPWindow() noexcept;
	void renderVisualsWindow() noexcept;
	void renderSkinChangerWindow() noexcept;
	void renderSoundWindow() noexcept;
	void renderExploitsWindow() noexcept;
	void renderGriefingWindow() noexcept;
	void renderMovementWindow() noexcept;
	void renderMiscWindow() noexcept;
	void renderStyleWindow() noexcept;
	void renderConfigWindow() noexcept;

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
