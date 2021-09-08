#include "CreationKit32.h"
#include "LipSynchAnim.h"

LipSynchAnim *LipSynchAnim::Generate(const char *WavPath, const char *ResamplePath, const char *DialogueText)
{
	if (Loader::GetGameVersion() == Loader::GameVersion::SkyrimOrEarlier)
	{
		return ((LipSynchAnim *(__cdecl *)(const char *, const char *, const char *, const char *, void *))(0x46ACD0))
			(WavPath, ResamplePath, "", DialogueText, nullptr);
	}
	else if (Loader::GetGameVersion() == Loader::GameVersion::Fallout4)
	{
		return ((LipSynchAnim *(__cdecl *)(const char *, const char *, const char *, const char *, void *, void *))(0x702160))
			(WavPath, ResamplePath, "", DialogueText, *(void **)0x3F1DDA4, nullptr);
	}

	return nullptr;
}

bool LipSynchAnim::SaveToFile(const char *Path, bool Compress, bool FacegenDefault)
{
	if (FILE *f; fopen_s(&f, Path, "wb") == 0)
	{
		// Warning: sub_587730 has been manually patched to take a FILE handle
		bool result = false;

		if (Loader::GetGameVersion() == Loader::GameVersion::SkyrimOrEarlier)
			result = ((bool(__thiscall *)(LipSynchAnim *, FILE *, bool, int, int))(0x587730))(this, f, Compress, DefaultNumTargetsSkyrim, FacegenDefault);
		else if (Loader::GetGameVersion() == Loader::GameVersion::Fallout4)
			result = ((bool(__thiscall *)(LipSynchAnim *, FILE *, bool, int, int))(0x95CB30))(this, f, Compress, DefaultNumTargetsFallout4, FacegenDefault);

		fclose(f);
		return result;
	}

	return false;
}

void LipSynchAnim::Free()
{
	if (Loader::GetGameVersion() == Loader::GameVersion::SkyrimOrEarlier)
		((void(__thiscall *)(LipSynchAnim *))(0x586A40))(this);
	else if (Loader::GetGameVersion() == Loader::GameVersion::Fallout4)
		((void(__thiscall *)(LipSynchAnim *))(0x95BC90))(this);

	CreationKit::MemoryManager_Free(nullptr, nullptr, this, false);
}

int LipSynchAnim::hk_call_00587816(FILE *File, void *_EDX, void *Data, int Size)
{
	return fwrite(Data, 1, Size, File);
}