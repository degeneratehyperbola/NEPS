#pragma once

#include <vector>
#include <memory>

struct IDirect3DTexture9;

struct Texture
{
	Texture(const char *path) noexcept;
	Texture(int resource, const char *type) noexcept;
	Texture(int width, int height, const unsigned char *data) noexcept;

	Texture() noexcept = default;
	Texture(const Texture &) noexcept = default;
	Texture(Texture &&) noexcept = default;

	~Texture() noexcept;

	void *get() noexcept { return texture; };
	IDirect3DTexture9 *getDX9() noexcept { return texture; };
private:
	IDirect3DTexture9 *texture = nullptr;
};
