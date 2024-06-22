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


#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <stdio.h>
#include <shellapi.h>

#include "main.h"
#include "cheats.h"
#include "cheatsearch.h"
#include "cpu.h"
#include "plugin.h"
#include "debugger.h"
#include "Settings.h"
#include "htmlHelp.h"
#include "resource.h"
#include "resource_cheat.h"
#include "RomTools_Common.h"
#include "Settings Api_2.h"
#include "Registry.h"

LARGE_INTEGER Frequency, Frames[NoOfFrames], LastFrame;
BOOL HaveDebugger, AutoLoadMapFile, ShowUnhandledMemory, ShowTLBMisses,
	ShowDListAListCount, ShowCompMem, Profiling, IndvidualBlock, AutoStart,
	AutoSleep, DisableRegCaching, UseIni, UseTlb, UseLinking, RomBrowser,
	IgnoreMove, Rercursion, ShowPifRamErrors, LimitFPS, ShowCPUPer, AutoZip,
	AutoFullScreen, SystemABL, AlwaysOnTop, BasicMode, DelaySI, RememberCheats, AudioSignal,
	DelayRSP, DelayRDP, EmulateAI, ForceClose;
DWORD CurrentFrame, CPU_Type, SystemCPU_Type, SelfModCheck, SystemSelfModCheck,
	RomsToRemember, RomDirsToRemember;
HWND hMainWindow, hHiddenWin, hStatusWnd, hCheatSearchDlg;
char CurrentSave[256];
HMENU hMainMenu;
HINSTANCE hInst;

void MenuSetText ( HMENU hMenu, int MenuPos, char * Title, char * ShotCut );
void RomInfo     ( void );
void GameInfoByRomID();
void GameInfoByGameInfoID(char* GameInfoID);
void SetupMenu   ( HWND hWnd );
void UninstallApplication(HWND hWnd);
void UninstallJabo(HWND hWnd);

LRESULT CALLBACK AboutIniBoxProc ( HWND, UINT, WPARAM, LPARAM );
LRESULT CALLBACK Main_Proc       ( HWND, UINT, WPARAM, LPARAM );
LRESULT CALLBACK RomInfoProc     ( HWND, UINT, WPARAM, LPARAM );

void AboutIniBox (void) {
	DialogBox(hInst, MAKEINTRESOURCE(IDD_About_Ini), hMainWindow, (DLGPROC)AboutIniBoxProc);
}

LRESULT CALLBACK AboutIniBoxProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static char RDBHomePage[300], CHTHomePage[300], RDXHomePage[300], JINIHomePage[300];

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			char * IniFile, String[200],String2[200];

			SetWindowText(hDlg, GS(INI_TITLE));

			//Language
			SetDlgItemText(hDlg,IDC_LAN,GS(INI_CURRENT_LANG));
			sprintf(String,"%s: %s",GS(INI_AUTHOR),GS(LANGUAGE_AUTHOR));
			SetDlgItemText(hDlg,IDC_LAN_AUTHOR,String);
			sprintf(String,"%s: %s",GS(INI_VERSION),GS(LANGUAGE_VERSION));
			SetDlgItemText(hDlg,IDC_LAN_VERSION,String);
			sprintf(String,"%s: %s",GS(INI_DATE),GS(LANGUAGE_DATE));
			SetDlgItemText(hDlg,IDC_LAN_DATE,String);
			if (strlen(GS(LANGUAGE_NAME)) == 0) {
				EnableWindow(GetDlgItem(hDlg,IDC_LAN),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_LAN_AUTHOR),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_LAN_VERSION),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_LAN_DATE),FALSE);
			}

			//RDB
			IniFile = GetIniFileName();
			SetDlgItemText(hDlg,IDC_RDB,GS(INI_CURRENT_RDB));
			_GetPrivateProfileString("Meta","Author","",String,sizeof(String),IniFile);
			if (strlen(String) == 0) {
				EnableWindow(GetDlgItem(hDlg,IDC_RDB),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_RDB_AUTHOR),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_RDB_VERSION),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_RDB_DATE),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_RDB_HOME),FALSE);
			}
			sprintf(String2,"%s: %s",GS(INI_AUTHOR),String);
			SetDlgItemText(hDlg,IDC_RDB_AUTHOR,String2);
			_GetPrivateProfileString("Meta","Version","",String,sizeof(String),IniFile);
			sprintf(String2,"%s: %s",GS(INI_VERSION),String);
			SetDlgItemText(hDlg,IDC_RDB_VERSION,String2);
			_GetPrivateProfileString("Meta","Date","",String,sizeof(String),IniFile);
			sprintf(String2,"%s: %s",GS(INI_DATE),String);
			SetDlgItemText(hDlg,IDC_RDB_DATE,String2);
			_GetPrivateProfileString("Meta","Homepage","",RDBHomePage,sizeof(RDBHomePage),IniFile);
			SetDlgItemText(hDlg,IDC_RDB_HOME,GS(INI_HOMEPAGE));
			if (strlen(RDBHomePage) == 0) {
				EnableWindow(GetDlgItem(hDlg,IDC_RDB_HOME),FALSE);
			}

			//Cheat
			SetDlgItemText(hDlg,IDC_CHT,GS(INI_CURRENT_CHT));
			IniFile = GetCheatIniFileName();
			_GetPrivateProfileString("Meta","Author","",String,sizeof(String),IniFile);
			if (strlen(String) == 0) {
				EnableWindow(GetDlgItem(hDlg,IDC_CHT),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_CHT_AUTHOR),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_CHT_VERSION),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_CHT_DATE),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_CHT_HOME),FALSE);
			}
			sprintf(String2,"%s: %s",GS(INI_AUTHOR),String);
			SetDlgItemText(hDlg,IDC_CHT_AUTHOR,String2);
			_GetPrivateProfileString("Meta","Version","",String,sizeof(String),IniFile);
			sprintf(String2,"%s: %s",GS(INI_VERSION),String);
			SetDlgItemText(hDlg,IDC_CHT_VERSION,String2);
			_GetPrivateProfileString("Meta","Date","",String,sizeof(String),IniFile);
			sprintf(String2,"%s: %s",GS(INI_DATE),String);
			SetDlgItemText(hDlg,IDC_CHT_DATE,String2);
			_GetPrivateProfileString("Meta","Homepage","",CHTHomePage,sizeof(CHTHomePage),IniFile);
			SetDlgItemText(hDlg,IDC_CHT_HOME,GS(INI_HOMEPAGE));
			if (strlen(CHTHomePage) == 0) {
				EnableWindow(GetDlgItem(hDlg,IDC_CHT_HOME),FALSE);
			}

			//Extended Info
			SetDlgItemText(hDlg,IDC_RDX,GS(INI_CURRENT_RDX));
			IniFile = GetExtIniFileName();
			_GetPrivateProfileString("Meta","Author","",String,sizeof(String),IniFile);
			if (strlen(String) == 0) {
				EnableWindow(GetDlgItem(hDlg,IDC_RDX),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_RDX_AUTHOR),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_RDX_VERSION),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_RDX_DATE),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_RDX_HOME),FALSE);
			}
			sprintf(String2,"%s: %s",GS(INI_AUTHOR),String);
			SetDlgItemText(hDlg,IDC_RDX_AUTHOR,String2);
			_GetPrivateProfileString("Meta","Version","",String,sizeof(String),IniFile);
			sprintf(String2,"%s: %s",GS(INI_VERSION),String);
			SetDlgItemText(hDlg,IDC_RDX_VERSION,String2);
			_GetPrivateProfileString("Meta","Date","",String,sizeof(String),IniFile);
			sprintf(String2,"%s: %s",GS(INI_DATE),String);
			SetDlgItemText(hDlg,IDC_RDX_DATE,String2);
			_GetPrivateProfileString("Meta","Homepage","",RDXHomePage,sizeof(CHTHomePage),IniFile);
			SetDlgItemText(hDlg,IDC_RDX_HOME,GS(INI_HOMEPAGE));
			if (strlen(RDXHomePage) == 0) {
				EnableWindow(GetDlgItem(hDlg,IDC_RDX_HOME),FALSE);
			}

			//Jabo Ini Info
			SetDlgItemText(hDlg, IDC_JINI, GS(INI_CURRENT_JINI));
			IniFile = GetJIniFileName();
			_GetPrivateProfileString("Meta", "Author", "", String, sizeof(String), IniFile);
			if (strlen(String) == 0) {
				EnableWindow(GetDlgItem(hDlg, IDC_JINI), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_JINI_AUTHOR), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_JINI_VERSION), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_JINI_DATE), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_JINI_HOME), FALSE);
			}
			sprintf(String2, "%s: %s", GS(INI_AUTHOR), String);
			SetDlgItemText(hDlg, IDC_JINI_AUTHOR, String2);
			_GetPrivateProfileString("Meta", "Version", "", String, sizeof(String), IniFile);
			sprintf(String2, "%s: %s", GS(INI_VERSION), String);
			SetDlgItemText(hDlg, IDC_JINI_VERSION, String2);
			_GetPrivateProfileString("Meta", "Date", "", String, sizeof(String), IniFile);
			sprintf(String2, "%s: %s", GS(INI_DATE), String);
			SetDlgItemText(hDlg, IDC_JINI_DATE, String2);
			_GetPrivateProfileString("Meta", "Homepage", "", JINIHomePage, sizeof(CHTHomePage), IniFile);
			SetDlgItemText(hDlg, IDC_JINI_HOME, GS(INI_HOMEPAGE));
			if (strlen(JINIHomePage) == 0) {
				EnableWindow(GetDlgItem(hDlg, IDC_JINI_HOME), FALSE);
			}
		}
		break;
	case WM_COMMAND: {
			char String[300];
			switch (LOWORD(wParam)) {
			case IDC_RDB_HOME: sprintf(String, "https://%s", RDBHomePage); ShellExecute(NULL,"open",String,NULL,NULL,SW_SHOWNORMAL); break;
			case IDC_CHT_HOME: sprintf(String, "https://%s", CHTHomePage); ShellExecute(NULL,"open",String,NULL,NULL,SW_SHOWNORMAL); break;
			case IDC_RDX_HOME: sprintf(String, "https://%s", RDXHomePage); ShellExecute(NULL,"open",String,NULL,NULL,SW_SHOWNORMAL); break;
			case IDC_JINI_HOME: sprintf(String, "https://%s", JINIHomePage); ShellExecute(NULL, "open", String, NULL, NULL, SW_SHOWNORMAL); break;

			case IDOK:
			case IDCANCEL:
				EndDialog(hDlg,0);
				break;
			}
	}
	default:
		return FALSE;
	}
	return TRUE;
}

void AlwaysOnTopWindow(HWND hWnd) {
	SetWindowPos(hWnd,(AlwaysOnTop?HWND_TOPMOST:HWND_NOTOPMOST),0,0,0,0,SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE);
}

DWORD AsciiToHex (char * HexValue) {
	DWORD Count, Finish, Value = 0;

	Finish = strlen(HexValue);
	if (Finish > 8 ) { Finish = 8; }

	for (Count = 0; Count < Finish; Count++){
		Value = (Value << 4);
		switch( HexValue[Count] ) {
		case '0': break;
		case '1': Value += 1; break;
		case '2': Value += 2; break;
		case '3': Value += 3; break;
		case '4': Value += 4; break;
		case '5': Value += 5; break;
		case '6': Value += 6; break;
		case '7': Value += 7; break;
		case '8': Value += 8; break;
		case '9': Value += 9; break;
		case 'A': case 'a': Value += 10; break;
		case 'B': case 'b': Value += 11; break;
		case 'C': case 'c': Value += 12; break;
		case 'D': case 'd': Value += 13; break;
		case 'E': case 'e': Value += 14; break;
		case 'F': case 'f': Value += 15; break;
		default:
			Value = (Value >> 4);
			Count = Finish;
		}
	}
	return Value;
}

void ChangeWinSize ( HWND hWnd, long width, long height, HWND hStatusBar ) {
	WINDOWPLACEMENT wndpl;
    RECT rc1, swrect;

	wndpl.length = sizeof(wndpl);
	GetWindowPlacement( hWnd, &wndpl);

	if ( hStatusBar != NULL ) {
		GetClientRect( hStatusBar, &swrect );
	    SetRect( &rc1, 0, 0, width, height + swrect.bottom );
	} else {
	    SetRect( &rc1, 0, 0, width, height );
	}

    AdjustWindowRectEx( &rc1,GetWindowLong( hWnd, GWL_STYLE ),
		GetMenu( hWnd ) != NULL, GetWindowLong( hWnd, GWL_EXSTYLE ) );

    MoveWindow( hWnd, wndpl.rcNormalPosition.left, wndpl.rcNormalPosition.top,
		rc1.right - rc1.left, rc1.bottom - rc1.top, TRUE );
}

void __cdecl DisplayError (char * Message, ...) {
	char Msg[1000];
	va_list ap;

	if (inFullScreen) { return; }

	va_start( ap, Message );
	vsprintf( Msg, Message, ap );
	va_end( ap );
	MessageBox(NULL,Msg,GS(MSG_MSGBOX_TITLE),MB_OK|MB_ICONERROR|MB_SETFOREGROUND);
	SetActiveWindow(hMainWindow);
}

void __cdecl DisplayErrorFatal (char * Message, ...) {
	char Msg[1000];
	va_list ap;
	
	if (inFullScreen) {
		CPU_Paused = TRUE;
		SendMessage(hMainWindow,WM_COMMAND,ID_OPTIONS_FULLSCREEN,0); //ID_FILE_ENDEMULATION
		CPU_Paused = FALSE;
	}

	va_start( ap, Message );
	vsprintf( Msg, Message, ap );
	va_end( ap );
	MessageBox(NULL,Msg,GS(MSG_MSGBOX_TITLE),MB_OK|MB_ICONERROR|MB_SETFOREGROUND);
	SetActiveWindow(hMainWindow);
}

