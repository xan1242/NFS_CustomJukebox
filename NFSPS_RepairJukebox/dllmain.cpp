#include "stdafx.h"
#include "stdio.h"
#include <windows.h>
#include "..\includes\injector\injector.hpp"

void(__thiscall* Jukebox_LoadData)(unsigned int dis, int something) = (void(__thiscall*)(unsigned int, int))0x00547620;
void(__thiscall* Jukebox_SaveData)(unsigned int dis, int something) = (void(__thiscall*)(unsigned int, int))0x005475B0;
void(__thiscall* Jukebox_DefaultData)(unsigned int dis, int something) = (void(__thiscall*)(unsigned int, int))0x00565260;

void(__thiscall* AudioSettings_LoadData)(unsigned int dis, int something) = (void(__thiscall*)(unsigned int, int))0x00535100;
void(__thiscall* AudioSettings_SaveData)(unsigned int dis, int something) = (void(__thiscall*)(unsigned int, int))0x00535010;
void(__thiscall* AudioSettings_DefaultData)(unsigned int dis, int something) = (void(__thiscall*)(unsigned int, int))0x00558900;

bool bFileExists(const char* filename)
{
	FILE* f = fopen(filename, "rb");
	if (!f)
		return false;
	fclose(f);
	return true;
}

void FixWorkingDirectory()
{
	char ExecutablePath[MAX_PATH];

	GetModuleFileName(GetModuleHandle(""), ExecutablePath, MAX_PATH);
	ExecutablePath[strrchr(ExecutablePath, '\\') - ExecutablePath] = 0;
	SetCurrentDirectory(ExecutablePath);
}

void __stdcall Jukebox_LoadDefaults(int something)
{
	uint32_t thethis;
	_asm mov thethis, ecx

	* (uint32_t*)0x00AB0F91 = false;
	Jukebox_DefaultData(thethis, something);
}

void __stdcall AudioSettings_LoadDefaults(int something)
{
	uint32_t thethis;
	_asm mov thethis, ecx

	AudioSettings_DefaultData(thethis, something);
}

void Init()
{
	FixWorkingDirectory();
	if (bFileExists("NFSPS_CustomJukebox.asi") || bFileExists("plugins\\NFSPS_CustomJukebox.asi") || bFileExists("scripts\\NFSPS_CustomJukebox.asi"))
		return;

	// force load the defaults to heal the data
	injector::WriteMemory<uint32_t>(0x00970CA0, (uint32_t)&Jukebox_LoadDefaults, true);
	injector::WriteMemory<uint32_t>(0x009709D8, (uint32_t)&AudioSettings_LoadDefaults, true);
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;
}

