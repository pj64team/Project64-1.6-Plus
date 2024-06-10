/*
 * Project 64 - A Nintendo 64 emulator.
 *
 * (c) Copyright 2001 zilmar (zilmar@emulation64.com) and 
 * Jabo (jabo@emulation64.com).
 *
 * pj64 homepage: www.pj64.net
 *
 * Permission to use, copy, modify and distribute Project64 in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Project64 is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Project64 or software derived from Project64.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so if they want them.
 *
 */

#include <Windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <stdio.h>
#include "main.h"
#include "cpu.h"
#include "plugin.h"
#include "resource.h"
#include "RomTools_Common.h"
#include "Settings Api_2.h"

#define NoOfSortKeys	3

typedef struct {
	char     szFullFileName[MAX_PATH+1];
	char     Status[60];
	char     FileName[200];
	char     InternalName[22];

	// This changes GoodName to GameGame

	char     GameName[200];
	char     CartID[3];
	char     PluginNotes[250];
	char     CoreNotes[250];
	char     UserNotes[250];
	char     Developer[30];
	char     ReleaseDate[30];
	char     Genre[15];
	int		 Players;
	int      RomSize;
	BYTE     Manufacturer;
	BYTE     Country;
	DWORD    CRC1;
	DWORD    CRC2;
	int      CicChip;
	char     ForceFeedback[15];
	char	 GameInfoID[250];

} ROM_INFO;

typedef struct {
	BYTE     Country;
	DWORD    CRC1;
	DWORD    CRC2;
	long     Fpos;
} ROM_LIST_INFO;

typedef struct {
	int    ListCount;
	int    ListAlloc;
	ROM_INFO * List;
} ITEM_LIST;

typedef struct {
	int    ListCount;
	int    ListAlloc;
	ROM_LIST_INFO * List;
} ROM_LIST;

typedef struct {
	int    Key[NoOfSortKeys];
	BOOL   KeyAscend[NoOfSortKeys];
} SORT_FIELDS;

typedef struct {
	char *status_name;
	COLORREF HighLight;
	COLORREF Text;
	COLORREF SelectedText;
} COLOR_ENTRY;

typedef struct {
	COLOR_ENTRY *List;
	int count;
	int max_allocated;
} COLOR_CACHE;

#define RB_FileName			0
#define RB_InternalName		1

// This changes define RD_GoodName to RB_GameGame
#define RB_GameName			2
#define RB_Status			3
#define RB_RomSize			4
#define RB_CoreNotes		5
#define RB_PluginNotes		6
#define RB_UserNotes		7
#define RB_CartridgeID		8
#define RB_Manufacturer		9
#define RB_Country			10
#define RB_Developer		11
#define RB_Crc1				12
#define RB_Crc2				13
#define RB_CICChip			14
#define RB_ReleaseDate		15
#define RB_Genre			16
#define RB_Players			17
#define RB_ForceFeedback	18
#define RB_GameInfoID		19

char * GetSortField          ( int Index );
void LoadRomList             ( void );
void RomList_SortList        ( void );
void FillRomExtensionInfo    ( ROM_INFO * pRomInfo );
BOOL FillRomInfo             ( ROM_INFO * pRomInfo );
void SetSortAscending        ( BOOL Ascending, int Index );
void SetSortField            ( char * FieldName, int Index );
void SaveRomList             ( void );

#define COLOR_TEXT 0
#define COLOR_SELECTED_TEXT 1
#define COLOR_HIGHLIGHTED 2

void AddToColorCache(COLOR_ENTRY color);
COLORREF GetColor(char *status, int selection);
int ColorIndex(char *status);
void ClearColorCache();
void SetColors(char *status);