void DisplayFPS (void) {
	if (CurrentFrame > (NoOfFrames << 3)) {
		LARGE_INTEGER Total;
		char Message[100];
		int count;

		Total.QuadPart = 0;
		for (count = 0; count < NoOfFrames; count ++) {
			Total.QuadPart += Frames[count].QuadPart;
		}
		sprintf(Message, "FPS: %.2f", Frequency.QuadPart/ ((double)Total.QuadPart / (NoOfFrames << 3)));
		SendMessage( hStatusWnd, SB_SETTEXT, 1, (LPARAM)Message );
	} else {
		SendMessage( hStatusWnd, SB_SETTEXT, 1, (LPARAM)"FPS: -.--" );
	}
}

void FixMenuLang (HMENU hMenu) {
	HMENU hSubMenu;

	MenuSetText(hMenu, 0, GS(MENU_FILE), NULL);
	MenuSetText(hMenu, 1, GS(MENU_SYSTEM), NULL);
	MenuSetText(hMenu, 2, GS(MENU_OPTIONS), NULL);
	MenuSetText(hMenu, 3, GS(MENU_DEBUGGER), NULL);
	MenuSetText(hMenu, 4, GS(MENU_HELP), NULL);

	//File
	hSubMenu = GetSubMenu(hMenu,0);
	MenuSetText(hSubMenu, 0, GS(MENU_OPEN), "Ctrl+O");
	MenuSetText(hSubMenu, 1, GS(MENU_ROM_INFO), NULL);
	MenuSetText(hSubMenu, 2, GS(MENU_GAME_INFO), NULL);
	MenuSetText(hSubMenu, 4, GS(MENU_START), "F11");
	MenuSetText(hSubMenu, 5, GS(MENU_END), "F12");
	MenuSetText(hSubMenu, 7, GS(MENU_LANGUAGE), NULL);
	MenuSetText(hSubMenu, 9, GS(MENU_CHOOSE_ROM), NULL);
	MenuSetText(hSubMenu, 10, GS(MENU_REFRESH), "F5");
	MenuSetText(hSubMenu, 12, GS(MENU_RECENT_ROM), NULL);
	MenuSetText(hSubMenu, 14, GS(MENU_RECENT_DIR), NULL);
	MenuSetText(hSubMenu, 15, GS(MENU_EXIT), "Alt+F4");

	//System
	hSubMenu = GetSubMenu(hMenu,1);
	MenuSetText(hSubMenu, 0, GS(MENU_RESET),"F1");
	MenuSetText(hSubMenu, 1, GS(CPU_Paused?MENU_RESUME:MENU_PAUSE),"F2");
	MenuSetText(hSubMenu, 2, GS(MENU_BITMAP),"F3");
	MenuSetText(hSubMenu, 4, GS(MENU_LIMIT_FPS),"F4");
	MenuSetText(hSubMenu, 6, GS(MENU_SAVE),"F5");
	MenuSetText(hSubMenu, 7, GS(MENU_SAVE_AS),"Ctrl+S");
	MenuSetText(hSubMenu, 8, GS(MENU_RESTORE),"F7");
	MenuSetText(hSubMenu, 9, GS(MENU_LOAD),"Ctrl+L");
	MenuSetText(hSubMenu,11, GS(MENU_CURRENT_SAVE),NULL);
	MenuSetText(hSubMenu,13, GS(MENU_CHEAT),"Ctrl+C");
	// TODO Witten: Translate Cheat Search
	MenuSetText(hSubMenu,15, GS(MENU_GS_BUTTON),"F9");

	//Options
	hSubMenu = GetSubMenu(hMenu,2);
	MenuSetText(hSubMenu, 0, GS(MENU_FULL_SCREEN), "Alt+Enter");
	MenuSetText(hSubMenu, 1, GS(MENU_ON_TOP), "Ctrl+A");
	MenuSetText(hSubMenu, 3, GS(MENU_CONFG_GFX), NULL);
	MenuSetText(hSubMenu, 4, GS(MENU_CONFG_AUDIO), NULL);
	MenuSetText(hSubMenu, 5, GS(MENU_CONFG_CTRL), NULL);
	MenuSetText(hSubMenu, 6, GS(MENU_CONFG_RSP), NULL);
	MenuSetText(hSubMenu, 8, GS(MENU_SHOW_CPU), NULL);
	MenuSetText(hSubMenu, 9, GS(MENU_SETTINGS), "Ctrl+T");

	//Help Menu
	hSubMenu = GetSubMenu(hMenu,4);
#ifdef BETA_VERSION
	MenuSetText(hSubMenu, 2, GS(MENU_USER_MAN), NULL);
	MenuSetText(hSubMenu, 3, GS(MENU_GAME_FAQ), NULL);
	MenuSetText(hSubMenu, 5, GS(MENU_ABOUT_INI), NULL);
	MenuSetText(hSubMenu, 6, GS(MENU_ABOUT_PJ64), NULL);
#else
	MenuSetText(hSubMenu, 0, GS(MENU_USER_MAN), NULL);
	MenuSetText(hSubMenu, 1, GS(MENU_GAME_FAQ), NULL);
	MenuSetText(hSubMenu, 3, GS(MENU_DISCORD), NULL);
	MenuSetText(hSubMenu, 4, GS(MENU_GITHUB), NULL);
	MenuSetText(hSubMenu, 6, GS(MENU_UNINSTALL), NULL);
	MenuSetText(hSubMenu, 8, GS(MENU_JABO_UNINSTALL), NULL);
	MenuSetText(hSubMenu, 9, GS(MENU_ABOUT_INI), NULL);
	MenuSetText(hSubMenu, 10, GS(MENU_ABOUT_PJ64), NULL);
#endif

	//Save Slot
	hSubMenu = GetSubMenu(hMenu, 1);
	hSubMenu = GetSubMenu(hSubMenu, 19);
	MenuSetText(hSubMenu, 0, GS(MENU_SLOT_DEFAULT), "0");
	MenuSetText(hSubMenu, 1, GS(MENU_SLOT_1), "1");
	MenuSetText(hSubMenu, 2, GS(MENU_SLOT_2), "2");
	MenuSetText(hSubMenu, 3, GS(MENU_SLOT_3), "3");
	MenuSetText(hSubMenu, 4, GS(MENU_SLOT_4), "4");
	MenuSetText(hSubMenu, 5, GS(MENU_SLOT_5), "5");
	MenuSetText(hSubMenu, 6, GS(MENU_SLOT_6), "6");
	MenuSetText(hSubMenu, 7, GS(MENU_SLOT_7), "7");
	MenuSetText(hSubMenu, 8, GS(MENU_SLOT_8), "8");
	MenuSetText(hSubMenu, 9, GS(MENU_SLOT_9), "9");
	MenuSetText(hSubMenu, 10, GS(MENU_SLOT_10), "Shift+0");
	MenuSetText(hSubMenu, 11, GS(MENU_SLOT_11), "Shift+1");
	MenuSetText(hSubMenu, 12, GS(MENU_SLOT_12), "Shift+2");
	MenuSetText(hSubMenu, 13, GS(MENU_SLOT_13), "Shift+3");
	MenuSetText(hSubMenu, 14, GS(MENU_SLOT_14), "Shift+4");
	MenuSetText(hSubMenu, 15, GS(MENU_SLOT_15), "Shift+5");
	MenuSetText(hSubMenu, 16, GS(MENU_SLOT_16), "Shift+6");
	MenuSetText(hSubMenu, 17, GS(MENU_SLOT_17), "Shift+7");
	MenuSetText(hSubMenu, 18, GS(MENU_SLOT_18), "Shift+8");
	MenuSetText(hSubMenu, 19, GS(MENU_SLOT_19), "Shift+9");

	//MenuSetText(hSubMenu, 11, GS(MENU_SLOT_10), "0");
}

char * GetExtIniFileName(void) {
	char path_buffer[_MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR];
	char fname[_MAX_FNAME],ext[_MAX_EXT];
	static char IniFileName[_MAX_PATH];

	GetModuleFileName(NULL,path_buffer,sizeof(path_buffer));
	_splitpath( path_buffer, drive, dir, fname, ext );

	// Moved RDX out of root and into the Config Folder (Gent)

	sprintf(IniFileName,"%s%sConfig\\%s",drive,dir,ExtIniName);
	return IniFileName;
}
char* GetJIniFileName(void) {
	char path_buffer[_MAX_PATH], drive[_MAX_DRIVE], dir[_MAX_DIR];
	char fname[_MAX_FNAME], ext[_MAX_EXT];
	static char IniFileName[_MAX_PATH];

	GetModuleFileName(NULL, path_buffer, sizeof(path_buffer));
	_splitpath(path_buffer, drive, dir, fname, ext);
	sprintf(IniFileName, "%s%sConfig\\%s", drive, dir, JIniName);
	return IniFileName;
}

char * GetIniFileName(void) {
	char path_buffer[_MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR];
	char fname[_MAX_FNAME],ext[_MAX_EXT];
	static char IniFileName[_MAX_PATH];

	GetModuleFileName(NULL,path_buffer,sizeof(path_buffer));
	_splitpath( path_buffer, drive, dir, fname, ext );

	// Moved RDB out of root and into the Config Folder (Gent)

	sprintf(IniFileName,"%s%sConfig\\%s",drive,dir,IniName);
	return IniFileName;
}

char * GetLangFileName(void) {
	char path_buffer[_MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR];
	char fname[_MAX_FNAME],ext[_MAX_EXT];
	static char IniFileName[_MAX_PATH];

	GetModuleFileName(NULL,path_buffer,sizeof(path_buffer));
	_splitpath( path_buffer, drive, dir, fname, ext );
	sprintf(IniFileName,"%s%s%s",drive,dir,LangFileName);
	return IniFileName;
}

char * GetNotesIniFileName(void) {
	char path_buffer[_MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR];
	char fname[_MAX_FNAME],ext[_MAX_EXT];
	static char IniFileName[_MAX_PATH];

	GetModuleFileName(NULL,path_buffer,sizeof(path_buffer));
	_splitpath( path_buffer, drive, dir, fname, ext );

	// Moved RDN out of root and into the Config Folder (Gent)

	sprintf(IniFileName,"%s%sConfig\\%s",drive,dir,NotesIniName);
	return IniFileName;
}

int GetStoredWinPos( char * WinName, DWORD * X, DWORD * Y ) {
	long lResult;
	HKEY hKeyResults = 0;
	char String[200];

	sprintf(String,"Software\\N64 Emulation\\%s\\Page Setup",AppName);
	lResult = RegOpenKeyEx( HKEY_CURRENT_USER,String,0, KEY_ALL_ACCESS,&hKeyResults);

	if (lResult == ERROR_SUCCESS) {
		DWORD Type, Value, Bytes = 4;

		sprintf(String,"%s Top",WinName);
		lResult = RegQueryValueEx(hKeyResults,String,0,&Type,(LPBYTE)(&Value),&Bytes);
		if (Type == REG_DWORD && lResult == ERROR_SUCCESS) {
			*Y = Value;
		} else {
			RegCloseKey(hKeyResults);
			return FALSE;
		}

		sprintf(String,"%s Left",WinName);
		lResult = RegQueryValueEx(hKeyResults,String,0,&Type,(LPBYTE)(&Value),&Bytes);
		if (Type == REG_DWORD && lResult == ERROR_SUCCESS) {
			*X = Value;
		} else {
			RegCloseKey(hKeyResults);
			return FALSE;
		}
		RegCloseKey(hKeyResults);
		return TRUE;
	}
	return FALSE;
}

int GetStoredWinSize( char * WinName, DWORD * Width, DWORD * Height ) {
	long lResult;
	HKEY hKeyResults = 0;
	char String[200];

	sprintf(String,"Software\\N64 Emulation\\%s\\Page Setup",AppName);
	lResult = RegOpenKeyEx( HKEY_CURRENT_USER,String,0, KEY_ALL_ACCESS,&hKeyResults);

	if (lResult == ERROR_SUCCESS) {
		DWORD Type, Value, Bytes = 4;

		sprintf(String,"%s Width",WinName);
		lResult = RegQueryValueEx(hKeyResults,String,0,&Type,(LPBYTE)(&Value),&Bytes);
		if (Type == REG_DWORD && lResult == ERROR_SUCCESS) {
			*Width = Value;
		} else {
			RegCloseKey(hKeyResults);
			return FALSE;
		}

		sprintf(String,"%s Height",WinName);
		lResult = RegQueryValueEx(hKeyResults,String,0,&Type,(LPBYTE)(&Value),&Bytes);
		if (Type == REG_DWORD && lResult == ERROR_SUCCESS) {
			*Height = Value;
		} else {
			RegCloseKey(hKeyResults);
			return FALSE;
		}
		RegCloseKey(hKeyResults);
		return TRUE;
	}
	return FALSE;
}

