#pragma once

#include "Vector.h"
#include "VirtualMethod.h"
#include "Pad.h"

template<typename FuncType>
__forceinline static FuncType qCallVFunction(void* ppClass, int index)
{
    int* pVTable = *(int**)ppClass;
    int dwAddress = pVTable[index];
    return (FuncType)(dwAddress);
}


class Input {
public:
	VIRTUAL_METHOD(UserCmd*, getUserCmd, 8, (int slot, int sequenceNumber), (this, slot, sequenceNumber))
    PAD(12)
    bool isTrackIRAvailable;
    bool isMouseInitialized;
    bool isMouseActive;
    PAD(158)
    bool isCameraInThirdPerson;
    PAD(1)
    Vector cameraOffset;
};
