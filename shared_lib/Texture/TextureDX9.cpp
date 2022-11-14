#include "TextureDX9.h"
#include <shared_lib/imgui/imgui_impl_dx9.h>
#include <shared_lib/stb/image.h>

#include <memory>
#include <d3d9.h>

Texture::Texture(const char *path) noexcept
{
	if (!path) return;

	stbi_set_flip_vertically_on_load_thread(false);
	auto data = stbi_load(path, (int *)&w, (int *)&h, nullptr, 4);
	textureData = reinterpret_cast<IDirect3DTexture9 *>(ImGui_ImplDX9_CreateTextureRGBA8(w, h, data));
	stbi_image_free(data);
}

Texture::Texture(HMODULE hModule, int resource, const wchar_t *type) noexcept
{
	if (!resource || !type) return;

	auto resourceHandle = FindResourceW(hModule, MAKEINTRESOURCEW(resource), type);
	if (resourceHandle)
	{
		int resourceSize = SizeofResource(hModule, resourceHandle);
		auto resourceData = LoadResource(hModule, resourceHandle);
		if (resourceData)
		{
			void *buffer = LockResource(resourceData);
			stbi_set_flip_vertically_on_load_thread(false);
			auto data = stbi_load_from_memory(reinterpret_cast<unsigned char *>(buffer), resourceSize, (int *)&w, (int *)&h, nullptr, 4);
			textureData = reinterpret_cast<IDirect3DTexture9 *>(ImGui_ImplDX9_CreateTextureRGBA8(w, h, data));
			stbi_image_free(data);
			FreeResource(resourceData);
		}
	}
}

Texture::Texture(std::size_t size, const void *data) noexcept
{
	if (!data) return;

	stbi_set_flip_vertically_on_load_thread(false);
	auto rawData = stbi_load_from_memory(reinterpret_cast<const unsigned char *>(data), size, (int *)&w, (int *)&h, nullptr, 4);
	textureData = reinterpret_cast<IDirect3DTexture9 *>(ImGui_ImplDX9_CreateTextureRGBA8(w, h, rawData));
	stbi_image_free(rawData);
}

Texture::Texture(unsigned int width, unsigned int height, const void *rawData) noexcept
{
	if (!rawData) return;

	w = width, h = height;
	textureData = reinterpret_cast<IDirect3DTexture9 *>(ImGui_ImplDX9_CreateTextureRGBA8(width, height, rawData));
}

void Texture::free() noexcept
{
	if (textureData)
	{
		textureData->Release();
		textureData = nullptr;
		w = 0U, h = 0U;
	}
}

Texture &Texture::operator=(const Texture &other) noexcept
{
	free();
	w = other.w, h = other.h;
	textureData = other.textureData;
	return *this;
}

Texture &Texture::operator=(Texture &&other) noexcept
{
	free();
	w = other.w, h = other.h;
	textureData = other.textureData;
	return *this;
}

Texture::~Texture() noexcept
{
	free();
}
