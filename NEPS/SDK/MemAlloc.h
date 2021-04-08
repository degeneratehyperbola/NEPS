#pragma once

#include "Pad.h"
#include "VirtualMethod.h"

class MemAlloc
{
public:
	VIRTUAL_METHOD(void *, alloc, 1, (size_t size), (this, size))
};