#include <stb.h>
#include "resource.h"
#include "CreationKit32.h"
#include "LipSynchAnim.h"
#include "Loader.h"

//
// Relocations aren't supported. To work around this, force this executable to be loaded at a base address of 0x10000. The CK.exe will be emulated 
// at a base address of 0x400000.
//
constexpr uintptr_t MinExeVirtualAddress = 0x400000;
constexpr uintptr_t MaxExeVirtualAddress = 0x7CC8000;

#pragma comment(linker, "/BASE:0x10000")
#pragma bss_seg(push)
#pragma bss_seg(".CKXCODE")
volatile char FORCE_LOAD_REQUIRED_DATA[(MinExeVirtualAddress - 0x10000) + MaxExeVirtualAddress];
#pragma bss_seg(pop)

namespace Loader
{
	GameVersion CurrentGameVersion = GameVersion::None;

	const static auto BAD_DLL = reinterpret_cast<HCUSTOMMODULE>(0xDEADBEEF);
	const static auto BAD_IMPORT = reinterpret_cast<FARPROC>(0xFEFEDEDE);

	bool Initialize(GameVersion Version)
	{
		if (CurrentGameVersion != GameVersion::None)
			return true;

		CurrentGameVersion = Version;

		if (!MapExecutable(Version))
			return false;

		if (!MapTLS(Version))
			return false;

		if (Version == GameVersion::SkyrimOrEarlier)
		{
			// Kill MemoryContextTracker ctor/dtor
			PatchMemory(0x948500, { 0xC2, 0x10, 0x00 });
			PatchMemory(0x948560, { 0xC3 });
			PatchMemory(0x9484E9, { 0x5E, 0xC3 });

			// Replace memory allocators with malloc and kill initializers
			PatchMemory(0x4010A0, { 0xC3 });
			DetourFunction(0x947D20, &CreationKit::MemoryManager_Alloc);
			DetourFunction(0x947320, &CreationKit::MemoryManager_Free);

			// Patch BSOStream::Write in LipSynchAnim::SaveToFile to use our own file handling
			PatchMemory(0x587810, { 0x90, 0x90, 0x90 });// Kill +0x4 pointer adjustment
			DetourFunction(0x587816, &LipSynchAnim::hk_call_00587816, true);
			DetourFunction(0x58781F, &LipSynchAnim::hk_call_00587816, true);

			// Patch WinMain in order to only run CRT initialization
			PatchMemory(0x48E8B0, { 0xC2, 0x10, 0x00 });
			PatchMemory(0xE84A16, { 0xEB, 0x0E });
			((void(__cdecl *)())(0xE84A7B))();

			CoInitializeEx(nullptr, 0);				// COM components
			((void(*)())(0x934B90))();				// BSResource (filesystem)
			((void(*)())(0x469FE0))();				// LipSynchManager::Init()
			PatchMemory(0x46AA59, { 0x90, 0x90 });	// Required to force update FonixData.cdf path in TLS

			// Add logging. Must be done after static constructors.
			((int(*)(void *))(0x8BF320))(&CreationKit::FaceFXLogCallback);
			DetourFunction(0x40AFC0, &CreationKit::LogCallback);

			return true;
		}
		else if (Version == GameVersion::Fallout4)
		{
			// Kill MemoryContextTracker ctor/dtor
			PatchMemory(0x1C4D530, { 0xC2, 0x10, 0x00 });
			PatchMemory(0x1C4D5F0, { 0xC3 });
			PatchMemory(0x1C4D85A, { 0x5F, 0x5E, 0xC3 });

			// Replace memory allocators with malloc and kill initializers
			PatchMemory(0x573520, { 0xC3 });
			DetourFunction(0x1C4B8A0, &CreationKit::MemoryManager_Alloc);
			DetourFunction(0x1C4BC30, &CreationKit::MemoryManager_Free);
			DetourFunction(0x1C4B530, &CreationKit::ScrapHeap_Alloc);
			DetourFunction(0x1C4B590, &CreationKit::ScrapHeap_Free);

			// Patch BSOStream::Write in LipSynchAnim::SaveToFile to use our own file handling
			PatchMemory(0x95CC07, { 0x0 });// Kill +0x4 pointer adjustment
			PatchMemory(0x95CC11, { 0x0 });// Kill +0x4 pointer adjustment
			DetourFunction(0x95CC08, &LipSynchAnim::hk_call_00587816, true);
			DetourFunction(0x95CC12, &LipSynchAnim::hk_call_00587816, true);

			// Prevent D3DCompiler.dll from being loaded
			PatchMemory(0x24A6490, { 0xC3 });

			// Kill the log writer thread functionality
			PatchMemory(0x7050F0, { 0x33, 0xC0, 0xC3 });
			PatchMemory(0x7055E3, { 0x90, 0x90 });
			PatchMemory(0x21D8E40, { 0xC2, 0x08, 0x00 });

			// Patch WinMain in order to only run CRT initialization
			PatchMemory(0x732150, { 0xC2, 0x10, 0x00 });
			PatchMemory(0x2755AD8, { 0xE9, 0x84, 0x00, 0x00, 0x00 });
			((void(__cdecl *)())(0x27559AD))();

			CoInitializeEx(nullptr, 0);										// COM components
			((void(*)())(0x209EED0))();										// BSResource (filesystem)
			((void(*)())(0x702800))();										// LipSynchManager::Init()
			PatchMemory(0x702F0C, { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });	// Required to force update FonixData.cdf path in TLS

			// Add logging. Must be done after static constructors.
			((int(*)(void *))(0x105BD10))(&CreationKit::FaceFXLogCallback);
			DetourFunction(0x1C490D0, &CreationKit::LogCallback);

			return true;
		}

		return false;
	}

