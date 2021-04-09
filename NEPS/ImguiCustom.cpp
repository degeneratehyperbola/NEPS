#include "imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_internal.h"

#include "ImguiCustom.h"
#include "Interfaces.h"
#include "SDK/InputSystem.h"

void ImGuiCustom::colorPicker(const char *name, float color[3], float *alpha, bool *rainbow, float *rainbowSpeed, bool *enable, float *thickness, float *rounding, bool *border) noexcept
{
	ImGui::PushID(name);
	if (enable)
	{
		ImGui::Checkbox("##check", enable);
		ImGui::SameLine(0.0f, 5.0f);
	}
	bool openPopup = ImGui::ColorButton("##btn", {color[0], color[1], color[2], alpha ? *alpha : 1.0f}, ImGuiColorEditFlags_AlphaPreviewHalf);
	if (ImGui::BeginDragDropTarget())
	{
		if (alpha)
		{
			if (const auto payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
			{
				std::copy((float *)payload->Data, (float *)payload->Data + 3, color);
				*alpha = 1.0f;
			}
			if (const auto payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
				std::copy((float *)payload->Data, (float *)payload->Data + 4, color);
		} else
		{
			if (const auto payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
				std::copy((float *)payload->Data, (float *)payload->Data + 3, color);
			if (const auto payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
				std::copy((float *)payload->Data, (float *)payload->Data + 3, color);
		}

		ImGui::EndDragDropTarget();
	}
	ImGui::SameLine();

	if (std::strncmp(name, "##", 2))
		ImGui::TextUnformatted(name, std::strstr(name, "##"));

	if (openPopup)
		ImGui::OpenPopup("##popup");

	if (ImGui::BeginPopup("##popup"))
	{
		if (alpha)
		{
			float col[] = {color[0], color[1], color[2], *alpha};
			ImGui::ColorPicker4("##picker", col, ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_Float);
			color[0] = col[0];
			color[1] = col[1];
			color[2] = col[2];
			*alpha = col[3];
		} else
		{
			ImGui::ColorPicker3("##picker", color, ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_Float);
		}

		if (rainbow || rainbowSpeed || thickness || rounding)
		{
			ImGui::SameLine();
			if (ImGui::BeginChild("##child", {86.0f, 0.0f}))
			{
				if (rainbow)
				{
					ImGui::Checkbox("Rainbow", rainbow);
				}
				ImGui::PushItemWidth(85.0f);
				if (rainbowSpeed)
				{
					ImGui::DragFloat("##speed", rainbowSpeed, 0.1f, -100.0f, 100.0f, "Speed %.1f");
				}
				if (rounding)
				{
					ImGui::DragFloat("##rounding", rounding, 0.1f, 0.0f, 100.0f, "Corner %.1f");
					*rounding = std::max(*rounding, 0.0f);
				}
				if (thickness)
				{
					ImGui::DragFloat("##thickness", thickness, 0.1f, 1.0f, 10.0f, "Thick %.2f");
					*thickness = std::max(*thickness, 1.0f);
				}
				if (border)
				{
					ImGui::Checkbox("Outline", border);
				}

				ImGui::PopItemWidth();
			}

			ImGui::EndChild();
		}
		ImGui::EndPopup();
	}
	ImGui::PopID();
}

void ImGuiCustom::colorPicker(const char *name, Color3Toggle &colorConfig) noexcept
{
	colorPicker(name, colorConfig.color.data(), nullptr, &colorConfig.rainbow, &colorConfig.rainbowSpeed, &colorConfig.enabled);
}

void ImGuiCustom::colorPicker(const char *name, Color4 &colorConfig, bool *enable, float *thickness) noexcept
{
	colorPicker(name, colorConfig.color.data(), &colorConfig.color[3], &colorConfig.rainbow, &colorConfig.rainbowSpeed, enable, thickness);
}

void ImGuiCustom::colorPicker(const char *name, Color4Border &colorConfig, bool *enable, float *thickness) noexcept
{
	colorPicker(name, colorConfig.color.data(), &colorConfig.color[3], &colorConfig.rainbow, &colorConfig.rainbowSpeed, enable, thickness, nullptr, &colorConfig.border);
}

void ImGuiCustom::colorPicker(const char *name, Color4BorderToggle &colorConfig, bool *enable, float *thickness) noexcept
{
	colorPicker(name, colorConfig.color.data(), &colorConfig.color[3], &colorConfig.rainbow, &colorConfig.rainbowSpeed, &colorConfig.enabled, thickness, nullptr, &colorConfig.border);
}

void ImGuiCustom::colorPicker(const char *name, Color4Toggle &colorConfig) noexcept
{
	colorPicker(name, colorConfig.color.data(), &colorConfig.color[3], &colorConfig.rainbow, &colorConfig.rainbowSpeed, &colorConfig.enabled);
}

void ImGuiCustom::colorPicker(const char *name, Color4ToggleRounding &colorConfig) noexcept
{
	colorPicker(name, colorConfig.color.data(), &colorConfig.color[3], &colorConfig.rainbow, &colorConfig.rainbowSpeed, &colorConfig.enabled, nullptr, &colorConfig.rounding);
}

void ImGuiCustom::colorPicker(const char *name, Color4ToggleThickness &colorConfig) noexcept
{
	colorPicker(name, colorConfig.color.data(), &colorConfig.color[3], &colorConfig.rainbow, &colorConfig.rainbowSpeed, &colorConfig.enabled, &colorConfig.thickness);
}

void ImGuiCustom::colorPicker(const char *name, Color4ToggleThicknessRounding &colorConfig) noexcept
{
	colorPicker(name, colorConfig.color.data(), &colorConfig.color[3], &colorConfig.rainbow, &colorConfig.rainbowSpeed, &colorConfig.enabled, &colorConfig.thickness, &colorConfig.rounding);
}

void ImGuiCustom::arrowButtonDisabled(const char *id, ImGuiDir dir) noexcept
{
	float sz = ImGui::GetFrameHeight();
	ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
	ImGui::ArrowButtonEx(id, dir, ImVec2{sz, sz}, ImGuiButtonFlags_Disabled);
	ImGui::PopStyleColor();
}

void ImGuiCustom::progressBarFullWidth(float fraction, float height) noexcept
{
	ImGuiWindow *window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext &g = *GImGui;
	const ImGuiStyle &style = g.Style;

	ImVec2 pos = window->DC.CursorPos;
	ImVec2 size = ImGui::CalcItemSize(ImVec2{-1, 0}, ImGui::CalcItemWidth(), std::max(height, style.FrameRounding * 2.0f));
	ImRect bb(pos, pos + size);
	ImGui::ItemSize(size, style.FramePadding.y);
	if (!ImGui::ItemAdd(bb, 0))
		return;

	// Render
	fraction = ImSaturate(fraction);
	ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
	bb.Expand(ImVec2(-style.FrameBorderSize, -style.FrameBorderSize));
	ImGui::RenderRectFilledRangeH(window->DrawList, bb, ImGui::GetColorU32(ImGuiCol_PlotHistogram), 0.0f, fraction, style.FrameRounding);
}

void ImGuiCustom::textUnformattedCentered(const char *text) noexcept
{
	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(text).x) / 2.0f);
	ImGui::TextUnformatted(text);
}

void ImGuiCustom::keyBind(const char *name, int *key, int *keyMode) noexcept
{
	if (!key) return;

	ImGui::PushID(name);
	ImGui::AlignTextToFramePadding();
	if (std::strncmp(name, "##", 2))
		ImGui::TextUnformatted(name);
	ImGui::SameLine();

	if (ImGui::GetActiveID() == ImGui::GetID(name))
	{
		ImGui::Button("...");

		ImGuiIO &io = ImGui::GetIO();
		if (io.KeysDown[VK_ESCAPE])
			ImGui::ClearActiveID();
		else
		{
			for (int i = 0; i < IM_ARRAYSIZE(io.KeysDown); i++)
				if (ImGui::IsKeyPressed(i) && i != config->misc.menuKey)
				{
					*key = i;
					ImGui::ClearActiveID();
				}

			for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)
				if (ImGui::IsMouseClicked(i) && i + (i > 1 ? 2 : 1) != config->misc.menuKey)
				{
					*key = i + (i > 1 ? 2 : 1);
					ImGui::ClearActiveID();
				}
		}
	} else
	{
		if (keyMode)
		{
			if (*keyMode == 1)
			{
				ImGui:: PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_TextDisabled));
				ImGui::ButtonEx("On", {}, ImGuiButtonFlags_Disabled);
				ImGui::PopStyleColor();
			} else if (*keyMode == 0)
			{
				ImGui:: PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_TextDisabled));
				ImGui::ButtonEx("Off", {}, ImGuiButtonFlags_Disabled);
				ImGui::PopStyleColor();
			} else if (*key)
			{
				if (ImGui::Button(interfaces->inputSystem->virtualKeyToString(*key)))
					ImGui::SetActiveID(ImGui::GetID(name), ImGui::GetCurrentWindow());
			} else
			{
				if (ImGui::Button("Bind"))
					ImGui::SetActiveID(ImGui::GetID(name), ImGui::GetCurrentWindow());
			}

			if (ImGui::IsItemHovered() && ImGui::GetIO().MouseClicked[1])
				ImGui::OpenPopup("##mode");

			if (ImGui::BeginPopup("##mode", ImGuiWindowFlags_AlwaysUseWindowPadding))
			{
				//ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, ImGui::GetStyle().WindowPadding.y));
				bool selected = *keyMode == 0;
				ImGui::Selectable("Off", &selected);
				if (selected) *keyMode = 0;
				selected = *keyMode == 1;
				ImGui::Selectable("Always", &selected);
				if (selected) *keyMode = 1;
				selected = *keyMode == 2;
				ImGui::Selectable("Hold", &selected);
				if (selected) *keyMode = 2;
				selected = *keyMode == 3;
				ImGui::Selectable("Toggle", &selected);
				if (selected) *keyMode = 3;
				if (ImGui::Selectable("Reset"))
					*key = 0;
				//ImGui::PopStyleVar();
				ImGui::EndPopup();
			}
		} else
		{
			if (*key)
			{
				if (ImGui::Button(interfaces->inputSystem->virtualKeyToString(*key)))
					ImGui::SetActiveID(ImGui::GetID(name), ImGui::GetCurrentWindow());
			} else
			{
				if (ImGui::Button("Bind"))
					ImGui::SetActiveID(ImGui::GetID(name), ImGui::GetCurrentWindow());
			}

			if (ImGui::IsItemHovered() && ImGui::GetIO().MouseClicked[1])
				*key = 0;
		}
	}
	ImGui::PopID();
}

