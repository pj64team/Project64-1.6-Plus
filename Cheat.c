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
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <winuser.h>
#include <string.h>
#include "main.h"
#include "cheats.h"
#include "cpu.h"
#include "resource.h"

#define UM_CLOSE_CHEATS         (WM_USER + 132)
#define UM_CHANGECODEEXTENSION  (WM_USER + 101)
#define IDC_MYTREE				0x500

#define MaxCheats				500

#define SelectCheat				1
#define EditCheat				2
#define NewCheat 				3 

HWND hManageWindow = NULL;
HWND hSelectCheat, hAddCheat, hCheatTree;
CHEAT_CODES Codes[MaxCheats];
int NoOfCodes;


/*******************************************************************************************
  Variables for Add Cheat
********************************************************************************************/
char codestring[2048];
char optionsstring[8192];

BOOL validname;
BOOL validcodes;
BOOL validoptions;
BOOL nooptions;
int codeformat;

int numcodes;
int numoptions;

int ReadCodeString (HWND hDlg);
void ReadOptionsString(HWND hDlg);
/********************************************************************************************/


BOOL CheatUsesCodeExtensions (char * CheatString);
void DeleteCheat           ( int CheatNo );
BOOL GetCheatName          ( int CheatNo, char * CheatName, int CheatNameLen );
BOOL LoadCheatExt          ( char * CheatName, char * CheatExt, int MaxCheatExtLen);
void RefreshCheatManager   ( void );
void RenameCheat           ( int CheatNo );
void SaveCheat             ( char * CheatName, BOOL Active );
void SaveCheatExt          ( char * CheatName, char * CheatExt );
int  _TreeView_GetCheckState(HWND hwndTreeView, HTREEITEM hItem);
BOOL _TreeView_SetCheckState(HWND hwndTreeView, HTREEITEM hItem, int State);
DWORD ConvertXP64Address (DWORD Address); //Witten
WORD ConvertXP64Value (WORD Value); //Witten

LRESULT CALLBACK ManageCheatsProc (HWND, UINT, WPARAM, LPARAM );

enum Dialog_State {
	CONTRACTED,
	EXPANDED
} DialogState;

enum Cheat_Type {
	SIMPLE,
	OPTIONS,
	RANGE
} CheatType;


enum TV_CHECK_STATE{
	TV_STATE_CLEAR,
	TV_STATE_CHECKED,
	TV_STATE_INDETERMINATE,
} DialogState;

int MinSizeDlg;
int MaxSizeDlg;

void AddCheatExtension(int CheatNo, char * CheatName, int CheatNameLen) {
	char *String = NULL, Identifier[100], CheatNumber[20];
	LPSTR IniFileName  = GetCheatIniFileName();

	sprintf(Identifier,"%08X-%08X-C:%X",*(DWORD *)(&RomHeader[0x10]),*(DWORD *)(&RomHeader[0x14]),RomHeader[0x3D]);
	sprintf(CheatNumber,"Cheat%d",CheatNo);

	_GetPrivateProfileString2(Identifier,CheatNumber,"",&String,IniFileName);

	//Add cheat extension to the end
	if (CheatUsesCodeExtensions(String)) {
		char CheatExt[200];
		if (!LoadCheatExt(CheatName,CheatExt,sizeof(CheatExt))) { strcpy(CheatExt,"?"); }
		sprintf(CheatName,"%s (=> %s)",CheatName,CheatExt);
	}

	if (String) { free(String); }
}

/********************************************************************************************
  ConvertXP64Address

  Purpose: Decode encoded XP64 address to physical address
  Parameters: 
  Returns: 
  Author: Witten

********************************************************************************************/
DWORD ConvertXP64Address (DWORD Address) {
	DWORD tmpAddress;

	tmpAddress = (Address ^ 0x68000000) & 0xFF000000;
	tmpAddress += ((Address + 0x002B0000) ^ 0x00810000) & 0x00FF0000;
	tmpAddress += ((Address + 0x00002B00) ^ 0x00008200) & 0x0000FF00;
	tmpAddress += ((Address + 0x0000002B) ^ 0x00000083) & 0x000000FF;
	return tmpAddress;
}

/********************************************************************************************
  ConvertXP64Value

  Purpose: Decode encoded XP64 value
  Parameters: 
  Returns: 
  Author: Witten

********************************************************************************************/
WORD ConvertXP64Value (WORD Value) {
	WORD  tmpValue;

	tmpValue = ((Value + 0x2B00) ^ 0x8400) & 0xFF00;
	tmpValue += ((Value + 0x002B) ^ 0x0085) & 0x00FF;
	return tmpValue;
}

void ApplyGSButton (void) {
	int count, count2, count3;
	DWORD Address;
	WORD  Memory;
	GAMESHARK_CODE PrevCode;

	for (count = 0; count < NoOfCodes; count++) {
		PrevCode.Command=0X00000000;
		PrevCode.Value=0x0000;

		for (count2 = 0; count2 < MaxGSEntries; count2++) {
			if ((PrevCode.Command & 0xFF000000) == 0x50000000) {
				int numrepeats = (PrevCode.Command & 0x0000FF00) >> 8;
				int offset = PrevCode.Command & 0x000000FF;
				int incr = PrevCode.Value;

				switch (Codes[count].Code[count2].Command & 0xFF000000) {
				// Gameshark / AR
				case 0x88000000:
					Address = 0x80000000 | (Codes[count].Code[count2].Command & 0xFFFFFF);
					Memory = Codes[count].Code[count2].Value;
					for (count3=0; count3<numrepeats; count3++) {
						r4300i_SB_VAddr(Address, (BYTE)Memory);
						Address += offset;
						Memory += incr;
					}
					break;
				case 0x89000000:
					Address = 0x80000000 | (Codes[count].Code[count2].Command & 0xFFFFFF);
					Memory = Codes[count].Code[count2].Value;
					for (count3=0; count3<numrepeats; count3++) {
						r4300i_SH_VAddr(Address, (WORD)Memory);
						Address += offset;
						Memory += incr;
					}
					break;

				// Xplorer64
				case 0xA8000000:
					Address = 0x80000000 | (ConvertXP64Address(Codes[count].Code[count2].Command) & 0xFFFFFF);
					Memory = ConvertXP64Value(Codes[count].Code[count2].Value);
					for (count3=0; count3<numrepeats; count3++) {
						r4300i_SB_VAddr(Address, (BYTE)Memory);
						Address += offset;
						Memory += incr;
					}
					break;
				case 0xA9000000:
					Address = 0x80000000 | (ConvertXP64Address(Codes[count].Code[count2].Command) & 0xFFFFFF);
					Memory = ConvertXP64Value(Codes[count].Code[count2].Value);
					for (count3=0; count3<numrepeats; count3++) {
						r4300i_SH_VAddr(Address, (WORD)Memory);
						Address += offset;
						Memory += incr;
					}
					break;
				}

			}
			else {
				switch (Codes[count].Code[count2].Command & 0xFF000000) {
				// Gameshark / AR
				case 0x88000000:
					Address = 0x80000000 | (Codes[count].Code[count2].Command & 0xFFFFFF);
					r4300i_SB_VAddr(Address,(BYTE)Codes[count].Code[count2].Value);
					break;
				case 0x89000000:
					Address = 0x80000000 | (Codes[count].Code[count2].Command & 0xFFFFFF);
					r4300i_SH_VAddr(Address,Codes[count].Code[count2].Value);
					break;
				// Xplorer64
				case 0xA8000000:
					Address = 0x80000000 | (ConvertXP64Address(Codes[count].Code[count2].Command) & 0xFFFFFF);
					r4300i_SB_VAddr(Address,(BYTE)ConvertXP64Value(Codes[count].Code[count2].Value));
					break;
				case 0xA9000000:
					Address = 0x80000000 | (ConvertXP64Address(Codes[count].Code[count2].Command) & 0xFFFFFF);
					r4300i_SH_VAddr(Address,ConvertXP64Value(Codes[count].Code[count2].Value));
					break;
				default:
					break;
				}
			}

			PrevCode.Command=Codes[count].Code[count2].Command;
			PrevCode.Value=Codes[count].Code[count2].Value;
		}
	}
}

/********************************************************************************************
  ApplyCheats

  Purpose: Patch codes into memory
  Parameters: None
  Returns: None

********************************************************************************************/
int ApplyCheatEntry (GAMESHARK_CODE * Code, BOOL Execute ) {
	DWORD Address;
	WORD  Memory;

	switch (Code->Command & 0xFF000000) {
	// Gameshark / AR
	case 0x50000000:													// Added by Witten (witten@pj64cheats.net)
		{
			int numrepeats = (Code->Command & 0x0000FF00) >> 8;
			int offset = Code->Command & 0x000000FF;
			int incr = Code->Value;
			int count;

			switch (Code[1].Command & 0xFF000000) {
			case 0x80000000:
				Address = 0x80000000 | (Code[1].Command & 0xFFFFFF);
				Memory = Code[1].Value;
				for (count=0; count<numrepeats; count++) {
					r4300i_SB_VAddr(Address, (BYTE)Memory);
					Address += offset;
					Memory += incr;
				}
				return 2;
			case 0x81000000:
				Address = 0x80000000 | (Code[1].Command & 0xFFFFFF);
				Memory = Code[1].Value;
				for (count=0; count<numrepeats; count++) {
					r4300i_SH_VAddr(Address, (WORD)Memory);
					Address += offset;
					Memory += incr;
				}
				return 2;
			default: return 1;
			}
		}
		break;
	case 0x80000000:
		Address = 0x80000000 | (Code->Command & 0xFFFFFF);
		if (Execute) { r4300i_SB_VAddr(Address,(BYTE)Code->Value); }
		break;
	case 0x81000000:
		Address = 0x80000000 | (Code->Command & 0xFFFFFF);
		if (Execute) { r4300i_SH_VAddr(Address,Code->Value); }
		break;
	case 0xA0000000:
		Address = 0xA0000000 | (Code->Command & 0xFFFFFF);
		if (Execute) { r4300i_SB_VAddr(Address,(BYTE)Code->Value);  }
		break;
	case 0xA1000000:
		Address = 0xA0000000 | (Code->Command & 0xFFFFFF);
		if (Execute) { r4300i_SH_VAddr(Address,Code->Value); }
		break;
	case 0xD0000000:													// Added by Witten (witten@pj64cheats.net)
		Address = 0x80000000 | (Code->Command & 0xFFFFFF);
		r4300i_LB_VAddr(Address, (BYTE*) &Memory);
		Memory &= 0x00FF;
		if (Memory != Code->Value) { Execute = FALSE; }
		return ApplyCheatEntry(&Code[1],Execute) + 1;
	case 0xD1000000:													// Added by Witten (witten@pj64cheats.net)
		Address = 0x80000000 | (Code->Command & 0xFFFFFF);
		r4300i_LH_VAddr(Address, (WORD*) &Memory);
		if (Memory != Code->Value) { Execute = FALSE; }
		return ApplyCheatEntry(&Code[1],Execute) + 1;
	case 0xD2000000:													// Added by Witten (witten@pj64cheats.net)
		Address = 0x80000000 | (Code->Command & 0xFFFFFF);
		r4300i_LB_VAddr(Address, (BYTE*) &Memory);
		Memory &= 0x00FF;
		if (Memory == Code->Value) { Execute = FALSE; }
		return ApplyCheatEntry(&Code[1],Execute) + 1;
	case 0xD3000000:													// Added by Witten (witten@pj64cheats.net)
		Address = 0x80000000 | (Code->Command & 0xFFFFFF);
		r4300i_LH_VAddr(Address, (WORD*) &Memory);
		if (Memory == Code->Value) { Execute = FALSE; }
		return ApplyCheatEntry(&Code[1],Execute) + 1;

	// Xplorer64 (Author: Witten)
	case 0x30000000:
	case 0x82000000:
	case 0x84000000:
		Address = 0x80000000 | (Code->Command & 0xFFFFFF);
		if (Execute) { r4300i_SB_VAddr(Address,(BYTE)Code->Value); }
		break;
	case 0x31000000:
	case 0x83000000:
	case 0x85000000:
		Address = 0x80000000 | (Code->Command & 0xFFFFFF);
		if (Execute) { r4300i_SH_VAddr(Address,Code->Value); }
		break;
	case 0xE8000000:
		Address = 0x80000000 | (ConvertXP64Address(Code->Command) & 0xFFFFFF);
		if (Execute) { r4300i_SB_VAddr(Address,(BYTE)ConvertXP64Value(Code->Value)); }
		break;
	case 0xE9000000:
		Address = 0x80000000 | (ConvertXP64Address(Code->Command) & 0xFFFFFF);
		if (Execute) { r4300i_SH_VAddr(Address,ConvertXP64Value(Code->Value)); }
		break;
	case 0xC8000000:
		Address = 0xA0000000 | (ConvertXP64Address(Code->Command) & 0xFFFFFF);
		if (Execute) { r4300i_SB_VAddr(Address,(BYTE)Code->Value);  }
		break;
	case 0xC9000000:
		Address = 0xA0000000 | (ConvertXP64Address(Code->Command) & 0xFFFFFF);
		if (Execute) { r4300i_SH_VAddr(Address,ConvertXP64Value(Code->Value)); }
		break;
	case 0xB8000000:
		Address = 0x80000000 | (ConvertXP64Address(Code->Command) & 0xFFFFFF);
		r4300i_LB_VAddr(Address, (BYTE*) &Memory);
		Memory &= 0x00FF;
		if (Memory != ConvertXP64Value(Code->Value)) { Execute = FALSE; }
		return ApplyCheatEntry(&Code[1],Execute) + 1;
	case 0xB9000000:
		Address = 0x80000000 | (ConvertXP64Address(Code->Command) & 0xFFFFFF);
		r4300i_LH_VAddr(Address, (WORD*) &Memory);
		if (Memory != ConvertXP64Value(Code->Value)) { Execute = FALSE; }
		return ApplyCheatEntry(&Code[1],Execute) + 1;
	case 0xBA000000:
		Address = 0x80000000 | (ConvertXP64Address(Code->Command) & 0xFFFFFF);
		r4300i_LB_VAddr(Address, (BYTE*) &Memory);
		Memory &= 0x00FF;
		if (Memory == ConvertXP64Value(Code->Value)) { Execute = FALSE; }
		return ApplyCheatEntry(&Code[1],Execute) + 1;
	case 0xBB000000:
		Address = 0x80000000 | (ConvertXP64Address(Code->Command) & 0xFFFFFF);
		r4300i_LH_VAddr(Address, (WORD*) &Memory);
		if (Memory == ConvertXP64Value(Code->Value)) { Execute = FALSE; }
		return ApplyCheatEntry(&Code[1],Execute) + 1;

	case 0: return MaxGSEntries; break;
	}
	return 1;
}

