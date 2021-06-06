#pragma once

#include "UtlVector.h"
#include "Pad.h"

struct VarEntry
{
	unsigned short type;
	unsigned short needsToInterpolate;
	PAD(8)
};

struct VarMap
{
	UtlVector<VarEntry> entries;
	int interpolatedEntries;
};