void ImGuiCustom::keyBind(const char *name, KeyBind &bind) noexcept
{
	keyBind(name, &bind.key, &bind.keyMode);
}

static bool singleStringGetter(void *data, int idx, const char **out_text)
{
	const char *items_separated_by_zeros = (const char *)data;
	int items_count = 0;
	const char *p = items_separated_by_zeros;
	while (*p)
	{
		if (idx == items_count)
			break;
		p += strlen(p) + 1;
		items_count++;
	}
	if (!*p)
		return false;
	if (out_text)
		*out_text = p;
	return true;
}

void ImGuiCustom::multiCombo(const char *name, int &flagValue, const char *items) noexcept
{
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

void ImGuiCustom::boolCombo(const char *name, bool &value, const char *items) noexcept
{
	int count = 0;
	const char *p = items;
	while (*p)
	{
		p += std::strlen(p) + 1;
		count++;
	}

	void *data = (void *)items;

	const char *preview;
	singleStringGetter(data, value, &preview);
	if (ImGui::BeginCombo(name, preview))
	{
		for (int i = 0; i < std::min(count, 2); i++)
		{
			bool selected = i == (int)value;

			const char *item;
			singleStringGetter(data, i, &item);

			ImGui::PushID(i);
			ImGui::Selectable(item, &selected);
			ImGui::PopID();

			if (selected) value = i;
		}
		ImGui::EndCombo();
	}
}

void ImGuiCustom::StyleColorsClassic(ImGuiStyle *dst) noexcept
{
	ImGuiStyle *style = dst ? dst : &ImGui::GetStyle();
	ImVec4 *colors = style->Colors;

	colors[ImGuiCol_Text] = {1.00f, 1.00f, 1.00f, 1.00f};
	colors[ImGuiCol_TextTitle] = {1.00f, 1.00f, 1.00f, 1.00f};
	colors[ImGuiCol_TextDisabled] = {0.49f, 0.49f, 0.49f, 1.00f};
	colors[ImGuiCol_WindowBg] = {0.17f, 0.17f, 0.17f, 0.90f};
	colors[ImGuiCol_ChildBg] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_PopupBg] = {0.13f, 0.13f, 0.13f, 0.90f};
	colors[ImGuiCol_Border] = {1.00f, 1.00f, 1.00f, 0.22f};
	colors[ImGuiCol_BorderShadow] = {0.00f, 0.00f, 0.00f, 0.40f};
	colors[ImGuiCol_FrameBg] = {0.13f, 0.13f, 0.13f, 0.48f};
	colors[ImGuiCol_FrameBgHovered] = {1.00f, 0.52f, 0.00f, 0.22f};
	colors[ImGuiCol_FrameBgActive] = {1.00f, 0.52f, 0.00f, 0.48f};
	colors[ImGuiCol_TitleBg] = {1.00f, 0.44f, 0.00f, 1.00f};
	colors[ImGuiCol_TitleBgActive] = {1.00f, 0.52f, 0.00f, 1.00f};
	colors[ImGuiCol_TitleBgCollapsed] = {0.20f, 0.20f, 0.20f, 0.90f};
	colors[ImGuiCol_MenuBarBg] = {0.20f, 0.20f, 0.20f, 0.90f};
	colors[ImGuiCol_ScrollbarBg] = {0.13f, 0.13f, 0.13f, 0.48f};
	colors[ImGuiCol_ScrollbarGrab] = {1.00f, 0.52f, 0.00f, 0.48f};
	colors[ImGuiCol_ScrollbarGrabHovered] = {1.00f, 0.52f, 0.00f, 0.67f};
	colors[ImGuiCol_ScrollbarGrabActive] = {1.00f, 0.52f, 0.00f, 1.00f};
	colors[ImGuiCol_CheckMark] = {1.00f, 0.52f, 0.00f, 0.77f};
	colors[ImGuiCol_SliderGrab] = {1.00f, 0.52f, 0.00f, 0.48f};
	colors[ImGuiCol_SliderGrabActive] = {1.00f, 0.52f, 0.00f, 1.00f};
	colors[ImGuiCol_Button] = {0.13f, 0.13f, 0.13f, 0.48f};
	colors[ImGuiCol_ButtonHovered] = {1.00f, 0.52f, 0.00f, 0.22f};
	colors[ImGuiCol_ButtonActive] = {1.00f, 0.52f, 0.00f, 0.48f};
	colors[ImGuiCol_Header] = {0.39f, 0.39f, 0.39f, 0.48f};
	colors[ImGuiCol_HeaderHovered] = {1.00f, 0.52f, 0.00f, 0.22f};
	colors[ImGuiCol_HeaderActive] = {1.00f, 0.52f, 0.00f, 0.48f};
	colors[ImGuiCol_Separator] = {1.00f, 0.52f, 0.00f, 1.00f};
	colors[ImGuiCol_SeparatorHovered] = {1.00f, 1.00f, 1.00f, 1.00f};
	colors[ImGuiCol_SeparatorActive] = {0.67f, 0.67f, 0.67f, 1.00f};
	colors[ImGuiCol_ResizeGrip] = {0.52f, 0.52f, 0.52f, 0.42f};
	colors[ImGuiCol_ResizeGripHovered] = {0.52f, 0.52f, 0.52f, 0.74f};
	colors[ImGuiCol_ResizeGripActive] = {1.00f, 0.52f, 0.00f, 0.48f};
	colors[ImGuiCol_Tab] = {0.34f, 0.34f, 0.68f, 0.79f};
	colors[ImGuiCol_TabHovered] = {0.45f, 0.45f, 0.90f, 0.80f};
	colors[ImGuiCol_TabActive] = {0.40f, 0.40f, 0.73f, 0.84f};
	colors[ImGuiCol_TabUnfocused] = {0.28f, 0.28f, 0.57f, 0.82f};
	colors[ImGuiCol_TabUnfocusedActive] = {0.35f, 0.35f, 0.65f, 0.84f};
	colors[ImGuiCol_PlotLines] = {1.00f, 1.00f, 1.00f, 1.00f};
	colors[ImGuiCol_PlotLinesHovered] = {0.90f, 0.70f, 0.00f, 1.00f};
	colors[ImGuiCol_PlotHistogram] = {1.00f, 0.52f, 0.00f, 1.00f};
	colors[ImGuiCol_PlotHistogramHovered] = {1.00f, 0.60f, 0.00f, 1.00f};
	colors[ImGuiCol_TextSelectedBg] = {1.00f, 0.52f, 0.00f, 0.43f};
	colors[ImGuiCol_DragDropTarget] = {1.00f, 0.52f, 0.00f, 1.00f};
	colors[ImGuiCol_NavHighlight] = {0.45f, 0.45f, 0.90f, 0.80f};
	colors[ImGuiCol_NavWindowingHighlight] = {1.00f, 1.00f, 1.00f, 0.70f};
	colors[ImGuiCol_NavWindowingDimBg] = {0.80f, 0.80f, 0.80f, 0.20f};
	colors[ImGuiCol_ModalWindowDimBg] = {0.20f, 0.20f, 0.20f, 0.35f};
}

