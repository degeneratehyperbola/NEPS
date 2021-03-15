#pragma once

#include "Config.h"
#include "imgui/imgui.h"

struct Color4;
struct Color4Toggle;
struct Color4ToggleRounding;
struct Color4ToggleThickness;
struct Color4ToggleThicknessRounding;

namespace ImGuiCustom
{
	void colorPicker(const char *name, std::array<float, 4> &color, bool *rainbow = nullptr, float *rainbowSpeed = nullptr, bool *enable = nullptr, float *thickness = nullptr, float *rounding = nullptr, bool *border = nullptr) noexcept;
	void colorPicker(const char *name, std::array<float, 3> &color, bool *enable = nullptr, bool *rainbow = nullptr, float *rainbowSpeed = nullptr) noexcept;
	void colorPicker(const char *name, Color3Toggle &colorConfig) noexcept;
	void colorPicker(const char *name, Color4 &colorConfig, bool *enable = nullptr, float *thickness = nullptr) noexcept;
	void colorPicker(const char *name, Color4Border &colorConfig, bool *enable = nullptr, float *thickness = nullptr) noexcept;
    void colorPicker(const char *name, Color4BorderToggle &colorConfig, bool *enable = nullptr, float *thickness = nullptr) noexcept;
    void colorPicker(const char *name, Color4Toggle &colorConfig) noexcept;
	void colorPicker(const char *name, Color4ToggleRounding &colorConfig) noexcept;
	void colorPicker(const char *name, Color4ToggleThickness &colorConfig) noexcept;
	void colorPicker(const char *name, Color4ToggleThicknessRounding &colorConfig) noexcept;
	void arrowButtonDisabled(const char *id, ImGuiDir dir) noexcept;
	void progressBarFullWidth(float fraction, float height = 0.0f) noexcept;
	void textUnformattedCentered(const char *text) noexcept;
    void keyBind(const char *name, int *key, int *keyMode = nullptr) noexcept;
    void keyBind(const char *name, KeyBind &bind) noexcept;
}
