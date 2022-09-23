#pragma once

namespace Debug
{
	void drawGUI() noexcept;
	void drawAdditionalContextMenuItems() noexcept;
	void drawOverlay() noexcept;
}

#ifndef NEPS_DEBUG
#define DEBUG_DRAW_GUI
#define DEBUG_DRAW_ADDITIONAL_CONTEXT_MENU_ITEMS
#define DEBUG_DRAW_OVERLAY
#else
#define DEBUG_DRAW_GUI Debug::drawGUI()
#define DEBUG_DRAW_ADDITIONAL_CONTEXT_MENU_ITEMS Debug::drawAdditionalContextMenuItems()
#define DEBUG_DRAW_OVERLAY Debug::drawOverlay()
#endif
