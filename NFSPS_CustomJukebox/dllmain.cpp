#include "stdafx.h"
#include "stdio.h"
#include <windows.h>
#include <stdlib.h>
#include <strsafe.h>
#include "..\includes\injector\injector.hpp"
#include "..\includes\mINI\src\mini\ini.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

#pragma runtime_checks( "", off )

using namespace std;

#define DEFAULT_PLAYLIST_FOLDER "CustomPlaylist"

#define JUKEBOXDEFAULT_ADDRESS 0x0055FF80

#define DISABLEATTRIBDESTRUCT3_ADDRESS1 0x007E265B
#define DISABLEATTRIBDESTRUCT3_ADDRESS2 0x007E2667

#define PFPARAMS_ADDR 0x00AAED2C

void(__thiscall* Attrib_Gen_Music)(unsigned int dis, uint32_t collectionKey, uint32_t msgPort) = (void(__thiscall*)(unsigned int, uint32_t, uint32_t))0x00509750;
void(__cdecl* FeMusicChyron_QueueChyronMessage)(const char* title, const char* desc1, const char* desc2) = (void(__cdecl*)(const char*, const char*, const char*))0x0059AF30;

// volume boost
#define GAMEFLOW_STATE_ADDR 0xABB510
void(__thiscall* SetDMixInput)(unsigned int dis, int idx, int input) = (void(__thiscall*)(unsigned int, int, int))0x00506650;
int(__cdecl* NFSMixShape_GetCurveOutput)(int nQ15Ratio, int bdBOut, bool unk) = (int(__cdecl*)(int, int, bool))0x00507AC0;
int* DMixInput_FEMusic = NULL;
int* DMixInput_IGMusic = NULL;
int VolumeBoost = 0;
int(__cdecl* NFSMixShape_GetdBFromQ15)(int nQ15) = (int(__cdecl*)(int))0x005079C0;
bool(__thiscall* DALManager_SetFloat)(unsigned int dis, int valueType, float setVal, int arg1, int arg2, int arg3) = (bool(__thiscall*)(unsigned int, int, float, int, int, int))0x00535F40;
void(__thiscall* AudioSettings_LoadData)(unsigned int dis, int something) = (void(__thiscall*)(unsigned int, int))0x00535100;
void(__thiscall* AudioSettings_SaveData)(unsigned int dis, int something) = (void(__thiscall*)(unsigned int, int))0x00535010;
void(__thiscall* AudioSettings_DefaultData)(unsigned int dis, int something) = (void(__thiscall*)(unsigned int, int))0x00558900;

// extended playability flags
#define FEHUBMANAGER_INSTANCE_ADDR 0x00AB2930
#define FEJUKEBOXSCREEN_ADDR 0x00B32B90
int(*FE_String_Printf)(void* FEObject, const char* fmt, ...) = (int(*)(void*, const char*, ...))0x005CE430;
bool(__thiscall* DALManager_IncrementInt)(unsigned int dis, int valueType, int incVal, int arg1, int arg2, int arg3) = (bool(__thiscall*)(unsigned int, int, int, int, int, int))0x007DFCF4;
uint32_t(__thiscall* SFXObj_Music_GenNextMusicTrackID)(unsigned int dis) = (uint32_t(__thiscall*)(unsigned int))0x0050BCC0;
uint32_t(__cdecl* bRandom)(int range) = (uint32_t(__cdecl*)(int))0x00431640;
void(__thiscall* FEHubRootStateManager_StartAutoSaveOnHubEnter)(unsigned int dis) = (void(__thiscall*)(unsigned int))0x006023D0;
void(__thiscall* FEHubRootStateManager_Destructor)(unsigned int dis, unsigned int unk) = (void(__thiscall*)(unsigned int, unsigned int))0x005DC120;
uint32_t(__cdecl* bGetTicker)() = (uint32_t(__cdecl*)())0x430FD0;
void(__thiscall* sub_4ED910)(unsigned int dis, int unk) = (void(__thiscall*)(unsigned int, int))0x4ED910;

bool bInHub = false;
bool bRandomizedPlayback = false;
bool bInitializeRandomly = false;
int IGMusicSequencer = 0;
int FEMusicSequencer = 0;

enum EATraxPlayability
{
	EATRAX_OF,
	EATRAX_FE,
	EATRAX_IG,
	EATRAX_AL,
	EATRAX_MAX
};