void ApplyCheats (void) {
	int CurrentCheat, CurrentEntry;

	for (CurrentCheat = 0; CurrentCheat < NoOfCodes; CurrentCheat ++) {
		for (CurrentEntry = 0; CurrentEntry < MaxGSEntries;) {
			CurrentEntry += ApplyCheatEntry(&Codes[CurrentCheat].Code[CurrentEntry],TRUE);
		}
	}
}

/*void ApplyCheats (void) {
	int count, count2, count3;
	DWORD Address;
	WORD Value;																	// Added by Witten (witten@pj64cheats.net)
	int numrepeats, offset, incr;												// Added by Witten (witten@pj64cheats.net)
	
	for (count = 0; count < NoOfCodes; count ++) {
		for (count2 = 0; count2 < MaxGSEntries; count2 ++) {
			switch (Codes[count].Code[count2].Command & 0xFF000000) {
			case 0x50000000:													// Added by Witten (witten@pj64cheats.net)
				numrepeats = (Codes[count].Code[count2].Command & 0x0000FF00) >> 8;
				offset = Codes[count].Code[count2].Command & 0x000000FF;
				incr = Codes[count].Code[count2].Value;
				count2++;
				switch (Codes[count].Code[count2].Command & 0xFF000000) {
				case 0x80000000:
					Address = 0x80000000 | (Codes[count].Code[count2].Command & 0xFFFFFF);
					Value = Codes[count].Code[count2].Value;
					for (count3=0; count3<numrepeats; count3++) {
						r4300i_SB_VAddr(Address, (BYTE)Value);
						Address += offset;
						Value += incr;
					}
					break;
				case 0x81000000:
					Address = 0x80000000 | (Codes[count].Code[count2].Command & 0xFFFFFF);
					Value = Codes[count].Code[count2].Value;
					for (count3=0; count3<numrepeats; count3++) {
						r4300i_SH_VAddr(Address, (WORD)Value);
						Address += offset;
						Value += incr;
					}
					break;
				default:
					break;
				}
				break;
			case 0x80000000:
				Address = 0x80000000 | (Codes[count].Code[count2].Command & 0xFFFFFF);
				r4300i_SB_VAddr(Address,(BYTE)Codes[count].Code[count2].Value);
				break;
			case 0x81000000:
				Address = 0x80000000 | (Codes[count].Code[count2].Command & 0xFFFFFF);
				r4300i_SH_VAddr(Address,Codes[count].Code[count2].Value);
				break;
			case 0xA0000000:
				Address = 0xA0000000 | (Codes[count].Code[count2].Command & 0xFFFFFF);
				r4300i_SB_VAddr(Address,(BYTE)Codes[count].Code[count2].Value);
				break;
			case 0xA1000000:
				Address = 0xA0000000 | (Codes[count].Code[count2].Command & 0xFFFFFF);
				r4300i_SH_VAddr(Address,Codes[count].Code[count2].Value);
				break;
			case 0xD0000000:													// Added by Witten (witten@pj64cheats.net)
				Address = 0x80000000 | (Codes[count].Code[count2].Command & 0xFFFFFF);
				r4300i_LB_VAddr(Address, (BYTE*) &Value);
				Value &= 0x00FF;
				if (Value == Codes[count].Code[count2].Value) {
					count2++;
					switch (Codes[count].Code[count2].Command & 0xFF000000) {
					case 0x80000000:
						Address = 0x80000000 | (Codes[count].Code[count2].Command & 0xFFFFFF);
						r4300i_SB_VAddr(Address,(BYTE)Codes[count].Code[count2].Value);
						break;
					case 0x81000000:
						Address = 0x80000000 | (Codes[count].Code[count2].Command & 0xFFFFFF);
						r4300i_SH_VAddr(Address,Codes[count].Code[count2].Value);
						break;
					default:
						break;
					}
				}
				else {
					count2++;
					break;
				}
				break;
			case 0xD1000000:													// Added by Witten (witten@pj64cheats.net)
				Address = 0x80000000 | (Codes[count].Code[count2].Command & 0xFFFFFF);
				r4300i_LH_VAddr(Address, (WORD*) &Value);
				if (Value == Codes[count].Code[count2].Value) {
					count2++;
					switch (Codes[count].Code[count2].Command & 0xFF000000) {
					case 0x80000000:
						Address = 0x80000000 | (Codes[count].Code[count2].Command & 0xFFFFFF);
						r4300i_SB_VAddr(Address,(BYTE)Codes[count].Code[count2].Value);
						break;
					case 0x81000000:
						Address = 0x80000000 | (Codes[count].Code[count2].Command & 0xFFFFFF);
						r4300i_SH_VAddr(Address,Codes[count].Code[count2].Value);
						break;
					default:
						break;
					}
				}
				else {
					count2++;
					break;
				}
				break;
			case 0xD2000000:													// Added by Witten (witten@pj64cheats.net)
				Address = 0x80000000 | (Codes[count].Code[count2].Command & 0xFFFFFF);
				r4300i_LB_VAddr(Address, (BYTE*) &Value);
				Value &= 0x00FF;
				if (Value != Codes[count].Code[count2].Value) {
					count2++;
					switch (Codes[count].Code[count2].Command & 0xFF000000) {
					case 0x80000000:
						Address = 0x80000000 | (Codes[count].Code[count2].Command & 0xFFFFFF);
						r4300i_SB_VAddr(Address,(BYTE)Codes[count].Code[count2].Value);
						break;
					case 0x81000000:
						Address = 0x80000000 | (Codes[count].Code[count2].Command & 0xFFFFFF);
						r4300i_SH_VAddr(Address,Codes[count].Code[count2].Value);
						break;
					default:
						break;
					}
				}
				else {
					count2++;
				}
				break;
			case 0xD3000000:													// Added by Witten (witten@pj64cheats.net)
				Address = 0x80000000 | (Codes[count].Code[count2].Command & 0xFFFFFF);
				r4300i_LH_VAddr(Address, (WORD*) &Value);
				if (Value != Codes[count].Code[count2].Value) {
					count2++;
					switch (Codes[count].Code[count2].Command & 0xFF000000) {
					case 0x80000000:
						Address = 0x80000000 | (Codes[count].Code[count2].Command & 0xFFFFFF);
						r4300i_SB_VAddr(Address,(BYTE)Codes[count].Code[count2].Value);
						break;
					case 0x81000000:
						Address = 0x80000000 | (Codes[count].Code[count2].Command & 0xFFFFFF);
						r4300i_SH_VAddr(Address,Codes[count].Code[count2].Value);
						break;
					default:
						break;
					}
				}
				else {
					count2++;
				}
				break;
			case 0: count2 = MaxGSEntries; break;
			}
		}
	} 
}*/

void ChangeRomCheats(HWND hwndOwner) {
	char OrigRomName[sizeof(RomName)], OrigFileName[sizeof(CurrentFileName)];
	BYTE OrigByteHeader[sizeof(RomHeader)];
	DWORD OrigFileSize;

	//Load information about target rom and back up current information
	strncpy(OrigRomName,RomName,sizeof(OrigRomName));	
	strncpy(OrigFileName,CurrentFileName,sizeof(OrigFileName));	
	memcpy(OrigByteHeader,RomHeader,sizeof(RomHeader));
	strncpy(CurrentFileName,CurrentRBFileName,sizeof(CurrentFileName));	
	OrigFileSize = RomFileSize;
	LoadRomHeader();
	if (!RememberCheats) { DisableAllCheats(); }
	
	ManageCheats(hwndOwner);

	//Restore details
	strncpy(RomName,OrigRomName,sizeof(RomName));
	strncpy(CurrentFileName,OrigFileName,sizeof(CurrentFileName));
	memcpy(RomHeader,OrigByteHeader,sizeof(RomHeader));
	RomFileSize = OrigFileSize;
}

/********************************************************************************************
  CheatActive

  Purpose: Checks in registry if cheat is active
  Parameters: char*
    Name: name of cheat
  Returns: Boolean
    True: cheat is active
	False: cheat isn't active or cheat isn't found in registry

********************************************************************************************/
BOOL CheatActive (char * Name) {
	char String[300], Identifier[100];
	HKEY hKeyResults = 0;
	long lResult;
	
	sprintf(Identifier,"%08X-%08X-C:%X",*(DWORD *)(&RomHeader[0x10]),*(DWORD *)(&RomHeader[0x14]),RomHeader[0x3D]);
	sprintf(String,"Software\\N64 Emulation\\%s\\Cheats\\%s",AppName,Identifier);
	lResult = RegOpenKeyEx( HKEY_CURRENT_USER,String,0, KEY_ALL_ACCESS,&hKeyResults); // check is game ID excists in registry
	if (lResult == ERROR_SUCCESS) {
		DWORD Type, Bytes, Active;
		char GameName[300];

		Bytes = sizeof(GameName);
		lResult = RegQueryValueEx(hKeyResults,"Name",0,&Type,(LPBYTE)GameName,&Bytes); // get gamename from registry
		Bytes = sizeof(Active);
		lResult = RegQueryValueEx(hKeyResults,Name,0,&Type,(LPBYTE)(&Active),&Bytes); // get cheat-state from registry
		RegCloseKey(hKeyResults);
		if (lResult == ERROR_SUCCESS) { return Active; } // if no errors return active state
	}
	return FALSE;
}