void ImGuiCustom::StyleColors1(ImGuiStyle *dst) noexcept
{
	ImGuiStyle *style = dst ? dst : &ImGui::GetStyle();
	ImVec4 *colors = style->Colors;

	colors[ImGuiCol_Text] = {1.00f, 1.00f, 1.00f, 1.00f};
	colors[ImGuiCol_TextTitle] = {1.00f, 1.00f, 1.00f, 1.00f};
	colors[ImGuiCol_TextDisabled] = {1.00f, 1.00f, 1.00f, 0.62f};
	colors[ImGuiCol_WindowBg] = {0.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_ChildBg] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_PopupBg] = {0.00f, 0.30f, 0.39f, 0.83f};
	colors[ImGuiCol_Border] = {0.00f, 0.76f, 1.00f, 0.71f};
	colors[ImGuiCol_BorderShadow] = {0.00f, 0.76f, 1.00f, 0.23f};
	colors[ImGuiCol_FrameBg] = {0.00f, 0.76f, 1.00f, 0.24f};
	colors[ImGuiCol_FrameBgHovered] = {0.00f, 0.76f, 1.00f, 0.45f};
	colors[ImGuiCol_FrameBgActive] = {0.00f, 0.76f, 1.00f, 0.73f};
	colors[ImGuiCol_TitleBg] = {0.00f, 0.19f, 0.25f, 1.00f};
	colors[ImGuiCol_TitleBgActive] = {0.00f, 0.39f, 0.51f, 1.00f};
	colors[ImGuiCol_TitleBgCollapsed] = {0.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_MenuBarBg] = {0.14f, 0.14f, 0.14f, 1.00f};
	colors[ImGuiCol_ScrollbarBg] = {0.00f, 0.00f, 0.00f, 0.30f};
	colors[ImGuiCol_ScrollbarGrab] = {0.00f, 0.76f, 1.00f, 0.24f};
	colors[ImGuiCol_ScrollbarGrabHovered] = {0.00f, 0.76f, 1.00f, 0.45f};
	colors[ImGuiCol_ScrollbarGrabActive] = {0.00f, 0.76f, 1.00f, 0.73f};
	colors[ImGuiCol_CheckMark] = {0.00f, 0.76f, 1.00f, 1.00f};
	colors[ImGuiCol_SliderGrab] = {0.00f, 0.76f, 1.00f, 1.00f};
	colors[ImGuiCol_SliderGrabActive] = {0.86f, 0.97f, 1.00f, 1.00f};
	colors[ImGuiCol_Button] = {0.00f, 0.76f, 1.00f, 0.67f};
	colors[ImGuiCol_ButtonHovered] = {0.33f, 0.84f, 1.00f, 1.00f};
	colors[ImGuiCol_ButtonActive] = {0.86f, 0.97f, 1.00f, 1.00f};
	colors[ImGuiCol_Header] = {0.00f, 0.76f, 1.00f, 0.37f};
	colors[ImGuiCol_HeaderHovered] = {0.00f, 0.76f, 1.00f, 0.58f};
	colors[ImGuiCol_HeaderActive] = {0.00f, 0.76f, 1.00f, 0.73f};
	colors[ImGuiCol_Separator] = {0.00f, 0.76f, 1.00f, 0.35f};
	colors[ImGuiCol_SeparatorHovered] = {0.10f, 0.40f, 0.75f, 0.78f};
	colors[ImGuiCol_SeparatorActive] = {1.00f, 1.00f, 1.00f, 1.00f};
	colors[ImGuiCol_ResizeGrip] = {0.00f, 0.76f, 1.00f, 0.24f};
	colors[ImGuiCol_ResizeGripHovered] = {0.00f, 0.76f, 1.00f, 0.45f};
	colors[ImGuiCol_ResizeGripActive] = {0.00f, 0.76f, 1.00f, 0.73f};
	colors[ImGuiCol_Tab] = {0.18f, 0.35f, 0.58f, 0.86f};
	colors[ImGuiCol_TabHovered] = {0.26f, 0.59f, 0.98f, 0.80f};
	colors[ImGuiCol_TabActive] = {0.20f, 0.41f, 0.68f, 1.00f};
	colors[ImGuiCol_TabUnfocused] = {0.07f, 0.10f, 0.15f, 0.97f};
	colors[ImGuiCol_TabUnfocusedActive] = {0.14f, 0.26f, 0.42f, 1.00f};
	colors[ImGuiCol_PlotLines] = {0.61f, 0.61f, 0.61f, 1.00f};
	colors[ImGuiCol_PlotLinesHovered] = {1.00f, 0.43f, 0.35f, 1.00f};
	colors[ImGuiCol_PlotHistogram] = {0.00f, 0.76f, 1.00f, 1.00f};
	colors[ImGuiCol_PlotHistogramHovered] = {1.00f, 0.60f, 0.00f, 1.00f};
	colors[ImGuiCol_TextSelectedBg] = {0.00f, 0.76f, 1.00f, 0.54f};
	colors[ImGuiCol_DragDropTarget] = {0.00f, 0.76f, 1.00f, 1.00f};
	colors[ImGuiCol_NavHighlight] = {0.26f, 0.59f, 0.98f, 1.00f};
	colors[ImGuiCol_NavWindowingHighlight] = {1.00f, 1.00f, 1.00f, 0.70f};
	colors[ImGuiCol_NavWindowingDimBg] = {0.80f, 0.80f, 0.80f, 0.20f};
	colors[ImGuiCol_ModalWindowDimBg] = {0.80f, 0.80f, 0.80f, 0.35f};
}

