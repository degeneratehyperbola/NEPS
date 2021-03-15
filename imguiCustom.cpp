#include "imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_internal.h"

#include "imguiCustom.h"
#include "Interfaces.h"
#include "SDK/InputSystem.h"

void ImGuiCustom::colorPicker(const char *name, std::array<float, 4> &color, bool *rainbow, float *rainbowSpeed, bool *enable, float *thickness, float *rounding, bool *border) noexcept
{
	ImGui::PushID(name);
	if (enable)
	{
		ImGui::Checkbox("##check", enable);
		ImGui::SameLine(0.0f, 5.0f);
	}
	bool openPopup = ImGui::ColorButton("##btn", color, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaPreview);
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
		{
			if (payload->DataSize == sizeof(float) * 3) // Fix for if payload comes from 3F
			{
				float oldAlpha = color[3];
				color = *(std::array<float, 4>*)payload->Data;
				color[3] = oldAlpha;
			} else
				color = *(std::array<float, 4>*)payload->Data;
		}

		ImGui::EndDragDropTarget();
	}
	ImGui::SameLine(0.0f, 5.0f);
	if (std::strncmp(name, "##", 2))
		ImGui::TextUnformatted(name);

	if (openPopup)
		ImGui::OpenPopup("##popup");

	if (ImGui::BeginPopup("##popup"))
	{
		ImGui::ColorPicker4("##picker", color.data(), ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaBar);

		if (rainbow || rainbowSpeed || thickness || rounding)
		{
			ImGui::SameLine();
			if (ImGui::BeginChild("##child", {150.0f, 0.0f}))
			{
				if (rainbow)
					ImGui::Checkbox("Rainbow", rainbow);
				ImGui::PushItemWidth(85.0f);
				if (rainbowSpeed)
					ImGui::InputFloat("Speed", rainbowSpeed, 0.0f, 0.0f, "%.2f");

				if (rounding || thickness || border)
					ImGui::Separator();

				if (rounding)
				{
					ImGui::InputFloat("Rounding", rounding, 0.0f, 0.0f, "%.1f");
					*rounding = std::fmaxf(*rounding, 0.0f);
				}

				if (thickness)
				{
					ImGui::InputFloat("Thickness", thickness, 0.0f, 0.0f, "%.1f");
					*thickness = std::fmaxf(*thickness, 1.0f);
				}

				if (border)
					ImGui::Checkbox("Border", border);

				ImGui::PopItemWidth();
				ImGui::EndChild();
			}
		}
		ImGui::EndPopup();
	}
	ImGui::PopID();
}

void ImGuiCustom::colorPicker(const char *name, std::array<float, 3> &color, bool *enable, bool *rainbow, float *rainbowSpeed) noexcept
{
	ImGui::PushID(name);
	if (enable)
	{
		ImGui::Checkbox("##check", enable);
		ImGui::SameLine(0.0f, 5.0f);
	}
	bool openPopup = ImGui::ColorButton("##btn", color.data(), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoAlpha);
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
			std::copy((float *)payload->Data, (float *)payload->Data + 3, color.data());
		ImGui::EndDragDropTarget();
	}
	ImGui::SameLine(0.0f, 5.0f);
	if (std::strncmp(name, "##", 2))
		ImGui::TextUnformatted(name);

	if (openPopup)
		ImGui::OpenPopup("##popup");

	if (ImGui::BeginPopup("##popup"))
	{
		ImGui::ColorPicker3("##picker", color.data(), ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_NoSidePreview);

		if (rainbow && rainbowSpeed)
		{
			ImGui::SameLine();

			if (ImGui::BeginChild("##child", {100.0f, 0.0f}))
			{
				ImGui::Checkbox("Rainbow", rainbow);
				ImGui::SetNextItemWidth(50.0f);
				ImGui::InputFloat("Speed", rainbowSpeed, 0.0f, 0.0f, "%.1f");
				ImGui::EndChild();
			}
		}
		ImGui::EndPopup();
	}
	ImGui::PopID();
}

void ImGuiCustom::colorPicker(const char *name, Color3Toggle &colorConfig) noexcept
{
	colorPicker(name, colorConfig.color, &colorConfig.enabled, &colorConfig.rainbow, &colorConfig.rainbowSpeed);
}

void ImGuiCustom::colorPicker(const char *name, Color4 &colorConfig, bool *enable, float *thickness) noexcept
{
	colorPicker(name, colorConfig.color, &colorConfig.rainbow, &colorConfig.rainbowSpeed, enable, thickness);
}

void ImGuiCustom::colorPicker(const char *name, Color4Border &colorConfig, bool *enable, float *thickness) noexcept
{
	colorPicker(name, colorConfig.color, &colorConfig.rainbow, &colorConfig.rainbowSpeed, enable, thickness, nullptr, &colorConfig.border);
}

void ImGuiCustom::colorPicker(const char *name, Color4BorderToggle &colorConfig, bool *enable, float *thickness) noexcept
{
	colorPicker(name, colorConfig.color, &colorConfig.rainbow, &colorConfig.rainbowSpeed, &colorConfig.enabled, thickness, nullptr, &colorConfig.border);
}

void ImGuiCustom::colorPicker(const char *name, Color4Toggle &colorConfig) noexcept
{
	colorPicker(name, colorConfig.color, &colorConfig.rainbow, &colorConfig.rainbowSpeed, &colorConfig.enabled);
}

void ImGuiCustom::colorPicker(const char *name, Color4ToggleRounding &colorConfig) noexcept
{
	colorPicker(name, colorConfig.color, &colorConfig.rainbow, &colorConfig.rainbowSpeed, &colorConfig.enabled, nullptr, &colorConfig.rounding);
}

void ImGuiCustom::colorPicker(const char *name, Color4ToggleThickness &colorConfig) noexcept
{
	colorPicker(name, colorConfig.color, &colorConfig.rainbow, &colorConfig.rainbowSpeed, &colorConfig.enabled, &colorConfig.thickness);
}

void ImGuiCustom::colorPicker(const char *name, Color4ToggleThicknessRounding &colorConfig) noexcept
{
	colorPicker(name, colorConfig.color, &colorConfig.rainbow, &colorConfig.rainbowSpeed, &colorConfig.enabled, &colorConfig.thickness, &colorConfig.rounding);
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
	ImVec2 size = ImGui::CalcItemSize(ImVec2{-1, 0}, ImGui::CalcItemWidth(), std::fmaxf(height, style.FrameRounding * 2.0f));
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