/********************************************************************************************
  CheatCodeExProc

  Purpose: Message handler for 
  Parameters:
  Returns:

********************************************************************************************/
LRESULT CALLBACK CheatsCodeExProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static DWORD CheatNo;
	switch (uMsg) {
	case WM_INITDIALOG:
		CheatNo = lParam;
		{
			char * String = NULL, Identifier[100], CheatName[300],CheatExt[300], * ReadPos;
			LPSTR IniFileName;
			DWORD len;

			SetWindowText(hDlg, GS(CHEAT_CODE_EXT_TITLE));
			SetDlgItemText(hDlg,IDC_NOTE, GS(CHEAT_CODE_EXT_TXT));
			SetDlgItemText(hDlg,IDOK, GS(CHEAT_OK));
			SetDlgItemText(hDlg,IDCANCEL, GS(CHEAT_CANCEL));
			
			GetCheatName(CheatNo,CheatName,sizeof(CheatName));
			SetDlgItemText(hDlg,IDC_CHEAT_NAME,CheatName);
			LoadCheatExt(CheatName,CheatExt,sizeof(CheatExt));

			IniFileName = GetCheatIniFileName();
			sprintf(Identifier,"%08X-%08X-C:%X",*(DWORD *)(&RomHeader[0x10]),*(DWORD *)(&RomHeader[0x14]),RomHeader[0x3D]);
			sprintf(CheatName,"Cheat%d_O",CheatNo);
			_GetPrivateProfileString2(Identifier,CheatName,"",&String,IniFileName);
			ReadPos = String;
			while (strlen(ReadPos) > 0) {
				int index;
				
				if(strchr(ReadPos,',') == NULL) {
					len = strlen(ReadPos);					
				} else {
					len = strchr(ReadPos,',') - ReadPos;
				}
				if (len >= sizeof(CheatName)) { len = sizeof(CheatName) - 1; }
				strncpy(CheatName,ReadPos,len);
				CheatName[len] = 0;
				index = SendMessage(GetDlgItem(hDlg,IDC_CHEAT_LIST),LB_ADDSTRING,0,(LPARAM)CheatName);
				if (strcmp(CheatExt,CheatName) == 0) { 
					SendMessage(GetDlgItem(hDlg,IDC_CHEAT_LIST),LB_SETCURSEL,index,0);
				}
				if (strchr(ReadPos,',') == NULL) {
					ReadPos += strlen(ReadPos);
				} else {
					ReadPos = strchr(ReadPos,',') + 1;
				}
			}
			if (String) { free(String); }
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CHEAT_LIST:
			if (HIWORD(wParam) == LBN_DBLCLK) { PostMessage(hDlg,WM_COMMAND,IDOK,0); break; }
			break;			
		case IDOK:
			{
				char CheatName[300], CheatExten[300];
				int index;

				index = SendMessage(GetDlgItem(hDlg,IDC_CHEAT_LIST),LB_GETCURSEL,0,0);
				if (index < 0) { index = 0; }
				GetDlgItemText(hDlg,IDC_CHEAT_NAME,CheatName,sizeof(CheatName));
				index = SendMessage(GetDlgItem(hDlg,IDC_CHEAT_LIST),LB_GETTEXT,index,(LPARAM)CheatExten);
				SaveCheatExt(CheatName,CheatExten);
				LoadCheats();
			}
			EndDialog(hDlg,0);
			break;
		case IDCANCEL:
			EndDialog(hDlg,0);
			break;
		}
	default:
		return FALSE;
	}
	return TRUE;
}

/********************************************************************************************
  CheatCodeQuantProc

  Purpose: Message handler for 
  Parameters:
  Returns:

********************************************************************************************/
LRESULT CALLBACK CheatsCodeQuantProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static WORD Start, Stop, SelStart, SelStop;
	static DWORD CheatNo;


	switch (uMsg) {
	case WM_INITDIALOG:
		CheatNo = lParam;
		{
			char * String = NULL, Identifier[100], CheatName[300],CheatExt[300], * ReadPos;
			LPSTR IniFileName;

			SetWindowText(hDlg, GS(IDD_Cheats_CodeEx));
			SetDlgItemText(hDlg, IDC_DIGITAL_TEXT, GS(CHEAT_CHOOSE_VALUE));
			SetDlgItemText(hDlg, IDC_VALUE_TEXT, GS(CHEAT_VALUE));
			SetDlgItemText(hDlg, IDC_NOTES_TEXT, GS(CHEAT_NOTES));
			
			IniFileName = GetCheatIniFileName();
			sprintf(Identifier,"%08X-%08X-C:%X",*(DWORD *)(&RomHeader[0x10]),*(DWORD *)(&RomHeader[0x14]),RomHeader[0x3D]);
			sprintf(CheatName,"Cheat%d_RN",CheatNo);
			_GetPrivateProfileString2(Identifier,CheatName,"",&String,IniFileName);
			SetDlgItemText(hDlg,IDC_NOTES,String);
			sprintf(CheatName,"Cheat%d_R",CheatNo);
			_GetPrivateProfileString2(Identifier,CheatName,"",&String,IniFileName);
			Start = (WORD)(String[0] == '$'?AsciiToHex(&String[1]):atol(String));
			ReadPos  = strrchr(String,'-');
			if (ReadPos != NULL) {
				Stop = (WORD)(ReadPos[1] == '$'?AsciiToHex(&ReadPos[2]):atol(&ReadPos[1]));			
			} else {
				Stop = 0;
			}
			
			GetCheatName(CheatNo,CheatName,sizeof(CheatName));
			SetDlgItemText(hDlg,IDC_CHEAT_NAME,CheatName);
			LoadCheatExt(CheatName,CheatExt,sizeof(CheatExt));
			SetDlgItemText(hDlg,IDC_VALUE,CheatExt);
			sprintf(CheatExt,"%s $%X %s $%X",GS(CHEAT_FROM),Start,GS(CHEAT_TO),Stop);
			SetDlgItemText(hDlg,IDC_RANGE,CheatExt);
			if (String) { free(String); }
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_VALUE:
			if (HIWORD(wParam) == EN_UPDATE) {
				TCHAR szTmp[10], szTmp2[10];
				DWORD Value;
				GetDlgItemText(hDlg,IDC_VALUE,szTmp,sizeof(szTmp));
				Value = szTmp[0] =='$'?AsciiToHex(&szTmp[1]):AsciiToHex(szTmp);
				if (Value > Stop) { Value = Stop; }
				if (Value < Start) { Value = Start; }
				sprintf(szTmp2,"$%X",Value);
				if (strcmp(szTmp,szTmp2) != 0) {
					SetDlgItemText(hDlg,IDC_VALUE,szTmp2);
					if (SelStop == 0) { SelStop = strlen(szTmp2); SelStart = SelStop; }
					SendDlgItemMessage(hDlg,IDC_VALUE,EM_SETSEL,(WPARAM)SelStart,(LPARAM)SelStop);
				} else {
					WORD NewSelStart, NewSelStop;
					SendDlgItemMessage(hDlg,IDC_VALUE,EM_GETSEL,(WPARAM)&NewSelStart,(LPARAM)&NewSelStop);
					if (NewSelStart != 0) { SelStart = NewSelStart; SelStop = NewSelStop; }
				}
			}
			break;
		case IDOK:
			{
				TCHAR CheatName[300], CheatExten[300], szTmp[10];
				DWORD Value;

				GetDlgItemText(hDlg,IDC_VALUE,szTmp,sizeof(szTmp));
				Value = szTmp[0] =='$'?AsciiToHex(&szTmp[1]):AsciiToHex(szTmp);
				if (Value > Stop) { Value = Stop; }
				if (Value < Start) { Value = Start; }
				
				GetDlgItemText(hDlg,IDC_CHEAT_NAME,CheatName,sizeof(CheatName));
				sprintf(CheatExten,"$%X",Value);
				SaveCheatExt(CheatName,CheatExten);
				LoadCheats();
			}
			EndDialog(hDlg,0);
			break;
		case IDCANCEL:
			EndDialog(hDlg,0);
			break;
		}
	default:
		return FALSE;
	}
	return TRUE;
}

/********************************************************************************************
  CheatUsesCodeExtensions

  Purpose: 
  Parameters:
  Returns:

********************************************************************************************/
BOOL CheatUsesCodeExtensions (char * CheatString) {
	BOOL CodeExtension;
	DWORD count, len;

	char * ReadPos;

	if (strlen(CheatString) == 0){ return FALSE; }
	if (strchr(CheatString,'"') == NULL) { return FALSE; }
	len = strrchr(CheatString,'"') - strchr(CheatString,'"') - 1;

	ReadPos = strrchr(CheatString,'"') + 2;
	CodeExtension = FALSE;
	for (count = 0; count < MaxGSEntries && CodeExtension == FALSE; count ++) {
		if (strchr(ReadPos,' ') == NULL) { break; }
		ReadPos = strchr(ReadPos,' ') + 1;
		if (ReadPos[0] == '?' && ReadPos[1]== '?') { CodeExtension = TRUE; }
		if (ReadPos[2] == '?' && ReadPos[3]== '?') { CodeExtension = TRUE; }
		if (strchr(ReadPos,',') == NULL) { continue; }
		ReadPos = strchr(ReadPos,',') + 1;
	}
	return CodeExtension;
}

int ReadCodeString (HWND hDlg) {
	int numlines, linecount, len;
	char str[128];
	int i;
	char* formatnormal =   "XXXXXXXX XXXX";
	char* formatoptionlb = "XXXXXXXX XX??";
	char* formatoptionw =  "XXXXXXXX ????";
	char tempformat[128];

	validcodes = TRUE;
	nooptions = TRUE;
	codeformat = -1;
	numcodes = 0;

	memset(codestring, '\0', 2048);

	numlines = SendDlgItemMessage(hDlg, IDC_CHEAT_CODES, EM_GETLINECOUNT, 0, 0);
	if (numlines == 0) { validcodes = FALSE; }
	
	for (linecount=0; linecount<numlines; linecount++) //read line after line (bypassing limitation GetDlgItemText)
	{
		memset(tempformat, 0, sizeof(tempformat));

		//str[0] = sizeof(str) > 255?255:sizeof(str);
		*(LPWORD)str = sizeof(str);
		len = SendDlgItemMessage(hDlg, IDC_CHEAT_CODES, EM_GETLINE, (WPARAM)linecount, (LPARAM)(LPCSTR)str);
		str[len] = 0;

		if (len <= 0) { continue; }

		for (i=0; i<128; i++) {
			if (((str[i] >= 'A') && (str[i] <= 'F')) || ((str[i] >= '0') && (str[i] <= '9'))) { // Is hexvalue
				tempformat[i] = 'X';
			}
			if ((str[i] == ' ') || (str[i] == '?')) {
				tempformat[i] = str[i];
			}
			if (str[i] == 0) { break; }
		}
		if (strcmp(tempformat, formatnormal) == 0) {
			strcat(codestring, ",");
			strcat(codestring, str);
			numcodes++;
			if (codeformat < 0) codeformat = 0;
		}
		else if (strcmp(tempformat, formatoptionlb) == 0) {
			if (codeformat != 2) {
				strcat(codestring, ",");
				strcat(codestring, str);
				numcodes++;
				codeformat = 1;
				nooptions = FALSE;
				validoptions = FALSE;
			}
			else validcodes = FALSE;
		}
		else if (strcmp(tempformat, formatoptionw) == 0) {
			if (codeformat != 1) {
				strcat(codestring, ",");
				strcat(codestring, str);
				numcodes++;
				codeformat = 2;
				nooptions = FALSE;
				validoptions = FALSE;
			}
			else validcodes = FALSE;
		}
		else {
			validcodes = FALSE;
		}
	}

	return 0;
}