void ImGuiCustom::StyleColors2(ImGuiStyle *dst) noexcept
{
	ImGuiStyle *style = dst ? dst : &ImGui::GetStyle();
	ImVec4 *colors = style->Colors;

	colors[ImGuiCol_Text] = {0.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_TextTitle] = {1.00f, 1.00f, 1.00f, 1.00f};
	colors[ImGuiCol_TextDisabled] = {0.00f, 0.00f, 0.00f, 0.18f};
	colors[ImGuiCol_WindowBg] = {1.00f, 1.00f, 1.00f, 1.00f};
	colors[ImGuiCol_ChildBg] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_PopupBg] = {1.00f, 1.00f, 1.00f, 1.00f};
	colors[ImGuiCol_Border] = {0.00f, 0.00f, 0.00f, 0.24f};
	colors[ImGuiCol_BorderShadow] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_FrameBg] = {0.94f, 0.94f, 0.94f, 1.00f};
	colors[ImGuiCol_FrameBgHovered] = {0.78f, 0.78f, 0.78f, 1.00f};
	colors[ImGuiCol_FrameBgActive] = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_TitleBg] = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_TitleBgActive] = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_TitleBgCollapsed] = {0.20f, 0.20f, 0.20f, 1.00f};
	colors[ImGuiCol_MenuBarBg] = {0.87f, 0.87f, 0.87f, 1.00f};
	colors[ImGuiCol_ScrollbarBg] = {0.84f, 0.84f, 0.84f, 1.00f};
	colors[ImGuiCol_ScrollbarGrab] = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_ScrollbarGrabHovered] = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_ScrollbarGrabActive] = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_CheckMark] = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_SliderGrab] = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_SliderGrabActive] = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_Button] = {0.90f, 0.90f, 0.90f, 1.00f};
	colors[ImGuiCol_ButtonHovered] = {0.78f, 0.78f, 0.78f, 1.00f};
	colors[ImGuiCol_ButtonActive] = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_Header] = {0.84f, 0.84f, 0.84f, 1.00f};
	colors[ImGuiCol_HeaderHovered] = {0.76f, 0.76f, 0.76f, 1.00f};
	colors[ImGuiCol_HeaderActive] = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_Separator] = {0.00f, 0.00f, 0.00f, 0.24f};
	colors[ImGuiCol_SeparatorHovered] = {1.00f, 1.00f, 1.00f, 1.00f};
	colors[ImGuiCol_SeparatorActive] = {0.67f, 0.67f, 0.67f, 1.00f};
	colors[ImGuiCol_ResizeGrip] = {0.94f, 0.94f, 0.94f, 1.00f};
	colors[ImGuiCol_ResizeGripHovered] = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_ResizeGripActive] = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_Tab] = {0.34f, 0.34f, 0.68f, 0.79f};
	colors[ImGuiCol_TabHovered] = {0.45f, 0.45f, 0.90f, 0.80f};
	colors[ImGuiCol_TabActive] = {0.40f, 0.40f, 0.73f, 0.84f};
	colors[ImGuiCol_TabUnfocused] = {0.28f, 0.28f, 0.57f, 0.82f};
	colors[ImGuiCol_TabUnfocusedActive] = {0.35f, 0.35f, 0.65f, 0.84f};
	colors[ImGuiCol_PlotLines] = {1.00f, 1.00f, 1.00f, 1.00f};
	colors[ImGuiCol_PlotLinesHovered] = {0.90f, 0.70f, 0.00f, 1.00f};
	colors[ImGuiCol_PlotHistogram] = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_PlotHistogramHovered] = {1.00f, 0.60f, 0.00f, 1.00f};
	colors[ImGuiCol_TextSelectedBg] = {1.00f, 0.00f, 0.00f, 0.24f};
	colors[ImGuiCol_DragDropTarget] = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_NavHighlight] = {0.45f, 0.45f, 0.90f, 0.80f};
	colors[ImGuiCol_NavWindowingHighlight] = {1.00f, 1.00f, 1.00f, 0.70f};
	colors[ImGuiCol_NavWindowingDimBg] = {0.80f, 0.80f, 0.80f, 0.20f};
	colors[ImGuiCol_ModalWindowDimBg] = {0.20f, 0.20f, 0.20f, 0.35f};
}

