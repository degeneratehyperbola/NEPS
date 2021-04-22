#pragma once

#include "VirtualMethod.h"
#include "Pad.h"

class NetworkChannel
{
public:
	VIRTUAL_METHOD(const char *, getName, 0, (), (this))
	VIRTUAL_METHOD(const char *, getAddress, 1, (), (this))
	VIRTUAL_METHOD(float, getLatency, 9, (int flow), (this, flow))

	PAD(24)
	int outSequenceNr;
	int inSequenceNr;
	int outSequenceNrAck;
	int outReliableState;
	int inReliableState;
	int chokedPackets;
};

class NetworkMessage
{
public:
	VIRTUAL_METHOD(int, getType, 7, (), (this))
};
