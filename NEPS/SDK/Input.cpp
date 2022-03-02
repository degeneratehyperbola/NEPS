#include "Input.h"
#include "UserCmd.h"

VerifiedUserCmd *Input::getVerifiedUserCmd(int sequenceNumber) noexcept
{
	return &verifiedCommands[sequenceNumber % 150];
}