int CALLBACK RomList_CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
int CALLBACK RomList_CompareItems2(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
char CurrentRBFileName[MAX_PATH+1] = {""};

ROMBROWSER_FIELDS RomBrowserFields[] =
{
	"File Name",              -1, RB_FileName,      218,RB_FILENAME,
	"Internal Name",          -1, RB_InternalName,  200,RB_INTERNALNAME,

// This changes Good Name to "Game Game

	"Game Name",               0, RB_GameName,      218,RB_GAMENAME,
	"Status",                  1, RB_Status,        92,RB_STATUS,
	"Rom Size",               -1, RB_RomSize,       100,RB_ROMSIZE,
	"Notes (Core)",            2, RB_CoreNotes,     120,RB_NOTES_CORE,
	"Notes (default plugins)", 3, RB_PluginNotes,   188,RB_NOTES_PLUGIN,
	"Notes (User)",           -1, RB_UserNotes,     100,RB_NOTES_USER,
	"Cartridge ID",           -1, RB_CartridgeID,   100,RB_CART_ID,
	"Manufacturer",           -1, RB_Manufacturer,  100,RB_MANUFACTUER,
	"Country",                -1, RB_Country,       100,RB_COUNTRY,
	"Developer",              -1, RB_Developer,     100,RB_DEVELOPER,
	"CRC1",                   -1, RB_Crc1,          100,RB_CRC1,
	"CRC2",                   -1, RB_Crc2,          100,RB_CRC2,
	"CIC Chip",               -1, RB_CICChip,       100,RB_CICCHIP,
	"Release Date",           -1, RB_ReleaseDate,   100,RB_RELEASE_DATE,
	"Genre",                  -1, RB_Genre,         100,RB_GENRE,
	"Players",                -1, RB_Players,       100,RB_PLAYERS,
	"Force Feedback",          4, RB_ForceFeedback, 100,RB_FORCE_FEEDBACK,
	"Game Info ID",			  -1, RB_GameInfoID,    100,RB_GAME_INFO_ID,
};

HWND hRomList= NULL;
int NoOfFields = sizeof(RomBrowserFields) / sizeof(RomBrowserFields[0]),
 FieldType[(sizeof(RomBrowserFields) / sizeof(RomBrowserFields[0])) + 1];
int DefaultSortField = 2;

ITEM_LIST ItemList = {0,0,NULL};
COLOR_CACHE ColorCache;

void AddRomToList (char * RomLocation) {
	LV_ITEM  lvItem;
	ROM_INFO * pRomInfo;
	int index;

	if (ItemList.ListAlloc == 0) {
		ItemList.List = (ROM_INFO *)malloc(100 * sizeof(ROM_INFO));
		ItemList.ListAlloc = 100;
	} else if (ItemList.ListAlloc == ItemList.ListCount) {
		ItemList.ListAlloc += 100;
		ItemList.List = (ROM_INFO *)realloc(ItemList.List, ItemList.ListAlloc * sizeof(ROM_INFO));
	}
	
	if (ItemList.List == NULL) {
		DisplayError(GS(MSG_MEM_ALLOC_ERROR));
		ExitThread(0);
	}

	pRomInfo = &ItemList.List[ItemList.ListCount];
	if (pRomInfo == NULL) { return; }

	memset(pRomInfo, 0, sizeof(ROM_INFO));	
	memset(&lvItem, 0, sizeof(lvItem));

	strncpy(pRomInfo->szFullFileName, RomLocation, MAX_PATH);
	if (!FillRomInfo(pRomInfo)) { return;  }
	
	lvItem.mask = LVIF_TEXT | LVIF_PARAM;		
	lvItem.iItem = ListView_GetItemCount(hRomList);
	lvItem.lParam = (LPARAM)ItemList.ListCount;
	lvItem.pszText = LPSTR_TEXTCALLBACK;
	ItemList.ListCount += 1;
	
	index = ListView_InsertItem(hRomList, &lvItem);
	if (_stricmp(pRomInfo->szFullFileName,LastRoms[0]) == 0) {
		ListView_SetItemState(hRomList,index,LVIS_SELECTED | LVIS_FOCUSED,LVIS_SELECTED | LVIS_FOCUSED);
		ListView_EnsureVisible(hRomList,index,FALSE);
	}
}

void CreateRomListControl (HWND hParent) {
	hRomList = CreateWindowEx( WS_EX_CLIENTEDGE,WC_LISTVIEW,NULL,
					WS_TABSTOP | WS_VISIBLE | WS_CHILD | LVS_OWNERDRAWFIXED |
					WS_BORDER | LVS_SINGLESEL | LVS_REPORT,
					0,0,0,0,hParent,(HMENU)IDC_ROMLIST,hInst,NULL);
	
	ResetRomBrowserColomuns();
	LoadRomList();
}

void FixRomBrowserColoumnLang (void) {
	ResetRomBrowserColomuns();
}

void HideRomBrowser (void) {
	DWORD X, Y;
	long Style;

	if (CPURunning) { return; }	
	if (hRomList == NULL) { return; }

	IgnoreMove = TRUE;
	if (IsRomBrowserMaximized()) { ShowWindow(hMainWindow,SW_RESTORE); }
	ShowWindow(hMainWindow,SW_HIDE);
	Style = GetWindowLong(hMainWindow,GWL_STYLE);
	Style = Style &	~(WS_SIZEBOX | WS_MAXIMIZEBOX);
	SetWindowLong(hMainWindow,GWL_STYLE,Style);
	if (GetStoredWinPos( "Main", &X, &Y ) ) {
		SetWindowPos(hMainWindow,NULL,X,Y,0,0, SWP_NOZORDER | SWP_NOSIZE);		 
	}			
	EnableWindow(hRomList,FALSE);
	ShowWindow(hRomList,SW_HIDE);
	SetupPlugins(hMainWindow);
	
	SendMessage(hMainWindow,WM_USER + 17,0,0);
	ShowWindow(hMainWindow,SW_SHOW);
	IgnoreMove = FALSE;
}

BOOL IsRomBrowserMaximized (void) {
	long lResult;
	HKEY hKeyResults = 0;
	char String[200];

	sprintf(String,"Software\\N64 Emulation\\%s\\Page Setup",AppName);
	lResult = RegOpenKeyEx( HKEY_CURRENT_USER,String,0, KEY_ALL_ACCESS,&hKeyResults);
	
	if (lResult == ERROR_SUCCESS) {
		DWORD Type, Value, Bytes = 4;

		lResult = RegQueryValueEx(hKeyResults,"RomBrowser Maximized",0,&Type,(LPBYTE)(&Value),&Bytes);
		if (Type == REG_DWORD && lResult == ERROR_SUCCESS) { 
			RegCloseKey(hKeyResults);
			return Value;
		}
		RegCloseKey(hKeyResults);
	}
	return FALSE;
}

BOOL IsSortAscending (int Index) {
	long lResult;
	HKEY hKeyResults = 0;
	char String[200];

	sprintf(String,"Software\\N64 Emulation\\%s\\Page Setup",AppName);
	lResult = RegOpenKeyEx( HKEY_CURRENT_USER,String,0, KEY_ALL_ACCESS,&hKeyResults);
	
	if (lResult == ERROR_SUCCESS) {
		DWORD Type, Value, Bytes = 4;
		char Key[100];
		
		sprintf(Key,"Sort Ascending %d",Index);
		lResult = RegQueryValueEx(hKeyResults,Key,0,&Type,(LPBYTE)(&Value),&Bytes);
		if (Type == REG_DWORD && lResult == ERROR_SUCCESS) { 
			RegCloseKey(hKeyResults);
			return Value;
		}
		RegCloseKey(hKeyResults);
	}
	return TRUE;
}

void LoadRomList (void) {
	char path_buffer[_MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR];
	char fname[_MAX_FNAME],ext[_MAX_EXT];
	char FileName[_MAX_PATH];
	int Size, count, index;
	ROM_INFO * pRomInfo;
	LV_ITEM  lvItem;
	DWORD dwRead;
	HANDLE hFile;

	GetModuleFileName(NULL,path_buffer,sizeof(path_buffer));
	_splitpath( path_buffer, drive, dir, fname, ext );
	sprintf(FileName,"%s%s%s",drive,dir,CacheFileName);

	hFile = CreateFile(FileName,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		RefreshRomBrowser();
		return;
	}
	Size = 0;
	ReadFile(hFile,&Size,sizeof(Size),&dwRead,NULL);
	if (Size != sizeof(ROM_INFO) || dwRead != sizeof(Size)) {
		CloseHandle(hFile);
		RefreshRomBrowser();
		return;
	}
	FreeRomBrowser();
	ReadFile(hFile,&ItemList.ListCount,sizeof(ItemList.ListCount),&dwRead,NULL);
	if (ItemList.ListCount == 0) {
		CloseHandle(hFile);
		return;
	}
	ItemList.List = (ROM_INFO *)malloc(ItemList.ListCount * sizeof(ROM_INFO));
	ItemList.ListAlloc = ItemList.ListCount;
	ReadFile(hFile,ItemList.List,sizeof(ROM_INFO) * ItemList.ListCount,&dwRead,NULL);
	CloseHandle(hFile);
	ListView_DeleteAllItems(hRomList);
	for (count = 0; count < ItemList.ListCount; count ++) {
		pRomInfo = &ItemList.List[count];
		memset(&lvItem, 0, sizeof(lvItem));

		lvItem.mask = LVIF_TEXT | LVIF_PARAM;		
		lvItem.iItem = ListView_GetItemCount(hRomList);
		lvItem.lParam = (LPARAM)count;
		lvItem.pszText = LPSTR_TEXTCALLBACK;

		index = ListView_InsertItem(hRomList, &lvItem);	
		if (_stricmp(pRomInfo->szFullFileName,LastRoms[0]) == 0) {
			ListView_SetItemState(hRomList,index,LVIS_SELECTED | LVIS_FOCUSED,LVIS_SELECTED | LVIS_FOCUSED);
			ListView_EnsureVisible(hRomList,index,FALSE);
		}
		SetColors(pRomInfo->Status);
	}
	RomList_SortList();
}

void LoadRomBrowserColoumnInfo (void) {
	char  String[200], szPos[10];
	DWORD Disposition = 0;
	HKEY  hKeyResults = 0;
	long  lResult;

	NoOfFields ;

	sprintf(String,"Software\\N64 Emulation\\%s\\Rom Browser",AppName);
	lResult = RegOpenKeyEx( HKEY_CURRENT_USER,String,0, KEY_ALL_ACCESS,&hKeyResults);	
	if (lResult == ERROR_SUCCESS) {
		DWORD Type, Value, count, Bytes = 4;
		for (count = 0; count < (DWORD)NoOfFields; count ++) {
			Bytes = sizeof(szPos);
			
			// Coloumn Postion
			lResult = RegQueryValueEx(hKeyResults,RomBrowserFields[count].Name,0,&Type,(LPBYTE)szPos,&Bytes);
			if (lResult == ERROR_SUCCESS) { RomBrowserFields[count].Pos = atoi(szPos); }
	
			//Coloumn Width
			Bytes = sizeof(Value);
			sprintf(String,"%s.Width",RomBrowserFields[count].Name);
			lResult = RegQueryValueEx(hKeyResults,String,0,&Type,(LPBYTE)(&Value),&Bytes);
			if (lResult == ERROR_SUCCESS) { RomBrowserFields[count].ColWidth = Value; }
		}
		RegCloseKey(hKeyResults);
	}
	FixRomBrowserColoumnLang();
}

void FillRomExtensionInfo(ROM_INFO * pRomInfo) {
	LPSTR IniFileName, ExtIniFileName, NotesIniFileName;
	char Identifier[100];

	IniFileName = GetIniFileName();
	NotesIniFileName = GetNotesIniFileName();
	ExtIniFileName = GetExtIniFileName();

	sprintf(Identifier,"%08X-%08X-C:%X", pRomInfo->CRC1, pRomInfo->CRC2, pRomInfo->Country);
	
	//Rom Notes
	if (RomBrowserFields[RB_UserNotes].Pos >= 0)
		GetString(Identifier, "Note", "", pRomInfo->UserNotes,sizeof(pRomInfo->UserNotes),NotesIniFileName);
	
	//Rom Extension info
	if (RomBrowserFields[RB_Developer].Pos >= 0)
		GetString(Identifier, "Developer", "", pRomInfo->Developer, sizeof(pRomInfo->Developer), ExtIniFileName);
	
	if (RomBrowserFields[RB_ReleaseDate].Pos >= 0)
		GetString(Identifier, "ReleaseDate", "", pRomInfo->ReleaseDate, sizeof(pRomInfo->ReleaseDate), ExtIniFileName);
	
	if (RomBrowserFields[RB_Genre].Pos >= 0)
		GetString(Identifier, "Genre", "", pRomInfo->Genre, sizeof(pRomInfo->Genre), ExtIniFileName);

	//if (RomBrowserFields[RB_GameInfoID].Pos >= 0)
		GetString(Identifier, "GameInformation", "", pRomInfo->GameInfoID, sizeof(pRomInfo->GameInfoID), ExtIniFileName);

	if (RomBrowserFields[RB_Players].Pos >= 0) {
		char junk[10];
		GetString(Identifier, "Players", "1", junk, sizeof(junk), ExtIniFileName);
		pRomInfo->Players = atoi(junk);
	}
		
	if (RomBrowserFields[RB_ForceFeedback].Pos >= 0)
		GetString(Identifier, "ForceFeedback", "unknown", pRomInfo->ForceFeedback, sizeof(pRomInfo->ForceFeedback), ExtIniFileName);

	//Rom Settings
	if (RomBrowserFields[RB_GameName].Pos >= 0)

		// This displays the message "Not in database? Add yourself or check for RDB updated"
		// in the ROM Browser if a ROM is not in the RDB (Game Database)
		// What i would like to achieve is to display the file name until the game is added
		// and then updated from that entry (Gent)

		//GetString(Identifier, "Game Name", GS(RB_NOT_IN_RDB), pRomInfo->GameName, sizeof(pRomInfo->GameName), IniFileName);

		// This allows the browser to display filename until the Game Name= is populated.
		// Once Game Name= is poulated (Game Added to databse) the browser uses that. (Gent / Witten)

		GetString(Identifier, "Game Name", pRomInfo->FileName, pRomInfo->GameName, sizeof(pRomInfo->GameName), IniFileName);

	GetString(Identifier, "Status", Default_RomStatus, pRomInfo->Status, sizeof(pRomInfo->Status), IniFileName);

	if (RomBrowserFields[RB_CoreNotes].Pos >= 0)
		GetString(Identifier, "Core Note", "", pRomInfo->CoreNotes, sizeof(pRomInfo->CoreNotes), IniFileName);
	
	if (RomBrowserFields[RB_PluginNotes].Pos >= 0)
		GetString(Identifier, "Plugin Note", "", pRomInfo->PluginNotes, sizeof(pRomInfo->PluginNotes), IniFileName);

	SetColors(pRomInfo->Status);
}

BOOL FillRomInfo(ROM_INFO * pRomInfo) {
	char drive[_MAX_DRIVE] ,dir[_MAX_DIR], ext[_MAX_EXT];
	BYTE RomData[0x1000];
	int count;
	
	if (RomBrowserFields[RB_CICChip].Pos >= 0) {
		if (!LoadDataFromRomFile(pRomInfo->szFullFileName,RomData,sizeof(RomData),&pRomInfo->RomSize)) { return FALSE; }
	} else {
		if (!LoadDataFromRomFile(pRomInfo->szFullFileName,RomData,0x40,&pRomInfo->RomSize)) { return FALSE; }
	}

	_splitpath( pRomInfo->szFullFileName, drive, dir, pRomInfo->FileName, ext );

	if (RomBrowserFields[RB_InternalName].Pos >= 0) {
		memcpy(pRomInfo->InternalName,(void *)(RomData + 0x20),20);
		for( count = 0 ; count < 20; count += 4 ) {
			pRomInfo->InternalName[count] ^= pRomInfo->InternalName[count+3];
			pRomInfo->InternalName[count + 3] ^= pRomInfo->InternalName[count];
			pRomInfo->InternalName[count] ^= pRomInfo->InternalName[count+3];			
			pRomInfo->InternalName[count + 1] ^= pRomInfo->InternalName[count + 2];
			pRomInfo->InternalName[count + 2] ^= pRomInfo->InternalName[count + 1];
			pRomInfo->InternalName[count + 1] ^= pRomInfo->InternalName[count + 2];			
		}
		pRomInfo->InternalName[21] = '\0';
	}
	pRomInfo->CartID[0] = *(RomData + 0x3F);
	pRomInfo->CartID[1] = *(RomData + 0x3E);
	pRomInfo->CartID[2] = '\0';
	pRomInfo->Manufacturer = *(RomData + 0x38);
	pRomInfo->Country = *(RomData + 0x3D);
	pRomInfo->CRC1 = *(DWORD *)(RomData + 0x10);
	pRomInfo->CRC2 = *(DWORD *)(RomData + 0x14);
	if (RomBrowserFields[RB_CICChip].Pos >= 0) {
		pRomInfo->CicChip = GetCicChipID(RomData);
	}
	
	FillRomExtensionInfo(pRomInfo);
	return TRUE;
}

int GetRomBrowserSize ( DWORD * nWidth, DWORD * nHeight ) {
	long lResult;
	HKEY hKeyResults = 0;
	char String[200];

	sprintf(String,"Software\\N64 Emulation\\%s\\Page Setup",AppName);
	lResult = RegOpenKeyEx( HKEY_CURRENT_USER,String,0, KEY_ALL_ACCESS,&hKeyResults);
	
	if (lResult == ERROR_SUCCESS) {
		DWORD Type, Value, Bytes = 4;

		lResult = RegQueryValueEx(hKeyResults,"Rom Browser Width",0,&Type,(LPBYTE)(&Value),&Bytes);
		if (Type == REG_DWORD && lResult == ERROR_SUCCESS) { 
			*nWidth = Value;
		} else {
			RegCloseKey(hKeyResults);
			return FALSE;
		}
	
		lResult = RegQueryValueEx(hKeyResults,"Rom Browser Height",0,&Type,(LPBYTE)(&Value),&Bytes);
		if (Type == REG_DWORD && lResult == ERROR_SUCCESS) { 
			*nHeight = Value;
		} else {
			RegCloseKey(hKeyResults);
			return FALSE;
		}
		RegCloseKey(hKeyResults);
		return TRUE;
	}
	return FALSE;
}

char * GetSortField ( int Index ) {
	static char String[200];
	long lResult;
	HKEY hKeyResults = 0;

	sprintf(String,"Software\\N64 Emulation\\%s\\Page Setup",AppName);
	lResult = RegOpenKeyEx( HKEY_CURRENT_USER,String,0, KEY_ALL_ACCESS,&hKeyResults);	
	if (lResult == ERROR_SUCCESS) {
		DWORD Type, Bytes = sizeof(String);
		char Key[100];

		sprintf(Key,"Rom Browser Sort Field %d",Index);

		lResult = RegQueryValueEx(hKeyResults,Key,0,&Type,(LPBYTE)String,&Bytes);
		if (lResult == ERROR_SUCCESS) { 
			RegCloseKey(hKeyResults);
			return String;
		}
		RegCloseKey(hKeyResults);
	}
	strcpy(String,"");
	return String;
}

void RefreshRomBrowser (void) {
	char RomDir[MAX_PATH+1];

	if (!hRomList) { return; }
	ListView_DeleteAllItems(hRomList);
	FreeRomBrowser();	
	GetRomDirectory(RomDir);
	FillRomList (RomDir);
	SaveRomList();
	RomList_SortList();
}

void ResetRomBrowserColomuns (void) {
	int Coloumn, index;
	LV_COLUMN lvColumn;
	char szString[300];

	//SaveRomBrowserColoumnInfo();
    memset(&lvColumn,0,sizeof(lvColumn));
	lvColumn.mask = LVCF_FMT;
	while (ListView_GetColumn(hRomList,0,&lvColumn)) {
		ListView_DeleteColumn(hRomList,0);
	}

	//Add Colomuns
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.pszText = szString;

	for (Coloumn = 0; Coloumn < NoOfFields; Coloumn ++) {
		for (index = 0; index < NoOfFields; index ++) {
			if (RomBrowserFields[index].Pos == Coloumn) { break; }
		}
		if (index == NoOfFields || RomBrowserFields[index].Pos != Coloumn) {
			FieldType[Coloumn] = -1;
			break;
		}
		FieldType[Coloumn] = RomBrowserFields[index].ID;
		lvColumn.cx = RomBrowserFields[index].ColWidth;
		strncpy(szString, GS(RomBrowserFields[index].LangID), sizeof(szString));
		ListView_InsertColumn(hRomList, Coloumn, &lvColumn);
	}
}

void ResizeRomListControl (WORD nWidth, WORD nHeight) {
	if (IsWindow(hRomList)) {
		if (IsWindow(hStatusWnd)) {
			RECT rc;

			GetWindowRect(hStatusWnd, &rc);
			nHeight -= (WORD)(rc.bottom - rc.top);
		}
		MoveWindow(hRomList, 0, 0, nWidth, nHeight, TRUE);
	}
}

void RomList_ColoumnSortList(LPNMLISTVIEW pnmv) {
	int index;

	for (index = 0; index < NoOfFields; index++) {
		if (RomBrowserFields[index].Pos == pnmv->iSubItem) { break; }
	}
	if (NoOfFields == index) { return; }
	if (_stricmp(GetSortField(0),RomBrowserFields[index].Name) == 0) {
		SetSortAscending(!IsSortAscending(0),0);
	} else {
		int count;

		for (count = NoOfSortKeys - 1; count > 0; count --) {
			SetSortField (GetSortField(count - 1), count);
			SetSortAscending(IsSortAscending(count - 1), count);
		}
		SetSortField (RomBrowserFields[index].Name, 0);
		SetSortAscending(TRUE,0);
	}
	RomList_SortList();
}

int CALLBACK RomList_CompareItems2(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
	SORT_FIELDS * SortFields = (SORT_FIELDS *)lParamSort;
	ROM_INFO * pRomInfo1,* pRomInfo2;
	int count, result;

	for (count = 0; count < 3; count ++) {
		pRomInfo1 = &ItemList.List[SortFields->KeyAscend[count]?lParam1:lParam2];
		pRomInfo2 = &ItemList.List[SortFields->KeyAscend[count]?lParam2:lParam1];
	
		switch (SortFields->Key[count]) {
		case RB_FileName: result = (int)lstrcmpi(pRomInfo1->FileName, pRomInfo2->FileName); break;
		case RB_InternalName: result =  (int)lstrcmpi(pRomInfo1->InternalName, pRomInfo2->InternalName); break;
		case RB_GameName: result =  (int)lstrcmpi(pRomInfo1->GameName, pRomInfo2->GameName); break;
		case RB_Status: result =  (int)lstrcmpi(pRomInfo1->Status, pRomInfo2->Status); break;
		case RB_RomSize: result =  (int)pRomInfo1->RomSize - (int)pRomInfo2->RomSize; break;
		case RB_CoreNotes: result =  (int)lstrcmpi(pRomInfo1->CoreNotes, pRomInfo2->CoreNotes); break;
		case RB_PluginNotes: result =  (int)lstrcmpi(pRomInfo1->PluginNotes, pRomInfo2->PluginNotes); break;
		case RB_UserNotes: result =  (int)lstrcmpi(pRomInfo1->UserNotes, pRomInfo2->UserNotes); break;
		case RB_CartridgeID: result =  (int)lstrcmpi(pRomInfo1->CartID, pRomInfo2->CartID); break;
		case RB_Manufacturer: result =  (int)pRomInfo1->Manufacturer - (int)pRomInfo2->Manufacturer; break;
		case RB_Country: {
			char junk1[50], junk2[50];
			CountryCodeToString(junk1, pRomInfo1->Country, 50);
			CountryCodeToString(junk2, pRomInfo2->Country, 50);
			result = lstrcmpi(junk1, junk2);
			break; }
		case RB_Developer: result =  (int)lstrcmpi(pRomInfo1->Developer, pRomInfo2->Developer); break;
		case RB_Crc1: result =  (int)pRomInfo1->CRC1 - (int)pRomInfo2->CRC1; break;
		case RB_Crc2: result =  (int)pRomInfo1->CRC2 - (int)pRomInfo2->CRC2; break;
		case RB_CICChip: result =  (int)pRomInfo1->CicChip - (int)pRomInfo2->CicChip; break;
		case RB_ReleaseDate: result =  (int)lstrcmpi(pRomInfo1->ReleaseDate, pRomInfo2->ReleaseDate); break;
		case RB_Players: result =  (int)pRomInfo1->Players - (int)pRomInfo2->Players; break;
		case RB_ForceFeedback: result = (int)lstrcmpi(pRomInfo1->ForceFeedback, pRomInfo2->ForceFeedback); break;
		case RB_Genre: result =  (int)lstrcmpi(pRomInfo1->Genre, pRomInfo2->Genre); break;
		case RB_GameInfoID: result = (int)lstrcmpi(pRomInfo1->GameInfoID, pRomInfo2->GameInfoID); break;
		default: result = 0; break;
		}
		if (result != 0) { return result; }
	}
	return 0;
}
 

void RomList_DeleteItem(LPNMHDR pnmh) {
}

void RomList_GetDispInfo(LPNMHDR pnmh) {
	LV_DISPINFO * lpdi = (LV_DISPINFO *)pnmh;
	ROM_INFO * pRomInfo = &ItemList.List[lpdi->item.lParam];

	switch(FieldType[lpdi->item.iSubItem]) {
	case RB_FileName: strncpy(lpdi->item.pszText, pRomInfo->FileName, lpdi->item.cchTextMax); break;
	case RB_InternalName: strncpy(lpdi->item.pszText, pRomInfo->InternalName, lpdi->item.cchTextMax); break;
	
		// TpRomInfo->GameName displays Game Name=Text there in the RDB,
	// TpRomInfo->FileName displays File Name but then will not update what is written
	// in Game Name=Text there in the RDB (Gent)

	case RB_GameName: strncpy(lpdi->item.pszText, pRomInfo->GameName, lpdi->item.cchTextMax); break;

	case RB_CoreNotes: strncpy(lpdi->item.pszText, pRomInfo->CoreNotes, lpdi->item.cchTextMax); break;
	case RB_PluginNotes: strncpy(lpdi->item.pszText, pRomInfo->PluginNotes, lpdi->item.cchTextMax); break;
	case RB_Status: strncpy(lpdi->item.pszText, pRomInfo->Status, lpdi->item.cchTextMax); break;
	case RB_RomSize: sprintf(lpdi->item.pszText,"%.1f MBit",(float)pRomInfo->RomSize/0x20000); break;
	case RB_CartridgeID: strncpy(lpdi->item.pszText, pRomInfo->CartID, lpdi->item.cchTextMax); break;
	case RB_Manufacturer:
		switch (pRomInfo->Manufacturer) {
		case 'N':strncpy(lpdi->item.pszText, "Nintendo", lpdi->item.cchTextMax); break;
		case 0:  strncpy(lpdi->item.pszText, "None", lpdi->item.cchTextMax); break;
		default: sprintf(lpdi->item.pszText, "(Unknown %c (%X))", pRomInfo->Manufacturer,pRomInfo->Manufacturer); break;
		}
		break;
	case RB_Country: {
		char junk[50];
		CountryCodeToString(junk, pRomInfo->Country, 50);
		strncpy(lpdi->item.pszText, junk, lpdi->item.cchTextMax);
		break; }
	case RB_Crc1: sprintf(lpdi->item.pszText,"0x%08X",pRomInfo->CRC1); break;
	case RB_Crc2: sprintf(lpdi->item.pszText,"0x%08X",pRomInfo->CRC2); break;

		// Disabled Unknown CIC Chip on CicChip 0 Message (Gent)
	/*case RB_CICChip: 
		if (pRomInfo->CicChip < 0) { 
			sprintf(lpdi->item.pszText, "Unknown CIC Chip");
		} else {
			sprintf(lpdi->item.pszText,"CIC-NUS-610%d",pRomInfo->CicChip); 
		}*/

		break;
	case RB_UserNotes: strncpy(lpdi->item.pszText, pRomInfo->UserNotes, lpdi->item.cchTextMax); break;
	case RB_Developer: strncpy(lpdi->item.pszText, pRomInfo->Developer, lpdi->item.cchTextMax); break;
	case RB_ReleaseDate: strncpy(lpdi->item.pszText, pRomInfo->ReleaseDate, lpdi->item.cchTextMax); break;
	case RB_Genre: strncpy(lpdi->item.pszText, pRomInfo->Genre, lpdi->item.cchTextMax); break;
	case RB_Players: sprintf(lpdi->item.pszText,"%d",pRomInfo->Players); break;
	case RB_ForceFeedback: strncpy(lpdi->item.pszText, pRomInfo->ForceFeedback, lpdi->item.cchTextMax); break;
	case RB_GameInfoID: strncpy(lpdi->item.pszText, pRomInfo->GameInfoID, lpdi->item.cchTextMax); break;
	default: strncpy(lpdi->item.pszText, " ", lpdi->item.cchTextMax);
	}
	if (strlen(lpdi->item.pszText) == 0) { strcpy(lpdi->item.pszText," "); }
}

void RomList_PopupMenu(LPNMHDR pnmh) {
	LV_DISPINFO * lpdi = (LV_DISPINFO *)pnmh;
	HMENU hMenu = LoadMenu(hInst,MAKEINTRESOURCE(IDR_POPUP));
	HMENU hPopupMenu = GetSubMenu(hMenu,0);
	ROM_INFO * pRomInfo;
	LV_ITEM lvItem;
	POINT Mouse;
	LONG iItem;

	GetCursorPos(&Mouse);

	iItem = ListView_GetNextItem(hRomList, -1, LVNI_SELECTED);
	if (iItem != -1) { 
		memset(&lvItem, 0, sizeof(LV_ITEM));
		lvItem.mask = LVIF_PARAM;
		lvItem.iItem = iItem;
		if (!ListView_GetItem(hRomList, &lvItem)) { return; }
		pRomInfo = &ItemList.List[lvItem.lParam];

		if (!pRomInfo) { return; }
		strcpy(CurrentRBFileName,pRomInfo->szFullFileName);
		strcpy(CurrentGameInfoID, pRomInfo->GameInfoID);
	} else {
		strcpy(CurrentRBFileName,"");
	}
	
	//Fix up menu
	MenuSetText(hPopupMenu, 0, GS(POPUP_PLAY), NULL);
	MenuSetText(hPopupMenu, 2, GS(MENU_REFRESH), NULL);
	MenuSetText(hPopupMenu, 3, GS(MENU_CHOOSE_ROM), NULL);
	MenuSetText(hPopupMenu, 5, GS(POPUP_INFO), NULL);
	MenuSetText(hPopupMenu, 6, GS(POPUP_GAMEINFO), NULL);
	MenuSetText(hPopupMenu, 8, GS(POPUP_SETTINGS), NULL);
	MenuSetText(hPopupMenu, 9, GS(POPUP_CHEATS), NULL);

	if (strlen(CurrentRBFileName) == 0) {
		DeleteMenu(hPopupMenu,9,MF_BYPOSITION);
		DeleteMenu(hPopupMenu,8,MF_BYPOSITION);
		DeleteMenu(hPopupMenu,6,MF_BYPOSITION);
		DeleteMenu(hPopupMenu,5,MF_BYPOSITION);
		DeleteMenu(hPopupMenu,4,MF_BYPOSITION);
		DeleteMenu(hPopupMenu,1,MF_BYPOSITION);
		DeleteMenu(hPopupMenu,0,MF_BYPOSITION);
	} else {
		if (BasicMode && !RememberCheats) { DeleteMenu(hPopupMenu,8,MF_BYPOSITION); }
		if (BasicMode) { DeleteMenu(hPopupMenu,8,MF_BYPOSITION); }
		if (BasicMode && !RememberCheats) { DeleteMenu(hPopupMenu,6,MF_BYPOSITION); }
	}
	
	TrackPopupMenu(hPopupMenu, 0, Mouse.x, Mouse.y, 0,hMainWindow, NULL);
	DestroyMenu(hMenu);
}

void RomList_SetFocus (void) {
	if (!RomListVisible()) { return; }
	SetFocus(hRomList);
}

void RomList_OpenRom(LPNMHDR pnmh) {
	ROM_INFO * pRomInfo;
	LV_ITEM lvItem;
	DWORD ThreadID;
	LONG iItem;

	iItem = ListView_GetNextItem(hRomList, -1, LVNI_SELECTED);
	if (iItem == -1) { return; }

	memset(&lvItem, 0, sizeof(LV_ITEM));
	lvItem.mask = LVIF_PARAM;
	lvItem.iItem = iItem;
	if (!ListView_GetItem(hRomList, &lvItem)) { return; }
	pRomInfo = &ItemList.List[lvItem.lParam];

	if (!pRomInfo) { return; }
	strcpy(CurrentFileName,pRomInfo->szFullFileName);
	CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)OpenChosenFile,NULL,0, &ThreadID);	
}