void ReadOptionsString(HWND hDlg)
{
	int numlines, linecount, len;
	char str[128];
	int i, j;

	int leftorder = 0;
	int rightorder = 0;


	validoptions = TRUE;
	numoptions = 0;

	memset(optionsstring, '\0', 2048);

	numlines = SendDlgItemMessage(hDlg, IDC_CHEAT_OPTIONS, EM_GETLINECOUNT, 0, 0);
	
	for (linecount=0; linecount<numlines; linecount++) //read line after line (bypassing limitation GetDlgItemText)
	{
		memset(str,0,sizeof(str));
		//str[0] = sizeof(str) > 255?255:sizeof(str);
		*(LPWORD)str = sizeof(str);
		len = SendDlgItemMessage(hDlg, IDC_CHEAT_OPTIONS, EM_GETLINE, (WPARAM)linecount, (LPARAM)(LPCSTR)str);
		str[len] = 0;

		if (len > 0) {
			switch (codeformat) {
			case 1: //option = lower byte
				if (len >= 2) {
					for (i=0; i<2; i++) {
						if (!(((str[i] >= 'a') && (str[i] <= 'f')) || ((str[i] >= 'A') && (str[i] <= 'F')) || ((str[i] >= '0') && (str[i] <= '9')))) {
							validoptions = FALSE;
							break;
						}
					}

					if ((str[2] != ' ') && (len > 2)) {
						validoptions = FALSE;
						break;
					}

					for (j=0; j<2; j++) {
						str[j] = toupper(str[j]);
					}

					strcat(optionsstring, ",$");
					strcat(optionsstring, str);
					numoptions++;
				}
				else {
					validoptions = FALSE;
					break;
				}
				break;

			case 2: //option = word
				if (len >= 4) {
					for (i=0; i<4; i++) {
						if (!(((str[i] >= 'a') && (str[i] <= 'f')) || ((str[i] >= 'A') && (str[i] <= 'F')) || ((str[i] >= '0') && (str[i] <= '9')))) {
							validoptions = FALSE;
							break;
						}
					}

					if (str[4] != ' ' && (len > 4)) {
						validoptions = FALSE;
						break;
					}

					for (j=0; j<4; j++) {
						str[j] = toupper(str[j]);
					}

					strcat(optionsstring, ",$");
					strcat(optionsstring, str);
					numoptions++;
				}
				else {
					validoptions = FALSE;
					break;
				}
				break;

			default:
				break;
			}
		}
	}

	if (numoptions < 1) validoptions = FALSE;
}

/********************************************************************************************
  CheatAddProc

  Purpose: 
  Parameters:
  Returns:

********************************************************************************************/
LRESULT CALLBACK CheatAddProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	char str[1024];

	switch (uMsg) {
	case WM_INITDIALOG:
		SetWindowText(hDlg,GS(CHEAT_ADDCHEAT_FRAME));
		SetWindowText(GetDlgItem(hDlg,IDC_NAME),GS(CHEAT_ADDCHEAT_NAME));
		SetWindowText(GetDlgItem(hDlg,IDC_CODE),GS(CHEAT_ADDCHEAT_CODE));
		SetWindowText(GetDlgItem(hDlg,IDC_LABEL_OPTIONS),GS(CHEAT_ADDCHEAT_OPT));
		SetWindowText(GetDlgItem(hDlg,IDC_CODE_DES),GS(CHEAT_ADDCHEAT_CODEDES));
		SetWindowText(GetDlgItem(hDlg,IDC_LABEL_OPTIONS_FORMAT),GS(CHEAT_ADDCHEAT_OPTDES));
		SetWindowText(GetDlgItem(hDlg,IDC_CHEATNOTES),GS(CHEAT_ADDCHEAT_NOTES));
		SetWindowText(GetDlgItem(hDlg,IDC_NEWCHEAT),GS(CHEAT_ADDCHEAT_NEW));
		SetWindowText(GetDlgItem(hDlg,IDC_ADD),GS(CHEAT_ADDCHEAT_ADD));
		validcodes = FALSE;
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			EndDialog(hDlg,0);
			break;
		case IDC_CODE_NAME:
			if (HIWORD(wParam) == EN_CHANGE) {
				if (validcodes && (validoptions || nooptions) && SendDlgItemMessage(hDlg,IDC_CODE_NAME,EM_LINELENGTH,0,0) > 0){
					EnableWindow(GetDlgItem(hDlg, IDC_ADD), TRUE);
				}
				else {
					EnableWindow(GetDlgItem(hDlg, IDC_ADD), FALSE);
				}
			}
			break;
		case IDC_CHEAT_CODES:
			if (HIWORD(wParam) == EN_CHANGE) {
				ReadCodeString(hDlg);
				GetDlgItemText(hDlg, IDC_CHEAT_CODES, str, 1024);
				if ((codeformat > 0) && !IsWindowEnabled(GetDlgItem(hDlg, IDC_LABEL_OPTIONS)) ) {
					EnableWindow(GetDlgItem(hDlg, IDC_LABEL_OPTIONS), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_LABEL_OPTIONS_FORMAT), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_CHEAT_OPTIONS), TRUE);
				}
				if ((codeformat <= 0) && IsWindowEnabled(GetDlgItem(hDlg, IDC_LABEL_OPTIONS)) ) {
					EnableWindow(GetDlgItem(hDlg, IDC_LABEL_OPTIONS), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_LABEL_OPTIONS_FORMAT), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_CHEAT_OPTIONS), FALSE);
				}

				if (!nooptions) ReadOptionsString(hDlg);

				if (validcodes && (validoptions || nooptions) && SendDlgItemMessage(hDlg,IDC_CODE_NAME,EM_LINELENGTH,0,0) > 0){
					EnableWindow(GetDlgItem(hDlg, IDC_ADD), TRUE);
				}
				else {
					EnableWindow(GetDlgItem(hDlg, IDC_ADD), FALSE);
				}
			}
			break;

		case IDC_CHEAT_OPTIONS:
			if (HIWORD(wParam) == EN_CHANGE) {
				ReadOptionsString(hDlg);
				if (validcodes && (validoptions || nooptions) && SendDlgItemMessage(hDlg,IDC_CODE_NAME,EM_LINELENGTH,0,0) > 0){
					EnableWindow(GetDlgItem(hDlg, IDC_ADD), TRUE);
				}
				else {
					EnableWindow(GetDlgItem(hDlg, IDC_ADD), FALSE);
				}
			}
			break;
		case IDC_ADD:
			{
				char Identifier[100], CheatName[200], NewCheatName[200], * cheat;
				int CheatLen, count, CheatNo;
				LPSTR IniFileName;

				GetDlgItemText(hDlg,IDC_CODE_NAME,NewCheatName,sizeof(NewCheatName));

				for (count = 0; count < MaxCheats; count ++) {
					GetCheatName(count,CheatName,sizeof(CheatName));
					if (strlen(CheatName) == 0) {
						CheatNo = count;
						break;
					}
					if (strcmp(CheatName,NewCheatName) == 0) {
						MessageBox(hDlg,"Cheat Name is already in use","Error",MB_OK|MB_ICONERROR);
						DisplayError(GS(MSG_CHEAT_NAME_IN_USE));
						SetFocus(GetDlgItem(hDlg,IDC_CODE_NAME));
						return TRUE;
					}
				}
				if (count == MaxCheats) {
					DisplayError(GS(MSG_MAX_CHEATS));
					return TRUE;
				}
				CheatLen = strlen(NewCheatName) + strlen(codestring);
				cheat = malloc(CheatLen + 3);
				sprintf(cheat,"\"%s\"",NewCheatName);

				strcat(cheat, codestring);
				//Add to ini
				IniFileName = GetCheatIniFileName();
				sprintf(Identifier,"%08X-%08X-C:%X",*(DWORD *)(&RomHeader[0x10]),*(DWORD *)(&RomHeader[0x14]),RomHeader[0x3D]);
				_WritePrivateProfileString(Identifier,"Name",RomName,IniFileName);
				sprintf(NewCheatName,"Cheat%d",CheatNo);
				_WritePrivateProfileString(Identifier,NewCheatName,cheat,IniFileName);				
				if (cheat) { free(cheat); cheat = NULL; }

				if (validoptions) {
					cheat = malloc(strlen(optionsstring) + 1);
					strcpy(cheat, optionsstring+1);
					sprintf(NewCheatName,"Cheat%d_O",CheatNo);
					_WritePrivateProfileString(Identifier,NewCheatName,cheat,IniFileName);
					if (cheat) { free(cheat); cheat = NULL; }
				}

				CheatLen = SendDlgItemMessage(hDlg,IDC_NOTES,WM_GETTEXTLENGTH,0,0) + 5;
				if (CheatLen > 5) {
					cheat = malloc(CheatLen);
					GetDlgItemText(hDlg,IDC_NOTES,cheat,CheatLen);
					sprintf(NewCheatName,"Cheat%d_N",CheatNo);
					_WritePrivateProfileString(Identifier,NewCheatName,cheat,IniFileName);
					if (cheat) { free(cheat); cheat = NULL; }
				}

				RefreshCheatManager();
				SetDlgItemText(hDlg,IDC_CODE_NAME,"");
				SetDlgItemText(hDlg,IDC_CHEAT_CODES,"");
				SetDlgItemText(hDlg,IDC_CHEAT_OPTIONS,"");
				SetDlgItemText(hDlg,IDC_NOTES,"");
				EnableWindow(GetDlgItem(hDlg, IDC_ADD), FALSE);
			}
			break;
		case IDC_NEWCHEAT:
			{
				SetDlgItemText(hDlg,IDC_CODE_NAME,"");
				SetDlgItemText(hDlg,IDC_CHEAT_CODES,"");
				SetDlgItemText(hDlg,IDC_CHEAT_OPTIONS,"");
				SetDlgItemText(hDlg,IDC_NOTES,"");
				EnableWindow(GetDlgItem(hDlg, IDC_ADD), FALSE);
			}
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK CheatEditProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static int CheatNo;

	switch (uMsg) {
	case WM_INITDIALOG:
		SetWindowText(hDlg,GS(CHEAT_EDITCHEAT_WINDOW));
		SetWindowText(GetDlgItem(hDlg,IDC_NAME),GS(CHEAT_ADDCHEAT_NAME));
		SetWindowText(GetDlgItem(hDlg,IDC_CODE),GS(CHEAT_ADDCHEAT_CODE));
		SetWindowText(GetDlgItem(hDlg,IDC_LABEL_OPTIONS),GS(CHEAT_ADDCHEAT_OPT));
		SetWindowText(GetDlgItem(hDlg,IDC_CODE_DES),GS(CHEAT_ADDCHEAT_CODEDES));
		SetWindowText(GetDlgItem(hDlg,IDC_LABEL_OPTIONS_FORMAT),GS(CHEAT_ADDCHEAT_OPTDES));
		SetWindowText(GetDlgItem(hDlg,IDC_CHEATNOTES),GS(CHEAT_ADDCHEAT_NOTES));
		SetWindowText(GetDlgItem(hDlg,IDC_ADD),GS(CHEAT_EDITCHEAT_UPDATE));
		{
			char * String = NULL,* ReadPos, *Buffer, Identifier[100], CheatName[500];
			LPSTR IniFileName;
			TVITEM item;
			int len;

			item.mask = TVIF_PARAM ;
			item.hItem = (HTREEITEM)lParam;
			TreeView_GetItem(hCheatTree,&item);
			CheatNo = item.lParam;

			IniFileName = GetCheatIniFileName();
			
			//Get Main cheat Entry
			sprintf(Identifier,"%08X-%08X-C:%X",*(DWORD *)(&RomHeader[0x10]),*(DWORD *)(&RomHeader[0x14]),RomHeader[0x3D]);
			sprintf(CheatName,"Cheat%d",CheatNo);
			_GetPrivateProfileString2(Identifier,CheatName,"",&String,IniFileName);
			
			//Set Cheat Name
			len = strrchr(String,'"') - strchr(String,'"') - 1;
			memset(CheatName,0,sizeof(CheatName));
			strncpy(CheatName,strchr(String,'"') + 1,len);
			SetDlgItemText(hDlg,IDC_CODE_NAME,CheatName);
			
			//Add Gameshark codes to screen			
			ReadPos = strrchr(String,'"') + 2;
			Buffer = malloc(strlen(ReadPos) + MaxGSEntries);
			strcpy(Buffer,"");
			do {
				strncat(Buffer,ReadPos,strchr(ReadPos,',') - ReadPos);
				ReadPos = strchr(ReadPos,',');
				if (ReadPos != NULL) { 
					strcat(Buffer,"\r\n");
					ReadPos += 1; 
				}
			} while (ReadPos);
			SetDlgItemText(hDlg,IDC_CHEAT_CODES,Buffer);
			if (Buffer) { free(Buffer); Buffer = NULL; }			
			if (String) { free(String); String = NULL; }
						
			//Add option values to screen			
			sprintf(CheatName,"Cheat%d_O",item.lParam);
			_GetPrivateProfileString2(Identifier,CheatName,"",&String,IniFileName);
			if (strlen(String) > 0) {
				ReadPos = strchr(String,'$') + 1;
				Buffer = malloc(strlen(ReadPos) + 2);
				strcpy(Buffer,"");
				do {
					strncat(Buffer,ReadPos,strchr(ReadPos,',') - ReadPos);
					ReadPos = strchr(ReadPos,'$');
					if (ReadPos != NULL) { 
						strcat(Buffer,"\r\n");
						ReadPos += 1; 
					}
				} while (ReadPos);
				SetDlgItemText(hDlg,IDC_CHEAT_OPTIONS,Buffer);
			}
			if (Buffer) { free(Buffer); Buffer = NULL; }			
			if (String) { free(String); String = NULL; }
			
			//Add cheat Notes
			sprintf(CheatName,"Cheat%d_N",item.lParam);
			_GetPrivateProfileString2(Identifier,CheatName,"",&String,IniFileName);
			SetDlgItemText(hDlg,IDC_NOTES,String);
			if (String) { free(String); String = NULL; }

			//Update Screen
			PostMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_CHEAT_CODES,EN_CHANGE),(LPARAM)GetDlgItem(hDlg,IDC_CHEAT_CODES));
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			EndDialog(hDlg,0);
			break;
		case IDC_CODE_NAME:
			if (HIWORD(wParam) == EN_CHANGE) {
				if (validcodes && (validoptions || nooptions) && SendDlgItemMessage(hDlg,IDC_CODE_NAME,EM_LINELENGTH,0,0) > 0){
					EnableWindow(GetDlgItem(hDlg, IDC_ADD), TRUE);
				}
				else {
					EnableWindow(GetDlgItem(hDlg, IDC_ADD), FALSE);
				}
			}
			break;
		case IDC_CHEAT_CODES:
			if (HIWORD(wParam) == EN_CHANGE) {
				ReadCodeString(hDlg);
				if ((codeformat > 0) && !IsWindowEnabled(GetDlgItem(hDlg, IDC_LABEL_OPTIONS)) ) {
					EnableWindow(GetDlgItem(hDlg, IDC_LABEL_OPTIONS), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_LABEL_OPTIONS_FORMAT), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_CHEAT_OPTIONS), TRUE);
				}
				if ((codeformat <= 0) && IsWindowEnabled(GetDlgItem(hDlg, IDC_LABEL_OPTIONS)) ) {
					EnableWindow(GetDlgItem(hDlg, IDC_LABEL_OPTIONS), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_LABEL_OPTIONS_FORMAT), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_CHEAT_OPTIONS), FALSE);
				}

				if (!nooptions) ReadOptionsString(hDlg);

				if (validcodes && (validoptions || nooptions) && SendDlgItemMessage(hDlg,IDC_CODE_NAME,EM_LINELENGTH,0,0) > 0){
					EnableWindow(GetDlgItem(hDlg, IDC_ADD), TRUE);
				}
				else {
					EnableWindow(GetDlgItem(hDlg, IDC_ADD), FALSE);
				}
			}
			break;
		case IDC_CHEAT_OPTIONS:
			if (HIWORD(wParam) == EN_CHANGE) {
				ReadOptionsString(hDlg);
				if (validcodes && (validoptions || nooptions) && SendDlgItemMessage(hDlg,IDC_CODE_NAME,EM_LINELENGTH,0,0) > 0){
					EnableWindow(GetDlgItem(hDlg, IDC_ADD), TRUE);
				}
				else {
					EnableWindow(GetDlgItem(hDlg, IDC_ADD), FALSE);
				}
			}
			break;
		case IDC_ADD:
			{
				char Identifier[100], Key[100], * Ext[] = {"", "_N", "_O", "_R" };
				char NewCheatName[200], * cheat;
				int CheatLen, type;
				LPSTR IniFileName;

				IniFileName = GetCheatIniFileName();
				sprintf(Identifier,"%08X-%08X-C:%X",*(DWORD *)(&RomHeader[0x10]),*(DWORD *)(&RomHeader[0x14]),RomHeader[0x3D]);

				//Delete old Entries
				for (type = 0; type < (sizeof(Ext) / sizeof(char *)); type ++) {
					sprintf(Key,"Cheat%d%s",CheatNo,Ext[type]);
					_DeletePrivateProfileString(Identifier,Key,IniFileName);
				}
				//Insert New Entries
				GetDlgItemText(hDlg,IDC_CODE_NAME,NewCheatName,sizeof(NewCheatName));
				CheatLen = strlen(NewCheatName) + strlen(codestring);
				cheat = malloc(CheatLen + 3);
				sprintf(cheat,"\"%s\"",NewCheatName);
				strcat(cheat, codestring);

				//Add to ini
				_WritePrivateProfileString(Identifier,"Name",RomName,IniFileName);
				sprintf(NewCheatName,"Cheat%d",CheatNo);
				_WritePrivateProfileString(Identifier,NewCheatName,cheat,IniFileName);				
				if (cheat) { free(cheat); cheat = NULL; }

				if (validoptions) {
					cheat = malloc(strlen(optionsstring) + 1);
					strcpy(cheat, optionsstring+1);
					sprintf(NewCheatName,"Cheat%d_O",CheatNo);
					_WritePrivateProfileString(Identifier,NewCheatName,cheat,IniFileName);
					if (cheat) { free(cheat); cheat = NULL; }
				}

				CheatLen = SendDlgItemMessage(hDlg,IDC_NOTES,WM_GETTEXTLENGTH,0,0) + 5;
				if (CheatLen > 5) {
					cheat = malloc(CheatLen);
					GetDlgItemText(hDlg,IDC_NOTES,cheat,CheatLen);
					sprintf(NewCheatName,"Cheat%d_N",CheatNo);
					_WritePrivateProfileString(Identifier,NewCheatName,cheat,IniFileName);
					if (cheat) { free(cheat); cheat = NULL; }
				}

				RefreshCheatManager();
				EndDialog(hDlg,0);
			}
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void ChangeChildrenStatus(HTREEITEM hParent, BOOL Checked) {
	HTREEITEM hItem = TreeView_GetChild(hCheatTree, hParent);
	if (hItem == NULL) {
		//Save Cheat
		char CheatName[500];
		TVITEM item;

		if (hParent == TVI_ROOT) { return; }

		item.mask = TVIF_PARAM ;
		item.hItem = hParent;
		TreeView_GetItem(hCheatTree,&item);

		GetCheatName(item.lParam,CheatName,sizeof(CheatName));
		SaveCheat(CheatName,Checked);
		return; 
	}
	while (hItem != NULL) {
		ChangeChildrenStatus(hItem,Checked);
		_TreeView_SetCheckState(hCheatTree,hItem,Checked?TV_STATE_CHECKED:TV_STATE_CLEAR); 
		hItem = TreeView_GetNextSibling(hCheatTree,hItem);
	}	
}