void LoadSettings (void) {
	HKEY hKeyResults;
	char String[200];
	long lResult;

	IgnoreMove = FALSE;
	CPU_Type = Default_CPU;
	SystemCPU_Type = Default_CPU;
	SystemSelfModCheck = Default_SelfModCheck;
	SystemRdramSize = Default_RdramSize;
	SystemABL = Default_AdvancedBlockLink;
	AutoStart = Default_AutoStart;
	AutoSleep = Default_AutoSleep;
	DisableRegCaching = Default_DisableRegCaching;
	UseIni = Default_UseIni;
	AutoZip = Default_AutoZip;
	AutoFullScreen = FALSE;
	RomsToRemember = Default_RomsToRemember;
	RomDirsToRemember = Default_RomsDirsToRemember;
	AutoLoadMapFile = Default_AutoMap;
	ShowUnhandledMemory = Default_ShowUnhandledMemory;
	ShowCPUPer = Default_ShowCPUPer;
	LimitFPS = Default_LimitFPS;
	AlwaysOnTop = Default_AlwaysOnTop;
	BasicMode = Default_BasicMode;
	RememberCheats = Default_RememberCheats;
	ShowTLBMisses = Default_ShowTLBMisses;
	Profiling = Default_ProfilingOn;
	IndvidualBlock = Default_IndvidualBlock;
	RomBrowser = Default_UseRB;
	Rercursion = Default_Rercursion;
	HaveDebugger = FALSE;

	sprintf(String,"Software\\N64 Emulation\\%s",AppName);
	lResult = RegOpenKeyEx( HKEY_CURRENT_USER,String,0,KEY_ALL_ACCESS,
		&hKeyResults);
	if (lResult == ERROR_SUCCESS) {
		DWORD Type, Bytes = 4;

		lResult = RegQueryValueEx(hKeyResults,"Remember Cheats",0,&Type,(LPBYTE)(&RememberCheats),&Bytes);
		if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { RememberCheats = Default_RememberCheats; }

		lResult = RegQueryValueEx(hKeyResults,"Basic Mode",0,&Type,(LPBYTE)(&BasicMode),&Bytes);
		if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { BasicMode = Default_BasicMode;	}

		lResult = RegQueryValueEx(hKeyResults,"Pause emulation when window is not active",0,&Type,(BYTE *)(&AutoSleep),&Bytes);
		if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { AutoSleep = Default_AutoSleep; }

		lResult = RegQueryValueEx(hKeyResults,"On open rom go full screen",0,&Type,(BYTE *)(&AutoFullScreen),&Bytes);
		if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { AutoFullScreen = FALSE; }

		if (!BasicMode) {
#if (!defined(EXTERNAL_RELEASE))
			DWORD Value;

			lResult = RegQueryValueEx(hKeyResults,"Debugger",0,&Type,(LPBYTE)(&Value),&Bytes);
			if (Type == REG_DWORD && lResult == ERROR_SUCCESS) {
				if (Value == 0x9348ae97) { HaveDebugger = TRUE; }
				// MODIFIED TO ALLOW DEBUGGER ALWAYS ON
				HaveDebugger = TRUE;
			}
#endif

			lResult = RegQueryValueEx(hKeyResults,"Limit FPS",0,&Type,(LPBYTE)(&LimitFPS),&Bytes);
			if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { LimitFPS = Default_LimitFPS;	}

			lResult = RegQueryValueEx(hKeyResults,"Roms To Remember",0,&Type,(BYTE *)(&RomsToRemember),&Bytes);
			if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { RomsToRemember = Default_RomsToRemember; }

			lResult = RegQueryValueEx(hKeyResults,"Rom Dirs To Remember",0,&Type,(BYTE *)(&RomDirsToRemember),&Bytes);
			if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { RomDirsToRemember = Default_RomsDirsToRemember; }

			lResult = RegQueryValueEx(hKeyResults,"Start Emulation when rom is opened",0,&Type,(BYTE *)(&AutoStart),&Bytes);
			if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { AutoStart = Default_AutoStart; }

			lResult = RegQueryValueEx(hKeyResults,"Use Rom Browser",0,&Type,(BYTE *)(&RomBrowser),&Bytes);
			if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { RomBrowser = Default_UseRB; }

			lResult = RegQueryValueEx(hKeyResults,"Use Recursion",0,&Type,(BYTE *)(&Rercursion),&Bytes);
			if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { Rercursion = Default_Rercursion; }

			lResult = RegQueryValueEx(hKeyResults,"Always overwrite default settings with ones from ini?",0,&Type,(BYTE *)(&UseIni),&Bytes);
			if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { UseIni = Default_UseIni; }

			lResult = RegQueryValueEx(hKeyResults,"Automatically compress instant saves",0,&Type,(BYTE *)(&AutoZip),&Bytes);
			if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { AutoZip = Default_AutoZip; }

			lResult = RegQueryValueEx(hKeyResults,"CPU Type",0,&Type,(BYTE *)(&SystemCPU_Type),&Bytes);
			if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { SystemCPU_Type = Default_CPU; }

			lResult = RegQueryValueEx(hKeyResults,"Self modifying code method",0,&Type,(LPBYTE)(&SystemSelfModCheck),&Bytes);
			if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { SystemSelfModCheck = Default_SelfModCheck; }

			lResult = RegQueryValueEx(hKeyResults,"Advanced Block Linking",0,&Type,(LPBYTE)(&SystemABL),&Bytes);
			if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { SystemABL = Default_AdvancedBlockLink; }

			lResult = RegQueryValueEx(hKeyResults,"Default RDRAM Size",0,&Type,(LPBYTE)(&SystemRdramSize),&Bytes);
			if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { SystemRdramSize = Default_RdramSize; }

			lResult = RegQueryValueEx(hKeyResults,"Show CPU %",0,&Type,(LPBYTE)(&ShowCPUPer),&Bytes);
			if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { ShowCPUPer = Default_ShowCPUPer;	}

			lResult = RegQueryValueEx(hKeyResults,"Always On Top",0,&Type,(LPBYTE)(&AlwaysOnTop),&Bytes);
			if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { AlwaysOnTop = Default_AlwaysOnTop;	}
		}

#if (!defined(EXTERNAL_RELEASE))
		if (HaveDebugger) {
			lResult = RegQueryValueEx(hKeyResults,"Auto Load Map File",0,&Type,(BYTE *)(&AutoLoadMapFile),&Bytes);
			if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { AutoLoadMapFile = Default_AutoMap; }

			lResult = RegQueryValueEx(hKeyResults,"Show Unhandled Memory Accesses",0,&Type,(LPBYTE)(&ShowUnhandledMemory),&Bytes);
			if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { ShowUnhandledMemory = Default_ShowUnhandledMemory;	}

			lResult = RegQueryValueEx(hKeyResults,"Show Load/Store TLB Misses",0,&Type,(LPBYTE)(&ShowTLBMisses),&Bytes);
			if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { ShowTLBMisses = Default_ShowTLBMisses;	}

			lResult = RegQueryValueEx(hKeyResults,"Show Dlist/Alist Count",0,&Type,(LPBYTE)(&ShowDListAListCount),&Bytes);
			if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { ShowDListAListCount = Default_ShowDlistCount;	}

			lResult = RegQueryValueEx(hKeyResults,"Show Compile Memory",0,&Type,(LPBYTE)(&ShowCompMem),&Bytes);
			if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { ShowCompMem = Default_ShowCompileMemory;	}

			lResult = RegQueryValueEx(hKeyResults,"Show Pif Ram Errors",0,&Type,(LPBYTE)(&ShowPifRamErrors),&Bytes);
			if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { ShowPifRamErrors = Default_ShowPifRamErrors;	}

			lResult = RegQueryValueEx(hKeyResults,"Profiling On",0,&Type,(LPBYTE)(&Profiling),&Bytes);
			if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { Profiling = Default_ProfilingOn; }

			lResult = RegQueryValueEx(hKeyResults,"Log Indvidual Blocks",0,&Type,(LPBYTE)(&IndvidualBlock),&Bytes);
			if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { IndvidualBlock = Default_IndvidualBlock; }

		}
#endif
		RegCloseKey(hKeyResults);
	}
}

int InitalizeApplication ( HINSTANCE hInstance ) {
	INITCOMMONCONTROLSEX IntComStruct;
	HKEY hKeyResults = 0;

	CoInitialize(NULL);

	IntComStruct.dwSize = sizeof(IntComStruct);
	IntComStruct.dwICC = ICC_TREEVIEW_CLASSES;
	InitCommonControls();
	InitCommonControlsEx(&IntComStruct);
	hInst = hInstance;

	{
		char String[200];

		sprintf(String,"Software\\N64 Emulation\\%s",AppName);
		//Language selection if none found
		LoadLanguage(String);
	}

	if (!Allocate_Memory()) {
		DisplayError(GS(MSG_MEM_ALLOC_ERROR));
		return FALSE;
	}

	hPauseMutex = CreateMutex(NULL,FALSE,NULL);

	InitiliazeCPUFlags();
	LoadSettings();
	SetupRegisters(&Registers);
	QueryPerformanceFrequency(&Frequency);
#if (!defined(EXTERNAL_RELEASE))
	LoadLogOptions(&LogOptions, FALSE);
	StartLog();
#endif
	LoadRomBrowserColoumnInfo ();
	InitilizeInitialCompilerVariable();
	return TRUE;
}

void CheckedMenuItem(UINT uMenuID, BOOL * Flag, char * FlagName) {
	char String[256];
	DWORD Disposition;
	HKEY hKeyResults;
	long lResult;
	UINT uState;

	uState = GetMenuState( hMainMenu, uMenuID, MF_BYCOMMAND);
	hKeyResults = 0;
	Disposition = 0;

	if ( uState & MFS_CHECKED ) {
		CheckMenuItem( hMainMenu, uMenuID, MF_BYCOMMAND | MFS_UNCHECKED );
		*Flag = FALSE;
	} else {
		CheckMenuItem( hMainMenu, uMenuID, MF_BYCOMMAND | MFS_CHECKED );
		*Flag = TRUE;
	}

	sprintf(String,"Software\\N64 Emulation\\%s",AppName);
	lResult = RegCreateKeyEx( HKEY_CURRENT_USER,String,0,"",
		REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKeyResults,&Disposition);

	if (lResult == ERROR_SUCCESS) {
		RegSetValueEx(hKeyResults,FlagName,0,REG_DWORD,(BYTE *)Flag,sizeof(DWORD));
	}
	RegCloseKey(hKeyResults);
}


LRESULT CALLBACK Main_Proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
#if (!defined(EXTERNAL_RELEASE))
	char String[256];
	DWORD Disposition;
	HKEY hKeyResults;
	long lResult;
	UINT uState;