void ImGuiCustom::StyleColors3(ImGuiStyle *dst) noexcept
{
	ImGuiStyle *style = dst ? dst : &ImGui::GetStyle();
	ImVec4 *colors = style->Colors;

	colors[ImGuiCol_Text] = {0.91f, 0.90f, 0.88f, 1.00f};
	colors[ImGuiCol_TextTitle] = {1.00f, 1.00f, 1.00f, 1.00f};
	colors[ImGuiCol_TextDisabled] = {1.00f, 1.00f, 1.00f, 0.37f};
	colors[ImGuiCol_WindowBg] = {0.06f, 0.17f, 0.00f, 0.79f};
	colors[ImGuiCol_ChildBg] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_PopupBg] = {0.06f, 0.17f, 0.00f, 0.79f};
	colors[ImGuiCol_Border] = {1.00f, 0.84f, 0.96f, 0.44f};
	colors[ImGuiCol_BorderShadow] = {1.00f, 0.00f, 0.77f, 0.28f};
	colors[ImGuiCol_FrameBg] = {1.00f, 0.00f, 0.77f, 0.24f};
	colors[ImGuiCol_FrameBgHovered] = {1.00f, 0.00f, 0.77f, 0.38f};
	colors[ImGuiCol_FrameBgActive] = {1.00f, 0.00f, 0.77f, 0.50f};
	colors[ImGuiCol_TitleBg] = {1.00f, 0.00f, 0.77f, 0.47f};
	colors[ImGuiCol_TitleBgActive] = {1.00f, 0.31f, 0.84f, 0.78f};
	colors[ImGuiCol_TitleBgCollapsed] = {0.50f, 0.00f, 0.38f, 0.47f};
	colors[ImGuiCol_MenuBarBg] = {0.26f, 0.00f, 0.20f, 0.50f};
	colors[ImGuiCol_ScrollbarBg] = {1.00f, 0.00f, 0.77f, 0.24f};
	colors[ImGuiCol_ScrollbarGrab] = {1.00f, 0.00f, 0.77f, 0.38f};
	colors[ImGuiCol_ScrollbarGrabHovered] = {1.00f, 0.00f, 0.77f, 0.50f};
	colors[ImGuiCol_ScrollbarGrabActive] = {1.00f, 0.00f, 0.77f, 1.00f};
	colors[ImGuiCol_CheckMark] = {1.00f, 0.00f, 0.77f, 1.00f};
	colors[ImGuiCol_SliderGrab] = {1.00f, 0.00f, 0.77f, 0.38f};
	colors[ImGuiCol_SliderGrabActive] = {1.00f, 0.00f, 0.77f, 1.00f};
	colors[ImGuiCol_Button] = {1.00f, 0.00f, 0.77f, 0.24f};
	colors[ImGuiCol_ButtonHovered] = {1.00f, 0.00f, 0.77f, 0.38f};
	colors[ImGuiCol_ButtonActive] = {1.00f, 0.00f, 0.77f, 0.50f};
	colors[ImGuiCol_Header] = {1.00f, 0.00f, 0.77f, 0.24f};
	colors[ImGuiCol_HeaderHovered] = {1.00f, 0.00f, 0.77f, 0.38f};
	colors[ImGuiCol_HeaderActive] = {1.00f, 0.00f, 0.77f, 0.50f};
	colors[ImGuiCol_Separator] = {1.00f, 0.00f, 0.77f, 1.00f};
	colors[ImGuiCol_SeparatorHovered] = {1.00f, 1.00f, 1.00f, 1.00f};
	colors[ImGuiCol_SeparatorActive] = {0.67f, 0.67f, 0.67f, 1.00f};
	colors[ImGuiCol_ResizeGrip] = {0.94f, 0.94f, 0.94f, 1.00f};
	colors[ImGuiCol_ResizeGripHovered] = {1.00f, 0.00f, 0.77f, 1.00f};
	colors[ImGuiCol_ResizeGripActive] = {1.00f, 0.00f, 0.77f, 1.00f};
	colors[ImGuiCol_Tab] = {0.34f, 0.34f, 0.68f, 0.79f};
	colors[ImGuiCol_TabHovered] = {0.45f, 0.45f, 0.90f, 0.80f};
	colors[ImGuiCol_TabActive] = {0.40f, 0.40f, 0.73f, 0.84f};
	colors[ImGuiCol_TabUnfocused] = {0.28f, 0.28f, 0.57f, 0.82f};
	colors[ImGuiCol_TabUnfocusedActive] = {0.35f, 0.35f, 0.65f, 0.84f};
	colors[ImGuiCol_PlotLines] = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_PlotLinesHovered] = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_PlotHistogram] = {1.00f, 0.00f, 0.77f, 1.00f};
	colors[ImGuiCol_PlotHistogramHovered] = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_TableHeaderBg] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_TableBorderStrong] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_TableBorderLight] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_TableRowBg] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_TableRowBgAlt] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_TextSelectedBg] = {0.06f, 0.17f, 0.00f, 0.79f};
	colors[ImGuiCol_DragDropTarget] = {1.00f, 0.32f, 0.84f, 1.00f};
	colors[ImGuiCol_NavHighlight] = {0.45f, 0.45f, 0.90f, 0.80f};
	colors[ImGuiCol_NavWindowingHighlight] = {1.00f, 1.00f, 1.00f, 0.70f};
	colors[ImGuiCol_NavWindowingDimBg] = {0.80f, 0.80f, 0.80f, 0.20f};
	colors[ImGuiCol_ModalWindowDimBg] = {0.20f, 0.20f, 0.20f, 0.35f};
}