void CheckParentStatus(HTREEITEM hParent) {
	int CurrentState, InitialState;
	HTREEITEM hItem;

	if (!hParent) { return; }
	hItem = TreeView_GetChild(hCheatTree, hParent);	
	InitialState = _TreeView_GetCheckState(hCheatTree,hParent);
	CurrentState = _TreeView_GetCheckState(hCheatTree,hItem);
	
	while (hItem != NULL) {
		if (_TreeView_GetCheckState(hCheatTree,hItem) != CurrentState) { 
			CurrentState = TV_STATE_INDETERMINATE; 
			break; 
		}
		hItem = TreeView_GetNextSibling(hCheatTree,hItem);
	}
	_TreeView_SetCheckState(hCheatTree,hParent,CurrentState); 
	if (InitialState != CurrentState) { 
		CheckParentStatus(TreeView_GetParent(hCheatTree,hParent));
	}
}

/********************************************************************************************
  CheatListProc

  Purpose: 
  Parameters:
  Returns:

********************************************************************************************/
LRESULT CALLBACK CheatListProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HTREEITEM hSelectedItem;

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			DWORD Style;
			RECT rcList;
			RECT rcButton;

			SetWindowText(GetDlgItem(hDlg,IDC_CHEATSFRAME),GS(CHEAT_LIST_FRAME));
			SetWindowText(GetDlgItem(hDlg,IDC_NOTESFRAME),GS(CHEAT_NOTES_FRAME));
			SetWindowText(GetDlgItem(hDlg,IDC_UNMARK),GS(CHEAT_MARK_NONE));

			GetWindowRect(GetDlgItem(hDlg, IDC_CHEATSFRAME), &rcList);
			GetWindowRect(GetDlgItem(hDlg, IDC_UNMARK), &rcButton);

			hCheatTree = CreateWindowEx(WS_EX_CLIENTEDGE,WC_TREEVIEW,"",
					WS_CHILD | WS_BORDER | WS_VISIBLE | WS_VSCROLL | TVS_HASLINES | 
					TVS_HASBUTTONS | TVS_LINESATROOT  | TVS_DISABLEDRAGDROP |WS_TABSTOP|
					TVS_FULLROWSELECT, 8, 15, rcList.right-rcList.left-16, rcButton.top-rcList.top-22, hDlg, (HMENU)IDC_MYTREE, hInst, NULL);
			Style = GetWindowLong(hCheatTree,GWL_STYLE);					
			SetWindowLong(hCheatTree,GWL_STYLE,TVS_CHECKBOXES |TVS_SHOWSELALWAYS| Style);

			{
				HIMAGELIST hImageList;
				HBITMAP hBmp;

				hImageList = ImageList_Create( 16,16, ILC_COLOR | ILC_MASK, 40, 40);
				hBmp = LoadBitmap(hInst,MAKEINTRESOURCE(IDB_BITMAP1));
				ImageList_AddMasked(hImageList,hBmp, RGB(255,0,255));
				DeleteObject(hBmp);
				
				TreeView_SetImageList(hCheatTree,hImageList,TVSIL_STATE);
			}
			hSelectedItem = NULL;

		}
		break;
	case WM_SIZE:
		{
			int nWidth = LOWORD(lParam);  // width of client area 
			int nHeight = HIWORD(lParam); // height of client area 

			SetWindowPos(GetDlgItem(hDlg,IDC_CHEATSFRAME),NULL,0,0,nWidth,nHeight - 100,SWP_NOOWNERZORDER);
			SetWindowPos(GetDlgItem(hDlg,IDC_NOTESFRAME),NULL,0,nHeight - 96,nWidth,95,SWP_NOOWNERZORDER);
			SetWindowPos(GetDlgItem(hDlg,IDC_NOTES),NULL,6,nHeight - 80,nWidth - 12,72,SWP_NOOWNERZORDER);
			SetWindowPos(GetDlgItem(hDlg,IDC_UNMARK),NULL,nWidth - 110,nHeight - 124,0,0,SWP_NOSIZE|SWP_NOOWNERZORDER);
			SetWindowPos(GetDlgItem(hDlg,IDC_DELETE),NULL,nWidth - 165,nHeight - 124,0,0,SWP_NOSIZE|SWP_NOOWNERZORDER);
			SetWindowPos(hCheatTree,NULL,0,0,nWidth - 13,nHeight - 142,SWP_NOMOVE|SWP_NOOWNERZORDER);

		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_POPUP_ADDNEWCHEAT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_Cheats_Add),hDlg,(DLGPROC)CheatAddProc);
			break;
		case ID_POPUP_EDIT:
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_Cheats_Edit),hDlg,(DLGPROC)CheatEditProc,(LPARAM)hSelectedItem);
			break;
		case ID_POPUP_DELETE:
			{
				TVITEM item;

				int Response = MessageBox(hDlg,GS(MSG_DEL_SURE),GS(MSG_DEL_TITLE),MB_YESNO|MB_ICONQUESTION);
				if (Response != IDYES) { break; }

				//Delete selected cheat
				item.hItem = hSelectedItem;
				item.mask = TVIF_PARAM ;
				TreeView_GetItem(hCheatTree,&item);

				DeleteCheat(item.lParam);
				RefreshCheatManager();
			}
			break;
		case IDC_UNMARK: 
			ChangeChildrenStatus(TVI_ROOT,FALSE); 
			LoadCheats();
			break;
		}
		break;
	case WM_NOTIFY:
		{
		   LPNMHDR lpnmh = (LPNMHDR) lParam;
    
		   if ((lpnmh->code  == NM_RCLICK) && (lpnmh->idFrom == IDC_MYTREE))
		   {
				{
					TVHITTESTINFO ht = {0};
					DWORD dwpos = GetMessagePos();

					// include <windowsx.h> and <windows.h> header files
					ht.pt.x = GET_X_LPARAM(dwpos);
					ht.pt.y = GET_Y_LPARAM(dwpos);
					MapWindowPoints(HWND_DESKTOP, lpnmh->hwndFrom, &ht.pt, 1);

					if (BasicMode) { return TRUE; }
					TreeView_HitTest(lpnmh->hwndFrom, &ht);
					hSelectedItem = ht.hItem;
				}
				{
					HMENU hMenu = LoadMenu(hInst,MAKEINTRESOURCE(IDR_CHEAT_MENU));
					HMENU hPopupMenu = GetSubMenu(hMenu,0);
					POINT Mouse;

					GetCursorPos(&Mouse);
					
					MenuSetText(hPopupMenu, 0, GS(CHEAT_ADDNEW), NULL);
					MenuSetText(hPopupMenu, 1, GS(CHEAT_EDIT), NULL);
					MenuSetText(hPopupMenu, 3, GS(CHEAT_DELETE), NULL);

					if (hSelectedItem == NULL || TreeView_GetChild(hCheatTree,hSelectedItem) != NULL) { 
						DeleteMenu(hPopupMenu,3,MF_BYPOSITION);
						DeleteMenu(hPopupMenu,2,MF_BYPOSITION);
						DeleteMenu(hPopupMenu,1,MF_BYPOSITION);
					}
					TrackPopupMenu(hPopupMenu, 0, Mouse.x, Mouse.y, 0,hDlg, NULL);
					DestroyMenu(hMenu);
				}		   
		   }
		   if ((lpnmh->code  == NM_CLICK) && (lpnmh->idFrom == IDC_MYTREE))
		   {
				TVHITTESTINFO ht = {0};
				DWORD dwpos = GetMessagePos();

				// include <windowsx.h> and <windows.h> header files
				ht.pt.x = GET_X_LPARAM(dwpos);
				ht.pt.y = GET_Y_LPARAM(dwpos);
				MapWindowPoints(HWND_DESKTOP, lpnmh->hwndFrom, &ht.pt, 1);

				TreeView_HitTest(lpnmh->hwndFrom, &ht);

				if(TVHT_ONITEMSTATEICON & ht.flags)
				{
					switch (_TreeView_GetCheckState(hCheatTree,ht.hItem)) {
					case TV_STATE_CLEAR:
					case TV_STATE_INDETERMINATE: 
						_TreeView_SetCheckState(hCheatTree,ht.hItem,TV_STATE_CHECKED); 
						ChangeChildrenStatus(ht.hItem,TRUE);
						CheckParentStatus(TreeView_GetParent(hCheatTree,ht.hItem));
						_TreeView_SetCheckState(hCheatTree,ht.hItem,TV_STATE_INDETERMINATE); 
						break;
					case TV_STATE_CHECKED: 
						_TreeView_SetCheckState(hCheatTree,ht.hItem,TV_STATE_CLEAR); 
						ChangeChildrenStatus(ht.hItem,FALSE);
						CheckParentStatus(TreeView_GetParent(hCheatTree,ht.hItem));
						_TreeView_SetCheckState(hCheatTree,ht.hItem,TV_STATE_CHECKED); 
						break;
					}
					LoadCheats();
				}
			}
		   if ((lpnmh->code  == NM_DBLCLK) && (lpnmh->idFrom == IDC_MYTREE))
		   {
				TVHITTESTINFO ht = {0};
				DWORD dwpos = GetMessagePos();

				// include <windowsx.h> and <windows.h> header files
				ht.pt.x = GET_X_LPARAM(dwpos);
				ht.pt.y = GET_Y_LPARAM(dwpos);
				MapWindowPoints(HWND_DESKTOP, lpnmh->hwndFrom, &ht.pt, 1);

				TreeView_HitTest(lpnmh->hwndFrom, &ht);

				if(TVHT_ONITEMLABEL & ht.flags)
				{
					PostMessage(hDlg, UM_CHANGECODEEXTENSION, 0, (LPARAM)ht.hItem);
				}
			}
			if ((lpnmh->code  == TVN_SELCHANGED) && (lpnmh->idFrom == IDC_MYTREE)) {
				HTREEITEM hItem;

				hItem = TreeView_GetSelection(hCheatTree);
				if (TreeView_GetChild(hCheatTree,hItem) == NULL) { 
					char * String = NULL, Lookup[40], Identifier[100];
					LPSTR IniFileName;
					TVITEM item;

					item.mask = TVIF_PARAM ;
					item.hItem = hItem;
					TreeView_GetItem(hCheatTree,&item);

					IniFileName = GetCheatIniFileName();
					sprintf(Identifier,"%08X-%08X-C:%X",*(DWORD *)(&RomHeader[0x10]),*(DWORD *)(&RomHeader[0x14]),RomHeader[0x3D]);

					sprintf(Lookup,"Cheat%d_N",item.lParam);
					_GetPrivateProfileString2(Identifier,Lookup,"",&String,IniFileName);
					SetDlgItemText(hDlg,IDC_NOTES,String);
					if (String) { free(String); }
				} else {
					SetDlgItemText(hDlg,IDC_NOTES,"");
				}
			}
		}
		break;
	case UM_CHANGECODEEXTENSION:
		{
			char Identifier[100], * String = NULL, CheatName[500],CheatExt[300];
			HTREEITEM hItemChanged = (HTREEITEM)lParam;
			LPSTR IniFileName;
			TVITEM item;
	
			item.mask = TVIF_PARAM ;
			item.hItem = hItemChanged;
			TreeView_GetItem(hCheatTree,&item);

			IniFileName = GetCheatIniFileName();
			sprintf(Identifier,"%08X-%08X-C:%X",*(DWORD *)(&RomHeader[0x10]),*(DWORD *)(&RomHeader[0x14]),RomHeader[0x3D]);
			sprintf(CheatName,"Cheat%d",item.lParam);
			_GetPrivateProfileString2(Identifier,CheatName,"",&String,IniFileName);
			if (!CheatUsesCodeExtensions(String)) { 
				if (String) { free(String); }
				break; 
			}
			sprintf(CheatName,"Cheat%d_O",item.lParam);
			_GetPrivateProfileString2(Identifier,CheatName,"",&String,IniFileName);
			if (strlen(String) > 0) {
				DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_Cheats_CodeEx),hDlg,(DLGPROC)CheatsCodeExProc,item.lParam);
			} else {
				sprintf(CheatName,"Cheat%d_R",item.lParam);
				_GetPrivateProfileString2(Identifier,CheatName,"",&String,IniFileName);
				if (strlen(String) > 0) {
					DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_Cheats_Range),hDlg,(DLGPROC)CheatsCodeQuantProc,item.lParam);
				}
			}
			if (String) { free(String); }
			GetCheatName(item.lParam,CheatName,sizeof(CheatName));
			if (!LoadCheatExt(CheatName,CheatExt,sizeof(CheatExt))) {
				sprintf(CheatExt,"?");
			}
			item.mask = TVIF_PARAM | TVIF_TEXT;
			item.pszText = CheatName;
			item.cchTextMax = sizeof(CheatName);
			if (strrchr(CheatName,'\\') != NULL) {
				strcpy(CheatName,strrchr(CheatName,'\\') + 1);
			}
			sprintf(CheatName,"%s (=> %s)",CheatName,CheatExt);
			TreeView_SetItem(hCheatTree,&item);
		}
		break; 
	default:
		return FALSE;
	}
	return TRUE;
}

