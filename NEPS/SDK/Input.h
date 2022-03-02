#pragma once

#include "Vector.h"
#include "VirtualMethod.h"
#include "Pad.h"

struct UserCmd;
struct VerifiedUserCmd;

class Input
{
public:
	VIRTUAL_METHOD(UserCmd *, getUserCmd, 8, (int slot, int sequenceNumber), (this, slot, sequenceNumber))
	VerifiedUserCmd *getVerifiedUserCmd(int sequenceNumber) noexcept;

	PAD(12)
	bool isTrackIRAvailable;
	bool isMouseInitialized;
	bool isMouseActive;
	PAD(158)
	bool isCameraInThirdPerson;
	PAD(2)
	Vector cameraOffset;
	PAD(56)
	UserCmd *commands;
	VerifiedUserCmd *verifiedCommands;
};
