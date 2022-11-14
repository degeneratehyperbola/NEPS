#pragma once

#include <vector>
#include <memory>
#include <windows.h>

struct IDirect3DTexture9;

struct Texture
{
	Texture(const char *path) noexcept;
	Texture(HMODULE hModule, int resource, const wchar_t *type) noexcept;
	Texture(std::size_t size, const void *data) noexcept;
	Texture(unsigned int width, unsigned int height, const void *rawData) noexcept;

	Texture() noexcept = default;
	Texture(const Texture &) noexcept = default;
	Texture(Texture &&) noexcept = default;

	Texture &operator=(const Texture &) noexcept;
	Texture &operator=(Texture &&) noexcept;

	~Texture() noexcept;

	void free() noexcept;
	void *get() noexcept { return textureData; };
private:
	IDirect3DTexture9 *textureData = nullptr;
	unsigned int w = 0U, h = 0U;
};