void DeleteCheat(int CheatNo) {
	char Identifier[100], Key[100], * Ext[] = {"", "_N", "_O", "_R" };
	LPSTR IniFileName;
	int type;

	IniFileName = GetCheatIniFileName();
	sprintf(Identifier,"%08X-%08X-C:%X",*(DWORD *)(&RomHeader[0x10]),*(DWORD *)(&RomHeader[0x14]),RomHeader[0x3D]);

	for (type = 0; type < (sizeof(Ext) / sizeof(char *)); type ++) {
		sprintf(Key,"Cheat%d%s",CheatNo,Ext[type]);
		_DeletePrivateProfileString(Identifier,Key,IniFileName);
	}
	RenameCheat(CheatNo);

}

void RenameCheat(int CheatNo) {
	char *Input = NULL, *Data = NULL, * Pos = NULL, Identifier[100], CurrentSection[300];
	int DataLen = 0, DataLeft, result, count;
	static long Fpos = 0;
	LPSTR IniFileName;
	long WritePos;
	FILE * fInput;

#ifdef WIN32
char * LineFeed = "\r\n";
#else
char * LineFeed = "\n";
#endif

	IniFileName = GetCheatIniFileName();
	sprintf(Identifier,"%08X-%08X-C:%X",*(DWORD *)(&RomHeader[0x10]),*(DWORD *)(&RomHeader[0x14]),RomHeader[0x3D]);

	fInput = fopen(IniFileName,"r+b");
	if (fInput == NULL) { 
		fInput = fopen(IniFileName,"w+b");
		if (IniFileName == NULL) { return; }
	}
	CurrentSection[0] = 0;

	do {
		if (strcmp(Identifier,CurrentSection) != 0) { 
			Fpos = ftell(fInput) - DataLeft;
		}
		result = fGetString2(fInput,&Input,&Data,&DataLen,&DataLeft);
		if (result <= 1) { continue; }
		
		Pos = Input;
		while (Pos != NULL) {
			Pos = strchr(Pos,'/');
			if (Pos != NULL) {
				if (Pos[1] == '/') { Pos[0] = 0; } else { Pos += 1; }
			}
		}
		
		for (count = strlen(&Input[0]) - 1; count >= 0; count --) {
			if (Input[count] != ' ' && Input[count] != '\r') { break; }
			Input[count] = 0;
		}
		//stip leading spaces
		if (strlen(Input) <= 1) { continue; }
		if (Input[0] == '[') {
			if (Input[strlen(Input) - 1] != ']') { continue; }
			if (strcmp(Identifier,CurrentSection) == 0) { 
				result = -1;
				continue;
			}
			strcpy(CurrentSection,&Input[1]);
			CurrentSection[strlen(CurrentSection) - 1] = 0;
			WritePos = ftell(fInput) - DataLeft;
			continue;
		}
		if (strcmp(Identifier,CurrentSection) != 0) { 
			continue;
		}
		Pos = strchr(Input,'=');
		if (Pos == NULL) { continue; }
		if (strncmp(Input,"Cheat",5) != 0) { 
			WritePos = ftell(fInput) - DataLeft;
			continue;
		}
		if (atoi(&Input[5]) < CheatNo) {
			WritePos = ftell(fInput) - DataLeft;
			continue; 
		}
		if (strchr(Input,'_') > 0 && strchr(Input,'_') < Pos) { Pos = strchr(Input,'_'); }
		{
			long OldLen = strlen(Input) + strlen(LineFeed);
			int Newlen = strlen(Pos) + strlen(LineFeed);
			long CurrentPos = ftell(fInput);
			char Header[100];

			sprintf(Header,"Cheat%d",atoi(&Input[5]) - 1);
			Newlen += strlen(Header);

			if (OldLen != Newlen) {
				fInsertSpaces(fInput,WritePos,Newlen - OldLen);
				CurrentPos += Newlen - OldLen;
			}
			fseek(fInput,WritePos,SEEK_SET);
			fprintf(fInput,"%s%s%s",Header,Pos,LineFeed);
			fflush(fInput);
			fseek(fInput,CurrentPos,SEEK_SET);
		}
		WritePos = ftell(fInput) - DataLeft;
		continue;
		break;
	} while (result >= 0);
	fclose(fInput);
	if (Input) { free(Input);  Input = NULL; }
	if (Data) {  free(Data);  Data = NULL; }
}

void DisableAllCheats(void) {
	char CheatName[500];
	int count;

	for (count = 0; count < MaxCheats; count ++ ) {
		if (!GetCheatName(count,CheatName,sizeof(CheatName))) { break; }
		SaveCheat(CheatName,FALSE);
	}
}

/********************************************************************************************
  GetCheatIniFileName

  Purpose: 
  Parameters:
  Returns:

********************************************************************************************/
char * GetCheatIniFileName(void) {
	char path_buffer[_MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR];
	char fname[_MAX_FNAME],ext[_MAX_EXT];
	static char IniFileName[_MAX_PATH];

	GetModuleFileName(NULL,path_buffer,sizeof(path_buffer));
	_splitpath( path_buffer, drive, dir, fname, ext );
	sprintf(IniFileName,"%s%s%s",drive,dir,CheatIniName);
	return IniFileName;
}

