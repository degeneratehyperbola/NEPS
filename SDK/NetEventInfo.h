#pragma once

class NetEventInfo
{
public:
	enum
	{
		EVENT_INDEX_BITS = 8,
		EVENT_DATA_LEN_BITS = 11,
		MAX_EVENT_DATA = 192,  // 1<<8 bits == 256, but only using 192 below
	};
	short classId;
	float fireDelay;
	const void *sendTable;
	const ClientClass *clientClass;
	int bits;
	uint8_t *data;
	int flags;
	PAD(0x1C);
	NetEventInfo *next;
};