enum eMUSIC_FLAGS
{
	NIS321_SET = 1,
	IS_SPLITSCREEN = 2,
	IS_PATH_INITED = 4,
	INTERACTIVE_DONE = 8,
	LOAD_INTERACTIVE = 16,
	MOVIE_ACTIVE = 32,
	CHYRON_SERVICED = 64,
	FADE_TRIGGERED = 128,
	WAIT_4_COMPOSED = 256,
	NIS_ISPLAYING = 512,
	IS_PATH_PAUSED = 1024,
	XENON_USERTUNES = 2048,
	X360_UIOVERRIDE = 4096,
	X360_UI_ISUP = 8192,
	SPLASH_SCREEN = 16384,
	RACE_ABANDONED = 0xffff8000,
	RESTRICTED = 0x10000,
	EATRAX_ON = 0x20000,
	INTERACTIVE_ON = 0x40000,
	JUKESCREEN_UP = 0x80000,
	EVENT_PENDING = 0x100000,
};

struct AudioSettings
{
	 long Padding_83[6];
	 float MasterVol;
	 float SpeechVol;
	 float FEMusicVol;
	 float IGMusicVol;
	 float SoundEffectsVol;
	 float EngineVol;
	 float CarVol;
	 float AmbientVol;
	 float SpeedVol;
	 int AudioMode;
	 int InteractiveMusicMode;
	 int EATraxMode;
	 int UseCarClassSongFiltering;
	 int PlayState;
};

vector<string> FileDirectoryListing;

struct SongAttrib
{
	unsigned int NameStringHash; // 0
	char* TrackName; // 4
	unsigned int FileNameStringHash; // 8
	char* TrackFileName; // C
	unsigned int ArtistStringHash; // 10
	char* TrackArtist; // 14
	unsigned int AlbumStringHash; // 18
	char* TrackAlbum; // 1C
	unsigned int EventID; // 20
};

struct JukeboxEntry
{
	uint32_t SongIndex;
	uint32_t SongKey;
	int8_t PlayabilityField;
	int8_t padding[3];
};

struct parserSongAttrib
{
	SongAttrib attrib;
	int index = 0;
	bool operator < (const parserSongAttrib& s) const
	{
		return (index < s.index);
	}
};

struct parserJukeboxEntry
{
	JukeboxEntry entry;
	bool operator < (const parserJukeboxEntry& s) const
	{
		return (entry.SongIndex < s.entry.SongIndex);
	}
};

struct DefaultPlayabilityEntry
{
	int8_t playability;
	int index = 0;
	bool operator < (const DefaultPlayabilityEntry& s) const
	{
		return (index < s.index);
	}
};

// ingame pointers
SongAttrib* songattribs;
SongAttrib* chyron_attrib = NULL;
JukeboxEntry* entries;
char dummyspace[32];
void* CustomUserProfile = NULL;

SongAttrib DummyTrack = { 0, "No tracks found", 0, "NULL", 0, "Please add tracks to the playlist folder.", 0, "-", 0 };
vector<parserSongAttrib> parser_attribs;
vector<parserJukeboxEntry> parser_entries;
vector<DefaultPlayabilityEntry> DefaultPlayabilityEntries;

char parser_name_str[256];
char parser_artist_str[256];
char parser_album_str[256];

char IniPath[MAX_PATH];
char IniName[64];
//char* PlaylistFolderName = DEFAULT_PLAYLIST_FOLDER;
char PlaylistFolderName[MAX_PATH];

int TrackCount = 0;

// volume boost
int GetCurveOutput_Hook(int nQ15Ratio, int bdBOut, bool unk)
{
	int* InputPointer;
	_asm mov InputPointer, edx

	int retval = NFSMixShape_GetCurveOutput(nQ15Ratio, bdBOut, unk);

	if ((InputPointer == DMixInput_FEMusic) || (InputPointer == DMixInput_IGMusic))
	{
		if (retval == 0xFFEF)
			retval = 0xFFFF;
	}

	return retval;
}

void __stdcall SetDMixInput_Hook(int idx, int input)
{
	uint32_t thethis;
	_asm mov thethis, ecx
	DMixInput_FEMusic = *(int**)(thethis + 8);

	if ((*(int*)GAMEFLOW_STATE_ADDR != 6)  && (input == 0) && (VolumeBoost == 1))
		input -= 1;

	return SetDMixInput(thethis, idx, input);
}

void __stdcall SetDMixInput_Hook_IG(int idx, int input)
{
	uint32_t thethis;
	_asm mov thethis, ecx
	DMixInput_IGMusic = (int*)(*(int*)(thethis + 8) + 4);
	
	if ((input == 0) && (VolumeBoost == 1))
		input -= 1;

	return SetDMixInput(thethis, idx, input);
}

uint32_t mastervol_exit = 0x4F8C7D;
void __declspec(naked) MasterVol_UpdateParams_Cave()
{
	_asm
	{
		push eax
		push 0
		mov ecx, edi
		call SetDMixInput_Hook
		jmp mastervol_exit
	}
}

