#pragma once

#include <cstddef>

#include "Vector.h"
#include "VirtualMethod.h"

#pragma region TraceMask

// Nower bits are stronger, and will eat weaker brushes completely
#define	CONTENTS_EMPTY 0 // No contents

#define	CONTENTS_SOLID 0x1 // An eye is never valid in a solid
#define	CONTENTS_WINDOW 0x2 // Translucent, but not watery (glass)
#define	CONTENTS_AUX 0x4
#define	CONTENTS_GRATE 0x8 // Alpha-tested "grate" textures.  Bullets/sight pass through, but solids don't
#define	CONTENTS_SLIME 0x10
#define	CONTENTS_WATER 0x20
#define	CONTENTS_BLOCKLOS 0x40 // Block AI line of sight
#define CONTENTS_OPAQUE 0x80 // Things that cannot be seen through (may be non-solid though)
#define	LAST_VISIBLE_CONTENTS CONTENTS_OPAQUE

#define ALL_VISIBLE_CONTENTS (LAST_VISIBLE_CONTENTS | (LAST_VISIBLE_CONTENTS-1))

#define CONTENTS_TESTFOGVOLUME 0x100
#define CONTENTS_UNUSED 0x200	

// Unused
// NOTE: If it's visible, grab from the top + update LAST_VISIBLE_CONTENTS if not visible, then grab from the bottom.
// CONTENTS_OPAQUE + SURF_NODRAW count as CONTENTS_OPAQUE (shadow-casting toolsblocklight textures)
#define CONTENTS_BLOCKLIGHT 0x400

#define CONTENTS_TEAM1 0x800 // per team contents used to differentiate collisions 
#define CONTENTS_TEAM2 0x1000 // between players and objects on different teams

// Ignore CONTENTS_OPAQUE on surfaces that have SURF_NODRAW
#define CONTENTS_IGNORE_NODRAW_OPAQUE 0x2000

// Hits entities which are MOVETYPE_PUSH (doors, plats, etc.)
#define CONTENTS_MOVEABLE 0x4000

// Remaining contents are non-visible, and don't eat brushes
#define	CONTENTS_AREAPORTAL 0x8000

#define	CONTENTS_PLAYERCLIP 0x10000
#define	CONTENTS_MONSTERCLIP 0x20000

#define	CONTENTS_BRUSH_PAINT 0x40000
#define	CONTENTS_GRENADECLIP 0x80000
#define	CONTENTS_UNUSED2 0x100000
#define	CONTENTS_UNUSED3 0x200000
#define	CONTENTS_UNUSED4 0x400000
#define	CONTENTS_UNUSED5 0x800000

#define	CONTENTS_ORIGIN 0x1000000 // Removed before bsping an entity

#define	CONTENTS_MONSTER 0x2000000 // Should never be on a brush, only in game
#define	CONTENTS_DEBRIS 0x4000000
#define	CONTENTS_DETAIL 0x8000000 // Brushes to be added after vis leafs
#define	CONTENTS_TRANSLUCENT 0x10000000 // Auto set if any surface has trans
#define	CONTENTS_LADDER 0x20000000
#define CONTENTS_HITBOX 0x40000000 // Use accurate hitboxes on trace

#pragma endregion

struct Ray {
    Ray(const Vector& src, const Vector& dest) : start(src), delta(dest - src) { isSwept = delta.x || delta.y || delta.z; }
	Vector start;
	INIT_PAD(4)
    Vector delta;
	INIT_PAD(40)
    bool isRay = true;
    bool isSwept;
};

class Entity;

struct TraceFilter {
    TraceFilter(const Entity* entity) : skip{ entity } { }
    virtual bool shouldHitEntity(Entity* entity, int) { return entity != skip; }
    virtual int getTraceType() const { return 0; }
    const void* skip;
};

namespace HitGroup {
    enum {
        Invalid = -1,
        Generic,
        Head,
        Chest,
        Stomach,
        LeftArm,
        RightArm,
        LeftLeg,
        RightLeg,
        Gear = 10
    };

    constexpr float getDamageMultiplier(int hitGroup) noexcept
    {
        switch (hitGroup) {
        case Head:
            return 4.0f;
        case Stomach:
            return 1.25f;
        case LeftLeg:
        case RightLeg:
            return 0.75f;
        default:
            return 1.0f;
        }
    }

    constexpr bool isArmored(int hitGroup, bool helmet) noexcept
    {
        switch (hitGroup) {
        case Head:
            return helmet;

        case Chest:
        case Stomach:
        case LeftArm:
        case RightArm:
            return true;
        default:
            return false;
        }
    }
}

struct Trace
{
	Vector startPos;
	Vector endPos;
	PAD(20)
	float fraction;
	int contents;
	unsigned short dispFlags;
	bool allSolid;
	bool startSolid;
	PAD(4)
	struct Surface
	{
		const char *name;
		short surfaceProps;
		unsigned short flags;
	} surface;
	int hitGroup;
	PAD(4)
	Entity *entity;
	int hitbox;
};

// #define TRACE_STATS // - enable to see how many rays are cast per frame

#ifdef TRACE_STATS
#include "../Memory.h"
#include "GlobalVars.h"
#endif

class EngineTrace
{
public:
	VIRTUAL_METHOD(int, getPointContents, 0, (const Vector &absPosition, int contentsMask), (this, std::cref(absPosition), contentsMask, nullptr))
	VIRTUAL_METHOD(void, _traceRay, 5, (const Ray &ray, unsigned int mask, const TraceFilter &filter, Trace &trace), (this, std::cref(ray), mask, std::cref(filter), std::ref(trace)))

	void traceRay(const Ray &ray, unsigned int mask, const TraceFilter &filter, Trace &trace) noexcept
	{
		#ifdef TRACE_STATS
		static int tracesThisFrame, lastFrame;

		if (lastFrame != memory->globalVars->framecount)
		{
			memory->debugMsg("traces: frame - %d | count - %d\n", lastFrame, tracesThisFrame);
			tracesThisFrame = 0;
			lastFrame = memory->globalVars->framecount;
		}

		++tracesThisFrame;
		#endif
		_traceRay(ray, mask, filter, trace);
	}
};
