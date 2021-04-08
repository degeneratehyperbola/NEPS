#pragma once

#include "VirtualMethod.h"

class NetworkChannel
{
public:
	VIRTUAL_METHOD(float, getLatency, 9, (int flow), (this, flow))

	std::byte pad[24];
	int outSequenceNr;
	int inSequenceNr;
	int outSequenceNrAck;
	int outReliableState;
	int InReliableState;
	int chokedPackets;
};

class NetworkMessage
{
public:
	VIRTUAL_METHOD(int, getType, 7, (), (this))
};