uint32_t mastervol_exit_IG = 0x4F8CD4;
void __declspec(naked) MasterVol_UpdateParams_Cave_IG()
{
	_asm
	{
		push eax
		push 1
		mov ecx, edi
		call SetDMixInput_Hook_IG
		jmp mastervol_exit_IG
	}
}

void __stdcall AudioSettings_LoadData_Hook(int something)
{
	uint32_t thethis;
	_asm mov thethis, ecx

	AudioSettings* settings = (AudioSettings*)thethis;

	// boundary checks for values to avoid crashing
	AudioSettings_LoadData(thethis, something);
	if ((*settings).MasterVol > 1.0 ||
		(*settings).SpeechVol > 1.0 ||
		(*settings).FEMusicVol > 1.0 ||
		(*settings).IGMusicVol > 1.0 ||
		(*settings).SoundEffectsVol > 1.0 ||
		(*settings).EngineVol > 1.0 ||
		(*settings).CarVol > 1.0 ||
		(*settings).AmbientVol > 1.0 ||
		(*settings).SpeedVol > 1.0)
	{
		AudioSettings_DefaultData(thethis, something);
	}

}

int GetdBFromQ15_Hook(int nQ15)
{
	int* InputPointer;
	_asm
	{
		mov eax, [esi + 4]
		mov InputPointer, eax
	}
	int retval = 0;
	if ((InputPointer == DMixInput_FEMusic) || (InputPointer == DMixInput_IGMusic))
		retval = NFSMixShape_GetdBFromQ15(nQ15 & 0xFFFF); // needed to avoid crashing beyond 100%
	else
		retval = NFSMixShape_GetdBFromQ15(nQ15);

	return retval;
}

bool __stdcall DALManager_SetFloat_Hook(int valueType, float setVal, int arg1, int arg2, int arg3)
{
	uint32_t thethis;
	_asm mov thethis, ecx

	switch (valueType)
	{
	case 0x139B:
	case 0x1397:
	case 0x1390:
		if (setVal > 1.0f)
			setVal = 1.0f;
		break;
	default:
		break;
	}

	return DALManager_SetFloat(thethis, valueType, setVal, arg1, arg2, arg3);
}

SongAttrib* __stdcall GetTrackAttribPointer(unsigned int TrackNumber)
{
	chyron_attrib = &songattribs[TrackNumber];
	return &songattribs[TrackNumber];
}

void __stdcall Attrib_GenMusic(uint32_t collectionKey, uint32_t msgPort)
{
	uint32_t thethis;
	_asm mov thethis, ecx

	Attrib_Gen_Music(thethis, collectionKey, msgPort);
	*(uint32_t*)(thethis) = (uint32_t)dummyspace;

	*(uint32_t*)(thethis + 4) = (uint32_t)GetTrackAttribPointer(collectionKey - 1);

	*(uint32_t*)0x00A4F700 = collectionKey - 1;
}

unsigned int GetNumberOfEATrax()
{
	return TrackCount;
}

void __stdcall SFXObj_Music_NotifyChyron_Hook()
{
	FeMusicChyron_QueueChyronMessage(chyron_attrib->TrackName, chyron_attrib->TrackArtist, chyron_attrib->TrackAlbum);
	*(uint8_t*)(*(uint32_t*)(PFPARAMS_ADDR)+0xE) = 0;
}

void FixWorkingDirectory()
{
	char ExecutablePath[_MAX_PATH];

	GetModuleFileName(GetModuleHandle(""), ExecutablePath, _MAX_PATH);
	ExecutablePath[strrchr(ExecutablePath, '\\') - ExecutablePath] = 0;
	SetCurrentDirectory(ExecutablePath);
}

char* chrStringToUpper(char* str)
{
	for (int i = 0; i < strlen(str); i++)
	{
		str[i] = toupper(str[i]);
	}
	return str;
}

bool bValidateHexString(char* str)
{
	for (int i = 0; i < strlen(str); i++)
	{
		if ((!isdigit(str[i])) &&
			(str[i] != 'A') &&
			(str[i] != 'B') &&
			(str[i] != 'C') &&
			(str[i] != 'D') &&
			(str[i] != 'E') &&
			(str[i] != 'F')
			)
			return false;
	}
	return true;
}

bool bCheckAlreadyAdded(uint32_t ID)
{
	for (int i = 0; i < parser_attribs.size(); i++)
	{
		if (parser_attribs.at(i).attrib.EventID == ID)
			return true;
	}
	return false;
}

void SetDummyTrack()
{
	parserSongAttrib at = { 0 };
	parserJukeboxEntry en = { 0 };
	parser_attribs.clear();
	parser_entries.clear();

	en.entry.SongIndex = 0;
	en.entry.SongKey = 1;
	en.entry.PlayabilityField = 3;

	memcpy(&(at.attrib), &DummyTrack, sizeof(SongAttrib));

	parser_attribs.push_back(at);
	parser_entries.push_back(en);
	TrackCount = 1;
}