#endif
	HMENU hMenu;

	switch (uMsg) {
	case WM_CREATE:
		if ( hHiddenWin ) {
			hStatusWnd = CreateStatusWindow( WS_CHILD | WS_VISIBLE, "", hWnd, 100 );
			SendMessage( hStatusWnd, SB_SETTEXT, 0, (LPARAM)"" );
			ChangeWinSize ( hWnd, 640, 480, hStatusWnd );
		}
		break;
	case WM_NCLBUTTONDBLCLK:
		switch (wParam) {
		case HTCAPTION:
			SetRomBrowserMaximized(TRUE);
			return DefWindowProc(hWnd,uMsg,wParam,lParam);
		default:
			return DefWindowProc(hWnd,uMsg,wParam,lParam);
		}
		break;
	case WM_SYSCOMMAND:
		switch (wParam) {
		case SC_SCREENSAVE:
		case SC_MONITORPOWER:
			return 0;
		case SC_MAXIMIZE:
			SetRomBrowserMaximized(TRUE);
			return DefWindowProc(hWnd,uMsg,wParam,lParam);
		default:
			return DefWindowProc(hWnd,uMsg,wParam,lParam);
		}
		break;
	case WM_PAINT:
		if (RomListVisible()) { ValidateRect(hWnd,NULL); break; }
		if (hWnd == hHiddenWin) { ValidateRect(hWnd,NULL); break; }
		__try {
			if (CPU_Paused && DrawScreen != NULL) { DrawScreen(); }
		} __except( r4300i_CPU_MemoryFilter( GetExceptionCode(), GetExceptionInformation()) ) {
			DisplayError(GS(MSG_UNKNOWN_MEM_ACTION));
			ExitThread(0);
		}
		ValidateRect(hWnd,NULL);
		break;
	case WM_MOVE:
		if (IgnoreMove) { break; }
		if (hWnd == hHiddenWin) { break; }
		if (MoveScreen != NULL && CPURunning) {
			MoveScreen((int)(short) LOWORD(lParam), (int)(short) HIWORD(lParam));
		}
		if (IsIconic(hWnd)) {
			if (!CPU_Paused && !inFullScreen) { PauseCpu(); break; }
			break;
		} else {
			if (!ManualPaused && (CPU_Paused || CPU_Action.Pause)) { PauseCpu(); break; }
		}
		if (!RomListVisible() || (RomListVisible() && !IsRomBrowserMaximized())) {
			StoreCurrentWinPos("Main", hWnd );
		}
		break;
	case WM_SIZE:
		if (hWnd == hHiddenWin) { break; }
		{
			RECT clrect, swrect;
			int Parts[2];

			GetClientRect( hWnd, &clrect );
			GetClientRect( hStatusWnd, &swrect );

			Parts[0] = (LOWORD( lParam) - (int)( clrect.right * 0.25 ));
			Parts[1] = LOWORD( lParam);

			SendMessage( hStatusWnd, SB_SETPARTS, 2, (LPARAM)&Parts[0] );
			MoveWindow ( hStatusWnd, 0, clrect.bottom - swrect.bottom,
				LOWORD( lParam ), HIWORD( lParam ), TRUE );
			DisplayFPS();
		}
		ResizeRomListControl(LOWORD(lParam),HIWORD( lParam ));
		if (!IgnoreMove) {
			if (wParam == SIZE_RESTORED && RomListVisible()) {
				SetRomBrowserMaximized(FALSE);
				SetRomBrowserSize(LOWORD(lParam),HIWORD( lParam ));
			}
		}
		break;
	case WM_SETFOCUS:
		if (hWnd == hHiddenWin) { break; }
		if (AutoSleep && !ManualPaused && (CPU_Paused || CPU_Action.Pause)) { PauseCpu(); }
		RomList_SetFocus();
		break;
	case WM_KILLFOCUS:
		if (hWnd == hHiddenWin) { break; }
		if (AutoSleep && !CPU_Paused) { PauseCpu(); }
		break;
	case WM_NOTIFY:
		if (wParam == IDC_ROMLIST) { RomListNotify((LPNMHDR)lParam); }
		return DefWindowProc(hWnd,uMsg,wParam,lParam);
		break;
	case WM_DRAWITEM:
		if (wParam == IDC_ROMLIST) { RomListDrawItem((LPDRAWITEMSTRUCT)lParam); }
		break;
	case WM_MENUSELECT:
		switch (LOWORD(wParam)) {
		case ID_PLAYGAME: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_PLAY_GAME)); break;
		case ID_POPUPMENU_ROMINFORMATION: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_ROM_INFO)); break;
		case ID_POPUPMENU_GAMEINFORMATION: SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_INFO)); break;
		case ID_EDITSETTINGS: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_GAME_SETTINGS)); break;
		case ID_EDITCHEATS: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_GAME_CHEATS)); break;
		case ID_FILE_OPEN_ROM: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_OPEN)); break;
		case ID_FILE_ROM_INFO: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_ROM_INFO)); break;
		case ID_FILE_GAME_INFO: SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_INFO)); break;
		case ID_FILE_STARTEMULATION: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_START)); break;
		case ID_FILE_ENDEMULATION: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_END)); break;
		case ID_FILE_ROMDIRECTORY: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_CHOOSE_ROM)); break;
		case ID_FILE_REFRESHROMLIST: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_REFRESH)); break;
		case ID_FILE_EXIT: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_EXIT)); break;
		case ID_CPU_RESET: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_RESET)); break;
		case ID_CPU_PAUSE: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_PAUSE)); break;
		case ID_SYSTEM_GENERATEBITMAP: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_BITMAP)); break;
		case ID_SYSTEM_LIMITFPS: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_LIMIT_FPS)); break;
		case ID_CPU_SAVE: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_SAVE)); break;
		case ID_CPU_SAVEAS: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_SAVE_AS)); break;
		case ID_CPU_RESTORE: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_RESTORE)); break;
		case ID_CPU_LOAD: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_LOAD)); break;
		case ID_OPTIONS_CHEATS: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_CHEAT)); break;
		case ID_SYSTEM_GSBUTTON: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_GS_BUTTON)); break;
		case ID_OPTIONS_FULLSCREEN: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_FULL_SCREEN)); break;
		case ID_OPTIONS_ALWAYSONTOP: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_ON_TOP)); break;
		case ID_OPTIONS_CONFIG_GFX: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_CONFG_GFX)); break;
		case ID_OPTIONS_CONFIG_AUDIO: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_CONFG_AUDIO)); break;
		case ID_OPTIONS_CONFIG_CONTROL: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_CONFG_CTRL)); break;
		case ID_OPTIONS_CONFIG_RSP: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_CONFG_RSP)); break;
		case ID_OPTIONS_SHOWCPUUSAGE: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_SHOW_CPU)); break;
		case ID_OPTIONS_SETTINGS: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_SETTINGS)); break;
		case ID_HELP_CONTENTS: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_USER_MAN)); break;
		case ID_HELP_GAMEFAQ: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_GAME_FAQ)); break;
		case ID_HELP_ABOUTSETTINGFILES: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_ABOUT_INI)); break;
		case ID_HELP_ABOUT: SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_ABOUT_PJ64)); break;
		
		case ID_CURRENTSAVE_DEFAULT:
			SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_SLOT));
			break;

		case ID_CURRENTSAVE_1:
			SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_SLOT));
			break;

		case ID_CURRENTSAVE_2:
			SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_SLOT));
			break;

		case ID_CURRENTSAVE_3:
			SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_SLOT));
			break;

		case ID_CURRENTSAVE_4:
			SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_SLOT));
			break;

		case ID_CURRENTSAVE_5:
			SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_SLOT));
			break;

		case ID_CURRENTSAVE_6:
			SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_SLOT));
			break;

		case ID_CURRENTSAVE_7:
			SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_SLOT));
			break;

		case ID_CURRENTSAVE_8:
			SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_SLOT));
			break;

		case ID_CURRENTSAVE_9:
			SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_SLOT));
			break;

			// Added an extra 10 save state slots with 10-19 on Shift+0-9 (Gent)

		case ID_CURRENTSAVE_10:
			SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_SLOT));
			break;

		case ID_CURRENTSAVE_11:
			SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_SLOT));
			break;

		case ID_CURRENTSAVE_12:
			SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_SLOT));
			break;

		case ID_CURRENTSAVE_13:
			SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_SLOT));
			break;

		case ID_CURRENTSAVE_14:
			SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_SLOT));
			break;

		case ID_CURRENTSAVE_15:
			SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_SLOT));
			break;

		case ID_CURRENTSAVE_16:
			SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_SLOT));
			break;

		case ID_CURRENTSAVE_17:
			SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_SLOT));
			break;

		case ID_CURRENTSAVE_18:
			SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_SLOT));
			break;

		case ID_CURRENTSAVE_19:
			SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_SLOT));
			break;


			// Removed this as we now have 0 to 9 - Gent

		/*case ID_CURRENTSAVE_0:
			SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MENUDES_GAME_SLOT));
			break;*/


		default:

			if (LOWORD(wParam) >= ID_FILE_RECENT_FILE && LOWORD(wParam) <= (ID_FILE_RECENT_FILE + RomsToRemember)) {
				SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_RECENT_ROM));
			} else if (LOWORD(wParam) >= ID_FILE_RECENT_DIR && LOWORD(wParam) <= (ID_FILE_RECENT_DIR + RomDirsToRemember)) {
				SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_RECENT_DIR));
			} else if (LOWORD(wParam) >= ID_LANG_SELECT && LOWORD(wParam) <= (ID_LANG_SELECT + 100)) {
				SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)GS(MENUDES_LANGUAGES));
			} else {
				SendMessage(hStatusWnd,SB_SETTEXT,0,(LPARAM)"");
			}
			break;
		}
		break;
	case WM_KEYDOWN:
		if (!CPURunning) { break; }
		if (WM_KeyDown) { WM_KeyDown(wParam, lParam); };
		break;
	case WM_KEYUP:
		if (!CPURunning) { break; }
		if (WM_KeyUp) { WM_KeyUp(wParam, lParam); };
		break;
	//case WM_ERASEBKGND: break;
	case WM_USER + 10:
		if (hWnd == hHiddenWin) { break; }
		if (!wParam) {
			while (ShowCursor(FALSE) >= 0) { Sleep(0); }
		} else {
			while (ShowCursor(TRUE) < 0) { Sleep(0); }
		}
		break;
	case WM_USER + 17:  SetFocus(hMainWindow); break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_PLAYGAME:
			if (strlen(CurrentRBFileName) > 0) {
				DWORD ThreadID;

				strcpy(CurrentFileName,CurrentRBFileName);
				CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)OpenChosenFile,NULL,0, &ThreadID);
			}
			break;
		case ID_EDITSETTINGS: ChangeRomSettings(hWnd); break;
		case ID_EDITCHEATS: ChangeRomCheats(hMainWindow); break;
		case ID_POPUPMENU_ROMINFORMATION:
			{
				char OrigRomName[sizeof(RomName)], OrigFileName[sizeof(CurrentFileName)];
				BYTE OrigByteHeader[sizeof(RomHeader)];
				DWORD OrigFileSize;

				//Make copy of current Game values
				strncpy(OrigRomName,RomName,sizeof(OrigRomName));
				strncpy(OrigFileName,CurrentFileName,sizeof(OrigFileName));
				memcpy(OrigByteHeader,RomHeader,sizeof(RomHeader));
				strncpy(CurrentFileName,CurrentRBFileName,sizeof(CurrentFileName));
				OrigFileSize = RomFileSize;

				//Load header of selected Rom
				LoadRomHeader();

				//Display Information
				RomInfo();

				//Restore settings
				strncpy(RomName,OrigRomName,sizeof(RomName));
				strncpy(CurrentFileName,OrigFileName,sizeof(CurrentFileName));
				memcpy(RomHeader,OrigByteHeader,sizeof(RomHeader));
				RomFileSize = OrigFileSize;

			}
			break;
		case ID_POPUPMENU_GAMEINFORMATION: GameInfoByGameInfoID(CurrentGameInfoID); break;
		case ID_FILE_OPEN_ROM: OpenN64Image(); break;
		case ID_FILE_ROM_INFO: RomInfo(); break;
		case ID_FILE_GAME_INFO: GameInfoByRomID(); break;
		case ID_FILE_STARTEMULATION:
			if (strlen(RomName) == 0) { break; }
			HideRomBrowser();
			StartEmulation();
			break;
		case ID_FILE_ENDEMULATION:
			if (inFullScreen) {
				CPU_Paused = TRUE;
				SendMessage(hMainWindow,WM_COMMAND,ID_OPTIONS_FULLSCREEN,0);
				CPU_Paused = FALSE;
			}
			CloseCpu();
			hMenu = GetMenu(hMainWindow);
			EnableMenuItem(hMenu,ID_FILE_STARTEMULATION,MFS_ENABLED|MF_BYCOMMAND);
			if (DrawScreen != NULL) { DrawScreen(); }
			CloseCheatWindow();
			if (RomBrowser) {
				ShowRomList(hMainWindow);
			}
			break;
		case ID_FILE_ROMDIRECTORY: SelectRomDir(); break;
		case ID_FILE_REFRESHROMLIST: RefreshRomBrowser(); break;
		case ID_FILE_EXIT: DestroyWindow(hWnd);	break;
		case ID_CPU_RESET:
			CloseCpu();
			StartEmulation();
			break;
		case ID_CPU_PAUSE: ManualPaused = TRUE; PauseCpu(); break;
		case ID_CPU_SAVE:
			if (CPU_Paused) {
				if (!Machine_SaveState()) {
					CPU_Action.SaveState = TRUE;
					CPU_Action.DoSomething = TRUE;
				}
			} else {
				CPU_Action.SaveState = TRUE;
				CPU_Action.DoSomething = TRUE;
			}
			break;
		case ID_CPU_SAVEAS:
			{
				char drive[_MAX_DRIVE] ,dir[_MAX_DIR], fname[_MAX_FNAME],ext[_MAX_EXT];
				char Directory[255], SaveFile[255];
				OPENFILENAME openfilename;

				if (BasicMode) { break; }
				memset(&SaveFile, 0, sizeof(SaveFile));
				memset(&openfilename, 0, sizeof(openfilename));

				GetInstantSaveDir( Directory );

				openfilename.lStructSize  = sizeof( openfilename );
				openfilename.hwndOwner    = hMainWindow;
				openfilename.lpstrFilter  = "PJ64 Saves (*.zip, *.pj)\0*.pj?;*.pj;*.zip;";
				openfilename.lpstrFile    = SaveFile;
				openfilename.lpstrInitialDir    = Directory;
				openfilename.nMaxFile     = MAX_PATH;
				openfilename.Flags        = OFN_HIDEREADONLY;

				if (GetSaveFileName (&openfilename)) {
					_splitpath( SaveFile, drive, dir, fname, ext );
					if (strcmp(ext, "pj") == 0 || strcmp(ext, "zip") == 0) {
						_makepath( SaveFile, drive, dir, fname, NULL );
					}
					strcpy(SaveAsFileName,SaveFile);
					if (CPU_Paused) {
						if (!Machine_SaveState()) {
							CPU_Action.SaveState = TRUE;
							CPU_Action.DoSomething = TRUE;
						}
					} else {
						CPU_Action.SaveState = TRUE;
						CPU_Action.DoSomething = TRUE;
					}
				}
			}
			break;
		case ID_CPU_RESTORE:
			CPU_Action.RestoreState = TRUE;
			CPU_Action.DoSomething = TRUE;
			break;
		case ID_CPU_LOAD:
			{
				char Directory[255], SaveFile[255];
				OPENFILENAME openfilename;

				if (BasicMode) { break; }
				memset(&SaveFile, 0, sizeof(SaveFile));
				memset(&openfilename, 0, sizeof(openfilename));

				GetInstantSaveDir( Directory );

				openfilename.lStructSize  = sizeof( openfilename );
				openfilename.hwndOwner    = hMainWindow;
				openfilename.lpstrFilter  = "PJ64 Saves (*.zip, *.pj)\0*.pj?;*.pj;*.zip;";
				openfilename.lpstrFile    = SaveFile;
				openfilename.lpstrInitialDir    = Directory;
				openfilename.nMaxFile     = MAX_PATH;
				openfilename.Flags        = OFN_HIDEREADONLY;

				if (GetOpenFileName (&openfilename)) {
					strcpy(LoadFileName,SaveFile);
					CPU_Action.RestoreState = TRUE;
					CPU_Action.DoSomething = TRUE;
				}
			}
			break;
		case ID_SYSTEM_GENERATEBITMAP:
			if (CaptureScreen) {
				char Directory[255];
				GetSnapShotDir(Directory);
				CaptureScreen(Directory);
			}
			break;
		case ID_SYSTEM_LIMITFPS:
			if (BasicMode) { break; }
			CheckedMenuItem(ID_SYSTEM_LIMITFPS,&LimitFPS,"Limit FPS");
			break;
	
				case ID_CURRENTSAVE_DEFAULT:
					SetCurrentSaveState(hWnd, LOWORD(wParam));
					break;

					// Removed this as we now have 0 to 9 - Gent

				/*case ID_CURRENTSAVE_0:
					SetCurrentSaveState(hWnd, LOWORD(wParam));
					break;*/

				case ID_CURRENTSAVE_1:
					SetCurrentSaveState(hWnd, LOWORD(wParam));
					break;

				case ID_CURRENTSAVE_2:
					SetCurrentSaveState(hWnd, LOWORD(wParam));
					break;

				case ID_CURRENTSAVE_3:
					SetCurrentSaveState(hWnd, LOWORD(wParam));
					break;

				case ID_CURRENTSAVE_4:
					SetCurrentSaveState(hWnd, LOWORD(wParam));
					break;

				case ID_CURRENTSAVE_5:
					SetCurrentSaveState(hWnd, LOWORD(wParam));
					break;

				case ID_CURRENTSAVE_6:
					SetCurrentSaveState(hWnd, LOWORD(wParam));
					break;

				case ID_CURRENTSAVE_7:
					SetCurrentSaveState(hWnd, LOWORD(wParam));
					break;

				case ID_CURRENTSAVE_8:
					SetCurrentSaveState(hWnd, LOWORD(wParam));
					break;

				case ID_CURRENTSAVE_9:
					SetCurrentSaveState(hWnd, LOWORD(wParam));
					break;
					
				// Added an extra 10 save state slots with 10-19 on Shift+0-9 (Gent)

				case ID_CURRENTSAVE_10:
					SetCurrentSaveState(hWnd, LOWORD(wParam));
					break;

				case ID_CURRENTSAVE_11:
					SetCurrentSaveState(hWnd, LOWORD(wParam));
					break;

				case ID_CURRENTSAVE_12:
					SetCurrentSaveState(hWnd, LOWORD(wParam));
					break;

				case ID_CURRENTSAVE_13:
					SetCurrentSaveState(hWnd, LOWORD(wParam));
					break;

				case ID_CURRENTSAVE_14:
					SetCurrentSaveState(hWnd, LOWORD(wParam));
					break;

				case ID_CURRENTSAVE_15:
					SetCurrentSaveState(hWnd, LOWORD(wParam));
					break;

				case ID_CURRENTSAVE_16:
					SetCurrentSaveState(hWnd, LOWORD(wParam));
					break;

				case ID_CURRENTSAVE_17:
					SetCurrentSaveState(hWnd, LOWORD(wParam));
					break;

				case ID_CURRENTSAVE_18:
					SetCurrentSaveState(hWnd, LOWORD(wParam));
					break;

				case ID_CURRENTSAVE_19:
					SetCurrentSaveState(hWnd, LOWORD(wParam));
					break;
			
			/* Witten */
		//Cheat search crashes on first scan so disabling for now (Gent)
		//case ID_SYSTEM_CHEATSEARCH: Show_CheatSearchDlg (hWnd); break;
		/* Witten */
		case ID_SYSTEM_GSBUTTON: ApplyGSButton(); break;
		case ID_OPTIONS_FULLSCREEN:
			if (CPU_Paused) {
				CPU_Action.ChangeWindow = FALSE;
				if (ChangeWindow) {
					ChangeWindow();
					inFullScreen = !inFullScreen;
					AlwaysOnTopWindow(hMainWindow);
				}
			} else {
				CPU_Action.ChangeWindow = TRUE;
				CPU_Action.DoSomething = TRUE;
			}
			
			if (!inFullScreen || CPU_Action.ChangeWindow == TRUE)
			{
				LONG style = GetWindowLong(hWnd, GWL_STYLE) | WS_SYSMENU | WS_CAPTION;
				SetWindowLongPtr(hWnd, GWL_STYLE, style);
				SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(hInst, "ICON"));
			}
			break;
		case ID_OPTIONS_ALWAYSONTOP:
			if (BasicMode) { break; }
			CheckedMenuItem(ID_OPTIONS_ALWAYSONTOP,&AlwaysOnTop,"Always On Top");
			AlwaysOnTopWindow(hWnd);
			break;
		case ID_OPTIONS_CONFIG_GFX: GFXDllConfig(hWnd); break;
		case ID_OPTIONS_CONFIG_AUDIO: AiDllConfig(hWnd); break;
		case ID_OPTIONS_CONFIG_RSP: RSPDllConfig(hWnd); break;
		case ID_OPTIONS_CONFIG_CONTROL: ContConfig(hWnd); break;
		case ID_OPTIONS_SETTINGS: ChangeSettings(hWnd); break;
		case ID_OPTIONS_CHEATS: ManageCheats(NULL); break;
		case ID_OPTIONS_SHOWCPUUSAGE:
			CheckedMenuItem(ID_OPTIONS_SHOWCPUUSAGE,&ShowCPUPer,"Show CPU %");
			break;
