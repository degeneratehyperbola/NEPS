#include "TextureDX9.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../stb/image.h"
#include "../../Hooks.h"

#include <memory>
#include <d3d9.h>

Texture::Texture(const char *path) noexcept
{
	if (!path) return;

	int width, height;
	auto data = stbi_load(path, &width, &height, nullptr, 4);
	texture = reinterpret_cast<IDirect3DTexture9 *>(ImGui_ImplDX9_CreateTextureRGBA8(width, height, data));
	stbi_image_free(data);
}

Texture::Texture(int resource, const char *type) noexcept
{
	if (!resource || !type) return;

	auto dllHandle = hooks->getDllHandle();
	auto resourceHandle = FindResourceA(dllHandle, MAKEINTRESOURCEA(resource), type);
	if (resourceHandle)
	{
		int width, height;
		int resourceSize = SizeofResource(dllHandle, resourceHandle);
		auto resourceData = LoadResource(dllHandle, resourceHandle);
		void *buffer = LockResource(resourceData);
		auto data = stbi_load_from_memory(reinterpret_cast<unsigned char *>(buffer), resourceSize, &width, &height, nullptr, 4);
		texture = reinterpret_cast<IDirect3DTexture9 *>(ImGui_ImplDX9_CreateTextureRGBA8(width, height, data));
		stbi_image_free(data);
		FreeResource(resourceData);
	}
}

Texture::Texture(int width, int height, const unsigned char *data) noexcept
{
	if (!data) return;

	texture = reinterpret_cast<IDirect3DTexture9 *>(ImGui_ImplDX9_CreateTextureRGBA8(width, height, data));
}

Texture::~Texture() noexcept
{
	if (texture) texture->Release();
}