DWORD GetDirectoryListing(const char* FolderPath)
{
	WIN32_FIND_DATA ffd = { 0 };
	TCHAR  szDir[MAX_PATH];
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0;
	unsigned int NameCounter = 0;

	FileDirectoryListing.clear();

	StringCchCopy(szDir, MAX_PATH, FolderPath);
	StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

	if (strlen(FolderPath) > (MAX_PATH - 3))
	{
		//printf("Directory path is too long.\n");
		return -1;
	}

	hFind = FindFirstFile(szDir, &ffd);

	if (INVALID_HANDLE_VALUE == hFind)
	{
		//printf("FindFirstFile error\n");
		return dwError;
	}

	do
	{
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			string fname(ffd.cFileName);
			if ((fname.find(".ini") != string::npos))
			{
				FileDirectoryListing.push_back(ffd.cFileName);
			}
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
	{
		//printf("FindFirstFile error\n");
	}
	FindClose(hFind);

	return dwError;
}

void CreatePlaylist()
{
	songattribs = (SongAttrib*)calloc(TrackCount, sizeof(SongAttrib));
	size_t userprofile_size = (sizeof(JukeboxEntry) * TrackCount) + 0x48;
	CustomUserProfile = calloc(1, userprofile_size);
	entries = (JukeboxEntry*)((size_t)CustomUserProfile + 0x48);

	*(uint32_t*)((size_t)CustomUserProfile + 0x38) = 0x970CAC;
	*(uint32_t*)((size_t)CustomUserProfile + 0x3C) = (uint32_t)entries;
	*(uint32_t*)((size_t)CustomUserProfile + 0x40) = TrackCount + 2;
	*(uint32_t*)((size_t)CustomUserProfile + 0x44) = TrackCount + 1;

	for (int i = 0; i < TrackCount; i++)
	{
		memcpy(&songattribs[i], &(parser_attribs.at(i).attrib), sizeof(SongAttrib));
		memcpy(&entries[i], &(parser_entries.at(i).entry), sizeof(JukeboxEntry));
	}

	parser_attribs.clear();
	parser_entries.clear();

	*(uint32_t*)0x00AB0F91 = 1; // song_info_loaded
}

void ParsePlaylistFolder(char* folder)
{
	char* cursor = 0;

	GetDirectoryListing(folder);

	if (FileDirectoryListing.size() <= 0)
	{
		SetDummyTrack();
		return;
	}

	mINI::INIFile userinifile("CustomJukeboxUser.ini");
	mINI::INIStructure userini;
	userinifile.read(userini);

	for (int i = 0; i < FileDirectoryListing.size(); i++)
	{
		// parse the ID from the filename
		strcpy(IniName, FileDirectoryListing.at(i).c_str());
		cursor = strchr(IniName, '.');
		if (cursor)
			*cursor = '\0';

		chrStringToUpper(IniName);

		cursor = IniName;
		if ((IniName[0] == '0') && (IniName[1] == 'x'))
			cursor += 2;

		if (bValidateHexString(cursor))
		{
			parserSongAttrib at = { 0 };
			parserJukeboxEntry en = { 0 };
			DefaultPlayabilityEntry dpe = { 0 };
			uint32_t ID = 0;
			uint32_t idx = 0;

			sscanf(cursor, "%X", &ID);
			sprintf(IniPath, "%s\\%s", folder, FileDirectoryListing.at(i).c_str());

			ID &= 0xFFFFFF;
			ID |= 0x01000000;

			if (!bCheckAlreadyAdded(ID))
			{
				en.entry.SongIndex = TrackCount;
				en.entry.SongKey = TrackCount + 1;

				at.attrib.EventID = ID;

				sprintf(parser_name_str, "Track %d", i + 1);
				sprintf(parser_artist_str, "Event 0x%08X", ID);
				strcpy(parser_album_str, "-");

				mINI::INIFile inifile(IniPath);
				mINI::INIStructure ini;
				inifile.read(ini);

				// these will be re-updated later on during sorting to fill in the gaps if any exist...
				if (ini.has("Entry"))
				{
					if (ini["Entry"].has("Index"))
						idx = stoi(ini["Entry"]["Index"]);
					else
						idx = TrackCount;

					en.entry.SongIndex = idx;
					en.entry.SongKey = idx + 1;

					at.index = idx;
					if (ini["Entry"].has("Name"))
					{
						at.attrib.TrackName = (char*)malloc(strlen(ini["Entry"]["Name"].c_str()));
						strcpy(at.attrib.TrackName, ini["Entry"]["Name"].c_str());
					}
					else
					{
						at.attrib.TrackName = (char*)malloc(strlen(parser_name_str) + 1);
						strcpy(at.attrib.TrackName, parser_name_str);
					}

					if (ini["Entry"].has("Artist"))
					{
						at.attrib.TrackArtist = (char*)malloc(strlen(ini["Entry"]["Artist"].c_str()));
						strcpy(at.attrib.TrackArtist, ini["Entry"]["Artist"].c_str());
					}
					else
					{
						at.attrib.TrackArtist = (char*)malloc(strlen(parser_artist_str) + 1);
						strcpy(at.attrib.TrackArtist, parser_artist_str);
					}

					if (ini["Entry"].has("Album"))
					{
						at.attrib.TrackAlbum = (char*)malloc(strlen(ini["Entry"]["Album"].c_str()));
						strcpy(at.attrib.TrackAlbum, ini["Entry"]["Album"].c_str());
					}
					else
					{
						at.attrib.TrackAlbum = (char*)malloc(strlen(parser_album_str) + 1);
						strcpy(at.attrib.TrackAlbum, parser_album_str);
					}

					if (ini["Entry"].has("Playability"))
					{
						en.entry.PlayabilityField = (int8_t)(stoi(ini["Entry"]["Playability"]) & 0xFF);
					}
					else
						en.entry.PlayabilityField = 3;

					dpe.playability = en.entry.PlayabilityField;
					dpe.index = en.entry.SongIndex;

					if (userini.has("PlayabilityFields"))
					{
						sprintf(parser_name_str, "%08X", ID);
						if (userini["PlayabilityFields"].has(parser_name_str))
							en.entry.PlayabilityField = (int8_t)(stoi(userini["PlayabilityFields"][parser_name_str]) & 0xFF);
					}
				}
				else
				{
					en.entry.SongIndex = TrackCount;
					en.entry.SongKey = TrackCount + 1;
					at.attrib.TrackName = (char*)malloc(strlen(parser_name_str) + 1);
					strcpy(at.attrib.TrackName, parser_name_str);
					at.attrib.TrackArtist = (char*)malloc(strlen(parser_artist_str) + 1);
					strcpy(at.attrib.TrackArtist, parser_artist_str);
					at.attrib.TrackAlbum = (char*)malloc(strlen(parser_album_str) + 1);
					strcpy(at.attrib.TrackAlbum, parser_album_str);
					en.entry.PlayabilityField = 3;
					dpe.playability = en.entry.PlayabilityField;
					dpe.index = en.entry.SongIndex;
				}

				parser_attribs.push_back(at);
				parser_entries.push_back(en);
				DefaultPlayabilityEntries.push_back(dpe);
				TrackCount++;
			}
		}
	}

	if (TrackCount <= 0)
	{
		SetDummyTrack();
		return;
	}

	// sort all the entries and attribs by their vector indicies
	sort(parser_attribs.begin(), parser_attribs.end());
	sort(parser_entries.begin(), parser_entries.end());
	sort(DefaultPlayabilityEntries.begin(), DefaultPlayabilityEntries.end());

	// then fill in the gaps in the entries by recounting the indicies
	for (int i = 0; i < TrackCount; i++)
	{
		parser_entries.at(i).entry.SongIndex = i;
		parser_entries.at(i).entry.SongKey = i + 1;
		DefaultPlayabilityEntries.at(i).index = i;
	}
}

// Reads back the default values into memory & empties the CustomJukeboxUser.ini file
void JukeboxStateManager_HandleResetToDefault_Hook()
{
	uint32_t FeJukeboxScreen = *(uint32_t*)FEJUKEBOXSCREEN_ADDR;
	if (!FeJukeboxScreen)
		return;
	for (int i = 0; i < TrackCount; i++)
		entries[i].PlayabilityField = DefaultPlayabilityEntries.at(i).playability;
	FILE* fini = fopen("CustomJukeboxUser.ini", "wb");
	if (fini)
	{
		fprintf(fini, "[PlayabilityFields]\r\n");
		fclose(fini);
	}

	void(__thiscall * FeJukeboxScreen_RefreshHeader)(unsigned int dis) = (void(__thiscall*)(unsigned int))*(uint32_t*)(*(uint32_t*)(FeJukeboxScreen + 0x20) + 8);

	return FeJukeboxScreen_RefreshHeader(FeJukeboxScreen + 0x20);
}

void __stdcall SetPlayability(uint32_t ID, int8_t playability)
{
	mINI::INIFile* inifile = new mINI::INIFile("CustomJukeboxUser.ini");
	mINI::INIStructure ini;
	char playability_str[3] = { 0 };
	char id_str[9] = { 0 };

	inifile->read(ini);
	sprintf(id_str, "%08X", ID);
	itoa(playability, playability_str, 10);
	ini["PlayabilityFields"][id_str] = playability_str;
	inifile->write(ini, true);

	delete inifile;
}

// extended playability flags
void __stdcall FEHubRootStateManager_StartAutoSaveOnHubEnter_Hook()
{
	uint32_t that;
	_asm mov that, ecx

	bInHub = true;

	return FEHubRootStateManager_StartAutoSaveOnHubEnter(that);
}

void __stdcall FEHubRootStateManager_Destructor_Hook(unsigned int unk)
{
	uint32_t that;
	_asm mov that, ecx

	bInHub = false;

	return FEHubRootStateManager_Destructor(that, unk);
}

void SetLanguageHash_Hook(void* FEObject, int hash)
{
	// catch the flag from registers
	uint8_t flag = 0;
	_asm mov al, [edi + eax + 8]
	_asm mov flag, al

	switch (flag)
	{
	case 3:
		FE_String_Printf(FEObject, "All");
		break;
	case 2:
		FE_String_Printf(FEObject, "Racing");
		break;
	case 1:
		FE_String_Printf(FEObject, "Menu");
		break;
	case 0:
		FE_String_Printf(FEObject, "Off");
		break;
	default:
		FE_String_Printf(FEObject, "On");
		break;
	}
}

bool __stdcall DALManager_IncrementInt_Hook(int valueType, int incVal, int arg1, int arg2, int arg3)
{
	int8_t flag = entries[arg1].PlayabilityField;

	flag += incVal;
	if (flag < 0)
		flag = 3;
	flag %= 4;

	entries[arg1].PlayabilityField = flag;

	SetPlayability(songattribs[entries[arg1].SongIndex].EventID, flag);

	return true;
}

uint32_t __stdcall SFXObj_Music_GenNextMusicTrackID_Custom()
{
	uint32_t that;
	_asm mov that, ecx

	uint32_t GameflowStatus = *(uint32_t*)GAMEFLOW_STATE_ADDR;
	uint32_t result = 0;

	// failsafe detect if we're in a hub
	bool bInHub2 = ((*(uint32_t*)FEHUBMANAGER_INSTANCE_ADDR) != 0);

	if (bRandomizedPlayback)
	{
		// build a list of allowed tracks in the current mode
		vector<JukeboxEntry> allowedentries;
		for (int i = 0; i < TrackCount; i++)
		{
			if (entries[i].PlayabilityField)
			{
				if (entries[i].PlayabilityField == EATRAX_AL)
					allowedentries.push_back(entries[i]);
				else
				{
					if (GameflowStatus == 6)
					{
						if (bInHub || bInHub2)
						{
							if ((entries[i].PlayabilityField == EATRAX_FE))
								allowedentries.push_back(entries[i]);
						}
						else if ((entries[i].PlayabilityField == EATRAX_IG))
							allowedentries.push_back(entries[i]);
					}
					else
					{
						if ((entries[i].PlayabilityField == EATRAX_FE))
							allowedentries.push_back(entries[i]);
					}
				}
			}
		}

		if (allowedentries.size() <= 0)
		{
			*(uint32_t*)(that + 0xAC) |= RESTRICTED;
			return 0;
		}

		// get a random entry
		result = allowedentries.at(bRandom(allowedentries.size())).SongKey;

		allowedentries.clear();
	}
	else
	{
		bool bFoundASong = false;
		IGMusicSequencer %= TrackCount;
		FEMusicSequencer %= TrackCount;

		// go through songs and find the first one that's playable
		if (GameflowStatus == 6)
		{
			if (bInHub || bInHub2)
			{
				for (int i = FEMusicSequencer; i < TrackCount; i++)
				{
					FEMusicSequencer = i;
					if ((entries[i].PlayabilityField == EATRAX_FE) || (entries[i].PlayabilityField == EATRAX_AL))
					{
						result = entries[i].SongKey;
						bFoundASong = true;
						break;
					}
				}
				FEMusicSequencer++;
				FEMusicSequencer %= TrackCount;
			}
			else
			{
				for (int i = IGMusicSequencer; i < TrackCount; i++)
				{
					IGMusicSequencer = i;
					if ((entries[i].PlayabilityField == EATRAX_IG) || (entries[i].PlayabilityField == EATRAX_AL))
					{
						result = entries[i].SongKey;
						bFoundASong = true;
						break;
					}
				}
				IGMusicSequencer++;
			}
		}
		else
		{
			for (int i = FEMusicSequencer; i < TrackCount; i++)
			{
				FEMusicSequencer = i;
				if ((entries[i].PlayabilityField == EATRAX_FE) || (entries[i].PlayabilityField == EATRAX_AL))
				{
					result = entries[i].SongKey;
					bFoundASong = true;
					break;
				}
			}
			FEMusicSequencer++;
		}

		if (!bFoundASong)
		{
			*(uint32_t*)(that + 0xAC) |= RESTRICTED;
			return 0;
		}
	}


	*(uint32_t*)(that + 0xD0) = 1;
	*(uint32_t*)(that + 0xAC) &= ~RESTRICTED;

	// DELETE PREVIOUSLY PLAYED PATHEVENT!!! BUGFIX FOR STOCK GAME! without this it can't play the same song twice!
	*(uint32_t*)(that + 0xB4) = 0;
	return result;
}

void InitConfig()
{
	//CIniReader ini("");
	//PlaylistFolderName = ini.ReadString("MAIN", "PlaylistFolder", DEFAULT_PLAYLIST_FOLDER);
	mINI::INIFile inifile("NFSPS_CustomJukebox.ini");
	mINI::INIStructure ini;
	inifile.read(ini);

	if (ini.has("MAIN"))
	{
		if (ini["MAIN"].has("PlaylistFolder"))
			strcpy(PlaylistFolderName, ini["MAIN"]["PlaylistFolder"].c_str());
		else
			strcpy(PlaylistFolderName, DEFAULT_PLAYLIST_FOLDER);

		if (ini["MAIN"].has("VolumeBoost"))
			VolumeBoost = stoi(ini["MAIN"]["VolumeBoost"].c_str());
		else
			VolumeBoost = 0;
		if (ini["MAIN"].has("RandomizedPlayback"))
			bRandomizedPlayback = (stoi(ini["MAIN"]["RandomizedPlayback"].c_str()) != 0);
		if (ini["MAIN"].has("InitializeRandomly"))
			bInitializeRandomly = (stoi(ini["MAIN"]["InitializeRandomly"].c_str()) != 0);
	}
	else
	{
		strcpy(PlaylistFolderName, DEFAULT_PLAYLIST_FOLDER);
		VolumeBoost = 0;
	}
	FixWorkingDirectory();
	ParsePlaylistFolder(PlaylistFolderName);
}

void __stdcall NullSub(int something)
{
	return;
}

void Init()
{
	CreatePlaylist();

	// playlist injection start
	injector::MakeCALL(0x50C06E, GetNumberOfEATrax, true);
	injector::MakeCALL(0x50C0E6, GetNumberOfEATrax, true);
	injector::MakeCALL(0x50C8D8, GetNumberOfEATrax, true);
	injector::MakeCALL(0x50C92C, GetNumberOfEATrax, true);
	injector::WriteMemory<uint32_t>(0x560605, (uint32_t)&GetNumberOfEATrax, true);

//	injector::MakeRET(JUKEBOXDEFAULT_ADDRESS, 0, true);
//	injector::MakeRET(0x00547620, 4, true);
//	injector::MakeRET(0x005475B0, 4, true);


	injector::WriteMemory<uint32_t>(0x00970C98, (uint32_t)&NullSub, true);
	injector::WriteMemory<uint32_t>(0x00970C9C, (uint32_t)&NullSub, true);
	injector::WriteMemory<uint32_t>(0x00970CA0, (uint32_t)&NullSub, true);
	injector::WriteMemory<uint32_t>(0x00970CA4, (uint32_t)&NullSub, true);
//	injector::MakeJMP(0x56084F, 0x56087B, true);

	injector::MakeNOP(0x007EAE96, 3, true);
	
	
	injector::MakeNOP(DISABLEATTRIBDESTRUCT3_ADDRESS1, 4, true);
	injector::MakeNOP(DISABLEATTRIBDESTRUCT3_ADDRESS2, 5, true);

	// disable localized strings for jukebox screen
	injector::MakeNOP(0x007E25AF, 2, true); 
	// fix the chyron
	injector::MakeCALL(0x00516D1B, SFXObj_Music_NotifyChyron_Hook, true);

	injector::WriteMemory<int*>(0x50C239, &TrackCount, true);
	injector::WriteMemory<int*>(0x0050BD0E, &TrackCount, true);
	injector::WriteMemory<int*>(0x0050BD3E, &TrackCount, true);
	injector::WriteMemory<int*>(0x50BDDE, &TrackCount, true);
	injector::WriteMemory<int*>(0x50C14C, &TrackCount, true);
	injector::WriteMemory<int*>(0x50C239, &TrackCount, true);
	injector::WriteMemory<int*>(0x50C239, &TrackCount, true);
	injector::WriteMemory<int*>(0x1E5F00C, &TrackCount, true);

	//injector::MakeJMP(0x01E5F02B, 0x1E5F082, true);

	// hook attrib music stuff and take control
	injector::MakeCALL(0x50C183, Attrib_GenMusic, true);
	injector::MakeCALL(0x50C286, Attrib_GenMusic, true);
	injector::MakeCALL(0x50CC90, Attrib_GenMusic, true);
	injector::MakeCALL(0x7E24FD, Attrib_GenMusic, true);

	injector::MakeJMP(0x19F5ADF, Attrib_GenMusic, true);
	injector::MakeJMP(0x19F77C7, Attrib_GenMusic, true);
	injector::MakeJMP(0x1E0305C, Attrib_GenMusic, true);

	// kill destructors
	injector::MakeNOP(0x0050C2A4, 5, true);
	injector::MakeNOP(0x0050C2DA, 5, true);
	injector::MakeNOP(0x0050C224, 5, true);
	injector::MakeNOP(0x0050CCF2, 5, true);
	injector::MakeNOP(0x007E2667, 5, true);
	injector::MakeNOP(0x007E2667, 5, true);
	injector::MakeNOP(0x019F5AFC, 5, true);
	injector::MakeNOP(0x019F78E8, 5, true);

	injector::WriteMemory<void**>(0x0050C160, &CustomUserProfile, true);
	injector::WriteMemory<void**>(0x0050BD01, &CustomUserProfile, true);

	injector::WriteMemory<void**>(0x7E24DC, &CustomUserProfile, true);
	injector::WriteMemory<void**>(0x7EAE5A, &CustomUserProfile, true);

	injector::WriteMemory<void**>(0x4CB544, &CustomUserProfile, true);

	injector::WriteMemory<void**>(0x07DFB47, &CustomUserProfile, true);

	injector::WriteMemory<void**>(0x19F7759, &CustomUserProfile, true);
	injector::WriteMemory<void**>(0x1E03026, &CustomUserProfile, true);
	injector::WriteMemory<void**>(0x1E5F032, &CustomUserProfile, true);

	// song playability updater
	//injector::MakeJMP(0x004CB59C, PlayabilityCave, true);

	// fix ingame music to play consistently
	injector::MakeJMP(0x0050C12A, 0x50C253, true);
	
	// volume boost
	switch (VolumeBoost)
	{
	case 2:
		injector::MakeJMP(0x004F8C73, MasterVol_UpdateParams_Cave, true);
		injector::MakeJMP(0x004F8CCA, MasterVol_UpdateParams_Cave_IG, true);
		injector::MakeCALL(0x00525249, GetdBFromQ15_Hook, true);
		injector::MakeCALL(0x005EB99A, DALManager_SetFloat_Hook, true); // limits the max values of other settings affected by the unlock below
		injector::WriteMemory<uint8_t>(0x005D1677, (uint8_t)0xEB, true); // unlocks maximums in the option widgets
		break;
	case 1:
		injector::MakeJMP(0x004F8C73, MasterVol_UpdateParams_Cave, true);
		injector::MakeJMP(0x004F8CCA, MasterVol_UpdateParams_Cave_IG, true);
		injector::MakeCALL(0x00525240, GetCurveOutput_Hook, true);
	default:
		injector::WriteMemory<uint32_t>(0x009709D8, (uint32_t)&AudioSettings_LoadData_Hook, true);
		break;
	}

	// extended playability flags
	// UI text stuff
	injector::MakeCALL(0x007E2653, SetLanguageHash_Hook, true);
	// DALManager incrementors
	injector::MakeCALL(0x007DFCF4, DALManager_IncrementInt_Hook, true);
	injector::MakeCALL(0x007DFD54, DALManager_IncrementInt_Hook, true);
	// Next track RNG
	injector::MakeCALL(0x0050C27B, SFXObj_Music_GenNextMusicTrackID_Custom, true);
	FEHubRootStateManager_StartAutoSaveOnHubEnter = (void(__thiscall*)(unsigned int))*(uint32_t*)0x0097AA7C;
	FEHubRootStateManager_Destructor = (void(__thiscall*)(unsigned int, unsigned int))*(uint32_t*)0x0097A934;
	injector::WriteMemory<uint32_t>(0x0097AA7C, (uint32_t)&FEHubRootStateManager_StartAutoSaveOnHubEnter_Hook, true);
	injector::WriteMemory<uint32_t>(0x0097A934, (uint32_t)&FEHubRootStateManager_Destructor_Hook, true);
	// Reset to default
	injector::MakeCALL(0x007EAE29, JukeboxStateManager_HandleResetToDefault_Hook, true);

	if (!bRandomizedPlayback && bInitializeRandomly)
	{
		// randomize the initial sequencer
		// set the seed
		uint32_t(__cdecl* bGetTicker)() = (uint32_t(__cdecl*)())0x430FD0;
		*(uint32_t*)0x00A9C904 = (uint32_t)(bGetTicker());
		// randomize
		IGMusicSequencer = bRandom(TrackCount);
		FEMusicSequencer = bRandom(TrackCount);
	}
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		InitConfig();
		Init();
	}
	return TRUE;
}