void RomList_SortList (void) {
	SORT_FIELDS SortFields;
	char * SortField;
	int count, index;

	for (count = 0; count < NoOfSortKeys; count ++) {
		SortField = GetSortField(count);
		if ((_stricmp(SortField, "") == 0) && (count==0)) {
			index = DefaultSortField;
			SetSortField(RomBrowserFields[index].Name, count);
			SetSortAscending(TRUE, count);
		}
		else {
			for (index = 0; index < NoOfFields; index++) {
				if (_stricmp(RomBrowserFields[index].Name, SortField) == 0) { break; }
			}
		}
		SortFields.Key[count] = index;
		SortFields.KeyAscend[count] = IsSortAscending(count);
	}
	ListView_SortItems(hRomList, RomList_CompareItems2, &SortFields );
}

void RomListDrawItem (LPDRAWITEMSTRUCT ditem) {
	RECT rcItem, rcDraw;
	ROM_INFO * pRomInfo;
	char String[300];
	LV_ITEM lvItem;
	BOOL bSelected;
	HBRUSH hBrush;
    LV_COLUMN lvc; 
	int nColumn;

	lvItem.mask = LVIF_PARAM;
	lvItem.iItem = ditem->itemID;
	if (!ListView_GetItem(hRomList, &lvItem)) { return; }
	lvItem.state = ListView_GetItemState(hRomList, ditem->itemID, -1);
	bSelected = (lvItem.state & LVIS_SELECTED);
	pRomInfo = &ItemList.List[lvItem.lParam];
	if (bSelected) {
		hBrush = CreateSolidBrush(GetColor(pRomInfo->Status, COLOR_HIGHLIGHTED));
		SetTextColor(ditem->hDC, GetColor(pRomInfo->Status, COLOR_SELECTED_TEXT));
	} else {
		hBrush = (HBRUSH)(COLOR_WINDOW + 1);
		SetTextColor(ditem->hDC, GetColor(pRomInfo->Status, COLOR_TEXT));
	}
	FillRect( ditem->hDC, &ditem->rcItem,hBrush);	
	SetBkMode( ditem->hDC, TRANSPARENT );
	
	//Draw
	ListView_GetItemRect(hRomList,ditem->itemID,&rcItem,LVIR_LABEL);
	ListView_GetItemText(hRomList,ditem->itemID, 0, String, sizeof(String)); 
	memcpy(&rcDraw,&rcItem,sizeof(RECT));
	rcDraw.right -= 3;
	DrawText(ditem->hDC, String, strlen(String), &rcDraw, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER);	
	
    memset(&lvc,0,sizeof(lvc));
	lvc.mask = LVCF_FMT | LVCF_WIDTH; 
	for(nColumn = 1; ListView_GetColumn(hRomList,nColumn,&lvc); nColumn += 1) {		
		rcItem.left = rcItem.right; 
        rcItem.right += lvc.cx; 

		ListView_GetItemText(hRomList,ditem->itemID, nColumn, String, sizeof(String)); 
		memcpy(&rcDraw,&rcItem,sizeof(RECT));
		rcDraw.right -= 3;
		DrawText(ditem->hDC, String, strlen(String), &rcDraw, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER);
	}

	DeleteObject(hBrush);
}

