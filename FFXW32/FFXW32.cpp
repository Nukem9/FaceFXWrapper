#include <windows.h>
#include <stdio.h>
#include <thread>
#include <atomic>
#include <array>
#include "Loader.h"
#include "CreationKit32.h"
#include "LipSynchAnim.h"

std::atomic_uint32_t g_CreationKitPID;

bool RunLipGeneration(const char *Language, const char *FonixDataPath, const char *WavPath, const char *ResampledWavPath, const char *LipPath, const char *Text, bool Resample)
{
	CreationKit::SetFaceFXDataPath(FonixDataPath);
	CreationKit::SetFaceFXLanguage(Language);
	CreationKit::SetFaceFXAutoResampling(Resample);

	auto lipAnim = LipSynchAnim::Generate(WavPath, ResampledWavPath, Text);

	if (!lipAnim)
		return false;

	bool result = lipAnim->SaveToFile(LipPath, true, true);

	lipAnim->Free();
	return result;
}

void IPCExitNotificationThread()
{
	// Grab a handle to the parent process and check for termination
	HANDLE parentProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, g_CreationKitPID);

	if (!parentProcess)
	{
		printf("A CreationKit parent process ID (0x%X) was supplied, but wasn't able to be queried (%d).\n", g_CreationKitPID.load(), GetLastError());
		return;
	}

	for (DWORD exitCode = 0;; Sleep(2000))
	{
		if (g_CreationKitPID == 0)
			break;

		if (GetExitCodeProcess(parentProcess, &exitCode) && exitCode == STILL_ACTIVE)
			continue;

		g_CreationKitPID = 0;
		break;
	}

	CloseHandle(parentProcess);
}

int StartCreationKitIPC(uint32_t ProcessID)
{
	// Disable any kind of buffering when using printf or related functions
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	// Establish tunnel
	char temp[128];

	sprintf_s(temp, "CkSharedMem%d", ProcessID);
	HANDLE mapping = OpenFileMappingA(FILE_MAP_ALL_ACCESS, TRUE, temp);

	if (!mapping)
	{
		printf("Could not create file mapping object (%d).\n", GetLastError());
		return 1;
	}

	auto tunnel = reinterpret_cast<CreationKit::LipGenTunnel *>(MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0x40000));

	if (!tunnel)
	{
		printf("Could not map view of file (%d).\n", GetLastError());

		CloseHandle(mapping);
		return 1;
	}

	sprintf_s(temp, "CkNotifyEvent%d", ProcessID);
	HANDLE notifyEvent = OpenEventA(EVENT_ALL_ACCESS, TRUE, temp);

	sprintf_s(temp, "CkWaitEvent%d", ProcessID);
	HANDLE waitEvent = OpenEventA(EVENT_ALL_ACCESS, TRUE, temp);

	if (!notifyEvent || !waitEvent)
	{
		printf("Could not open event handle(s) (%d).\n", GetLastError());

		UnmapViewOfFile(tunnel);
		CloseHandle(mapping);
		return 1;
	}

	// Thread to check for parent process exit
	std::thread exitThread(IPCExitNotificationThread);
	exitThread.detach();

	// Wait until the creation kit asks to do something, then lazy initialize the loader
	printf("FaceFXWrapper IPC established\n");

	g_CreationKitPID = ProcessID;
	bool loaderInitialized = false;

	while (true)
	{
		DWORD waitStatus = WaitForSingleObject(notifyEvent, 2500);

		if (waitStatus == WAIT_FAILED)
		{
			printf("Failed waiting for CK64 notification (%d).\n", GetLastError());
			break;
		}

		// If the parent process exited, bail
		if ((waitStatus == WAIT_OBJECT_0 && strlen(tunnel->InputWAVPath) <= 0) || g_CreationKitPID == 0)
			break;

		if (waitStatus == WAIT_TIMEOUT)
			continue;

		if (!std::exchange(loaderInitialized, true))
		{
			if (!Loader::Initialize(Loader::GameVersion::SkyrimOrEarlier))
				break;
		}

		printf("Attempting to create LIP file:\n");
		printf("     Input WAV: '%s'\n", tunnel->InputWAVPath);
		printf("     Resampled input WAV: '%s'\n", tunnel->ResampleTempWAVPath);
		printf("     Text: '%s'\n", tunnel->DialogueText);
		printf("     Fonix data: '%s'\n", tunnel->FonixDataPath);
		printf("     Language: '%s'\n", tunnel->Language);

		const char *lipPath = "Data\\Sound\\Voice\\Processing\\Temp.lip";
		printf("Writing temporary LIP file to '%s'\n", lipPath);

		if (!RunLipGeneration(tunnel->Language, tunnel->FonixDataPath, tunnel->InputWAVPath, tunnel->ResampleTempWAVPath, lipPath, tunnel->DialogueText, true))
			printf("Unable to generate LIP data or unable to save to '%s'!\n", lipPath);

		memset(tunnel->InputWAVPath, 0, sizeof(tunnel->InputWAVPath));
		tunnel->UnknownStatus = true;

		// Done
		SetEvent(waitEvent);
		WaitForSingleObject(notifyEvent, INFINITE);
		SetEvent(waitEvent);
	}

	printf("FaceFXWrapper IPC shutdown\n");

	CloseHandle(notifyEvent);
	CloseHandle(waitEvent);
	UnmapViewOfFile(tunnel);
	CloseHandle(mapping);

	g_CreationKitPID = 0;
	return 0;
}

