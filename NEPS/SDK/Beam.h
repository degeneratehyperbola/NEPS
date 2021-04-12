#pragma once

#include "Entity.h"
#include "EngineTrace.h"
#include "Vector.h"

enum
{
	FBEAM_STARTENTITY = 0x00000001,
	FBEAM_ENDENTITY = 0x00000002,
	FBEAM_FADEIN = 0x00000004,
	FBEAM_FADEOUT = 0x00000008,
	FBEAM_SINENOISE = 0x00000010,
	FBEAM_SOLID = 0x00000020,
	FBEAM_SHADEIN = 0x00000040,
	FBEAM_SHADEOUT = 0x00000080,
	FBEAM_ONLYNOISEONCE = 0x00000100, // Only calculate our noise once
	FBEAM_NOTILE = 0x00000200,
	FBEAM_USE_HITBOXES = 0x00000400, // Attachment indices represent hitbox indices instead when this is set
	FBEAM_STARTVISIBLE = 0x00000800, // Has this client actually seen this beam's start entity yet?
	FBEAM_ENDVISIBLE = 0x00001000, // Has this client actually seen this beam's end entity yet?
	FBEAM_ISACTIVE = 0x00002000,
	FBEAM_FOREVER = 0x00004000,
	FBEAM_HALOBEAM = 0x00008000, // When drawing a beam with a halo, don't ignore the segments and endwidth
	FBEAM_REVERSED = 0x00010000,
	NUM_BEAM_FLAGS = 17 // KEEP THIS UPDATED!
};

enum
{
	TE_BEAMPOINTS = 0x00, // Beam effect between two points
	TE_SPRITE = 0x01, // Additive sprite, plays 1 cycle
	TE_BEAMDISK = 0x02, // Disk that expands to max radius over lifetime
	TE_BEAMCYLINDER = 0x03, // Cylinder that expands to max radius over lifetime
	TE_BEAMFOLLOW = 0x04, // Create a line of decaying beam segments until entity stops moving
	TE_BEAMRING = 0x05, // Connect a beam ring to two entities
	TE_BEAMSPLINE = 0x06,
	TE_BEAMRINGPOINT = 0x07,
	TE_BEAMLASER = 0x08, // Fades according to viewpoint
	TE_BEAMTESLA = 0x09,
};

struct Beam
{
	PAD(52)
	int flags;
	PAD(144)
	float die;
};

struct BeamTrail
{
	BeamTrail *next;
	float die;
	Vector origin;
	Vector velocity;
};

struct BeamInfo
{
	int type = TE_BEAMPOINTS;
	Entity *startEnt;
	int startAttachment;
	Entity *endEnt;
	int endAttachment;
	Vector start;
	Vector end;
	int modelIndex = -1;
	const char *modelName = nullptr;
	int haloIndex = -1;
	const char *haloName = nullptr;
	float haloScale;
	float life;
	float width;
	float endWidth;
	float fadeLength;
	float amplitude;
	float brightness;
	float speed;
	int startFrame;
	float frameRate;
	float red;
	float green;
	float blue;
	bool renderable = true;
	int segments = -1;
	int flags = 0;
	Vector center;
	float startRadius;
	float endRadius;
};

struct ViewRenderBeams
{
	VIRTUAL_METHOD(Beam *, createBeamEnts, 10, (BeamInfo &beamInfo), (this, std::ref(beamInfo)))
	VIRTUAL_METHOD(Beam *, createBeamEntPoints, 11, (BeamInfo &beamInfo), (this, std::ref(beamInfo)))
	VIRTUAL_METHOD(Beam *, createBeamPoints, 12, (BeamInfo &beamInfo), (this, std::ref(beamInfo)))
	VIRTUAL_METHOD(Beam *, createBeamRing, 13, (BeamInfo &beamInfo), (this, std::ref(beamInfo)))
	VIRTUAL_METHOD(Beam *, createBeamRingPoint, 14, (BeamInfo &beamInfo), (this, std::ref(beamInfo)))
	VIRTUAL_METHOD(Beam *, createBeamCirclePoints, 15, (BeamInfo &beamInfo), (this, std::ref(beamInfo)))
	VIRTUAL_METHOD(Beam *, createBeamFollow, 16, (BeamInfo &beamInfo), (this, std::ref(beamInfo)))
	VIRTUAL_METHOD(void, freeBeam, 17, (Beam *beam), (this, beam))
	VIRTUAL_METHOD(void, updateBeamInfo, 18, (Beam *beam, BeamInfo &beamInfo), (this, beam, std::ref(beamInfo)))
};
