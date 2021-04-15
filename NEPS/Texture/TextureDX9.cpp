#include "TextureDX9.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../stb/image.h"
#include "../Hooks.h"

#include <memory>
#include <d3d9.h>

Texture::Texture(const char *path) noexcept
{
	int width, height;
	auto data = stbi_load(path, &width, &height, nullptr, 4);
	texture = reinterpret_cast<IDirect3DTexture9 *>(ImGui_ImplDX9_CreateTextureRGBA8(width, height, data));
	stbi_image_free(data);
}

Texture::Texture(int resource, const char *type) noexcept
{
	int width, height;
	auto dllHandle = hooks->getDllHandle();
	auto resourceHandle = FindResourceA(dllHandle, MAKEINTRESOURCEA(resource), type);
	if (resourceHandle)
	{
		int resourceSz = SizeofResource(dllHandle, resourceHandle);
		void *buffer = LoadResource(dllHandle, resourceHandle);
		auto data = stbi_load_from_memory(reinterpret_cast<unsigned char *>(buffer), resourceSz, &width, &height, nullptr, 4);
		texture = reinterpret_cast<IDirect3DTexture9 *>(ImGui_ImplDX9_CreateTextureRGBA8(width, height, data));
		stbi_image_free(data);
		FreeResource(buffer);
	} else
	{
		texture = nullptr;
	}
}

Texture::Texture(int width, int height, const unsigned char *data) noexcept
{
	texture = reinterpret_cast<IDirect3DTexture9 *>(ImGui_ImplDX9_CreateTextureRGBA8(width, height, data));
}

Texture::~Texture() noexcept
{
	texture->Release();
}