#if (!defined(EXTERNAL_RELEASE))
		case ID_OPTIONS_PROFILING_ON:
		case ID_OPTIONS_PROFILING_OFF:
			if (HaveDebugger) {

				hMenu = GetMenu( hWnd );
				uState = GetMenuState( hMenu, ID_OPTIONS_PROFILING_ON, MF_BYCOMMAND);
				hKeyResults = 0;
				Disposition = 0;

				if ( uState & MFS_CHECKED ) {
					CheckMenuItem( hMenu, ID_OPTIONS_PROFILING_ON, MF_BYCOMMAND | MFS_UNCHECKED );
					CheckMenuItem( hMenu, ID_OPTIONS_PROFILING_OFF, MF_BYCOMMAND | MFS_CHECKED );
					Profiling = FALSE;
					ResetTimerList();
				} else {
					CheckMenuItem( hMenu, ID_OPTIONS_PROFILING_ON, MF_BYCOMMAND | MFS_CHECKED );
					CheckMenuItem( hMenu, ID_OPTIONS_PROFILING_OFF, MF_BYCOMMAND | MFS_UNCHECKED );
					ResetTimerList();
					Profiling = TRUE;
				}

				sprintf(String,"Software\\N64 Emulation\\%s",AppName);
				lResult = RegCreateKeyEx( HKEY_CURRENT_USER,String,0,"",
					REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKeyResults,&Disposition);

				if (lResult == ERROR_SUCCESS) {
					RegSetValueEx(hKeyResults,"Profiling On",0,REG_DWORD,(BYTE *)&Profiling,sizeof(DWORD));
				}
				RegCloseKey(hKeyResults);
			}
			break;
		case ID_OPTIONS_PROFILING_RESETSTATS:
			if (HaveDebugger) { ResetTimerList(); }
			break;
		case ID_OPTIONS_PROFILING_GENERATELOG:
			if (HaveDebugger) { GenerateTimerResults(); }
			break;
		case ID_OPTIONS_PROFILING_LOGINDIVIDUALBLOCKS:
			if (HaveDebugger) {
				hMenu = GetMenu( hWnd );
				uState = GetMenuState( hMenu, ID_OPTIONS_PROFILING_LOGINDIVIDUALBLOCKS, MF_BYCOMMAND);
				hKeyResults = 0;
				Disposition = 0;

				if ( uState & MFS_CHECKED ) {
					CheckMenuItem( hMenu, ID_OPTIONS_PROFILING_LOGINDIVIDUALBLOCKS, MF_BYCOMMAND | MFS_UNCHECKED );
					IndvidualBlock = FALSE;
				} else {
					CheckMenuItem( hMenu, ID_OPTIONS_PROFILING_LOGINDIVIDUALBLOCKS, MF_BYCOMMAND | MFS_CHECKED );
					IndvidualBlock = TRUE;
				}

				sprintf(String,"Software\\N64 Emulation\\%s",AppName);
				lResult = RegCreateKeyEx( HKEY_CURRENT_USER,String,0,"",
					REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKeyResults,&Disposition);

				if (lResult == ERROR_SUCCESS) {
					RegSetValueEx(hKeyResults,"Log Indvidual Blocks",0,REG_DWORD,
						(BYTE *)&IndvidualBlock,sizeof(DWORD));
				}
				RegCloseKey(hKeyResults);
			}
			break;
		case ID_OPTIONS_MAPPINGS_OPENMAPFILE: ChooseMapFile(hWnd); break;
		case ID_OPTIONS_MAPPINGS_CLOSEMAPFILE: ResetMappings(); break;
		case ID_OPTIONS_MAPPINGS_AUTOLOADMAPFILE:
			if (HaveDebugger) {
				hMenu = GetMenu( hWnd );
				uState = GetMenuState( hMenu, ID_OPTIONS_MAPPINGS_AUTOLOADMAPFILE, MF_BYCOMMAND);
				hKeyResults = 0;
				Disposition = 0;

				if ( uState & MFS_CHECKED ) {
					CheckMenuItem( hMenu, ID_OPTIONS_MAPPINGS_AUTOLOADMAPFILE, MF_BYCOMMAND | MFS_UNCHECKED );
					AutoLoadMapFile = FALSE;
				} else {
					CheckMenuItem( hMenu, ID_OPTIONS_MAPPINGS_AUTOLOADMAPFILE, MF_BYCOMMAND | MFS_CHECKED );
					AutoLoadMapFile = TRUE;
				}

				sprintf(String,"Software\\N64 Emulation\\%s",AppName);
				lResult = RegCreateKeyEx( HKEY_CURRENT_USER,String,0,"",
					REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKeyResults,&Disposition);

				if (lResult == ERROR_SUCCESS) {
					RegSetValueEx(hKeyResults,"Auto Load Map File",0,REG_DWORD,
						(BYTE *)&AutoLoadMapFile,sizeof(DWORD));
				}
				RegCloseKey(hKeyResults);
			}
			break;
		case ID_DEBUGGER_SETBREAKPOINT: Enter_BPoint_Window(); break;
		case ID_DEBUGGER_R4300ICOMMANDS: Enter_R4300i_Commands_Window(); break;
		case ID_DEBUGGER_R4300IREGISTERS: Enter_R4300i_Register_Window(); break;
		case ID_DEBUGGER_LOGOPTIONS: EnterLogOptions(hWnd); break;
		case ID_DEBUGGER_GENERATELOG:
			if (HaveDebugger) {
				hMenu = GetMenu( hWnd );
				uState = GetMenuState( hMenu, ID_DEBUGGER_GENERATELOG, MF_BYCOMMAND);
				hKeyResults = 0;
				Disposition = 0;

				if ( uState & MFS_CHECKED ) {
					CheckMenuItem( hMenu, ID_DEBUGGER_GENERATELOG, MF_BYCOMMAND | MFS_UNCHECKED );
					LogOptions.GenerateLog = FALSE;
				} else {
					CheckMenuItem( hMenu, ID_DEBUGGER_GENERATELOG, MF_BYCOMMAND | MFS_CHECKED );
					LogOptions.GenerateLog = TRUE;
				}

				sprintf(String,"Software\\N64 Emulation\\%s\\Logging",AppName);
				lResult = RegCreateKeyEx( HKEY_CURRENT_USER,String,0,"",
					REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKeyResults,&Disposition);

				if (lResult == ERROR_SUCCESS) {
					RegSetValueEx(hKeyResults,"Generate Log File",0,
						REG_DWORD,(BYTE *)&LogOptions.GenerateLog,sizeof(DWORD));
				}
				RegCloseKey(hKeyResults);
				LoadLogOptions(&LogOptions, FALSE);
#ifndef EXTERNAL_RELEASE
				StartLog();
#endif
			}
			break;
		case ID_DEBUGGER_MEMORY: Enter_Memory_Window(); break;
		case ID_DEBUGGER_TLBENTRIES: Enter_TLB_Window(); break;
		case ID_DEBUGGER_SHOWUNHANDLEDMEMORYACCESSES:
			if (HaveDebugger) {
				hMenu = GetMenu( hWnd );
				uState = GetMenuState( hMenu, ID_DEBUGGER_SHOWUNHANDLEDMEMORYACCESSES, MF_BYCOMMAND);
				hKeyResults = 0;
				Disposition = 0;

				if ( uState & MFS_CHECKED ) {
					CheckMenuItem( hMenu, ID_DEBUGGER_SHOWUNHANDLEDMEMORYACCESSES, MF_BYCOMMAND | MFS_UNCHECKED );
					ShowUnhandledMemory = FALSE;
				} else {
					CheckMenuItem( hMenu, ID_DEBUGGER_SHOWUNHANDLEDMEMORYACCESSES, MF_BYCOMMAND | MFS_CHECKED );
					ShowUnhandledMemory = TRUE;
				}

				sprintf(String,"Software\\N64 Emulation\\%s",AppName);
				lResult = RegCreateKeyEx( HKEY_CURRENT_USER,String,0,"",
					REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKeyResults,&Disposition);

				if (lResult == ERROR_SUCCESS) {
					RegSetValueEx(hKeyResults,"Show Unhandled Memory Accesses",0,
						REG_DWORD,(BYTE *)&ShowUnhandledMemory,sizeof(DWORD));
				}
				RegCloseKey(hKeyResults);
			}
			break;
		case ID_DEBUGGER_SHOWTLBMISSES:
			if (HaveDebugger) {
				hMenu = GetMenu( hWnd );
				uState = GetMenuState( hMenu, ID_DEBUGGER_SHOWTLBMISSES, MF_BYCOMMAND);
				hKeyResults = 0;
				Disposition = 0;

				if ( uState & MFS_CHECKED ) {
					CheckMenuItem( hMenu, ID_DEBUGGER_SHOWTLBMISSES, MF_BYCOMMAND | MFS_UNCHECKED );
					ShowTLBMisses = FALSE;
				} else {
					CheckMenuItem( hMenu, ID_DEBUGGER_SHOWTLBMISSES, MF_BYCOMMAND | MFS_CHECKED );
					ShowTLBMisses = TRUE;
				}

				sprintf(String,"Software\\N64 Emulation\\%s",AppName);
				lResult = RegCreateKeyEx( HKEY_CURRENT_USER,String,0,"",
					REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKeyResults,&Disposition);

				if (lResult == ERROR_SUCCESS) {
					RegSetValueEx(hKeyResults,"Show Load/Store TLB Misses",0,
						REG_DWORD,(BYTE *)&ShowTLBMisses,sizeof(DWORD));
				}
				RegCloseKey(hKeyResults);
			}
			break;
		case ID_DEBUGGER_SHOWDLISTALISTCOUNT:
			if (HaveDebugger) {
				hMenu = GetMenu( hWnd );
				uState = GetMenuState( hMenu, ID_DEBUGGER_SHOWDLISTALISTCOUNT, MF_BYCOMMAND);
				hKeyResults = 0;
				Disposition = 0;

				if ( uState & MFS_CHECKED ) {
					CheckMenuItem( hMenu, ID_DEBUGGER_SHOWDLISTALISTCOUNT, MF_BYCOMMAND | MFS_UNCHECKED );
					ShowDListAListCount = FALSE;
				} else {
					CheckMenuItem( hMenu, ID_DEBUGGER_SHOWDLISTALISTCOUNT, MF_BYCOMMAND | MFS_CHECKED );
					ShowDListAListCount = TRUE;
				}

				sprintf(String,"Software\\N64 Emulation\\%s",AppName);
				lResult = RegCreateKeyEx( HKEY_CURRENT_USER,String,0,"",
					REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKeyResults,&Disposition);

				if (lResult == ERROR_SUCCESS) {
					RegSetValueEx(hKeyResults,"Show Dlist/Alist Count",0,
						REG_DWORD,(BYTE *)&ShowDListAListCount,sizeof(DWORD));
				}
				RegCloseKey(hKeyResults);
			}
			break;
		case ID_DEBUGGER_SHOWCOMPMEM:
			if (HaveDebugger) {
				hMenu = GetMenu( hWnd );
				uState = GetMenuState( hMenu, ID_DEBUGGER_SHOWCOMPMEM, MF_BYCOMMAND);
				hKeyResults = 0;
				Disposition = 0;

				if ( uState & MFS_CHECKED ) {
					CheckMenuItem( hMenu, ID_DEBUGGER_SHOWCOMPMEM, MF_BYCOMMAND | MFS_UNCHECKED );
					ShowCompMem = FALSE;
				} else {
					CheckMenuItem( hMenu, ID_DEBUGGER_SHOWCOMPMEM, MF_BYCOMMAND | MFS_CHECKED );
					ShowCompMem = TRUE;
				}

				sprintf(String,"Software\\N64 Emulation\\%s",AppName);
				lResult = RegCreateKeyEx( HKEY_CURRENT_USER,String,0,"",
					REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKeyResults,&Disposition);

				if (lResult == ERROR_SUCCESS) {
					RegSetValueEx(hKeyResults,"Show Compile Memory",0,
						REG_DWORD,(BYTE *)&ShowCompMem,sizeof(DWORD));
				}
				RegCloseKey(hKeyResults);
			}
			break;
		case ID_DEBUGGER_SHOWPIFRAMERRORS:
			if (HaveDebugger) {
				hMenu = GetMenu( hWnd );
				uState = GetMenuState( hMenu, ID_DEBUGGER_SHOWPIFRAMERRORS, MF_BYCOMMAND);
				hKeyResults = 0;
				Disposition = 0;

				if ( uState & MFS_CHECKED ) {
					CheckMenuItem( hMenu, ID_DEBUGGER_SHOWPIFRAMERRORS, MF_BYCOMMAND | MFS_UNCHECKED );
					ShowPifRamErrors = FALSE;
				} else {
					CheckMenuItem( hMenu, ID_DEBUGGER_SHOWPIFRAMERRORS, MF_BYCOMMAND | MFS_CHECKED );
					ShowPifRamErrors = TRUE;
				}

				sprintf(String,"Software\\N64 Emulation\\%s",AppName);
				lResult = RegCreateKeyEx( HKEY_CURRENT_USER,String,0,"",
					REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKeyResults,&Disposition);

				if (lResult == ERROR_SUCCESS) {
					RegSetValueEx(hKeyResults,"Show Pif Ram Errors",0,
						REG_DWORD,(BYTE *)&ShowPifRamErrors,sizeof(DWORD));
				}
				RegCloseKey(hKeyResults);
			}
			break;