void RomListNotify(LPNMHDR pnmh) {
	switch (pnmh->code) {
	case LVN_DELETEITEM:  RomList_DeleteItem(pnmh); break;
	case LVN_GETDISPINFO: RomList_GetDispInfo(pnmh); break;
	case LVN_COLUMNCLICK: RomList_ColoumnSortList((LPNMLISTVIEW)pnmh); break;
	case NM_RETURN:       RomList_OpenRom(pnmh); break;
	case NM_DBLCLK:       RomList_OpenRom(pnmh); break;
	case NM_RCLICK:       RomList_PopupMenu(pnmh); break;
	}
}

BOOL RomListVisible(void) {
	if (hRomList == NULL) { return FALSE; }
	return (IsWindowVisible(hRomList));
}

void SaveRomBrowserColoumnInfo (void) {
	DWORD Disposition = 0;
	HKEY  hKeyResults = 0;
	char  String[200];
	long  lResult;

	sprintf(String,"Software\\N64 Emulation\\%s\\Rom Browser",AppName);
	lResult = RegCreateKeyEx( HKEY_CURRENT_USER, String,0,"", REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
	if (lResult == ERROR_SUCCESS) {
		int Coloumn, index;
		LV_COLUMN lvColumn;

	    memset(&lvColumn,0,sizeof(lvColumn));
		lvColumn.mask = LVCF_WIDTH;
	
		for (Coloumn = 0;ListView_GetColumn(hRomList,Coloumn,&lvColumn); Coloumn++) {
			for (index = 0; index < NoOfFields; index++) {
				if (RomBrowserFields[index].Pos == Coloumn) { break; }
			}
			RomBrowserFields[index].ColWidth = lvColumn.cx;
			sprintf(String,"%s.Width",RomBrowserFields[index].Name);
			RegSetValueEx(hKeyResults,String,0,REG_DWORD,(BYTE *)&lvColumn.cx,sizeof(DWORD));
		}
		RegCloseKey(hKeyResults);
	}
}

void SaveRomBrowserColoumnPosition (int index, int Position) {
	char  String[200], szPos[10];
	DWORD Disposition = 0;
	HKEY  hKeyResults = 0;
	long  lResult;

	sprintf(String,"Software\\N64 Emulation\\%s\\Rom Browser",AppName);
	lResult = RegCreateKeyEx( HKEY_CURRENT_USER, String,0,"", REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
	if (lResult == ERROR_SUCCESS) {
		sprintf(szPos,"%d",Position);
		RegSetValueEx(hKeyResults,RomBrowserFields[index].Name,0,REG_SZ,(CONST BYTE *)szPos,strlen(szPos));
		RegCloseKey(hKeyResults);
	}
}

void SaveRomList (void) {
	char path_buffer[_MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR];
	char fname[_MAX_FNAME],ext[_MAX_EXT];
	char FileName[_MAX_PATH];
	DWORD dwWritten;
	HANDLE hFile;
	int Size;

	GetModuleFileName(NULL,path_buffer,sizeof(path_buffer));
	_splitpath( path_buffer, drive, dir, fname, ext );
	sprintf(FileName,"%s%s%s",drive,dir,CacheFileName);
	
	hFile = CreateFile(FileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
	Size = sizeof(ROM_INFO);
	WriteFile(hFile,&Size,sizeof(Size),&dwWritten,NULL);
	WriteFile(hFile,&ItemList.ListCount,sizeof(ItemList.ListCount),&dwWritten,NULL);
	WriteFile(hFile,ItemList.List,Size * ItemList.ListCount,&dwWritten,NULL);
	CloseHandle(hFile);
}

int CALLBACK SelectRomDirCallBack(HWND hwnd,DWORD uMsg,DWORD lp, DWORD lpData) {
  switch(uMsg)
  {
    case BFFM_INITIALIZED:
      // WParam is TRUE since you are passing a path.
      // It would be FALSE if you were passing a pidl.
      if (lpData)
      {
        SendMessage((HWND)hwnd,BFFM_SETSELECTION,TRUE,lpData);
      }
      break;
  } 
  return 0;
}

void SelectRomDir (void) {
	char Buffer[MAX_PATH], Directory[255];
	char RomDir[MAX_PATH+1];
	LPITEMIDLIST pidl;
	BROWSEINFO bi;

	GetRomDirectory(RomDir);

	bi.hwndOwner = hMainWindow;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = Buffer;
	bi.lpszTitle = GS(SELECT_ROM_DIR);
	bi.ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	bi.lpfn = (BFFCALLBACK)SelectRomDirCallBack;
	bi.lParam = (DWORD)RomDir;
	if ((pidl = SHBrowseForFolder(&bi)) != NULL) {
		if (SHGetPathFromIDList(pidl, Directory)) {
			int len = strlen(Directory);

			if (Directory[len - 1] != '\\') {
				strcat(Directory,"\\");
			}
			SetRomDirectory(Directory);
			RefreshRomBrowser();
		}
	}
}

void SetRomBrowserMaximized (BOOL Maximized) {
	long lResult;
	HKEY hKeyResults = 0;
	DWORD Disposition = 0;
	char String[200];

	sprintf(String,"Software\\N64 Emulation\\%s\\Page Setup",AppName);
	lResult = RegCreateKeyEx( HKEY_CURRENT_USER, String,0,"", REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
	if (lResult == ERROR_SUCCESS) {
		RegSetValueEx(hKeyResults,"RomBrowser Maximized",0, REG_DWORD,(CONST BYTE *)(&Maximized),sizeof(DWORD));
	}
	RegCloseKey(hKeyResults);
}

void SetRomBrowserSize ( int nWidth, int nHeight ) {
	long lResult;
	HKEY hKeyResults = 0;
	DWORD Disposition = 0;
	char String[200];

	sprintf(String,"Software\\N64 Emulation\\%s\\Page Setup",AppName);
	lResult = RegCreateKeyEx( HKEY_CURRENT_USER, String,0,"", REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
	if (lResult == ERROR_SUCCESS) {
		RegSetValueEx(hKeyResults,"Rom Browser Width",0, REG_DWORD,(CONST BYTE *)(&nWidth),sizeof(DWORD));
		RegSetValueEx(hKeyResults,"Rom Browser Height",0, REG_DWORD,(CONST BYTE *)(&nHeight),sizeof(DWORD));
	}
	RegCloseKey(hKeyResults);
}

void SetSortAscending (BOOL Ascending, int Index) {
	long lResult;
	HKEY hKeyResults = 0;
	DWORD Disposition = 0;
	char String[200];

	sprintf(String,"Software\\N64 Emulation\\%s\\Page Setup",AppName);
	lResult = RegCreateKeyEx( HKEY_CURRENT_USER, String,0,"", REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
	if (lResult == ERROR_SUCCESS) {
		char Key[100];

		sprintf(Key,"Sort Ascending %d",Index);
		RegSetValueEx(hKeyResults,Key,0, REG_DWORD,(CONST BYTE *)(&Ascending),sizeof(DWORD));
	}
	RegCloseKey(hKeyResults);
}

void SetSortField (char * FieldName, int Index) {
	long lResult;
	HKEY hKeyResults = 0;
	DWORD Disposition = 0;
	char String[200];

	sprintf(String,"Software\\N64 Emulation\\%s\\Page Setup",AppName);
	lResult = RegCreateKeyEx( HKEY_CURRENT_USER, String,0,"", REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
	if (lResult == ERROR_SUCCESS) {
		char Key[100];

		sprintf(Key,"Rom Browser Sort Field %d",Index);
		RegSetValueEx(hKeyResults,Key,0, REG_SZ,(CONST BYTE *)FieldName,strlen(FieldName));
	}
	RegCloseKey(hKeyResults);
}

void FillRomList (char * Directory) {
	char FullPath[MAX_PATH+1], FileName[MAX_PATH+1], SearchSpec[MAX_PATH+1];
	WIN32_FIND_DATA fd;
	HANDLE hFind;

	strcpy(SearchSpec,Directory);
	if (SearchSpec[strlen(Directory) - 1] != '\\') { strcat(SearchSpec,"\\"); }
	strcat(SearchSpec,"*.*");

	hFind = FindFirstFile(SearchSpec, &fd);
	if (hFind == INVALID_HANDLE_VALUE) { return; }
	do {
		char drive[_MAX_DRIVE] ,dir[_MAX_DIR], ext[_MAX_EXT];

		if (strcmp(fd.cFileName, ".") == 0) { continue; }
		if (strcmp(fd.cFileName, "..") == 0) { continue; }

		strcpy(FullPath,Directory);
		if (FullPath[strlen(Directory) - 1] != '\\') { strcat(FullPath,"\\"); }
		strcat(FullPath,fd.cFileName);
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
			if (Rercursion) { FillRomList(FullPath); }
			continue;
		}
		_splitpath( FullPath, drive, dir, FileName, ext );
		if (_stricmp(ext, ".zip") == 0) { AddRomToList(FullPath); continue; }
		if (_stricmp(ext, ".v64") == 0) { AddRomToList(FullPath); continue; }
		if (_stricmp(ext, ".z64") == 0) { AddRomToList(FullPath); continue; }
		if (_stricmp(ext, ".n64") == 0) { AddRomToList(FullPath); continue; }
		if (_stricmp(ext, ".rom") == 0) { AddRomToList(FullPath); continue; }
		if (_stricmp(ext, ".jap") == 0) { AddRomToList(FullPath); continue; }
		if (_stricmp(ext, ".pal") == 0) { AddRomToList(FullPath); continue; }
		if (_stricmp(ext, ".usa") == 0) { AddRomToList(FullPath); continue; }
		if (_stricmp(ext, ".eur") == 0) { AddRomToList(FullPath); continue; }
		if (_stricmp(ext, ".bin") == 0) { AddRomToList(FullPath); continue; }
	} while (FindNextFile(hFind, &fd));

	FindClose(hFind);
}

void ShowRomList(HWND hParent) {
	DWORD X, Y, Width, Height;
	long Style;
	int iItem;

	if (CPURunning) { return; }
	if (hRomList != NULL && IsWindowVisible(hRomList)) { return; }

	SetupMenu(hMainWindow);
	IgnoreMove = TRUE;
	SetupPlugins(hHiddenWin);
	ShowWindow(hMainWindow,SW_HIDE);
	if (hRomList == NULL) {
		CreateRomListControl(hParent);
	} else {
		EnableWindow(hRomList,TRUE);
	}
	if (!GetRomBrowserSize(&Width,&Height)) { Width = 640; Height= 480; }
	ChangeWinSize ( hMainWindow, Width, Height, NULL );
	iItem = ListView_GetNextItem(hRomList, -1, LVNI_SELECTED);
	ListView_EnsureVisible(hRomList,iItem,FALSE);

	ShowWindow(hRomList,SW_SHOW);
	InvalidateRect(hParent,NULL,TRUE);
	Style = GetWindowLong(hMainWindow,GWL_STYLE) | WS_SIZEBOX | WS_MAXIMIZEBOX;
	SetWindowLong(hMainWindow,GWL_STYLE,Style);
	if (!GetStoredWinPos( "Main", &X, &Y ) ) {
  		X = (GetSystemMetrics( SM_CXSCREEN ) - Width) / 2;
		Y = (GetSystemMetrics( SM_CYSCREEN ) - Height) / 2;
	}		
	SetWindowPos(hMainWindow,HWND_NOTOPMOST,X,Y,0,0,SWP_NOSIZE);		 
	if (IsRomBrowserMaximized()) { 
		ShowWindow(hMainWindow,SW_MAXIMIZE); 
	} else {
		ShowWindow(hMainWindow,SW_SHOW);
		DrawMenuBar(hMainWindow);
		ChangeWinSize ( hMainWindow, Width, Height, NULL );
	}
	IgnoreMove = FALSE;
	SetupMenu(hMainWindow);
	
	SetFocus(hRomList);
}

void FreeRomBrowser ( void )
{
	if (ItemList.ListAlloc != 0) {
		free(ItemList.List);
		ItemList.ListAlloc = 0;
		ItemList.ListCount = 0;
		ItemList.List = NULL;
	}
	
	FreeIndex();
	ClearColorCache();
}

void AddToColorCache(COLOR_ENTRY color) {

	// Allocate more memory if there is not enough to store the new entry.
	if (ColorCache.count == ColorCache.max_allocated) {
		COLOR_ENTRY *temp;
		const int increase = ColorCache.max_allocated + 5;

		temp = (COLOR_ENTRY *)realloc(ColorCache.List, sizeof(COLOR_ENTRY) * increase);

		if (temp == NULL)
			return;

		ColorCache.List = temp;
		ColorCache.max_allocated = increase;
	}

	ColorCache.List[ColorCache.count] = color;
	ColorCache.count++;
}

COLORREF GetColor(char *status, int selection) {
	int i = ColorIndex(status);

	switch(selection) {
	case COLOR_SELECTED_TEXT:
		if (i == -1)
			return RGB(0xFF, 0xFF, 0xFF);
		return ColorCache.List[i].SelectedText;
	case COLOR_HIGHLIGHTED:
		if (i == -1) 
			return RGB(0, 0, 0);
		return ColorCache.List[i].HighLight;
	default:
		if (i == -1)
			return RGB(0, 0, 0);
		return ColorCache.List[i].Text;
	}
}

int ColorIndex(char *status) {
	int i;

	for (i = 0; i < ColorCache.count; i++)
		if (strcmp(status, ColorCache.List[i].status_name) == 0)
			return i;

	return -1;
}

void ClearColorCache() {
	int i;
	
	for (i = 0; i < ColorCache.count; i++) {
		free(ColorCache.List[i].status_name);
	}

	free(ColorCache.List);

	ColorCache.count = 0;
	ColorCache.List = NULL;
	ColorCache.max_allocated = 0;
}

void SetColors(char *status) {
	int count;
	COLOR_ENTRY colors;
	char String[100];
	LPCSTR IniFileName;

	IniFileName = GetIniFileName();

	if (ColorIndex(status) == -1) {
		_GetPrivateProfileString("Rom Status", status, "000000", String, 7, IniFileName);
		count = (AsciiToHex(String) & 0xFFFFFF);
		colors.Text = (count & 0x00FF00) | ((count >> 0x10) & 0xFF) | ((count & 0xFF) << 0x10);

		sprintf(String,"%s.Sel",status);
		_GetPrivateProfileString("Rom Status", String, "FFFFFFFF", String, 9, IniFileName);
		count = AsciiToHex(String);
		if (count < 0) {
			colors.HighLight = COLOR_HIGHLIGHT + 1;
		} else {
			count = (AsciiToHex(String) & 0xFFFFFF);
			count = (count & 0x00FF00) | ((count >> 0x10) & 0xFF) | ((count & 0xFF) << 0x10);
			colors.HighLight = count;
		}
		
		sprintf(String,"%s.Seltext",status);
		_GetPrivateProfileString("Rom Status", String, "FFFFFF", String, 7, IniFileName);
		count = (AsciiToHex(String) & 0xFFFFFF);
		colors.SelectedText = (count & 0x00FF00) | ((count >> 0x10) & 0xFF) | ((count & 0xFF) << 0x10);

		colors.status_name = (char *)malloc(strlen(status) + 1);
		strcpy(colors.status_name, status);
		AddToColorCache(colors);
	}
}