#include "RCS.h"

#include "../SDK/Entity.h"
#include "../SDK/Vector.h"
#include "../SDK/UserCmd.h"

float random(const float& min, const float& max)
{
	srand(memory->globalVars->realtime + rand());
	return ((max - min) * ((float)rand() / RAND_MAX)) + min;
}

void Rcs::run(UserCmd* cmd) noexcept
{

	if (static Helpers::KeyBindState flag; !flag[config->rcs.bind]) {
		return;
	}

	if (!(cmd->buttons & UserCmd::Button_Attack))
		return;

	if (!localPlayer->isAlive() || !localPlayer)
		return;

	const auto activeWeaponType = localPlayer->getActiveWeapon()->getWeaponType();
	if (!(activeWeaponType == WeaponType::Rifle || activeWeaponType == WeaponType::SubMachinegun || activeWeaponType == WeaponType::Machinegun))
		return;

	int shotCount = localPlayer->shotsFired();
	int oldShotCount = 0;
	static Vector3 dwOldPunchAngle;
	const auto dwPunch = localPlayer->aimPunchAngle();
	const auto dwViewAngles = interfaces->engine->getViewAngles();
	const float totalPunch = dwPunch.x + dwPunch.y;
	
	if (totalPunch != 0.f && shotCount >= 1 && shotCount != oldShotCount)
	{
		static float randX = 1.f;
		static float randY = 1.f;
		if (config->rcs.humanize) {
			randX *= 1.f - random(.2f, .5f);
			randY *= 1.f - random(.2f, .4f);
		}
		const float recoilForce = config->rcs.recoilForce;
		const float recoilForceX = recoilForce * randX * config->rcs.shiftX;
		const float recoilForceY = recoilForce * randY * config->rcs.shiftY;
		const float angleX = (dwViewAngles.x + dwOldPunchAngle.x) - (dwPunch.x * recoilForceX);
		const float angleY = (dwViewAngles.y + dwOldPunchAngle.y) - (dwPunch.y * recoilForceY);
		const float angleZ = 0.f;
		const auto compensatedAngle = Vector3{ angleX,angleY,angleZ };
		auto AimAngle = compensatedAngle;
		AimAngle.clamp();
		interfaces->engine->setViewAngles(AimAngle);
		dwOldPunchAngle = Vector3{ dwPunch.x * recoilForceX, dwPunch.y * recoilForceY, 0.f };
		oldShotCount = shotCount;
		randX = randY = 1.f;
	}
	else {
		dwOldPunchAngle = Vector3{ 0.f,0.f,0.f };
		oldShotCount = 0;
	}

}