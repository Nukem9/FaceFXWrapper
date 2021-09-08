#pragma once

#include <stdio.h>

struct LipHeader
{
	enum LipHeaderFlags : int
	{
		Compressed = 1,
		BigEndian = 2,
		HasGestures = 4,
		VariableTargets = 8,
	};

	int Version;
	int Size;
	LipHeaderFlags Flags;
};
static_assert(sizeof(LipHeader) == 0xC);

class LipSynchAnim
{
private:
	constexpr static int DefaultNumTargetsSkyrim = 16;
	constexpr static int DefaultNumTargetsFallout4 = 43;

public:
	static LipSynchAnim *Generate(const char *WavPath, const char *ResamplePath, const char *DialogueText);
	bool SaveToFile(const char *Path, bool Compress = true, bool FacegenDefault = true);
	void Free();

	static int __fastcall hk_call_00587816(FILE *File, void *_EDX, void *Data, int Size);
};