int StartCommandLine()
{
	auto initVersionFromArgv = []()
	{
		if (!_stricmp(__argv[1], "Skyrim"))
			return Loader::Initialize(Loader::GameVersion::SkyrimOrEarlier);

		if (!_stricmp(__argv[1], "Fallout4"))
			return Loader::Initialize(Loader::GameVersion::Fallout4);

		printf("Unknown generator type \"%s\"\n", __argv[1]);
		return false;
	};

	switch (__argc)
	{
	case 7:
		if (!initVersionFromArgv())
			return 1;

		// Resampling disabled - use same path for WavPath and ResampledWavPath
		if (!RunLipGeneration(__argv[2], __argv[3], __argv[4], __argv[4], __argv[5], __argv[6], false))
		{
			printf("LIP generation failed\n");
			return 1;
		}

		return 0;

	case 8:
		if (!initVersionFromArgv())
			return 1;

		if (!RunLipGeneration(__argv[2], __argv[3], __argv[4], __argv[5], __argv[6], __argv[7], true))
		{
			printf("LIP generation failed\n");
			return 1;
		}

		return 0;

	default:
		printf("\n\nUsage:\n");
		printf("\tFaceFXWrapper [Type] [Lang] [FonixDataPath] [WavPath] [ResampledWavPath] [LipPath] [Text]\n");
		printf("\tFaceFXWrapper [Type] [Lang] [FonixDataPath] [ResampledWavPath] [LipPath] [Text]\n");
		printf("\n");
		printf("Examples:\n");
		printf("\tFaceFXWrapper \"Skyrim\" \"USEnglish\" \"C:\\FonixData.cdf\" \"C:\\input.wav\" \"C:\\input_resampled.wav\" \"C:\\output.lip\" \"Blah Blah Blah\"\n");
		printf("\tFaceFXWrapper \"Fallout4\" \"USEnglish\" \"C:\\FonixData.cdf\" \"C:\\input_resampled.wav\" \"C:\\output.lip\" \"Blah Blah Blah\"\n");
		printf("\n");

		return 1;
	}

	return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// Create the IPC tunnel if this was launched from CreationKit.exe
	if (const char *pid = getenv("Ckpid"); pid && strlen(pid) > 0)
	{
		int result = StartCreationKitIPC(atoi(pid));
		return result;
	}

	// Use command line processing instead
	std::array<FILE *, 3> handles {};
	bool attached = AttachConsole(ATTACH_PARENT_PROCESS);

	if (attached)
	{
		handles[0] = freopen("CONIN$", "r", stdin);
		handles[1] = freopen("CONOUT$", "w", stdout);
		handles[2] = freopen("CONOUT$", "w", stderr);
	}

	int result = StartCommandLine();

	if (attached)
	{
		for (auto handle : handles)
		{
			if (handle)
				fclose(handle);
		}

		// Prevent the parent console from hanging after main returns
		SendMessageA(GetConsoleWindow(), WM_CHAR, VK_RETURN, 0);
		FreeConsole();
	}

	return result;
}