	bool MapExecutable(GameVersion Version)
	{
		ForceReference();

		HMODULE module = GetModuleHandleA(nullptr);
		uint32_t resourceId = 0;

		if (Version == GameVersion::SkyrimOrEarlier)
			resourceId = IDR_CK_LIP_BINARY1;
		else if (Version == GameVersion::Fallout4)
			resourceId = IDR_CK_LIP_BINARY2;

		// Decompress the embedded exe and then initialize it
		HRSRC resource = FindResourceA(module, MAKEINTRESOURCE(resourceId), "CK_LIP_BINARY");
		uint32_t resourceSize = SizeofResource(module, resource);

		if (!resource || resourceSize <= 0)
		{
			printf("Failed to find embedded CK resource\n");
			return false;
		}

		auto data = reinterpret_cast<unsigned char *>(LockResource(LoadResource(module, resource)));
		auto exeData = new unsigned char[stb_decompress_length(data)];

		if (stb_decompress(exeData, data, resourceSize) == 0)
		{
			printf("CK resource decompression failure\n");
			return false;
		}

		HMEMORYMODULE creationKitExe = MemoryLoadLibraryEx(exeData, stb_decompress_length(data), MmMemoryAlloc, MemoryDefaultFree, MmGetLibrary, MmGetLibraryProcAddr, MemoryDefaultFreeLibrary, nullptr);
		delete[] exeData;

		if (!creationKitExe)
		{
			printf("Failed to map CK executable into memory\n");
			return false;
		}

		return true;
	}

