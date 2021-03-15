#pragma once

#include "Vector.h"
#include "VirtualMethod.h"

class Input {
public:
	VIRTUAL_METHOD(UserCmd*, getUserCmd, 8, (int slot, int sequenceNumber), (this, slot, sequenceNumber))
    std::byte pad[12];
    bool isTrackIRAvailable;
    bool isMouseInitialized;
    bool isMouseActive;
    std::byte pad1[158];
    bool isCameraInThirdPerson;
    std::byte pad2;
    Vector cameraOffset;
};
