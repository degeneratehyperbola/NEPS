#pragma once

#include "../Config.h"
#include <shared_lib/imgui/imgui.h>

namespace ImGuiCustom
{
	void colorPicker(const char *name, float color[3], float *alpha = nullptr, bool *rainbow = nullptr, float *rainbowSpeed = nullptr, bool *enable = nullptr, float *thickness = nullptr, float *rounding = nullptr, bool *outline = nullptr) noexcept;
	void colorPicker(const char *name, Color3Toggle &colorConfig) noexcept;
	void colorPicker(const char *name, Color4 &colorConfig, bool *enable = nullptr, float *thickness = nullptr) noexcept;
	void colorPicker(const char *name, Color4Outline &colorConfig, bool *enable = nullptr, float *thickness = nullptr) noexcept;
	void colorPicker(const char *name, Color4OutlineToggle &colorConfig, bool *enable = nullptr, float *thickness = nullptr) noexcept;
	void colorPicker(const char *name, Color4Toggle &colorConfig) noexcept;
	void colorPicker(const char *name, Color4ToggleRounding &colorConfig) noexcept;
	void colorPicker(const char *name, Color4ToggleThickness &colorConfig) noexcept;
	void colorPicker(const char *name, Color4ToggleThicknessRounding &colorConfig) noexcept;

	void arrowButtonDisabled(const char *id, ImGuiDir dir) noexcept;

	void progressBarFullWidth(float fraction, float height = 0.0f) noexcept;

	void textUnformattedCentered(const char *text) noexcept;

	void keyBind(const char *name, int *key, int *keyMode = nullptr) noexcept;
	void keyBind(const char *name, KeyBind &bind) noexcept;

	template <typename F>
	void multiCombo(const char *name, F &flagValue, const char *items) noexcept
	{
		constexpr auto singleStringGetter = [](void *data, int idx, const char **outText) noexcept
		{
			const char *itemsSeparatedByZeros = (const char *)data;
			int itemsCount = 0;
			const char *p = itemsSeparatedByZeros;
			while (*p)
			{
				if (idx == itemsCount)
					break;
				p += std::strlen(p) + 1;
				itemsCount++;
			}
			if (!*p)
				return false;
			if (outText)
				*outText = p;
			return true;
		};

		int count = 0;
		const char *p = items;
		while (*p)
		{
			p += std::strlen(p) + 1;
			count++;
		}

		const char *preview = "...";
		if (flagValue == (1 << count) - 1)
			preview = "All";
		else if (!flagValue)
			preview = "None";

		void *data = (void *)items;

		if (ImGui::BeginCombo(name, preview))
		{
			for (int i = 0; i < count; i++)
			{
				bool selected = flagValue & (1 << i);

				const char *item;
				singleStringGetter(data, i, &item);

				ImGui::PushID(i);
				ImGui::Selectable(item, &selected, ImGuiSelectableFlags_DontClosePopups);
				ImGui::PopID();

				if (selected)
					flagValue |= (1 << i);
				else
					flagValue &= ~(1 << i);
			}
			ImGui::EndCombo();
		}
	}

	void boolCombo(const char *name, bool &value, const char *items) noexcept;

	bool arrowButtonPopup(const char *id) noexcept;

	void StyleColorsClassic(ImGuiStyle *dst = nullptr) noexcept;
	void StyleColors1(ImGuiStyle *dst = nullptr) noexcept;
	void StyleColors2(ImGuiStyle *dst = nullptr) noexcept;
	void StyleColors3(ImGuiStyle *dst = nullptr) noexcept;
	void StyleColors4(ImGuiStyle *dst = nullptr) noexcept;
	void StyleColors5(ImGuiStyle *dst = nullptr) noexcept;
	void StyleColors6(ImGuiStyle *dst = nullptr) noexcept;

	void StyleSizesRounded(ImGuiStyle *dst = nullptr);

	void drawTriangleFromCenter(ImDrawList *drawList, const ImVec2 &pos, unsigned color = 0x88FFFFFF, bool outline = true) noexcept;
	ImVec2 drawText(ImDrawList *drawList, const char *text, const ImVec2 &pos, Color4 color = {1.0f, 1.0f, 1.0f, 1.0f}, bool outline = true, bool centered = true, bool adjustHeight = true) noexcept;
	ImVec2 drawProgressBar(ImDrawList *drawList, float fraction, const ImVec2 &pos, ImVec2 size, bool vertical = false, bool reverse = false, Color4 color = {1.0f, 1.0f, 1.0f, 1.0f}, bool background = false, bool border = true) noexcept;
}