void ImGuiCustom::StyleColors4(ImGuiStyle *dst) noexcept
{
	ImGuiStyle *style = dst ? dst : &ImGui::GetStyle();
	ImVec4 *colors = style->Colors;

	colors[ImGuiCol_Text] = {0.86f, 0.97f, 1.00f, 1.00f};
	colors[ImGuiCol_TextTitle] = {0.86f, 0.97f, 1.00f, 1.00f};
	colors[ImGuiCol_TextDisabled] = {1.00f, 1.00f, 1.00f, 0.37f};
	colors[ImGuiCol_WindowBg] = {0.20f, 0.22f, 0.23f, 1.00f};
	colors[ImGuiCol_ChildBg] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_PopupBg] = {0.20f, 0.22f, 0.23f, 1.00f};
	colors[ImGuiCol_Border] = {0.86f, 0.97f, 1.00f, 0.21f};
	colors[ImGuiCol_BorderShadow] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_FrameBg] = {0.86f, 0.97f, 1.00f, 0.00f};
	colors[ImGuiCol_FrameBgHovered] = {0.86f, 0.97f, 1.00f, 0.09f};
	colors[ImGuiCol_FrameBgActive] = {0.86f, 0.97f, 1.00f, 0.09f};
	colors[ImGuiCol_TitleBg] = {0.88f, 0.69f, 0.20f, 1.00f};
	colors[ImGuiCol_TitleBgActive] = {0.88f, 0.69f, 0.20f, 1.00f};
	colors[ImGuiCol_TitleBgCollapsed] = {0.88f, 0.69f, 0.20f, 1.00f};
	colors[ImGuiCol_MenuBarBg] = {0.27f, 0.30f, 0.31f, 1.00f};
	colors[ImGuiCol_ScrollbarBg] = {0.00f, 0.00f, 0.00f, 0.10f};
	colors[ImGuiCol_ScrollbarGrab] = {0.88f, 0.69f, 0.20f, 1.00f};
	colors[ImGuiCol_ScrollbarGrabHovered] = {0.88f, 0.69f, 0.20f, 1.00f};
	colors[ImGuiCol_ScrollbarGrabActive] = {0.88f, 0.69f, 0.20f, 1.00f};
	colors[ImGuiCol_CheckMark] = {0.88f, 0.69f, 0.20f, 1.00f};
	colors[ImGuiCol_SliderGrab] = {0.88f, 0.69f, 0.20f, 1.00f};
	colors[ImGuiCol_SliderGrabActive] = {0.88f, 0.69f, 0.20f, 1.00f};
	colors[ImGuiCol_Button] = {0.86f, 0.97f, 1.00f, 0.00f};
	colors[ImGuiCol_ButtonHovered] = {0.86f, 0.97f, 1.00f, 0.17f};
	colors[ImGuiCol_ButtonActive] = {0.88f, 0.69f, 0.20f, 1.00f};
	colors[ImGuiCol_Header] = {0.86f, 0.97f, 1.00f, 0.17f};
	colors[ImGuiCol_HeaderHovered] = {0.86f, 0.97f, 1.00f, 0.09f};
	colors[ImGuiCol_HeaderActive] = {0.86f, 0.97f, 1.00f, 0.17f};
	colors[ImGuiCol_Separator] = {0.88f, 0.69f, 0.20f, 1.00f};
	colors[ImGuiCol_SeparatorHovered] = {0.88f, 0.69f, 0.20f, 1.00f};
	colors[ImGuiCol_SeparatorActive] = {0.88f, 0.69f, 0.20f, 1.00f};
	colors[ImGuiCol_ResizeGrip] = {0.88f, 0.69f, 0.20f, 1.00f};
	colors[ImGuiCol_ResizeGripHovered] = {0.88f, 0.69f, 0.20f, 1.00f};
	colors[ImGuiCol_ResizeGripActive] = {0.88f, 0.69f, 0.20f, 1.00f};
	colors[ImGuiCol_Tab] = {0.34f, 0.34f, 0.68f, 0.79f};
	colors[ImGuiCol_TabHovered] = {0.45f, 0.45f, 0.90f, 0.80f};
	colors[ImGuiCol_TabActive] = {0.40f, 0.40f, 0.73f, 0.84f};
	colors[ImGuiCol_TabUnfocused] = {0.28f, 0.28f, 0.57f, 0.82f};
	colors[ImGuiCol_TabUnfocusedActive] = {0.35f, 0.35f, 0.65f, 0.84f};
	colors[ImGuiCol_PlotLines] = {0.88f, 0.69f, 0.20f, 1.00f};
	colors[ImGuiCol_PlotLinesHovered] = {0.86f, 0.97f, 1.00f, 1.00f};
	colors[ImGuiCol_PlotHistogram] = {0.88f, 0.69f, 0.20f, 1.00f};
	colors[ImGuiCol_PlotHistogramHovered] = {0.86f, 0.97f, 1.00f, 1.00f};
	colors[ImGuiCol_TableHeaderBg] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_TableBorderStrong] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_TableBorderLight] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_TableRowBg] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_TableRowBgAlt] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_TextSelectedBg] = {0.86f, 0.97f, 1.00f, 0.17f};
	colors[ImGuiCol_DragDropTarget] = {0.88f, 0.69f, 0.20f, 1.00f};
	colors[ImGuiCol_NavHighlight] = {0.45f, 0.45f, 0.90f, 0.80f};
	colors[ImGuiCol_NavWindowingHighlight] = {1.00f, 1.00f, 1.00f, 0.70f};
	colors[ImGuiCol_NavWindowingDimBg] = {0.80f, 0.80f, 0.80f, 0.20f};
	colors[ImGuiCol_ModalWindowDimBg] = {0.20f, 0.20f, 0.20f, 0.35f};
}