#endif
		case ID_HELP_CONTENTS:
			ShellExecute(NULL, "open", "https://github.com/pj64team/Project64-1.6-Plus/wiki/User-Guide", NULL, NULL, SW_SHOWMAXIMIZED); // Changed chm to online version for better user experience
			break;

			// The call causes a crash if 1 is there at the end, no idea why it's even there as it works fine without it.
			/*if (HtmlHelp(hWnd, HelpFileName, HH_DISPLAY_TOPIC, (DWORD)NULL) == NULL) {
				ShellExecute(hWnd, "open", HelpFileName, NULL, NULL, SW_SHOW);
			}
		}
		break;*/
		/*		case ID_HELPMNU_INDEX:
					{
						char path_buffer[_MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR];
						char fname[_MAX_FNAME],ext[_MAX_EXT], HelpFileName[_MAX_PATH];

						GetModuleFileName(NULL,path_buffer,sizeof(path_buffer));
						_splitpath( path_buffer, drive, dir, fname, ext );
						//_makepath( HelpFileName, drive, dir, "Project64", "chm" );
						sprintf(HelpFileName, "%s%sConfig\\%s", drive, dir, ChmFileName);

						if (HtmlHelp(hWnd, HelpFileName, HH_DISPLAY_INDEX, 0) == NULL) {
							ShellExecute(hWnd, "open", HelpFileName, NULL, NULL, SW_SHOW);
						}
					}
					break;*/
		case ID_HELP_GAMEFAQ:
			ShellExecute(NULL, "open", "https://www.n64gamespedia.com/", NULL, NULL, SW_SHOWMAXIMIZED); // Changed chm to online version for better user experience
			break;


			// Gent no longer indexes these files so the old value of 1 no longer points to something valid, using NULL instead
			/*if (HtmlHelp(hWnd, HelpFileName, HH_DISPLAY_TOPIC, (DWORD)NULL) == NULL) {
				ShellExecute(hWnd, "open", HelpFileName, NULL, NULL, SW_SHOW);
			}
		}
		break;*/
		case ID_HELP_DISCORD: ShellExecute(NULL, "open", "https://discord.gg/TnFmnW6WQE", NULL, NULL, SW_SHOWMAXIMIZED); break;
		case ID_HELP_GITHUB: ShellExecute(NULL, "open", "https://github.com/pj64team/Project64-1.6-Plus", NULL, NULL, SW_SHOWMAXIMIZED); break;
		case ID_HELP_UNINSTALL: UninstallApplication(hWnd); break;
		case ID_HELP_JABO_UNINSTALL: UninstallJabo(hWnd); break;
		case ID_HELP_ABOUT: AboutBox(); break;
		case ID_HELP_ABOUTSETTINGFILES: AboutIniBox(); break;
		default:
			if (LOWORD(wParam) >= ID_FILE_RECENT_FILE && LOWORD(wParam) <= (ID_FILE_RECENT_FILE + RomsToRemember)) {
				LoadRecentRom(LOWORD(wParam));
			} else if (LOWORD(wParam) >= ID_FILE_RECENT_DIR && LOWORD(wParam) <= (ID_FILE_RECENT_DIR + RomDirsToRemember)) {
				SetRecentRomDir(LOWORD(wParam));
			} else if (LOWORD(wParam) >= ID_LANG_SELECT && LOWORD(wParam) <= (ID_LANG_SELECT + 100)) {
				SelectLangMenuItem(GetMenu(hWnd),LOWORD(wParam));
				FixRomBrowserColoumnLang();
				SetupMenu(hWnd);
			} else if (LOWORD(wParam) > 5000 && LOWORD(wParam) <= 5100 ) {
				if (RspDebug.ProcessMenuItem != NULL) {
					RspDebug.ProcessMenuItem(LOWORD(wParam));
				}
			} else if (LOWORD(wParam) > 5100 && LOWORD(wParam) <= 5200 ) {
				if (GFXDebug.ProcessMenuItem != NULL) {
					GFXDebug.ProcessMenuItem(LOWORD(wParam));
				}
			}
		}
		break;
	case WM_DESTROY:
		if (!ForceClose)
			SaveRomBrowserColoumnInfo();
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}
	return TRUE;
}

void MenuSetText ( HMENU hMenu, int MenuPos, char * Title, char * ShotCut) {
	MENUITEMINFO MenuInfo;
	char String[256];

	if (Title == NULL || strlen(Title) == 0) { return; }

	memset(&MenuInfo, 0, sizeof(MENUITEMINFO));
	MenuInfo.cbSize = sizeof(MENUITEMINFO);
	MenuInfo.fMask = MIIM_TYPE;
	MenuInfo.fType = MFT_STRING;
	MenuInfo.fState = MFS_ENABLED;
	MenuInfo.dwTypeData = String;
	MenuInfo.cch = 256;

	GetMenuItemInfo(hMenu,MenuPos,TRUE,&MenuInfo);
	if (strchr(Title,'\t') != NULL) { *(strchr(Title,'\t')) = '\0'; }
	strcpy(String,Title);
	if (ShotCut) { sprintf(String,"%s\t%s",String,ShotCut); }
	SetMenuItemInfo(hMenu,MenuPos,TRUE,&MenuInfo);
}

void RegisterExtension ( char * Extension, BOOL RegisterWithPj64 ) {
	char ShortAppName[] = { "PJ64" };
	char sKeyValue[] = { "Project 64" };
  	char app_path[_MAX_PATH];

	char String[200];
	DWORD Disposition = 0;
	HKEY hKeyResults = 0;
	long lResult;

	//Find Application name
	GetModuleFileName(NULL,app_path,sizeof(app_path));

	//creates a Root entry for sKeyName
	lResult = RegCreateKeyEx( HKEY_CLASSES_ROOT, ShortAppName,0,"", REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
	RegSetValueEx(hKeyResults,"",0,REG_SZ,(BYTE *)sKeyValue,sizeof(sKeyValue));
	RegCloseKey(hKeyResults);

	// Set the command line for "MyApp".
	sprintf(String,"%s\\DefaultIcon",ShortAppName);
	lResult = RegCreateKeyEx( HKEY_CLASSES_ROOT, String,0,"", REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
	sprintf(String,"%s",app_path);
	RegSetValueEx(hKeyResults,"",0,REG_SZ,(BYTE *)String,strlen(String));
	RegCloseKey(hKeyResults);

	//set the icon for the file extension
	sprintf(String,"%s\\shell\\open\\command",ShortAppName);
	lResult = RegCreateKeyEx( HKEY_CLASSES_ROOT, String,0,"", REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
	sprintf(String,"%s %%1",app_path);
	RegSetValueEx(hKeyResults,"",0,REG_SZ,(BYTE *)String,strlen(String));
	RegCloseKey(hKeyResults);

	// creates a Root entry for passed associated with sKeyName
	lResult = RegCreateKeyEx( HKEY_CLASSES_ROOT, Extension,0,"", REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
	if (RegisterWithPj64) {
		RegSetValueEx(hKeyResults,"",0,REG_SZ,(BYTE *)ShortAppName,sizeof(ShortAppName));
	} else {
		RegSetValueEx(hKeyResults,"",0,REG_SZ,(BYTE *)"",1);
	}

	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0);
}

int RegisterWinClass ( void ) {
	WNDCLASS wcl;

	wcl.style			= CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wcl.cbClsExtra		= 0;
	wcl.cbWndExtra		= 0;
	wcl.hIcon			= LoadIcon(hInst,"ICON");
	wcl.hCursor			= LoadCursor(hInst,IDC_ARROW);
	wcl.hInstance		= hInst;

	wcl.lpfnWndProc		= (WNDPROC)Main_Proc;
	wcl.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);
	wcl.lpszMenuName	= MAKEINTRESOURCE(MAIN_MENU);
	wcl.lpszClassName	= AppName;
	if (RegisterClass(&wcl)  == 0) return FALSE;

	wcl.lpfnWndProc		= (WNDPROC)Cheat_Proc;
	wcl.hbrBackground	= (HBRUSH)(COLOR_BTNFACE + 1);
	wcl.lpszMenuName	= NULL;
	wcl.lpszClassName	= "PJ64.Cheats";
	if (RegisterClass(&wcl)  == 0) return FALSE;

	return TRUE;
}

void RomInfo (void) {
	DialogBox(hInst, "ROM_INFO_DIALOG", hMainWindow, (DLGPROC)RomInfoProc);
}

LRESULT CALLBACK RomInfoProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	char String[255], count;
	char drive[_MAX_DRIVE] ,dir[_MAX_DIR];
	char fname[_MAX_FNAME],ext[_MAX_EXT];

	switch (uMsg) {
	case WM_INITDIALOG:
		SetWindowText(hDlg, GS(INFO_TITLE));
		SetDlgItemText(hDlg, IDC_ROM_NAME, GS(INFO_ROM_NAME_TEXT));
		SetDlgItemText(hDlg, IDC_FILE_NAME, GS(INFO_FILE_NAME_TEXT));
		SetDlgItemText(hDlg, IDC_LOCATION, GS(INFO_LOCATION_TEXT));
		SetDlgItemText(hDlg, IDC_ROM_SIZE, GS(INFO_SIZE_TEXT));
		SetDlgItemText(hDlg, IDC_CART_ID, GS(INFO_CART_ID_TEXT));
		SetDlgItemText(hDlg, IDC_MANUFACTURER, GS(INFO_MANUFACTURER_TEXT));
		SetDlgItemText(hDlg, IDC_COUNTRY, GS(INFO_COUNTRY_TEXT));
		SetDlgItemText(hDlg, IDC_CRC1, GS(INFO_CRC1_TEXT));
		SetDlgItemText(hDlg, IDC_CRC2, GS(INFO_CRC2_TEXT));
		SetDlgItemText(hDlg, IDC_CIC_CHIP, GS(INFO_CIC_CHIP_TEXT));


		memcpy(&String[1],(void *)(&RomHeader[0x20]),20);
		for( count = 1 ; count < 21; count += 4 ) {
			String[count] ^= String[count+3];
			String[count + 3] ^= String[count];
			String[count] ^= String[count+3];
			String[count + 1] ^= String[count + 2];
			String[count + 2] ^= String[count + 1];
			String[count + 1] ^= String[count + 2];
		}
		String[0] = ' ';
		String[21] = '\0';
		SetDlgItemText(hDlg,IDC_INFO_ROMNAME,String);

		_splitpath(CurrentFileName, drive, dir, fname, ext);
		strcpy(&String[1],fname);
		strcat(String,ext);
		SetDlgItemText(hDlg,IDC_INFO_FILENAME,String);

		strcpy(&String[1],drive);
		strcat(String,dir);
		SetDlgItemText(hDlg,IDC_INFO_LOCATION,String);

		sprintf(&String[1],"%.1f MBit",(float)RomFileSize/0x20000);
		SetDlgItemText(hDlg,IDC_INFO_ROMSIZE,String);

		String[1] = RomHeader[0x3F];
		String[2] = RomHeader[0x3E];
		String[3] = '\0';
		SetDlgItemText(hDlg,IDC_INFO_CARTID,String);

		switch (RomHeader[0x38]) {
		case 'N': SetDlgItemText(hDlg,IDC_INFO_MANUFACTURER," Nintendo"); break;
		case 0: SetDlgItemText(hDlg,IDC_INFO_MANUFACTURER," None"); break;
		default: SetDlgItemText(hDlg,IDC_INFO_MANUFACTURER,"(Unknown)"); break;
		}

		CountryCodeToString(&String[1], RomHeader[0x3D], 255 - 1);
		SetDlgItemText(hDlg,IDC_INFO_COUNTRY, String);
		
		sprintf(&String[1],"0x%08X",*(DWORD *)(&RomHeader[0x10]));
		SetDlgItemText(hDlg,IDC_INFO_CRC1,String);
		
		sprintf(&String[1],"0x%08X",*(DWORD *)(&RomHeader[0x14]));
		SetDlgItemText(hDlg,IDC_INFO_CRC2,String);

		if (GetCicChipID(RomHeader) < 0) {
			sprintf(&String[1],"Unknown");
		} else {
			sprintf(&String[1],"CIC-NUS-610%d",GetCicChipID(RomHeader));
		}
		SetDlgItemText(hDlg,IDC_INFO_CIC,String);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			EndDialog(hDlg,0);
			break;
		case IDC_CLOSE_BUTTON:
			EndDialog(hDlg,0);
			break;
		}
	default:
		return FALSE;
	}
	return TRUE;
}