/********************************************************************************************
  GetCheatName

  Purpose: 
  Parameters:
  Returns:

********************************************************************************************/
BOOL GetCheatName(int CheatNo, char * CheatName, int CheatNameLen) {
	char *String = NULL, Identifier[100];
	DWORD len;

	LPSTR IniFileName;
	IniFileName = GetCheatIniFileName();
	sprintf(Identifier,"%08X-%08X-C:%X",*(DWORD *)(&RomHeader[0x10]),*(DWORD *)(&RomHeader[0x14]),RomHeader[0x3D]);
	sprintf(CheatName,"Cheat%d",CheatNo);
	_GetPrivateProfileString2(Identifier,CheatName,"",&String,IniFileName);
	if (strlen(String) == 0) {
		memset(CheatName,0,CheatNameLen);
		if (String) { free(String); }
		return FALSE;		
	}
	len = strrchr(String,'"') - strchr(String,'"') - 1;
	memset(CheatName,0,CheatNameLen);
	strncpy(CheatName,strchr(String,'"') + 1,len);

	if (String) { free(String); }
	
	return TRUE;
}

void CloseCheatWindow (void) {
	if (!hManageWindow) { return; }
	SendMessage(hManageWindow,UM_CLOSE_CHEATS,0,0);
}

/********************************************************************************************
  LoadCheatExt

  Purpose: 
  Parameters:
  Returns:

********************************************************************************************/
BOOL LoadCheatExt(char * CheatName, char * CheatExt, int MaxCheatExtLen) {
	char String[350], Identifier[100];
	HKEY hKeyResults = 0;
	long lResult;
	
	if (CheatName == NULL)
	{ 
		return FALSE;
	}

	sprintf(Identifier,"%08X-%08X-C:%X",*(DWORD *)(&RomHeader[0x10]),*(DWORD *)(&RomHeader[0x14]),RomHeader[0x3D]);
	sprintf(String,"Software\\N64 Emulation\\%s\\Cheats\\%s",AppName,Identifier);

	lResult = RegOpenKeyEx( HKEY_CURRENT_USER,String,0, KEY_ALL_ACCESS,&hKeyResults);	
	if (lResult == ERROR_SUCCESS) {		
		DWORD Type, Bytes;

		sprintf(String,"%s.exten",CheatName);
		Bytes = MaxCheatExtLen;
		lResult = RegQueryValueEx(hKeyResults,String,0,&Type,(LPBYTE)CheatExt,&Bytes);	
		RegCloseKey(hKeyResults);
		if (lResult == ERROR_SUCCESS) { return TRUE; }
	}
	return FALSE;
}

void LoadCode (LPSTR CheatName, LPSTR CheatString)
{
	char * ReadPos = CheatString;
	int count2;

	for (count2 = 0; count2 < MaxGSEntries; count2 ++) {
		char CheatExt[200];
		WORD Value;

		Codes[NoOfCodes].Code[count2].Command = AsciiToHex(ReadPos);
		if (strchr(ReadPos,' ') == NULL) { break; }
		ReadPos = strchr(ReadPos,' ') + 1;
		if (strncmp(ReadPos,"????",4) == 0) {
			if (LoadCheatExt(CheatName,CheatExt,sizeof(CheatExt))) {
				Value = CheatExt[0] == '$'?(WORD)AsciiToHex(&CheatExt[1]):(WORD)atol(CheatExt);
			} else {
				count2 = 0; break;
			}
			Codes[NoOfCodes].Code[count2].Value = Value;
		} else if (strncmp(ReadPos,"??",2) == 0) {
			Codes[NoOfCodes].Code[count2].Value = (BYTE)(AsciiToHex(ReadPos));
			if (LoadCheatExt(CheatName,CheatExt,sizeof(CheatExt))) {
				Value = CheatExt[0] == '$'?(BYTE)AsciiToHex(&CheatExt[1]):(BYTE)atol(CheatExt);
			} else {
				count2 = 0; break;
			}
			Codes[NoOfCodes].Code[count2].Value += (Value << 16);
		} else if (strncmp(&ReadPos[2],"??",2) == 0) {				
			Codes[NoOfCodes].Code[count2].Value = (WORD)(AsciiToHex(ReadPos) << 16);
			if (LoadCheatExt(CheatName,CheatExt,sizeof(CheatExt))) {
				Value = CheatExt[0] == '$'?(BYTE)AsciiToHex(&CheatExt[1]):(BYTE)atol(CheatExt);
			} else {
				count2 = 0; break;
			}
			Codes[NoOfCodes].Code[count2].Value += Value;
		} else {
			Codes[NoOfCodes].Code[count2].Value = (WORD)AsciiToHex(ReadPos);
		}
		if (strchr(ReadPos,',') == NULL) { continue; }
		ReadPos = strchr(ReadPos,',') + 1;
	}
	if (count2 == 0) { return; }
	if (count2 < MaxGSEntries) {
		Codes[NoOfCodes].Code[count2].Command = 0;
		Codes[NoOfCodes].Code[count2].Value   = 0;
	}
	NoOfCodes += 1;
}

void LoadPermCheats (void) 
{
	LPSTR IniFileName;
	char * String = NULL;
	char Identifier[100];
	int count;

	IniFileName = GetIniFileName();
	sprintf(Identifier,"%08X-%08X-C:%X",*(DWORD *)(&RomHeader[0x10]),*(DWORD *)(&RomHeader[0x14]),RomHeader[0x3D]);
	
	for (count = 0; count < MaxCheats; count ++ ) 
	{
		char CheatName[300];
		
		sprintf(CheatName,"Cheat%d",count);
		_GetPrivateProfileString2(Identifier,CheatName,"",&String,IniFileName);
		if (strlen(String) == 0){ break; }
		LoadCode (NULL, String);
	}
	if (String) { free(String); }
}


/********************************************************************************************
  LoadCheats

  Purpose: 
  Parameters:
  Returns:

********************************************************************************************/
void LoadCheats (void) {
	DWORD len, count;
	LPSTR IniFileName;
	char * String = NULL;
	char Identifier[100];
	char CheatName[300];
	
	IniFileName = GetCheatIniFileName();
	sprintf(Identifier,"%08X-%08X-C:%X",*(DWORD *)(&RomHeader[0x10]),*(DWORD *)(&RomHeader[0x14]),RomHeader[0x3D]);
	NoOfCodes = 0;

	LoadPermCheats();
	
	for (count = 0; count < MaxCheats; count ++ ) {
		char * ReadPos;

		sprintf(CheatName,"Cheat%d",count);
		_GetPrivateProfileString2(Identifier,CheatName,"",&String,IniFileName);
		if (strlen(String) == 0){ break; }
		if (strchr(String,'"') == NULL) { continue; }
		len = strrchr(String,'"') - strchr(String,'"') - 1;
		if ((int)len < 1) { continue; }
		memset(CheatName,0,sizeof(CheatName));
		strncpy(CheatName,strchr(String,'"') + 1,len);
		if (strlen(CheatName) == 0) { continue; }
		//if (strrchr(CheatName,'\\') != NULL) {
		//	strcpy(CheatName,strrchr(CheatName,'\\') + 1);
		//}		
		if (!CheatActive (CheatName)) { continue; }
		ReadPos = strrchr(String,'"') + 2;
		LoadCode(CheatName, ReadPos);
	}
	if (String) { free(String); }
}

void ManageCheats (HWND hParent) {
#define DefaultWindowWidth  315
#define DefaultWindowHeight 415
	DWORD X, Y, WindowWidth, WindowHeight,  Style;

	if (hManageWindow != NULL) {
		SetForegroundWindow(hManageWindow);
		return;
	}
	/*if (hParent) {
		DialogBox(hInst, MAKEINTRESOURCE(IDD_MANAGECHEATS), hParent, (DLGPROC)ManageCheatsProc);
	}else {
		CreateDialog(hInst, MAKEINTRESOURCE(IDD_MANAGECHEATS), hParent, (DLGPROC)ManageCheatsProc);
	}*/

	if ( !GetStoredWinSize( "Cheat", &WindowWidth, &WindowHeight ) ) {
  		WindowWidth = DefaultWindowWidth;
  		WindowHeight = DefaultWindowHeight;
	}
	if ( !GetStoredWinPos( "Cheat", &X, &Y ) ) {
  		X = (GetSystemMetrics( SM_CXSCREEN ) - WindowWidth) / 2;
		Y = (GetSystemMetrics( SM_CYSCREEN ) - WindowHeight) / 2;
	}
	if (hParent) { Style = WS_SIZEBOX|WS_SYSMENU; }
	if (!hParent) { Style = WS_SIZEBOX|WS_SYSMENU|WS_MINIMIZEBOX; }
	hManageWindow = CreateWindow("PJ64.Cheats", GS(CHEAT_TITLE),Style,
		X,Y,WindowWidth,WindowHeight,hParent,NULL,hInst,NULL);
	RefreshCheatManager();
	ShowWindow(hManageWindow,SW_SHOW);
	if (hParent) {
		MSG msg;
		EnableWindow(hParent,FALSE);
		while (hManageWindow) {
			if (!GetMessage(&msg,NULL,0,0)) {
				DestroyWindow(hManageWindow);
				hManageWindow = NULL;
				PostMessage(msg.hwnd,msg.message,msg.wParam,msg.lParam);
				continue;
			}
			if (IsDialogMessage( hManageWindow,&msg)) { continue; }
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		EnableWindow(hParent,TRUE);
		SetFocus(hParent);
	}
}

LRESULT CALLBACK Cheat_Proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
#define MinHeight 260
#define MinWidth  190
	switch (uMsg) {	
	case WM_CREATE:
		hSelectCheat = CreateDialog(hInst, MAKEINTRESOURCE(IDD_Cheats_List),hWnd,(DLGPROC)CheatListProc);
		SetWindowPos(hSelectCheat,HWND_TOP, 5, 8, 0, 0, SWP_NOSIZE);
		ShowWindow(hSelectCheat,SW_SHOW);
		break;	
	case WM_MOVE:
		if (IsIconic(hWnd)) { break; }
		StoreCurrentWinPos("Cheat", hWnd );
		break;
	case UM_CLOSE_CHEATS:
		DestroyWindow(hWnd);
		break;
	case WM_SIZING:
		{
			LPRECT lprc = (LPRECT) lParam;
			int fwSide = wParam;

			if ((lprc->bottom - lprc->top) <= MinHeight) {
				switch (fwSide) {
				case WMSZ_TOPLEFT:
				case WMSZ_TOP:
				case WMSZ_TOPRIGHT:
					lprc->top = lprc->bottom - MinHeight;
					break;
				case WMSZ_BOTTOMLEFT:
				case WMSZ_BOTTOM:
				case WMSZ_BOTTOMRIGHT:
					lprc->bottom = lprc->top + MinHeight;
					break;
				}
			}
			if ((lprc->right - lprc->left) <= MinWidth) {
				switch (fwSide) {
				case WMSZ_TOPLEFT:
				case WMSZ_LEFT:
				case WMSZ_BOTTOMLEFT:
					lprc->left = lprc->right - MinWidth;
					break;
				case WMSZ_TOPRIGHT:
				case WMSZ_RIGHT:
				case WMSZ_BOTTOMRIGHT:
					lprc->right = lprc->left + MinWidth;
					break;
				}
			}
		}
		break;
	case WM_SIZE:
		{
			int nWidth = LOWORD(lParam);  // width of client area 
			int nHeight = HIWORD(lParam); // height of client area 
			SetWindowPos(hSelectCheat,HWND_TOP,5,5,nWidth - 8,nHeight - 10,SWP_NOOWNERZORDER);
			StoreCurrentWinSize("Cheat",hWnd);
		}
		break;
	case WM_DESTROY:
		hManageWindow = NULL;
		break;
	default:
		return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}
	return TRUE;
}

