#pragma once

struct ViewSetup
{
	std::byte pad[172];
	void *csm;
	float fov;
	float fovViewmodel;
	Vector origin;
	Vector angles;
	float nearZ;
	float farZ;
};