void GameInfoByRomID() {
	char Identifier[100];
	char GameInfoID[250];

	GetRomIdentifier(Identifier);
	GetString(Identifier, "GameInformation", "", GameInfoID, sizeof(GameInfoID), GetExtIniFileName());
	GameInfoByGameInfoID(GameInfoID);
}

void GameInfoByGameInfoID(char * GameInfoID) {
	char String[300];

	if (strcmp(GameInfoID, "") == 0) {
		MessageBox(NULL, GS(MSG_NO_GAME_INFORMATION), GS(MSG_MSGBOX_TITLE), MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);
	}
	else {
		sprintf(String, "https://www.project64-legacy.com/index.php?id=%s", GameInfoID);
		ShellExecute(NULL, "open", String, NULL, NULL, SW_SHOWNORMAL);
	}
}

BOOL TestExtensionRegistered ( char * Extension ) {
	char ShortAppName[] = { "PJ64" };
	HKEY hKeyResults = 0;
	char Association[100];
	long lResult;
	DWORD Type, Bytes;

	lResult = RegOpenKey( HKEY_CLASSES_ROOT,Extension,&hKeyResults);
	if (lResult != ERROR_SUCCESS) { return FALSE; }

	Bytes = sizeof(Association);
	lResult = RegQueryValueEx(hKeyResults,"",0,&Type,(LPBYTE)(&Association),&Bytes);
	RegCloseKey(hKeyResults);
	if (lResult != ERROR_SUCCESS) { return FALSE;  }

	if (strcmp(Association,ShortAppName) != 0) { return FALSE; }
	return TRUE;
}


void DeleteAdvanceMenuOptions (HMENU hMenu) {
	HMENU hSubMenu;

	if (!BasicMode) { return; }
	//File
	hSubMenu = GetSubMenu(hMenu,0);
	//if (CPURunning) { DeleteMenu(hSubMenu,5,MF_BYPOSITION); } //Line
	DeleteMenu(hSubMenu,2,MF_BYPOSITION); //Line
	//System
	hSubMenu = GetSubMenu(hMenu,1);
	DeleteMenu(hSubMenu,3,MF_BYPOSITION); //Line

	DeleteMenu(hMenu,ID_FILE_STARTEMULATION,MF_BYCOMMAND);
	DeleteMenu(hMenu,ID_SYSTEM_GENERATEBITMAP,MF_BYCOMMAND);
	DeleteMenu(hMenu,ID_CPU_SAVEAS,MF_BYCOMMAND);
	DeleteMenu(hMenu,ID_CPU_LOAD,MF_BYCOMMAND);
	DeleteMenu(hMenu,ID_OPTIONS_ALWAYSONTOP,MF_BYCOMMAND);
	DeleteMenu(hMenu,ID_OPTIONS_SHOWCPUUSAGE,MF_BYCOMMAND);
	DeleteMenu(hMenu,ID_HELP_ABOUTSETTINGFILES,MF_BYCOMMAND);
	DeleteMenu(hMenu,ID_SYSTEM_LIMITFPS,MF_BYCOMMAND);
	DeleteMenu(hMenu,ID_OPTIONS_CONFIG_RSP,MF_BYCOMMAND);
	DeleteMenu(hMenu,ID_FILE_ROM_INFO,MF_BYCOMMAND);

	if (CPURunning) {
	//	DeleteMenu(hMenu,ID_FILE_ROMDIRECTORY,MF_BYCOMMAND);
	//	DeleteMenu(hMenu,ID_FILE_REFRESHROMLIST,MF_BYCOMMAND);
	} else {
		DeleteMenu(hMenu,1,MF_BYPOSITION);	// System
	}
}

void SetupMenu ( HWND hWnd ) {
	HMENU hMenu = GetMenu(hWnd), hSubMenu;
	int State;

	if (IsMenu(GFXDebug.hGFXMenu)) { RemoveMenu(hMenu,(DWORD)GFXDebug.hGFXMenu, MF_BYCOMMAND); }
	if (IsMenu(RspDebug.hRSPMenu)) { RemoveMenu(hMenu,(DWORD)RspDebug.hRSPMenu, MF_BYCOMMAND); }
	DestroyMenu(hMenu);
	hMenu = LoadMenu(hInst,MAKEINTRESOURCE(MAIN_MENU));

	FixMenuLang(hMenu);
	CreateLangList(GetSubMenu(hMenu,0),7, ID_LANG_SELECT);

	CreateRecentDirList(hMenu);
	CreateRecentFileList(hMenu);
	CheckMenuItem( hMenu, CurrentSaveSlot, MF_BYCOMMAND | MFS_CHECKED );

	if (LimitFPS) {
		CheckMenuItem( hMenu, ID_SYSTEM_LIMITFPS, MF_BYCOMMAND | MFS_CHECKED );
	}
	if (ShowCPUPer) {
		CheckMenuItem( hMenu, ID_OPTIONS_SHOWCPUUSAGE, MF_BYCOMMAND | MFS_CHECKED );
	}
	if (AlwaysOnTop) {
		CheckMenuItem( hMenu, ID_OPTIONS_ALWAYSONTOP, MF_BYCOMMAND | MFS_CHECKED );
	}
#if (!defined(EXTERNAL_RELEASE))
	if (HaveDebugger) {
		if (AutoLoadMapFile) {
			CheckMenuItem( hMenu, ID_OPTIONS_MAPPINGS_AUTOLOADMAPFILE, MF_BYCOMMAND | MFS_CHECKED );
		}
		if (LogOptions.GenerateLog) {
			CheckMenuItem( hMenu, ID_DEBUGGER_GENERATELOG, MF_BYCOMMAND | MFS_CHECKED );
		}
		if (ShowUnhandledMemory) {
			CheckMenuItem( hMenu, ID_DEBUGGER_SHOWUNHANDLEDMEMORYACCESSES, MF_BYCOMMAND | MFS_CHECKED );
		}
		if (ShowTLBMisses) {
			CheckMenuItem( hMenu, ID_DEBUGGER_SHOWTLBMISSES, MF_BYCOMMAND | MFS_CHECKED );
		}
		if (ShowDListAListCount) {
			CheckMenuItem( hMenu, ID_DEBUGGER_SHOWDLISTALISTCOUNT, MF_BYCOMMAND | MFS_CHECKED );
		}
		if (ShowCompMem) {
			CheckMenuItem( hMenu, ID_DEBUGGER_SHOWCOMPMEM, MF_BYCOMMAND | MFS_CHECKED );
		}
		if (ShowPifRamErrors) {
			CheckMenuItem( hMenu, ID_DEBUGGER_SHOWPIFRAMERRORS, MF_BYCOMMAND | MFS_CHECKED );
		}
		if (IndvidualBlock) {
			CheckMenuItem( hMenu, ID_OPTIONS_PROFILING_LOGINDIVIDUALBLOCKS, MF_BYCOMMAND | MFS_CHECKED );
		}

		if (Profiling) {
			CheckMenuItem( hMenu, ID_OPTIONS_PROFILING_ON, MF_BYCOMMAND | MFS_CHECKED );
		} else {
			CheckMenuItem( hMenu, ID_OPTIONS_PROFILING_OFF, MF_BYCOMMAND | MFS_CHECKED );
		}
	}
#endif

#ifdef BETA_VERSION
	{
		MENUITEMINFO menuinfo;
		char String[256];

		hSubMenu = GetSubMenu(hMenu,4);
		memset(&menuinfo, 0, sizeof(MENUITEMINFO));
		menuinfo.cbSize = sizeof(MENUITEMINFO);
		menuinfo.fMask = MIIM_TYPE;
		menuinfo.fType = MFT_STRING;
		menuinfo.fState = MFS_ENABLED;
		menuinfo.dwTypeData = String;
		menuinfo.cch = sizeof(String);
		strcpy(String,"Beta For: ...");
		InsertMenuItem(hSubMenu, 0, TRUE, &menuinfo);
		strcpy(String,"Email: ...");
		InsertMenuItem(hSubMenu, 1, TRUE, &menuinfo);
	}
#endif

	//Plugins
	EnableMenuItem(hMenu, ID_OPTIONS_CONFIG_AUDIO, MF_BYCOMMAND | (AiDllConfig == NULL?MF_GRAYED:MF_ENABLED));
	EnableMenuItem(hMenu, ID_OPTIONS_CONFIG_GFX, MF_BYCOMMAND | (GFXDllConfig == NULL?MF_GRAYED:MF_ENABLED));
	EnableMenuItem(hMenu, ID_OPTIONS_CONFIG_RSP, MF_BYCOMMAND | (RSPDllConfig == NULL?MF_GRAYED:MF_ENABLED));
	EnableMenuItem(hMenu, ID_OPTIONS_CONFIG_CONTROL, MF_BYCOMMAND | (ContConfig == NULL?MF_GRAYED:MF_ENABLED));

	if (HaveDebugger) {
		MENUITEMINFO lpmii;
		hSubMenu = GetSubMenu(hMenu,3);
		if (IsMenu(RspDebug.hRSPMenu)) {
			InsertMenu (hSubMenu, 3, MF_POPUP|MF_BYPOSITION, (DWORD)RspDebug.hRSPMenu, "&RSP");
			lpmii.cbSize = sizeof(MENUITEMINFO);
			lpmii.fMask = MIIM_STATE;
			lpmii.fState = 0;
			SetMenuItemInfo(hSubMenu, (DWORD)RspDebug.hRSPMenu, MF_BYCOMMAND,&lpmii);
		}
		if (IsMenu(GFXDebug.hGFXMenu)) {
			InsertMenu (hSubMenu, 3, MF_POPUP|MF_BYPOSITION, (DWORD)GFXDebug.hGFXMenu, "&RDP");
			lpmii.cbSize = sizeof(MENUITEMINFO);
			lpmii.fMask = MIIM_STATE;
			lpmii.fState = 0;
			SetMenuItemInfo(hSubMenu, (DWORD)GFXDebug.hGFXMenu, MF_BYCOMMAND,&lpmii);
		}
	}

	if (strlen(RomName) > 0) {
		EnableMenuItem(hMenu,ID_FILE_ROM_INFO,MFS_ENABLED|MF_BYCOMMAND);
		EnableMenuItem(hMenu, ID_FILE_GAME_INFO, MFS_ENABLED | MF_BYCOMMAND);
		EnableMenuItem(hMenu,ID_FILE_STARTEMULATION,MFS_ENABLED|MF_BYCOMMAND);
		EnableMenuItem(hMenu,ID_SYSTEM_GSBUTTON,MFS_ENABLED|MF_BYCOMMAND);						//added by Witten on 10/03/2002
		if (HaveDebugger) {
			EnableMenuItem(hMenu,ID_DEBUGGER_MEMORY,MFS_ENABLED|MF_BYCOMMAND);
			EnableMenuItem(hMenu,ID_DEBUGGER_TLBENTRIES,MFS_ENABLED|MF_BYCOMMAND);
			EnableMenuItem(hMenu,ID_DEBUGGER_R4300IREGISTERS,MFS_ENABLED|MF_BYCOMMAND);
			if (IsMenu(RspDebug.hRSPMenu)) {
				EnableMenuItem(hMenu,(DWORD)RspDebug.hRSPMenu,MFS_ENABLED|MF_BYCOMMAND);
			}
			if (IsMenu(GFXDebug.hGFXMenu)) {
				EnableMenuItem(hMenu,(DWORD)GFXDebug.hGFXMenu,MFS_ENABLED|MF_BYCOMMAND);
			}
		}
	}

	//Enable if cpu is running
	State = CPURunning?MFS_ENABLED:MFS_DISABLED;
	EnableMenuItem(hMenu,ID_FILE_ENDEMULATION,State|MF_BYCOMMAND);
	EnableMenuItem(hMenu,ID_OPTIONS_FULLSCREEN,State|MF_BYCOMMAND);
	EnableMenuItem(hMenu,ID_CPU_RESET,State|MF_BYCOMMAND);
	EnableMenuItem(hMenu,ID_CPU_PAUSE,State|MF_BYCOMMAND);
	EnableMenuItem(hMenu,ID_OPTIONS_ALWAYSONTOP,State|MF_BYCOMMAND);
	EnableMenuItem(hMenu,ID_OPTIONS_CHEATS,State|MF_BYCOMMAND);
	//Cheat search crashes on first scan so disabling for now (Gent)
	//EnableMenuItem(hMenu,ID_SYSTEM_CHEATSEARCH,State|MF_BYCOMMAND);
	if (CPURunning && CaptureScreen != NULL) {
		EnableMenuItem(hMenu,ID_SYSTEM_GENERATEBITMAP,MFS_ENABLED|MF_BYCOMMAND);
	} else {
		EnableMenuItem(hMenu,ID_SYSTEM_GENERATEBITMAP,MFS_DISABLED|MF_BYCOMMAND);
	}
	EnableMenuItem(hMenu,ID_CPU_SAVE,State|MF_BYCOMMAND);
	EnableMenuItem(hMenu,ID_CPU_SAVEAS,State|MF_BYCOMMAND);
	EnableMenuItem(hMenu,ID_CPU_RESTORE,State|MF_BYCOMMAND);
	EnableMenuItem(hMenu,ID_CPU_LOAD,State|MF_BYCOMMAND);

	hSubMenu = GetSubMenu(hMenu,1); 	//System
	EnableMenuItem(hSubMenu,11,State|MF_BYPOSITION);  //Save State

	//Disable if cpu is running
	State = CPURunning?MFS_DISABLED:MFS_ENABLED;
	EnableMenuItem(hMenu,ID_FILE_ROMDIRECTORY,State|MF_BYCOMMAND);
	EnableMenuItem(hMenu,ID_FILE_REFRESHROMLIST,State|MF_BYCOMMAND);
	if (State == MFS_DISABLED) { EnableMenuItem(hMenu,ID_FILE_STARTEMULATION,State|MF_BYCOMMAND); }

	/* Debugger Menu */
	if (!HaveDebugger) { DeleteMenu(hMenu,3,MF_BYPOSITION);  }
	DeleteAdvanceMenuOptions(hMenu);
	if ((BasicMode && CPURunning) || !RomListVisible()) {
		DeleteMenu(GetSubMenu(hMenu,0),8,MF_BYPOSITION);  //Line
		DeleteMenu(hMenu,ID_FILE_ROMDIRECTORY,MF_BYCOMMAND);
		DeleteMenu(hMenu,ID_FILE_REFRESHROMLIST,MF_BYCOMMAND);
	}
	SetMenu(hWnd, hMenu);
	DrawMenuBar(hWnd);
	hMainMenu = hMenu;
}