	bool MapTLS(GameVersion Version)
	{
		// Allocate an arbitrary memory region to replace TLS accesses. MemoryModule doesn't handle static TLS slots. These variables are
		// static on purpose.
		static void *tlsRegion = VirtualAlloc(nullptr, 64 * 1024, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		static void **pointerToTlsRegionPointer = &tlsRegion;

		// 64 A1 2C 00 00 00		mov eax, large fs:2Ch
		// 64 8B 0D 2C 00 00 00		mov ecx, large fs:2Ch
		auto patchTLSAccess = [](uintptr_t Address)
		{
			uint8_t data[7];
			memset(data, 0x90, sizeof(data));

			if (*(uint8_t *)(Address + 0x1) == 0xA1)
			{
				data[1] = 0xA1;
				*(void **)&data[2] = &pointerToTlsRegionPointer;

				PatchMemory(Address, data, 6);
			}
			else
			{
				*(uint16_t *)&data[1] = *(uint16_t *)(Address + 0x1);
				*(void **)&data[3] = &pointerToTlsRegionPointer;

				PatchMemory(Address, data, 7);
			}
		};

		if (Version == GameVersion::SkyrimOrEarlier)
		{
#include "LoaderTLS_SK.inl"

			for (uintptr_t addr : TLSPatchAddresses_SK)
				patchTLSAccess(addr);

			return true;
		}
		else if (Version == GameVersion::Fallout4)
		{
#include "LoaderTLS_F4.inl"

			for (uintptr_t addr : TLSPatchAddresses_F4)
				patchTLSAccess(addr);

			return true;
		}

		return false;
	}

	void SaveResourceToDisk()
	{
		auto generateCompressedBin = [](const char *Source, const char *Dest)
		{
			if (FILE *f; fopen_s(&f, Source, "rb") == 0)
			{
				fseek(f, 0, SEEK_END);
				long size = ftell(f);
				rewind(f);

				auto exeData = new char[size];
				fread(exeData, sizeof(char), size, f);

				char temp[1024];
				strcpy_s(temp, Dest);
				stb_compress_tofile(temp, exeData, size);

				delete[] exeData;
				fclose(f);
			}
		};

		generateCompressedBin("E:\\Program Files (x86)\\Steam\\steamapps\\common\\Fallout 4\\Tools\\LipGen\\CreationKit32.exe", "compressed_ck_f4.bin");
		generateCompressedBin("E:\\Program Files (x86)\\Steam\\steamapps\\common\\skyrim\\CreationKit.exe.unpacked.exe", "compressed_ck_sk.bin");
	}

	void ForceReference()
	{
		FORCE_LOAD_REQUIRED_DATA[0] = 0;
		FORCE_LOAD_REQUIRED_DATA[sizeof(FORCE_LOAD_REQUIRED_DATA) - 1] = 0;
	}

	GameVersion GetGameVersion()
	{
		return CurrentGameVersion;
	}

	HCUSTOMMODULE MmGetLibrary(LPCSTR Name, void *Userdata)
	{
		static const char *moduleBlacklist[] =
		{
			"SSCE5432.DLL",
			"SSCE5532.DLL",
			"D3D9.DLL",
			"DSOUND.DLL",
			"STEAM_API.DLL",
			"X3DAUDIO1_7.DLL",
			"DBGHELP.DLL",
			"D3DX9_42.DLL",
			"XINPUT1_3.DLL",
			"GFSDK_GODRAYSLIB.WIN32.DLL",
		};

		// Check for blacklisted DLLs first
		for (auto module : moduleBlacklist)
		{
			if (!_stricmp(Name, module))
				return BAD_DLL;
		}

		return reinterpret_cast<HCUSTOMMODULE>(LoadLibraryA(Name));
	}

	FARPROC MmGetLibraryProcAddr(HCUSTOMMODULE Module, LPCSTR Name, void *Userdata)
	{
		if (Module == BAD_DLL)
			return BAD_IMPORT;

		return MemoryDefaultGetProcAddress(Module, Name, Userdata);
	}

	LPVOID MmMemoryAlloc(LPVOID Address, SIZE_T Size, DWORD AllocationType, DWORD Protect, void *Userdata)
	{
		auto addr = reinterpret_cast<uintptr_t>(Address);

		if (addr >= MinExeVirtualAddress && (addr + Size) <= MaxExeVirtualAddress)
		{
			auto minAddr = reinterpret_cast<uintptr_t>(&FORCE_LOAD_REQUIRED_DATA[0]);
			auto maxAddr = reinterpret_cast<uintptr_t>(&FORCE_LOAD_REQUIRED_DATA[sizeof(FORCE_LOAD_REQUIRED_DATA)]);

			// Sanity check
			if (addr < minAddr)
				__debugbreak();

			if ((addr + Size) >= maxAddr)
				__debugbreak();

			DWORD old;
			VirtualProtect(Address, Size, Protect, &old);

			return Address;
		}

		return VirtualAlloc(Address, Size, AllocationType, Protect);
	}

	void PatchMemory(uintptr_t Address, const uint8_t *Data, size_t Size)
	{
		auto asPointer = reinterpret_cast<void *>(Address);
		DWORD protect = 0;

		VirtualProtect(asPointer, Size, PAGE_EXECUTE_READWRITE, &protect);

		for (uintptr_t i = Address; i < (Address + Size); i++)
			*reinterpret_cast<uint8_t *>(i) = *Data++;

		VirtualProtect(asPointer, Size, protect, &protect);
		FlushInstructionCache(GetCurrentProcess(), asPointer, Size);
	}

	void PatchMemory(uintptr_t Address, std::initializer_list<uint8_t> Data)
	{
		PatchMemory(Address, Data.begin(), Data.size());
	}

	void DetourFunction(uintptr_t Target, uintptr_t Destination, bool Call)
	{
		uint8_t data[5];

		data[0] = Call ? 0xE8 : 0xE9;
		*reinterpret_cast<int32_t *>(&data[1]) = static_cast<int32_t>(Destination - Target) - 5;

		PatchMemory(Target, data, sizeof(data));
	}
}