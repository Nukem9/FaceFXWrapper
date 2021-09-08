#include <stdio.h>
#include "CreationKit32.h"

namespace CreationKit
{
	void SetFaceFXDataPath(const char *Path)
	{
		static char tempPath[1024];
		static bool init = [&]()
		{
			// Manually patch Fonix.cdf lookup path - alternate global variable
			auto fonixPathPtr = reinterpret_cast<uintptr_t>(&tempPath);

			switch (Loader::GetGameVersion())
			{
			case Loader::GameVersion::SkyrimOrEarlier:
				Loader::PatchMemory(0x469CA8, reinterpret_cast<uint8_t *>(&fonixPathPtr), sizeof(fonixPathPtr));
				break;

			case Loader::GameVersion::Fallout4:
				Loader::PatchMemory(0x702F27, reinterpret_cast<uint8_t *>(&fonixPathPtr), sizeof(fonixPathPtr));
				break;
			}

			return true;
		}();

		// "C:\Directory\My\FonixData.cdf"
		strcpy_s(tempPath, Path);
	}

	void SetFaceFXLanguage(const char *Language)
	{
		static char tempLanguage[128];
		static bool init = [&]()
		{
			// Manually patch language - alternate global variable
			auto languagePtr = reinterpret_cast<uintptr_t>(&tempLanguage);

			switch (Loader::GetGameVersion())
			{
			case Loader::GameVersion::SkyrimOrEarlier:
				Loader::PatchMemory(0x11B0AEC, reinterpret_cast<uint8_t *>(&languagePtr), sizeof(languagePtr));
				break;

			case Loader::GameVersion::Fallout4:
				Loader::PatchMemory(0x2E065A0, reinterpret_cast<uint8_t *>(&languagePtr), sizeof(languagePtr));
				break;
			}

			return true;
		}();

		// "USEnglish"
		strcpy_s(tempLanguage, Language);
	}

	void SetFaceFXAutoResampling(bool Resample)
	{
		// If automatic resampling is disabled, the resampled wav must exist already. The original wav file path will be unused.
		if (Loader::GetGameVersion() == Loader::GameVersion::SkyrimOrEarlier)
		{
			if (Resample)
				Loader::PatchMemory(0x470BA0, { 0x81, 0xEC, 0x68, 0x01, 0x00, 0x00 });
			else
				Loader::PatchMemory(0x470BA0, { 0xB0, 0x01, 0xC3 });
		}
		else if (Loader::GetGameVersion() == Loader::GameVersion::Fallout4)
		{
			if (Resample)
				Loader::PatchMemory(0x719090, { 0x55, 0x8B, 0xEC });
			else
				Loader::PatchMemory(0x719090, { 0xB0, 0x01, 0xC3 });
		}
	}

	void FaceFXLogCallback(const char *Text, int Type)
	{
		printf("[FaceFX %02d]: %s\n", Type, Text);
	}

	void LogCallback(int Type, const char *Format, ...)
	{
		char buffer[2048];
		va_list va;

		va_start(va, Format);
		_vsnprintf_s(buffer, _TRUNCATE, Format, va);
		va_end(va);

		printf("[CKIT32 %02d]: %s\n", Type, buffer);
	}

	void *__fastcall MemoryManager_Alloc(void *Thisptr, void *_EDX, uint32_t Size, uint32_t Alignment, bool Aligned)
	{
		if (Size <= 0)
			Size = 1;

		if (void *data = malloc(Size); data)
			return memset(data, 0, Size);

		return nullptr;
	}

	void __fastcall MemoryManager_Free(void *Thisptr, void *_EDX, void *Ptr, bool Aligned)
	{
		if (!Ptr)
			return;

		free(Ptr);
	}

	void *__fastcall ScrapHeap_Alloc(void **Thisptr, void *_EDX, uint32_t Size, uint32_t Alignment)
	{
		*Thisptr = MemoryManager_Alloc(nullptr, nullptr, Size, Alignment, Alignment != 0);
		return Thisptr;
	}

	void __fastcall ScrapHeap_Free(void **Thisptr)
	{
		if (Thisptr)
			MemoryManager_Free(nullptr, nullptr, *Thisptr, false);
	}
}