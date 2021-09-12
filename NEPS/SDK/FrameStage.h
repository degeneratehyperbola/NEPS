#pragma once

enum class FrameStage
{
	Undefined = -1,
	Start,
	NetUpdateStart,
	NetUpdatePostUpdateStart,
	NetUpdatePostUpdateEnd,
	NetUpdateEnd,
	RenderStart,
	RenderEnd
};