void ImGuiCustom::StyleColors5(ImGuiStyle *dst) noexcept
{
	ImGuiStyle *style = dst ? dst : &ImGui::GetStyle();
	ImVec4 *colors = style->Colors;

	colors[ImGuiCol_Text] = {0.86f, 0.97f, 1.00f, 1.00f};
	colors[ImGuiCol_TextTitle] = {0.86f, 0.97f, 1.00f, 1.00f};
	colors[ImGuiCol_TextDisabled] = {1.00f, 1.00f, 1.00f, 0.37f};
	colors[ImGuiCol_WindowBg] = {0.16f, 0.16f, 0.20f, 0.89f};
	colors[ImGuiCol_ChildBg] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_PopupBg] = {0.16f, 0.16f, 0.20f, 0.89f};
	colors[ImGuiCol_Border] = {0.86f, 0.97f, 1.00f, 0.21f};
	colors[ImGuiCol_BorderShadow] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_FrameBg] = {0.86f, 0.97f, 1.00f, 0.00f};
	colors[ImGuiCol_FrameBgHovered] = {0.86f, 0.97f, 1.00f, 0.09f};
	colors[ImGuiCol_FrameBgActive] = {0.86f, 0.97f, 1.00f, 0.09f};
	colors[ImGuiCol_TitleBg] = {1.00f, 0.00f, 0.37f, 1.00f};
	colors[ImGuiCol_TitleBgActive] = {1.00f, 0.00f, 0.37f, 1.00f};
	colors[ImGuiCol_TitleBgCollapsed] = {0.48f, 0.09f, 0.23f, 1.00f};
	colors[ImGuiCol_MenuBarBg] = {0.16f, 0.16f, 0.20f, 1.00f};
	colors[ImGuiCol_ScrollbarBg] = {0.00f, 0.00f, 0.00f, 0.10f};
	colors[ImGuiCol_ScrollbarGrab] = {1.00f, 0.00f, 0.37f, 1.00f};
	colors[ImGuiCol_ScrollbarGrabHovered] = {1.00f, 0.00f, 0.37f, 1.00f};
	colors[ImGuiCol_ScrollbarGrabActive] = {1.00f, 0.42f, 0.54f, 1.00f};
	colors[ImGuiCol_CheckMark] = {1.00f, 0.00f, 0.37f, 1.00f};
	colors[ImGuiCol_SliderGrab] = {1.00f, 0.00f, 0.37f, 1.00f};
	colors[ImGuiCol_SliderGrabActive] = {1.00f, 0.00f, 0.37f, 1.00f};
	colors[ImGuiCol_Button] = {1.00f, 0.00f, 0.37f, 0.50f};
	colors[ImGuiCol_ButtonHovered] = {1.00f, 0.00f, 0.37f, 1.00f};
	colors[ImGuiCol_ButtonActive] = {1.00f, 0.42f, 0.54f, 1.00f};
	colors[ImGuiCol_Header] = {0.86f, 0.97f, 1.00f, 0.17f};
	colors[ImGuiCol_HeaderHovered] = {0.86f, 0.97f, 1.00f, 0.09f};
	colors[ImGuiCol_HeaderActive] = {0.86f, 0.97f, 1.00f, 0.17f};
	colors[ImGuiCol_Separator] = {1.00f, 0.00f, 0.37f, 0.50f};
	colors[ImGuiCol_SeparatorHovered] = {1.00f, 0.00f, 0.37f, 1.00f};
	colors[ImGuiCol_SeparatorActive] = {1.00f, 0.42f, 0.54f, 1.00f};
	colors[ImGuiCol_ResizeGrip] = {1.00f, 0.00f, 0.37f, 0.50f};
	colors[ImGuiCol_ResizeGripHovered] = {1.00f, 0.00f, 0.37f, 1.00f};
	colors[ImGuiCol_ResizeGripActive] = {1.00f, 0.42f, 0.54f, 1.00f};
	colors[ImGuiCol_Tab] = {0.34f, 0.34f, 0.68f, 0.79f};
	colors[ImGuiCol_TabHovered] = {0.45f, 0.45f, 0.90f, 0.80f};
	colors[ImGuiCol_TabActive] = {0.40f, 0.40f, 0.73f, 0.84f};
	colors[ImGuiCol_TabUnfocused] = {0.28f, 0.28f, 0.57f, 0.82f};
	colors[ImGuiCol_TabUnfocusedActive] = {0.35f, 0.35f, 0.65f, 0.84f};
	colors[ImGuiCol_PlotLines] = {1.00f, 0.00f, 0.37f, 1.00f};
	colors[ImGuiCol_PlotLinesHovered] = {1.00f, 0.42f, 0.54f, 1.00f};
	colors[ImGuiCol_PlotHistogram] = {1.00f, 0.00f, 0.37f, 1.00f};
	colors[ImGuiCol_PlotHistogramHovered] = {1.00f, 0.42f, 0.54f, 1.00f};
	colors[ImGuiCol_TableHeaderBg] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_TableBorderStrong] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_TableBorderLight] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_TableRowBg] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_TableRowBgAlt] = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_TextSelectedBg] = {0.86f, 0.97f, 1.00f, 0.17f};
	colors[ImGuiCol_DragDropTarget] = {1.00f, 0.00f, 0.37f, 1.00f};
	colors[ImGuiCol_NavHighlight] = {0.45f, 0.45f, 0.90f, 0.80f};
	colors[ImGuiCol_NavWindowingHighlight] = {1.00f, 1.00f, 1.00f, 0.70f};
	colors[ImGuiCol_NavWindowingDimBg] = {0.80f, 0.80f, 0.80f, 0.20f};
	colors[ImGuiCol_ModalWindowDimBg] = {0.20f, 0.20f, 0.20f, 0.35f};
}