void SetCurrentSaveState (HWND hWnd, int State) {
	char String[256];
	HMENU hMenu;

	hMenu = GetMenu(hWnd);
	if (!CPURunning)
		State = ID_CURRENTSAVE_DEFAULT;

	CheckMenuItem(hMenu, ID_CURRENTSAVE_DEFAULT, MF_BYCOMMAND | MFS_UNCHECKED);
	
	// Removed this as we now have 0 to 9 - Gent
	//CheckMenuItem(hMenu, ID_CURRENTSAVE_0, MF_BYCOMMAND | MFS_UNCHECKED);
	
	CheckMenuItem(hMenu, ID_CURRENTSAVE_1, MF_BYCOMMAND | MFS_UNCHECKED);
	
	CheckMenuItem(hMenu, ID_CURRENTSAVE_2, MF_BYCOMMAND | MFS_UNCHECKED);
	
	CheckMenuItem(hMenu, ID_CURRENTSAVE_3, MF_BYCOMMAND | MFS_UNCHECKED);
	
	CheckMenuItem(hMenu, ID_CURRENTSAVE_4, MF_BYCOMMAND | MFS_UNCHECKED);
	
	CheckMenuItem(hMenu, ID_CURRENTSAVE_5, MF_BYCOMMAND | MFS_UNCHECKED);
	
	CheckMenuItem(hMenu, ID_CURRENTSAVE_6, MF_BYCOMMAND | MFS_UNCHECKED);
	
	CheckMenuItem(hMenu, ID_CURRENTSAVE_7, MF_BYCOMMAND | MFS_UNCHECKED);
	
	CheckMenuItem(hMenu, ID_CURRENTSAVE_8, MF_BYCOMMAND | MFS_UNCHECKED);
	
	CheckMenuItem(hMenu, ID_CURRENTSAVE_9, MF_BYCOMMAND | MFS_UNCHECKED);
	
	CheckMenuItem(hMenu, ID_CURRENTSAVE_10, MF_BYCOMMAND | MFS_UNCHECKED);
	
	CheckMenuItem(hMenu, ID_CURRENTSAVE_11, MF_BYCOMMAND | MFS_UNCHECKED);
	
	CheckMenuItem(hMenu, ID_CURRENTSAVE_12, MF_BYCOMMAND | MFS_UNCHECKED);
	
	CheckMenuItem(hMenu, ID_CURRENTSAVE_13, MF_BYCOMMAND | MFS_UNCHECKED);
	
	CheckMenuItem(hMenu, ID_CURRENTSAVE_14, MF_BYCOMMAND | MFS_UNCHECKED);
	
	CheckMenuItem(hMenu, ID_CURRENTSAVE_15, MF_BYCOMMAND | MFS_UNCHECKED);
	
	CheckMenuItem(hMenu, ID_CURRENTSAVE_16, MF_BYCOMMAND | MFS_UNCHECKED);
	
	CheckMenuItem(hMenu, ID_CURRENTSAVE_17, MF_BYCOMMAND | MFS_UNCHECKED);
	
	CheckMenuItem(hMenu, ID_CURRENTSAVE_18, MF_BYCOMMAND | MFS_UNCHECKED);
	
	CheckMenuItem(hMenu, ID_CURRENTSAVE_19, MF_BYCOMMAND | MFS_UNCHECKED);
	
	CheckMenuItem(hMenu, State, MF_BYCOMMAND | MFS_CHECKED);
	if (strlen(RomName) == 0) { return; }

	strcpy(CurrentSave,RomName);

	switch (State) {
	case ID_CURRENTSAVE_DEFAULT:
		strcat(CurrentSave, ".pj0");
		break;

		// Removed this as we now have 0 to 9 - Gent

	/*case ID_CURRENTSAVE_0:
		strcat(CurrentSave, ".pj0");
		break;*/

	case ID_CURRENTSAVE_1:
		strcat(CurrentSave, ".pj1");
		break;

	case ID_CURRENTSAVE_2:
		strcat(CurrentSave, ".pj2");
		break;

	case ID_CURRENTSAVE_3:
		strcat(CurrentSave, ".pj3");
		break;

	case ID_CURRENTSAVE_4:
		strcat(CurrentSave, ".pj4");
		break;

	case ID_CURRENTSAVE_5:
		strcat(CurrentSave, ".pj5");
		break;

	case ID_CURRENTSAVE_6:
		strcat(CurrentSave, ".pj6");
		break;

	case ID_CURRENTSAVE_7:
		strcat(CurrentSave, ".pj7");
		break;

	case ID_CURRENTSAVE_8:
		strcat(CurrentSave, ".pj8");
		break;

	case ID_CURRENTSAVE_9:
		strcat(CurrentSave, ".pj9");
		break;

	case ID_CURRENTSAVE_10:
		strcat(CurrentSave, ".pj10");
		break;

	case ID_CURRENTSAVE_11:
		strcat(CurrentSave, ".pj11");
		break;

	case ID_CURRENTSAVE_12:
		strcat(CurrentSave, ".pj12");
		break;

	case ID_CURRENTSAVE_13:
		strcat(CurrentSave, ".pj13");
		break;

	case ID_CURRENTSAVE_14:
		strcat(CurrentSave, ".pj14");
		break;

	case ID_CURRENTSAVE_15:
		strcat(CurrentSave, ".pj15");
		break;

	case ID_CURRENTSAVE_16:
		strcat(CurrentSave, ".pj16");
		break;

	case ID_CURRENTSAVE_17:
		strcat(CurrentSave, ".pj17");
		break;

	case ID_CURRENTSAVE_18:
		strcat(CurrentSave, ".pj18");
		break;

	case ID_CURRENTSAVE_19:
		strcat(CurrentSave, ".pj19");
		break;
	}

	sprintf(String,"%s: %s",GS(MSG_SAVE_SLOT),CurrentSave);
	SendMessage( hStatusWnd, SB_SETTEXT, 0, (LPARAM)String );
	CurrentSaveSlot = State;
}

void UninstallApplication(HWND hWnd) {
	char path_buffer[_MAX_PATH], drive[_MAX_DRIVE], dir[_MAX_DIR];
	char fname[_MAX_FNAME], ext[_MAX_EXT];
	char FileName[_MAX_PATH];
	char RegistryKey[300];
	char ErrorMessage[300];

	if (MessageBox(NULL, GS(MSG_CONFIRMATION_UNINSTALL), AppName, MB_OKCANCEL | MB_ICONEXCLAMATION | MB_SETFOREGROUND) ==  IDOK) {
		// Delete cache file
		GetModuleFileName(NULL, path_buffer, sizeof(path_buffer));
		_splitpath(path_buffer, drive, dir, fname, ext);
		sprintf(FileName, "%s%sConfig\\%s", drive, dir, CacheFileName);
		if (remove(FileName) != 0) {
			sprintf(ErrorMessage, "%s: %s", GS(MSG_DELETE_FILE_FAILED), FileName);
			DisplayError(ErrorMessage);
		}

		// Delete registry keys recursive
		sprintf(RegistryKey, "Software\\N64 Emulation\\%s", AppName);
		if (!RegDelnode(HKEY_CURRENT_USER, RegistryKey)) {
			DisplayError(GS(MSG_DELETE_SETTINGS_FAILED));
		}

		MessageBox(NULL, GS(MSG_RESTART_APPLICATION), AppName, MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);

		ForceClose = TRUE;
		DestroyWindow(hWnd);
	}

}


void ShutdownApplication ( void ) {
	CloseCpu();
	if (TargetInfo != NULL) { VirtualFree(TargetInfo,0,MEM_RELEASE); }
	FreeRomBrowser();
	ShutdownPlugins();
	if (!ForceClose)
		SaveRecentFiles();
	Release_Memory();
	ResetTimerList();
#if (!defined(EXTERNAL_RELEASE))
	StopLog();
#endif
	CloseHandle(hPauseMutex);
	CoUninitialize();
}

void UninstallJabo(HWND hWnd) {
	char path_buffer[_MAX_PATH], drive[_MAX_DRIVE], dir[_MAX_DIR];
	char fname[_MAX_FNAME], ext[_MAX_EXT];
	char FileName[_MAX_PATH];
	char RegistryKey[300];
	char ErrorMessage[300];

	if (MessageBox(NULL, GS(MSG_CONFIRMATION_UNINSTALL), JaboPlugins, MB_OKCANCEL | MB_ICONEXCLAMATION | MB_SETFOREGROUND) == IDOK) {

		// Delete registry keys recursive
		sprintf(RegistryKey, "Software\\JaboSoft\\%s", JaboPlugins);
		if (!RegDelnode(HKEY_CURRENT_USER, RegistryKey)) {
			DisplayError(GS(MSG_DELETE_SETTINGS_FAILED));
		}

		MessageBox(NULL, GS(MSG_JABO_REMOVE), JaboPlugins, MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);
	}

}


void StoreCurrentWinPos (  char * WinName, HWND hWnd ) {
	long lResult;
	HKEY hKeyResults = 0;
	DWORD Disposition = 0;
	RECT WinRect;
	char String[200];

	GetWindowRect(hWnd, &WinRect );
	sprintf(String,"Software\\N64 Emulation\\%s\\Page Setup",AppName);
	lResult = RegCreateKeyEx( HKEY_CURRENT_USER, String,0,"", REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
	if (lResult == ERROR_SUCCESS) {
		sprintf(String,"%s Top",WinName);
		RegSetValueEx(hKeyResults,String,0, REG_DWORD,(CONST BYTE *)(&WinRect.top),
			sizeof(DWORD));
		sprintf(String,"%s Left",WinName);
		RegSetValueEx(hKeyResults,String,0, REG_DWORD,(CONST BYTE *)(&WinRect.left),
			sizeof(DWORD));
	}
	RegCloseKey(hKeyResults);
}

void StoreCurrentWinSize (  char * WinName, HWND hWnd ) {
	long lResult;
	HKEY hKeyResults = 0;
	DWORD Disposition = 0;
	RECT WinRect;
	char String[200];

	GetWindowRect(hWnd, &WinRect );
	WinRect.bottom -= WinRect.top;
	WinRect.right -= WinRect.left;
	sprintf(String,"Software\\N64 Emulation\\%s\\Page Setup",AppName);
	lResult = RegCreateKeyEx( HKEY_CURRENT_USER, String,0,"", REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
	if (lResult == ERROR_SUCCESS) {
		sprintf(String,"%s Height",WinName);
		RegSetValueEx(hKeyResults,String,0, REG_DWORD,(CONST BYTE *)(&WinRect.bottom),
			sizeof(DWORD));
		sprintf(String,"%s Width",WinName);
		RegSetValueEx(hKeyResults,String,0, REG_DWORD,(CONST BYTE *)(&WinRect.right),
			sizeof(DWORD));
	}
	RegCloseKey(hKeyResults);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszArgs, int nWinMode) {
#define WindowWidth  640
#define WindowHeight 480
	HACCEL AccelWinMode, AccelCPURunning, AccelRomBrowser;
	DWORD X, Y;
	MSG msg;

	//DisplayError("%X",r4300i_COP1_MF);
	if ( !InitalizeApplication (hInstance) ) { return FALSE; }
	if ( !RegisterWinClass () ) { return FALSE; }
	if ( !GetStoredWinPos( "Main", &X, &Y ) ) {
  		X = (GetSystemMetrics( SM_CXSCREEN ) - WindowWidth) / 2;
		Y = (GetSystemMetrics( SM_CYSCREEN ) - WindowHeight) / 2;
	}

	AccelWinMode    = LoadAccelerators(hInst,MAKEINTRESOURCE(IDR_WINDOWMODE));
	AccelCPURunning = LoadAccelerators(hInst,MAKEINTRESOURCE(IDR_CPURUNNING));
	AccelRomBrowser = LoadAccelerators(hInst,MAKEINTRESOURCE(IDR_ROMBROWSER));

	hHiddenWin = CreateWindow( AppName, AppName, WS_OVERLAPPED | WS_CLIPCHILDREN |
		WS_CLIPSIBLINGS | WS_SYSMENU | WS_MINIMIZEBOX,X,Y,WindowWidth,WindowHeight,
		NULL,NULL,hInst,NULL
	);

	if ( !hHiddenWin ) { return FALSE; }

	hMainWindow = CreateWindow( AppName, AppName, WS_OVERLAPPED | WS_CLIPCHILDREN |
		WS_CLIPSIBLINGS | WS_SYSMENU | WS_MINIMIZEBOX,X,Y,WindowWidth,WindowHeight,
		NULL,NULL,hInst,NULL
	);

	if ( !hMainWindow ) { return FALSE; }
	if (strlen(lpszArgs) > 0) {
		DWORD ThreadID;

		SetupPlugins(hMainWindow);
		SetupMenu(hMainWindow);
		ShowWindow(hMainWindow, nWinMode);
		strcpy(CurrentFileName,lpszArgs);
		CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)OpenChosenFile,NULL,0, &ThreadID);
	} else {
		if (RomBrowser) {
			ShowRomList(hMainWindow);
		} else {
			SetupPlugins(hMainWindow);
			SetupMenu(hMainWindow);
			ShowWindow(hMainWindow, nWinMode);
		}
	}

	while (GetMessage(&msg,NULL,0,0)) {
		if (!CPURunning && TranslateAccelerator(hMainWindow,AccelRomBrowser,&msg)) { continue; }
		if (CPURunning) {
			if (TranslateAccelerator(hMainWindow,AccelCPURunning,&msg)) { continue; }
			if (!inFullScreen && TranslateAccelerator(hMainWindow,AccelWinMode,&msg)) { continue; }
		}
		if (IsDialogMessage( hManageWindow,&msg)) { continue; }
        if(!IsDialogMessage(hCheatSearchDlg, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
	}
	ShutdownApplication ();
	return msg.wParam;
}