/********************************************************************************************
  ManageCheatsProc

  Purpose: 
  Parameters:
  Returns:

********************************************************************************************/
LRESULT CALLBACK ManageCheatsProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static int CurrentPanel = SelectCheat;
	static RECT rcDisp;
	RECT clientrect;
	static RECT rcList;
	static RECT rcAdd;
	HANDLE hIcon;
	HANDLE hStateButton;

	switch (uMsg) {
	case WM_INITDIALOG:
		hManageWindow = hDlg;
		{
			WINDOWPLACEMENT WndPlac;
			RECT *rc;

			WndPlac.length = sizeof(WndPlac);
			GetWindowPlacement(hDlg, &WndPlac);
			rc = &WndPlac.rcNormalPosition;

			SetWindowText(hDlg, GS(CHEAT_TITLE));
			hSelectCheat = CreateDialog(hInst, MAKEINTRESOURCE(IDD_Cheats_List),hDlg,(DLGPROC)CheatListProc);
			SetWindowPos(hSelectCheat,HWND_TOP, 5, 8, 0, 0, SWP_NOSIZE);
			ShowWindow(hSelectCheat,SW_SHOW);

			hAddCheat = CreateDialog(hInst, MAKEINTRESOURCE(IDD_Cheats_Add),hDlg,(DLGPROC)CheatAddProc);
			SetWindowPos(hAddCheat, HWND_TOP, (rc->right - rc->left)/2, 8, 0, 0, SWP_NOSIZE);
			ShowWindow(hAddCheat,SW_HIDE);

			GetWindowRect(GetDlgItem(hSelectCheat, IDC_CHEATSFRAME), &rcList);
			GetWindowRect(GetDlgItem(hAddCheat, IDC_ADDCHEATSFRAME), &rcAdd);
			MinSizeDlg = rcList.right-rcList.left + 32;
			MaxSizeDlg = rcAdd.right-rcList.left + 32;

			DialogState = CONTRACTED;
			WndPlac.rcNormalPosition.right = WndPlac.rcNormalPosition.left + MinSizeDlg;
			SetWindowPlacement(hDlg, &WndPlac);

			GetClientRect(hDlg, rc);
			hStateButton = GetDlgItem(hDlg, IDC_STATE);
			SetWindowPos(hStateButton, HWND_TOP, (rc->right - rc->left) - 16, 0, 16, rc->bottom - rc->top, 0);
            hIcon = LoadImage(hInst, MAKEINTRESOURCE(IDI_RIGHT),IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR );
			SendDlgItemMessage(hDlg, IDC_STATE, BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) (HANDLE) hIcon);
		}
		hManageWindow = hDlg;
		RefreshCheatManager();
		break;
	//case WM_SIZE:
	//	GetClientRect( hDlg, &rcDisp);
	//	TabCtrl_AdjustRect( GetDlgItem(hDlg,IDC_TAB), FALSE, &rcDisp );
	//	break;
	/*case WM_NOTIFY:
		switch (((NMHDR *)lParam)->code) {
		case TCN_SELCHANGE:
			{
				TC_ITEM item;
				HWND hTab;

				hTab = GetDlgItem(hDlg,IDC_TAB);
				InvalidateRect( hTab, &rcDisp, TRUE );
				switch (CurrentPanel) {				
				case SelectCheat: ShowWindow(hSelectCheat,SW_HIDE); break;
				case NewCheat: ShowWindow(hAddCheat,SW_HIDE); break;
				}
				item.mask = TCIF_PARAM;
				TabCtrl_GetItem( hTab, TabCtrl_GetCurSel( hTab ), &item );
				CurrentPanel = item.lParam;
				switch (CurrentPanel) {
				case SelectCheat: ShowWindow(hSelectCheat,SW_SHOW); break;
				case NewCheat: ShowWindow(hAddCheat,SW_SHOW); break;
				}
				break;
			}
		}
		break;*/
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			EndDialog(hDlg,0);
			DestroyWindow(hDlg);
			break;
		case IDC_STATE:
			{
				WINDOWPLACEMENT WndPlac;
				WndPlac.length = sizeof(WndPlac);
				GetWindowPlacement(hDlg, &WndPlac);
	
				if (DialogState == CONTRACTED)
				{
					DialogState = EXPANDED;
					WndPlac.rcNormalPosition.right = WndPlac.rcNormalPosition.left + MaxSizeDlg;
					SetWindowPlacement(hDlg, &WndPlac);

					GetClientRect(hDlg, &clientrect);
					hStateButton = GetDlgItem(hDlg, IDC_STATE);
					SetWindowPos(hStateButton, HWND_TOP, (clientrect.right - clientrect.left) - 16, 0, 16, clientrect.bottom - clientrect.top, 0);

					hIcon = LoadImage(hInst, MAKEINTRESOURCE(IDI_LEFT),IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR );
					SendDlgItemMessage(hDlg, IDC_STATE, BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) (HANDLE) hIcon);
		
					ShowWindow(hAddCheat,SW_SHOW);
				}
				else
				{
					DialogState = CONTRACTED;
					WndPlac.rcNormalPosition.right = WndPlac.rcNormalPosition.left + MinSizeDlg;
					SetWindowPlacement(hDlg, &WndPlac);

		            hIcon = LoadImage(hInst, MAKEINTRESOURCE(IDI_RIGHT),IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR );
					SendDlgItemMessage(hDlg, IDC_STATE, BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) (HANDLE) hIcon);

					GetClientRect(hDlg, &clientrect);
					hStateButton = GetDlgItem(hDlg, IDC_STATE);
					SetWindowPos(hStateButton, HWND_TOP, (clientrect.right - clientrect.left) - 16, 0, 16, clientrect.bottom - clientrect.top, 0);
		
					ShowWindow(hAddCheat,SW_HIDE);
				}

			}
			break;
		}
		break;
	// *** Add in Build 53
	case WM_DESTROY:
		hManageWindow = NULL;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void AddCodeLayers (int CheatNumber, char * CheatName, HTREEITEM hParent, BOOL CheatActive) {
	char Text[500], Item[500];
	TV_INSERTSTRUCT tv;
	
	//Work out text to add
	strcpy(Text,CheatName);
	if (strchr(Text,'\\') > 0) { *strchr(Text,'\\') = 0; }

	//See if text is already added
	tv.item.mask       = TVIF_TEXT;
	tv.item.pszText    = Item;
	tv.item.cchTextMax = sizeof(Item);
	tv.item.hItem      = TreeView_GetChild(hCheatTree,hParent);
	while (tv.item.hItem) {
		TreeView_GetItem(hCheatTree,&tv.item);
		if (strcmp(Text,Item) == 0) { 
			//If already exists then just use existing one
			int State = _TreeView_GetCheckState(hCheatTree,tv.item.hItem);
			if ((CheatActive && State == TV_STATE_CLEAR) || (!CheatActive && State == TV_STATE_CHECKED)) { 
				_TreeView_SetCheckState(hCheatTree,tv.item.hItem,TV_STATE_INDETERMINATE); 
			}
			AddCodeLayers(CheatNumber,CheatName + strlen(Text) + 1, tv.item.hItem, CheatActive);
			return; 
		}
		tv.item.hItem = TreeView_GetNextSibling(hCheatTree,tv.item.hItem);
	}

	//Add to dialog
	tv.hInsertAfter = TVI_SORT;
	tv.item.mask    = TVIF_TEXT | TVIF_PARAM;
	tv.item.pszText = Text;
	tv.item.lParam  = CheatNumber;
	tv.hParent      = hParent;
	hParent = TreeView_InsertItem(hCheatTree,&tv);
	_TreeView_SetCheckState(hCheatTree,hParent,CheatActive?TV_STATE_CHECKED:TV_STATE_CLEAR);

	if (strcmp(Text,CheatName) == 0) { return; }
	AddCodeLayers(CheatNumber,CheatName + strlen(Text) + 1, hParent, CheatActive);
}

/********************************************************************************************
  RefreshCheatManager

  Purpose: 
  Parameters:
  Returns:

********************************************************************************************/
void RefreshCheatManager(void) {
	char CheatName[500];
	BOOL IsCheatActive;
	DWORD count;

	if (hManageWindow == NULL) { return; }

	TreeView_DeleteAllItems(hCheatTree);
	for (count = 0; count < MaxCheats; count ++ ) {
		if (!GetCheatName(count,CheatName,sizeof(CheatName))) { break; }
		IsCheatActive = CheatActive (CheatName);
		AddCheatExtension(count,CheatName,sizeof(CheatName));		
		AddCodeLayers(count,CheatName,TVI_ROOT, IsCheatActive);
	}
}

void SaveCheat(char * CheatName, BOOL Active) {
	char String[300], Identifier[100];
	DWORD Disposition = 0;
	HKEY hKeyResults = 0;
	long lResult;
	
	sprintf(Identifier,"%08X-%08X-C:%X",*(DWORD *)(&RomHeader[0x10]),*(DWORD *)(&RomHeader[0x14]),RomHeader[0x3D]);
	sprintf(String,"Software\\N64 Emulation\\%s\\Cheats\\%s",AppName,Identifier);
	lResult = RegCreateKeyEx( HKEY_CURRENT_USER, String,0,"", REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
	if (lResult == ERROR_SUCCESS) {		
		RegSetValueEx(hKeyResults,"Name",0,REG_SZ,(CONST BYTE *)RomName,strlen(RomName));							
		if (Active) {
			RegSetValueEx(hKeyResults,CheatName,0, REG_DWORD,(CONST BYTE *)(&Active),sizeof(DWORD));
		} else {
			RegDeleteValue(hKeyResults,CheatName);
		}
		RegCloseKey(hKeyResults);
	}
}

/********************************************************************************************
  SaveCheatExt

  Purpose: 
  Parameters:
  Returns:

********************************************************************************************/
void SaveCheatExt(char * CheatName, char * CheatExt) {
	char String[300], Identifier[100];
	DWORD Disposition = 0;
	HKEY hKeyResults = 0;
	long lResult;
	
	sprintf(Identifier,"%08X-%08X-C:%X",*(DWORD *)(&RomHeader[0x10]),*(DWORD *)(&RomHeader[0x14]),RomHeader[0x3D]);
	sprintf(String,"Software\\N64 Emulation\\%s\\Cheats\\%s",AppName,Identifier);
	lResult = RegCreateKeyEx( HKEY_CURRENT_USER, String,0,"", REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
	if (lResult == ERROR_SUCCESS) {		
		sprintf(String,"%s.exten",CheatName);
		RegSetValueEx(hKeyResults,"Name",0,REG_SZ,(CONST BYTE *)RomName,strlen(RomName));				
		RegSetValueEx(hKeyResults,String,0,REG_SZ,(CONST BYTE *)CheatExt,strlen(CheatExt));
		RegCloseKey(hKeyResults);
	}
}

int _TreeView_GetCheckState(HWND hwndTreeView, HTREEITEM hItem)
{
    TVITEM tvItem;

    // Prepare to receive the desired information.
    tvItem.mask = TVIF_HANDLE | TVIF_STATE;
    tvItem.hItem = hItem;
    tvItem.stateMask = TVIS_STATEIMAGEMASK;

    // Request the information.
    TreeView_GetItem(hwndTreeView, &tvItem);

    // Return zero if it's not checked, or nonzero otherwise.
	switch (tvItem.state >> 12) {
	case 1: return TV_STATE_CHECKED;
	case 2: return TV_STATE_CLEAR;
	case 3: return TV_STATE_INDETERMINATE;
	}
	return ((int)(tvItem.state >> 12) -1);
}

BOOL _TreeView_SetCheckState(HWND hwndTreeView, HTREEITEM hItem, int state)
{
    TVITEM tvItem;

    tvItem.mask = TVIF_HANDLE | TVIF_STATE;
    tvItem.hItem = hItem;
    tvItem.stateMask = TVIS_STATEIMAGEMASK;

    /*Image 1 in the tree-view check box image list is the
    unchecked box. Image 2 is the checked box.*/

	switch (state) {
	case TV_STATE_CHECKED: tvItem.state = INDEXTOSTATEIMAGEMASK(1); break;
	case TV_STATE_CLEAR: tvItem.state = INDEXTOSTATEIMAGEMASK(2); break;
	case TV_STATE_INDETERMINATE: tvItem.state = INDEXTOSTATEIMAGEMASK(3); break;
	default: tvItem.state = INDEXTOSTATEIMAGEMASK(0); break;
	}
    return TreeView_SetItem(hwndTreeView, &tvItem);
}


