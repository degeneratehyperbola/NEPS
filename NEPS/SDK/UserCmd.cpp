#include "UserCmd.h"
#include "CRC.h"

uint32_t UserCmd::computeCRC() const noexcept
{
	SetupCRC crc;

	crc.process(&commandNumber, sizeof(commandNumber));
	crc.process(&tickCount, sizeof(tickCount));
	crc.process(&viewangles, sizeof(viewangles));
	crc.process(&aimdirection, sizeof(aimdirection));
	crc.process(&forwardmove, sizeof(forwardmove));
	crc.process(&sidemove, sizeof(sidemove));
	crc.process(&upmove, sizeof(upmove));
	crc.process(&buttons, sizeof(buttons));
	crc.process(&impulse, sizeof(impulse));
	crc.process(&weaponSelect, sizeof(weaponSelect));
	crc.process(&weaponSubtype, sizeof(weaponSubtype));
	crc.process(&randomSeed, sizeof(randomSeed));
	crc.process(&mousedx, sizeof(mousedx));
	crc.process(&mousedy, sizeof(mousedy));

	return crc;
}
