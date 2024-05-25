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
#include <shlwapi.h>
#include <shlobj.h>
#include <stdio.h>
#include "main.h"
#include "cpu.h"
#include "plugin.h"
#include "debugger.h"
#include "resource.h"

BOOL CALLBACK DefaultOptionsProc   ( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK GeneralOptionsProc   ( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK DirSelectProc        ( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK PluginSelectProc     ( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK RomBrowserProc       ( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK RomSettingsProc      ( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK RomNotesProc         ( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK ShellIntegrationProc ( HWND, UINT, WPARAM, LPARAM );

typedef struct {
	int     LanguageID;
	WORD    TemplateID;
	DLGPROC pfnDlgProc;
} SETTINGS_TAB;

SETTINGS_TAB SettingsTabs[] = {
	{ TAB_PLUGIN,          IDD_Settings_PlugSel,   PluginSelectProc     },
	{ TAB_DIRECTORY,       IDD_Settings_Directory, DirSelectProc        },
	{ TAB_OPTIONS,         IDD_Settings_General,   GeneralOptionsProc   },
	{ TAB_ROMSELECTION,    IDD_Settings_RomBrowser,RomBrowserProc       },
	{ TAB_ADVANCED,        IDD_Settings_Options,   DefaultOptionsProc   },
	{ TAB_ROMSETTINGS,     IDD_Settings_Rom,       RomSettingsProc      },
	{ TAB_ROMNOTES,        IDD_Settings_RomNotes,  RomNotesProc         },
	{ TAB_SHELLINTERGATION,IDD_Settings_ShellInt,  ShellIntegrationProc }
};

SETTINGS_TAB SettingsTabsBasic[] = {
	{ TAB_PLUGIN, IDD_Settings_PlugSel,   PluginSelectProc     },
	{ TAB_OPTIONS,IDD_Settings_General,   GeneralOptionsProc   },
};

SETTINGS_TAB SettingsTabsRom[] = {
	{ TAB_ROMSETTINGS, IDD_Settings_Rom,      RomSettingsProc  },
	{ TAB_ROMNOTES,    IDD_Settings_RomNotes, RomNotesProc     },
};

void ChangeRomSettings(HWND hwndOwner) {
	char OrigRomName[sizeof(RomName)], OrigFileName[sizeof(CurrentFileName)];
    PROPSHEETPAGE psp[sizeof(SettingsTabsRom) / sizeof(SETTINGS_TAB)];
	BYTE OrigByteHeader[sizeof(RomHeader)];
    PROPSHEETHEADER psh;
	DWORD OrigFileSize, count;

	strncpy(OrigRomName,RomName,sizeof(OrigRomName));	
	strncpy(OrigFileName,CurrentFileName,sizeof(OrigFileName));	
	memcpy(OrigByteHeader,RomHeader,sizeof(RomHeader));
	strncpy(CurrentFileName,CurrentRBFileName,sizeof(CurrentFileName));	
	OrigFileSize = RomFileSize;
	LoadRomHeader();
	
	for (count = 0; count < (sizeof(SettingsTabsRom) / sizeof(SETTINGS_TAB)); count ++) {
		psp[count].dwSize = sizeof(PROPSHEETPAGE);
		psp[count].dwFlags = PSP_USETITLE;
		psp[count].hInstance = hInst;
		psp[count].pszTemplate = MAKEINTRESOURCE(SettingsTabsRom[count].TemplateID);
		psp[count].pfnDlgProc = SettingsTabsRom[count].pfnDlgProc;
		psp[count].pszTitle = GS(SettingsTabsRom[count].LanguageID);
		psp[count].lParam = 0;
		psp[count].pfnCallback = NULL;
	}

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    psh.hwndParent = hwndOwner;
    psh.hInstance = hInst;
    psh.pszCaption = (LPSTR)GS(OPTIONS_TITLE);
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;
    psh.pfnCallback = NULL;

	PropertySheet(&psh);

	strncpy(RomName,OrigRomName,sizeof(RomName));
	strncpy(CurrentFileName,OrigFileName,sizeof(CurrentFileName));
	memcpy(RomHeader,OrigByteHeader,sizeof(RomHeader));
	RomFileSize = OrigFileSize;
}

void ChangeSettings(HWND hwndOwner) {
    PROPSHEETPAGE psp[sizeof(SettingsTabs) / sizeof(SETTINGS_TAB)];
    PROPSHEETPAGE BasicPsp[sizeof(SettingsTabsBasic) / sizeof(SETTINGS_TAB)];
    PROPSHEETHEADER psh;
	int count;

	for (count = 0; count < (sizeof(SettingsTabs) / sizeof(SETTINGS_TAB)); count ++) {
		psp[count].dwSize = sizeof(PROPSHEETPAGE);
		psp[count].dwFlags = PSP_USETITLE;
		psp[count].hInstance = hInst;
		psp[count].pszTemplate = MAKEINTRESOURCE(SettingsTabs[count].TemplateID);
		psp[count].pfnDlgProc = SettingsTabs[count].pfnDlgProc;
		psp[count].pszTitle = GS(SettingsTabs[count].LanguageID);
		psp[count].lParam = 0;
		psp[count].pfnCallback = NULL;
	}

	for (count = 0; count < (sizeof(SettingsTabsBasic) / sizeof(SETTINGS_TAB)); count ++) {
		BasicPsp[count].dwSize = sizeof(PROPSHEETPAGE);
		BasicPsp[count].dwFlags = PSP_USETITLE;
		BasicPsp[count].hInstance = hInst;
		BasicPsp[count].pszTemplate = MAKEINTRESOURCE(SettingsTabsBasic[count].TemplateID);
		BasicPsp[count].pfnDlgProc = SettingsTabsBasic[count].pfnDlgProc;
		BasicPsp[count].pszTitle = GS(SettingsTabsBasic[count].LanguageID);
		BasicPsp[count].lParam = 0;
		BasicPsp[count].pfnCallback = NULL;
	}

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    psh.hwndParent = hwndOwner;
    psh.hInstance = hInst;
    psh.pszCaption = (LPSTR)GS(OPTIONS_TITLE);
    psh.nPages = (BasicMode?sizeof(BasicPsp):sizeof(psp)) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = BasicMode?(LPCPROPSHEETPAGE) &BasicPsp:(LPCPROPSHEETPAGE) &psp;
    psh.pfnCallback = NULL;

	PropertySheet(&psh);
	LoadSettings();
	SetupMenu(hMainWindow);
	if (!AutoSleep && !ManualPaused && (CPU_Paused || CPU_Action.Pause)) { PauseCpu(); }
	return;
}

void SetFlagControl (HWND hDlg, BOOL * Flag, WORD CtrlID, int StringID) {
	SetDlgItemText(hDlg,CtrlID,GS(StringID));	
	if (*Flag) { SendMessage(GetDlgItem(hDlg,CtrlID),BM_SETCHECK, BST_CHECKED,0); }
}

BOOL CALLBACK GeneralOptionsProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		SetFlagControl(hDlg, &AutoSleep,      IDC_AUTOSLEEP,       OPTION_AUTO_SLEEP);
		SetFlagControl(hDlg, &AutoFullScreen, IDC_LOAD_FULLSCREEN, OPTION_AUTO_FULLSCREEN);
		SetFlagControl(hDlg, &BasicMode,      IDC_BASIC_MODE,      OPTION_BASIC_MODE);
		SetFlagControl(hDlg, &RememberCheats, IDC_REMEMBER_CHEAT,  OPTION_REMEMBER_CHEAT);
		break;	
	case WM_NOTIFY:
		if (((NMHDR FAR *) lParam)->code == PSN_APPLY) {
			long lResult;
			HKEY hKeyResults = 0;
			DWORD Disposition = 0;
			char String[200];
		
			sprintf(String,"Software\\N64 Emulation\\%s",AppName);
			lResult = RegCreateKeyEx( HKEY_CURRENT_USER, String,0,"", REG_OPTION_NON_VOLATILE,
				KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
			if (lResult == ERROR_SUCCESS) {
				AutoFullScreen = SendMessage(GetDlgItem(hDlg,IDC_LOAD_FULLSCREEN),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;
				RegSetValueEx(hKeyResults,"On open rom go full screen",0,REG_DWORD,(BYTE *)&AutoFullScreen,sizeof(DWORD));

				BasicMode = SendMessage(GetDlgItem(hDlg,IDC_BASIC_MODE),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;
				RegSetValueEx(hKeyResults,"Basic Mode",0,REG_DWORD,(BYTE *)&BasicMode,sizeof(DWORD));

				RememberCheats = SendMessage(GetDlgItem(hDlg,IDC_REMEMBER_CHEAT),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;
				RegSetValueEx(hKeyResults,"Remember Cheats",0,REG_DWORD,(BYTE *)&RememberCheats,sizeof(DWORD));

				AutoSleep = SendMessage(GetDlgItem(hDlg,IDC_AUTOSLEEP),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;
				RegSetValueEx(hKeyResults,"Pause emulation when window is not active",0,REG_DWORD,(BYTE *)&AutoSleep,sizeof(DWORD));				
			}
			RegCloseKey(hKeyResults);
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void AddDropDownItem (HWND hDlg, WORD CtrlID, int StringID, int ItemData, int * Variable) {
	HWND hCtrl = GetDlgItem(hDlg,CtrlID);
	int indx;

	indx = SendMessage(hCtrl,CB_ADDSTRING,0,(LPARAM)GS(StringID));
	SendMessage(hCtrl,CB_SETITEMDATA,indx,ItemData);
	if (*Variable == ItemData) { SendMessage(hCtrl,CB_SETCURSEL,indx,0); }
	if (SendMessage(hCtrl,CB_GETCOUNT,0,0) == 0) { SendMessage(hCtrl,CB_SETCURSEL,0,0); }
}

BOOL CALLBACK DefaultOptionsProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	int indx;

	switch (uMsg) {
	case WM_INITDIALOG:		
		SetDlgItemText(hDlg,IDC_INFO,GS(ADVANCE_INFO));	
		SetDlgItemText(hDlg,IDC_CORE_DEFAULTS,GS(ADVANCE_DEFAULTS));	
		SetDlgItemText(hDlg,IDC_TEXT2,GS(ADVANCE_CPU_STYLE));	
		SetDlgItemText(hDlg,IDC_TEXT3,GS(ADVANCE_SMCM));	
		SetDlgItemText(hDlg,IDC_TEXT4,GS(ADVANCE_MEM_SIZE));	
		SetDlgItemText(hDlg,IDC_TEXT5,GS(ADVANCE_ABL));	
		SetFlagControl(hDlg,&AutoStart, IDC_START_ON_ROM_OPEN, ADVANCE_AUTO_START);
		SetFlagControl(hDlg,&UseIni, IDC_USEINI, ADVANCE_OVERWRITE);
		SetFlagControl(hDlg,&AutoZip, IDC_ZIP, ADVANCE_COMPRESS);

		AddDropDownItem(hDlg,IDC_CPU_TYPE,CORE_INTERPTER,CPU_Interpreter,&SystemCPU_Type);
		AddDropDownItem(hDlg,IDC_CPU_TYPE,CORE_RECOMPILER,CPU_Recompiler,&SystemCPU_Type);
		if (HaveDebugger) { AddDropDownItem(hDlg,IDC_CPU_TYPE,CORE_SYNC,CPU_SyncCores,&SystemCPU_Type); }

		AddDropDownItem(hDlg,IDC_SELFMOD,SMCM_NONE,ModCode_None,&SystemSelfModCheck);
		AddDropDownItem(hDlg,IDC_SELFMOD,SMCM_CACHE,ModCode_Cache,&SystemSelfModCheck);
		AddDropDownItem(hDlg,IDC_SELFMOD,SMCM_PROECTED,ModCode_ProtectedMemory,&SystemSelfModCheck);
		AddDropDownItem(hDlg,IDC_SELFMOD,SMCM_CHECK_MEM,ModCode_CheckMemoryCache,&SystemSelfModCheck);
		AddDropDownItem(hDlg,IDC_SELFMOD,SMCM_CHANGE_MEM,ModCode_ChangeMemory,&SystemSelfModCheck);
		AddDropDownItem(hDlg,IDC_SELFMOD,SMCM_CHECK_ADV,ModCode_CheckMemory2,&SystemSelfModCheck);

		AddDropDownItem(hDlg,IDC_RDRAM_SIZE,RDRAM_4MB,0x400000,&SystemRdramSize);
		AddDropDownItem(hDlg,IDC_RDRAM_SIZE,RDRAM_8MB,0x800000,&SystemRdramSize);

		AddDropDownItem(hDlg,IDC_ABL,ABL_ON,TRUE,&SystemABL);
		AddDropDownItem(hDlg,IDC_ABL,ABL_OFF,FALSE,&SystemABL);
		break;
	case WM_NOTIFY:
		if (((NMHDR FAR *) lParam)->code == PSN_APPLY) {
			long lResult;
			HKEY hKeyResults = 0;
			DWORD Disposition = 0;
			char String[200];
		
			sprintf(String,"Software\\N64 Emulation\\%s",AppName);
			lResult = RegCreateKeyEx( HKEY_CURRENT_USER, String,0,"", REG_OPTION_NON_VOLATILE,
				KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
			if (lResult == ERROR_SUCCESS) {
				AutoStart = SendMessage(GetDlgItem(hDlg,IDC_START_ON_ROM_OPEN),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;
				RegSetValueEx(hKeyResults,"Start Emulation when rom is opened",0,REG_DWORD,(BYTE *)&AutoStart,sizeof(DWORD));

				UseIni = SendMessage(GetDlgItem(hDlg,IDC_USEINI),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;
				RegSetValueEx(hKeyResults,"Always overwrite default settings with ones from ini?",0,REG_DWORD,(BYTE *)&UseIni,sizeof(DWORD));

				AutoZip = SendMessage(GetDlgItem(hDlg,IDC_ZIP),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;
				RegSetValueEx(hKeyResults,"Automatically compress instant saves",0,REG_DWORD,(BYTE *)&AutoZip,sizeof(DWORD));

				indx = SendMessage(GetDlgItem(hDlg,IDC_CPU_TYPE),CB_GETCURSEL,0,0); 
				SystemCPU_Type = SendMessage(GetDlgItem(hDlg,IDC_CPU_TYPE),CB_GETITEMDATA,indx,0);
				RegSetValueEx(hKeyResults,"CPU Type",0,REG_DWORD,(BYTE *)&SystemCPU_Type,sizeof(DWORD));

				indx = SendMessage(GetDlgItem(hDlg,IDC_SELFMOD),CB_GETCURSEL,0,0); 
				SystemSelfModCheck = SendMessage(GetDlgItem(hDlg,IDC_SELFMOD),CB_GETITEMDATA,indx,0);
				RegSetValueEx(hKeyResults,"Self modifying code method",0,REG_DWORD,(BYTE *)&SystemSelfModCheck,sizeof(DWORD));

				indx = SendMessage(GetDlgItem(hDlg,IDC_RDRAM_SIZE),CB_GETCURSEL,0,0); 
				SystemRdramSize = SendMessage(GetDlgItem(hDlg,IDC_RDRAM_SIZE),CB_GETITEMDATA,indx,0);
				RegSetValueEx(hKeyResults,"Default RDRAM Size",0,REG_DWORD,(BYTE *)&SystemRdramSize,sizeof(DWORD));
				
				indx = SendMessage(GetDlgItem(hDlg,IDC_ABL),CB_GETCURSEL,0,0); 
				SystemABL = SendMessage(GetDlgItem(hDlg,IDC_ABL),CB_GETITEMDATA,indx,0);
				RegSetValueEx(hKeyResults,"Advanced Block Linking",0,REG_DWORD,(BYTE *)&SystemABL,sizeof(DWORD));
			}
			RegCloseKey(hKeyResults);
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

int CALLBACK SelectDirCallBack(HWND hwnd,DWORD uMsg,DWORD lp, DWORD lpData) {
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

BOOL CALLBACK DirSelectProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			HKEY hKeyResults = 0;
			char String[256];
			char Directory[255];
			long lResult;
		
			sprintf(String,"Software\\N64 Emulation\\%s",AppName);
			lResult = RegOpenKeyEx( HKEY_CURRENT_USER,String,0, KEY_ALL_ACCESS,&hKeyResults);	
			if (lResult == ERROR_SUCCESS) {
				DWORD Type, Value, Bytes = 4;
	
				lResult = RegQueryValueEx(hKeyResults,"Use Default Plugin Dir",0,&Type,(LPBYTE)(&Value),&Bytes);
				if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { Value = TRUE; }
				SendMessage(GetDlgItem(hDlg,Value?IDC_PLUGIN_DEFAULT:IDC_PLUGIN_OTHER),BM_SETCHECK, BST_CHECKED,0);

				lResult = RegQueryValueEx(hKeyResults,"Use Default Rom Dir",0,&Type,(LPBYTE)(&Value),&Bytes);
				if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { Value = FALSE; }
				SendMessage(GetDlgItem(hDlg,Value?IDC_ROM_DEFAULT:IDC_ROM_OTHER),BM_SETCHECK, BST_CHECKED,0);

				lResult = RegQueryValueEx(hKeyResults,"Use Default Auto Save Dir",0,&Type,(LPBYTE)(&Value),&Bytes);
				if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { Value = TRUE; }
				SendMessage(GetDlgItem(hDlg,Value?IDC_AUTO_DEFAULT:IDC_AUTO_OTHER),BM_SETCHECK, BST_CHECKED,0);

				lResult = RegQueryValueEx(hKeyResults,"Use Default Instant Save Dir",0,&Type,(LPBYTE)(&Value),&Bytes);
				if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { Value = TRUE; }
				SendMessage(GetDlgItem(hDlg,Value?IDC_INSTANT_DEFAULT:IDC_INSTANT_OTHER),BM_SETCHECK, BST_CHECKED,0);

				lResult = RegQueryValueEx(hKeyResults,"Use Default Snap Shot Dir",0,&Type,(LPBYTE)(&Value),&Bytes);
				if (Type != REG_DWORD || lResult != ERROR_SUCCESS) { Value = TRUE; }
				SendMessage(GetDlgItem(hDlg,Value?IDC_SNAP_DEFAULT:IDC_SNAP_OTHER),BM_SETCHECK, BST_CHECKED,0);

				Bytes = sizeof(Directory);
				lResult = RegQueryValueEx(hKeyResults,"Plugin Directory",0,&Type,(LPBYTE)Directory,&Bytes);
				if (lResult != ERROR_SUCCESS) { GetPluginDir(Directory ); }
				SetDlgItemText(hDlg,IDC_PLUGIN_DIR,Directory);

				Bytes = sizeof(Directory);
				lResult = RegQueryValueEx(hKeyResults,"Instant Save Directory",0,&Type,(LPBYTE)Directory,&Bytes);
				if (lResult != ERROR_SUCCESS) { GetInstantSaveDir(Directory); }
				SetDlgItemText(hDlg,IDC_INSTANT_DIR,Directory);

				Bytes = sizeof(Directory);
				lResult = RegQueryValueEx(hKeyResults,"Auto Save Directory",0,&Type,(LPBYTE)Directory,&Bytes);
				if (lResult != ERROR_SUCCESS) { GetAutoSaveDir(Directory); }
				SetDlgItemText(hDlg,IDC_AUTO_DIR,Directory);

				Bytes = sizeof(Directory);
				lResult = RegQueryValueEx(hKeyResults,"Snap Shot Directory",0,&Type,(LPBYTE)Directory,&Bytes);
				if (lResult != ERROR_SUCCESS) { GetSnapShotDir(Directory); }
				SetDlgItemText(hDlg,IDC_SNAP_DIR,Directory);
			} else {
				SendMessage(GetDlgItem(hDlg,IDC_PLUGIN_DEFAULT),BM_SETCHECK, BST_CHECKED,0);
				SendMessage(GetDlgItem(hDlg,IDC_ROM_DEFAULT),BM_SETCHECK, BST_CHECKED,0);
				SendMessage(GetDlgItem(hDlg,IDC_AUTO_DEFAULT),BM_SETCHECK, BST_CHECKED,0);
				SendMessage(GetDlgItem(hDlg,IDC_INSTANT_DEFAULT),BM_SETCHECK, BST_CHECKED,0);
				SendMessage(GetDlgItem(hDlg,IDC_SNAP_DEFAULT),BM_SETCHECK, BST_CHECKED,0);
				GetPluginDir(Directory );
				SetDlgItemText(hDlg,IDC_PLUGIN_DIR,Directory);
				GetInstantSaveDir(Directory);
				SetDlgItemText(hDlg,IDC_INSTANT_DIR,Directory);
				GetAutoSaveDir(Directory);
				SetDlgItemText(hDlg,IDC_AUTO_DIR,Directory);
				GetSnapShotDir(Directory);
				SetDlgItemText(hDlg,IDC_SNAP_DIR,Directory);
			}			
			GetRomDirectory( Directory );
			SetDlgItemText(hDlg,IDC_ROM_DIR,Directory);

			SetDlgItemText(hDlg,IDC_DIR_FRAME1,GS(DIR_PLUGIN));
			SetDlgItemText(hDlg,IDC_DIR_FRAME2,GS(DIR_ROM));
			SetDlgItemText(hDlg,IDC_DIR_FRAME3,GS(DIR_AUTO_SAVE));
			SetDlgItemText(hDlg,IDC_DIR_FRAME4,GS(DIR_INSTANT_SAVE));
			SetDlgItemText(hDlg,IDC_DIR_FRAME5,GS(DIR_SCREEN_SHOT));
			SetDlgItemText(hDlg,IDC_ROM_DEFAULT,GS(DIR_ROM_DEFAULT));
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SELECT_PLUGIN_DIR:
		case IDC_SELECT_ROM_DIR:
		case IDC_SELECT_INSTANT_DIR:
		case IDC_SELECT_AUTO_DIR:
		case IDC_SELECT_SNAP_DIR:
			{
				char Buffer[MAX_PATH], Directory[255], Title[255];
				LPITEMIDLIST pidl;
				BROWSEINFO bi;

				switch (LOWORD(wParam)) {
				case IDC_SELECT_PLUGIN_DIR:
					strcpy(Title,GS(DIR_SELECT_PLUGIN)); 
					GetPluginDir(Directory);
					break;
				case IDC_SELECT_ROM_DIR: 
					GetRomDirectory(Directory);
					strcpy(Title,GS(DIR_SELECT_ROM)); 
					break;
				case IDC_SELECT_AUTO_DIR: 
					GetAutoSaveDir(Directory); 
					strcpy(Title,GS(DIR_SELECT_AUTO));
					break;
				case IDC_SELECT_INSTANT_DIR: 
					GetInstantSaveDir(Directory);
					strcpy(Title,GS(DIR_SELECT_INSTANT)); 
					break;
				case IDC_SELECT_SNAP_DIR: 
					GetSnapShotDir(Directory); 
					strcpy(Title,GS(DIR_SELECT_SCREEN)); 
					break;
				}

				bi.hwndOwner = hDlg;
				bi.pidlRoot = NULL;
				bi.pszDisplayName = Buffer;
				bi.lpszTitle = Title;
				bi.ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
				bi.lpfn = (BFFCALLBACK)SelectDirCallBack;
				bi.lParam = (DWORD)Directory;
				if ((pidl = SHBrowseForFolder(&bi)) != NULL) {
					if (SHGetPathFromIDList(pidl, Directory)) {
						int len = strlen(Directory);

						if (Directory[len - 1] != '\\') { strcat(Directory,"\\"); }
						switch (LOWORD(wParam)) {
						case IDC_SELECT_PLUGIN_DIR: 
							SetDlgItemText(hDlg,IDC_PLUGIN_DIR,Directory);
							SendMessage(GetDlgItem(hDlg,IDC_PLUGIN_DEFAULT),BM_SETCHECK, BST_UNCHECKED,0);
							SendMessage(GetDlgItem(hDlg,IDC_PLUGIN_OTHER),BM_SETCHECK, BST_CHECKED,0);
							break;
						case IDC_SELECT_ROM_DIR: 
							SetDlgItemText(hDlg,IDC_ROM_DIR,Directory);
							SendMessage(GetDlgItem(hDlg,IDC_ROM_DEFAULT),BM_SETCHECK, BST_UNCHECKED,0);
							SendMessage(GetDlgItem(hDlg,IDC_ROM_OTHER),BM_SETCHECK, BST_CHECKED,0);
							break;
						case IDC_SELECT_INSTANT_DIR: 
							SetDlgItemText(hDlg,IDC_INSTANT_DIR,Directory);
							SendMessage(GetDlgItem(hDlg,IDC_INSTANT_DEFAULT),BM_SETCHECK, BST_UNCHECKED,0);
							SendMessage(GetDlgItem(hDlg,IDC_INSTANT_OTHER),BM_SETCHECK, BST_CHECKED,0);
							break;
						case IDC_SELECT_AUTO_DIR: 
							SetDlgItemText(hDlg,IDC_AUTO_DIR,Directory);
							SendMessage(GetDlgItem(hDlg,IDC_AUTO_DEFAULT),BM_SETCHECK, BST_UNCHECKED,0);
							SendMessage(GetDlgItem(hDlg,IDC_AUTO_OTHER),BM_SETCHECK, BST_CHECKED,0);
							break;
						case IDC_SELECT_SNAP_DIR: 
							SetDlgItemText(hDlg,IDC_SNAP_DIR,Directory);
							SendMessage(GetDlgItem(hDlg,IDC_SNAP_DEFAULT),BM_SETCHECK, BST_UNCHECKED,0);
							SendMessage(GetDlgItem(hDlg,IDC_SNAP_OTHER),BM_SETCHECK, BST_CHECKED,0);
							break;
						}
					}
				}
			}
			break;
		}
		break;
	case WM_NOTIFY:
		if (((NMHDR FAR *) lParam)->code == PSN_APPLY) { 
			long lResult;
			HKEY hKeyResults = 0;
			DWORD Disposition = 0;
			char String[200];
		
			sprintf(String,"Software\\N64 Emulation\\%s",AppName);
			lResult = RegCreateKeyEx( HKEY_CURRENT_USER, String,0,"", REG_OPTION_NON_VOLATILE,
				KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
			if (lResult == ERROR_SUCCESS) {
				DWORD Value;
							
				Value = SendMessage(GetDlgItem(hDlg,IDC_PLUGIN_DEFAULT),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;
				RegSetValueEx(hKeyResults,"Use Default Plugin Dir",0,REG_DWORD,(BYTE *)&Value,sizeof(DWORD));
				if (Value == FALSE) {
					GetDlgItemText(hDlg,IDC_PLUGIN_DIR,String,sizeof(String));
					RegSetValueEx(hKeyResults,"Plugin Directory",0,REG_SZ,(CONST BYTE *)String,strlen(String));
				}

				Value = SendMessage(GetDlgItem(hDlg,IDC_ROM_DEFAULT),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;
				RegSetValueEx(hKeyResults,"Use Default Rom Dir",0,REG_DWORD,(BYTE *)&Value,sizeof(DWORD));
				if (Value == FALSE) {
					GetDlgItemText(hDlg,IDC_ROM_DIR,String,sizeof(String));
					RegSetValueEx(hKeyResults,"Rom Directory",0,REG_SZ,(CONST BYTE *)String,strlen(String));
				}

				Value = SendMessage(GetDlgItem(hDlg,IDC_AUTO_DEFAULT),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;
				RegSetValueEx(hKeyResults,"Use Default Auto Save Dir",0,REG_DWORD,(BYTE *)&Value,sizeof(DWORD));
				if (Value == FALSE) {
					GetDlgItemText(hDlg,IDC_AUTO_DIR,String,sizeof(String));
					RegSetValueEx(hKeyResults,"Auto Save Directory",0,REG_SZ,(CONST BYTE *)String,strlen(String));
				}
				
				Value = SendMessage(GetDlgItem(hDlg,IDC_INSTANT_DEFAULT),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;
				RegSetValueEx(hKeyResults,"Use Default Instant Save Dir",0,REG_DWORD,(BYTE *)&Value,sizeof(DWORD));
				if (Value == FALSE) {
					GetDlgItemText(hDlg,IDC_INSTANT_DIR,String,sizeof(String));
					RegSetValueEx(hKeyResults,"Instant Save Directory",0,REG_SZ,(CONST BYTE *)String,strlen(String));
				}
				
				Value = SendMessage(GetDlgItem(hDlg,IDC_SNAP_DEFAULT),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;
				RegSetValueEx(hKeyResults,"Use Default Snap Shot Dir",0,REG_DWORD,(BYTE *)&Value,sizeof(DWORD));
				if (Value == FALSE) {
					GetDlgItemText(hDlg,IDC_SNAP_DIR,String,sizeof(String));
					RegSetValueEx(hKeyResults,"Snap Shot Directory",0,REG_SZ,(CONST BYTE *)String,strlen(String));
				}
			}
			RegCloseKey(hKeyResults);
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL PluginsChanged ( HWND hDlg ) {
	DWORD index;

//#ifndef EXTERNAL_RELEASE
	index = SendMessage(GetDlgItem(hDlg,RSP_LIST),CB_GETCURSEL,0,0);
	index = SendMessage(GetDlgItem(hDlg,RSP_LIST),CB_GETITEMDATA,(WPARAM)index,0);
	if ((int)index >= 0) {
		if(_stricmp(RspDLL,PluginNames[index]) != 0) { return TRUE; }
	}
//#endif

	index = SendMessage(GetDlgItem(hDlg,GFX_LIST),CB_GETCURSEL,0,0);
	index = SendMessage(GetDlgItem(hDlg,GFX_LIST),CB_GETITEMDATA,(WPARAM)index,0);
	if ((int)index >= 0) {
		if(_stricmp(GfxDLL,PluginNames[index]) != 0) { return TRUE; }
	}

	index = SendMessage(GetDlgItem(hDlg,AUDIO_LIST),CB_GETCURSEL,0,0);
	index = SendMessage(GetDlgItem(hDlg,AUDIO_LIST),CB_GETITEMDATA,(WPARAM)index,0);
	if ((int)index >= 0) {
		if(_stricmp(AudioDLL,PluginNames[index]) != 0) { return TRUE; }
	}

	index = SendMessage(GetDlgItem(hDlg,CONT_LIST),CB_GETCURSEL,0,0);
	index = SendMessage(GetDlgItem(hDlg,CONT_LIST),CB_GETITEMDATA,(WPARAM)index,0);
	if ((int)index >= 0) {
		if(_stricmp(ControllerDLL,PluginNames[index]) != 0) { return TRUE; }
	}
	return FALSE;
}

void FreePluginList() {
	unsigned int count;
	for (count = 0; count <	PluginCount; count ++ ) { 
		free(PluginNames[count]); 
	}
	PluginCount = 0;
}

BOOL CALLBACK PluginSelectProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	char Plugin[300];
	HANDLE hLib;
	DWORD index;

	switch (uMsg) {
	case WM_INITDIALOG:
		SetupPluginScreen(hDlg);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {		
//#ifndef EXTERNAL_RELEASE
		case RSP_LIST:
			if (HIWORD(wParam) == CBN_SELCHANGE) {				
				index = SendMessage(GetDlgItem(hDlg,RSP_LIST),CB_GETCURSEL,0,0);				
				if (index == CB_ERR) { break; } // *** Add in Build 53
				index = SendMessage(GetDlgItem(hDlg,RSP_LIST),CB_GETITEMDATA,(WPARAM)index,0);

				GetPluginDir(Plugin);
				strcat(Plugin,PluginNames[index]);
				hLib = LoadLibrary(Plugin);		
				if (hLib == NULL) { DisplayError("%s %s",GS(MSG_FAIL_LOAD_PLUGIN),Plugin); }
				RSPDllAbout = (void (__cdecl *)(HWND))GetProcAddress( hLib, "DllAbout" );
				EnableWindow(GetDlgItem(hDlg,RSP_ABOUT),RSPDllAbout != NULL ? TRUE:FALSE);
			}
			break;
//#endif
		case GFX_LIST:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				index = SendMessage(GetDlgItem(hDlg,GFX_LIST),CB_GETCURSEL,0,0);
				if (index == CB_ERR) { break; } // *** Add in Build 53
				index = SendMessage(GetDlgItem(hDlg,GFX_LIST),CB_GETITEMDATA,(WPARAM)index,0);

				GetPluginDir(Plugin);
				strcat(Plugin,PluginNames[index]);
				hLib = LoadLibrary(Plugin);		
				if (hLib == NULL) { DisplayError("%s %s",GS(MSG_FAIL_LOAD_PLUGIN),Plugin); }
				GFXDllAbout = (void (__cdecl *)(HWND))GetProcAddress( hLib, "DllAbout" );
				EnableWindow(GetDlgItem(hDlg,GFX_ABOUT),GFXDllAbout != NULL ? TRUE:FALSE);
			}
			break;
		case AUDIO_LIST:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				index = SendMessage(GetDlgItem(hDlg,AUDIO_LIST),CB_GETCURSEL,0,0);
				if (index == CB_ERR) { break; } // *** Add in Build 53
				index = SendMessage(GetDlgItem(hDlg,AUDIO_LIST),CB_GETITEMDATA,(WPARAM)index,0);

				GetPluginDir(Plugin);
				strcat(Plugin,PluginNames[index]);
				hLib = LoadLibrary(Plugin);		
				if (hLib == NULL) { DisplayError("%s %s",GS(MSG_FAIL_LOAD_PLUGIN),Plugin); }
				AiDllAbout = (void (__cdecl *)(HWND))GetProcAddress( hLib, "DllAbout" );
				EnableWindow(GetDlgItem(hDlg,GFX_ABOUT),GFXDllAbout != NULL ? TRUE:FALSE);
			}
			break;
		case CONT_LIST:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				index = SendMessage(GetDlgItem(hDlg,CONT_LIST),CB_GETCURSEL,0,0);
				if (index == CB_ERR) { break; } // *** Add in Build 53
				index = SendMessage(GetDlgItem(hDlg,CONT_LIST),CB_GETITEMDATA,(WPARAM)index,0);

				GetPluginDir(Plugin);
				strcat(Plugin,PluginNames[index]);
				hLib = LoadLibrary(Plugin);		
				if (hLib == NULL) { DisplayError("%s %s",GS(MSG_FAIL_LOAD_PLUGIN),Plugin); }
				ContDllAbout = (void (__cdecl *)(HWND))GetProcAddress( hLib, "DllAbout" );
				EnableWindow(GetDlgItem(hDlg,CONT_ABOUT),ContDllAbout != NULL ? TRUE:FALSE);
			}
			break;
//#ifndef EXTERNAL_RELEASE
		case RSP_ABOUT: RSPDllAbout(hDlg); break;
//#endif
		case GFX_ABOUT: GFXDllAbout(hDlg); break;
		case CONT_ABOUT: ContDllAbout(hDlg); break;
		case AUDIO_ABOUT: AiDllAbout(hDlg); break;
		}
		break;
	case WM_NOTIFY:
		if (((NMHDR FAR *) lParam)->code == PSN_APPLY) { 
			long lResult;
			HKEY hKeyResults = 0;
			DWORD Disposition = 0;
			char String[200];

			if (PluginsChanged(hDlg) == FALSE) { FreePluginList(); break; }

			if (CPURunning) { 
				int Response;

				ShowWindow(hDlg,SW_HIDE);
				Response = MessageBox(hMainWindow,GS(MSG_PLUGIN_CHANGE),GS(MSG_PLUGIN_CHANGE_TITLE),MB_YESNO|MB_ICONQUESTION);
				if (Response != IDYES) { FreePluginList(); break; }
			}

			sprintf(String,"Software\\N64 Emulation\\%s\\Dll",AppName);
			lResult = RegCreateKeyEx( HKEY_CURRENT_USER, String,0,"", REG_OPTION_NON_VOLATILE,
				KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
			if (lResult == ERROR_SUCCESS) {
				DWORD index;

//#ifndef EXTERNAL_RELEASE
				index = SendMessage(GetDlgItem(hDlg,RSP_LIST),CB_GETCURSEL,0,0);
				index = SendMessage(GetDlgItem(hDlg,RSP_LIST),CB_GETITEMDATA,(WPARAM)index,0);
				sprintf(String,"%s",PluginNames[index]);
				RegSetValueEx(hKeyResults,"RSP Dll",0,REG_SZ,(CONST BYTE *)String,
					strlen(String));
//#endif
				index = SendMessage(GetDlgItem(hDlg,GFX_LIST),CB_GETCURSEL,0,0);
				index = SendMessage(GetDlgItem(hDlg,GFX_LIST),CB_GETITEMDATA,(WPARAM)index,0);
				sprintf(String,"%s",PluginNames[index]);
				RegSetValueEx(hKeyResults,"Graphics Dll",0,REG_SZ,(CONST BYTE *)String,
					strlen(String));
				index = SendMessage(GetDlgItem(hDlg,AUDIO_LIST),CB_GETCURSEL,0,0);
				index = SendMessage(GetDlgItem(hDlg,AUDIO_LIST),CB_GETITEMDATA,(WPARAM)index,0);
				sprintf(String,"%s",PluginNames[index]);
				RegSetValueEx(hKeyResults,"Audio Dll",0,REG_SZ,(CONST BYTE *)String,
					strlen(String));
				index = SendMessage(GetDlgItem(hDlg,CONT_LIST),CB_GETCURSEL,0,0);
				index = SendMessage(GetDlgItem(hDlg,CONT_LIST),CB_GETITEMDATA,(WPARAM)index,0);
				sprintf(String,"%s",PluginNames[index]);
				RegSetValueEx(hKeyResults,"Controller Dll",0,REG_SZ,(CONST BYTE *)String,
					strlen(String));
			}
			RegCloseKey(hKeyResults);
			if (CPURunning) { 
				CloseCpu();
				ShutdownPlugins();
				SetupPlugins(hMainWindow);				
				StartEmulation();
			} else {
				ShutdownPlugins();
				if (!RomBrowser) { SetupPlugins(hMainWindow); }
				if (RomBrowser) { SetupPlugins(hHiddenWin); }
			}
			FreePluginList();
		}
		if (((NMHDR FAR *) lParam)->code == PSN_RESET) {
			FreePluginList();
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void RomAddFieldToList (HWND hDlg, char * Name, int Pos, int ID) {
	int listCount, index;

	if (Pos < 0) { 
		index = SendDlgItemMessage(hDlg,IDC_AVALIABLE,LB_ADDSTRING,0,(LPARAM)Name);
		SendDlgItemMessage(hDlg,IDC_AVALIABLE,LB_SETITEMDATA,index,ID);
		return;
	}
	listCount = SendDlgItemMessage(hDlg,IDC_USING,LB_GETCOUNT,0,0);
	if (Pos > listCount) { Pos = listCount; }
	index = SendDlgItemMessage(hDlg,IDC_USING,LB_INSERTSTRING,Pos,(LPARAM)Name);
	SendDlgItemMessage(hDlg,IDC_USING,LB_SETITEMDATA,index,ID);
}

BOOL CALLBACK RomBrowserProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:		
		if (RomBrowser) { SendMessage(GetDlgItem(hDlg,IDC_USE_ROMBROWSER),BM_SETCHECK, BST_CHECKED,0); }
		if (Rercursion) { SendMessage(GetDlgItem(hDlg,IDC_RECURSION),BM_SETCHECK, BST_CHECKED,0); }
		{ 
			int count;

			for (count = 0; count < NoOfFields; count ++) {
				RomAddFieldToList(hDlg,GS(RomBrowserFields[count].LangID),RomBrowserFields[count].Pos, count);
			}
		}
		{
			char String[256];
			sprintf(String,"%d",RomsToRemember);
			SetDlgItemText(hDlg,IDC_REMEMBER,String);
			sprintf(String,"%d",RomDirsToRemember);
			SetDlgItemText(hDlg,IDC_REMEMBERDIR,String);
			
			SetDlgItemText(hDlg,IDC_ROMSEL_TEXT1,GS(RB_MAX_ROMS));
			SetDlgItemText(hDlg,IDC_ROMSEL_TEXT2,GS(RB_ROMS));
			SetDlgItemText(hDlg,IDC_ROMSEL_TEXT3,GS(RB_MAX_DIRS));
			SetDlgItemText(hDlg,IDC_ROMSEL_TEXT4,GS(RB_DIRS));
			SetDlgItemText(hDlg,IDC_USE_ROMBROWSER,GS(RB_USE));
			SetDlgItemText(hDlg,IDC_RECURSION,GS(RB_DIR_RECURSION));
			SetDlgItemText(hDlg,IDC_ROMSEL_TEXT5,GS(RB_AVALIABLE_FIELDS));
			SetDlgItemText(hDlg,IDC_ROMSEL_TEXT6,GS(RB_SHOW_FIELDS));
			SetDlgItemText(hDlg,IDC_ADD,GS(RB_ADD));
			SetDlgItemText(hDlg,IDC_REMOVE,GS(RB_REMOVE));
			SetDlgItemText(hDlg,IDC_UP,GS(RB_UP));
			SetDlgItemText(hDlg,IDC_DOWN,GS(RB_DOWN));
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ADD:
			{
				char String[100];
				int index, listCount, Data;

				index = SendMessage(GetDlgItem(hDlg,IDC_AVALIABLE),LB_GETCURSEL,0,0);
				if (index < 0) { break; }
				SendMessage(GetDlgItem(hDlg,IDC_AVALIABLE),LB_GETTEXT,index,(LPARAM)String);
				Data = SendMessage(GetDlgItem(hDlg,IDC_AVALIABLE),LB_GETITEMDATA,index,0);
				SendDlgItemMessage(hDlg,IDC_AVALIABLE,LB_DELETESTRING,index,0);
				listCount = SendDlgItemMessage(hDlg,IDC_AVALIABLE,LB_GETCOUNT,0,0);
				if (index >= listCount) { index -= 1;}
				SendDlgItemMessage(hDlg,IDC_AVALIABLE,LB_SETCURSEL,index,0);
				index = SendDlgItemMessage(hDlg,IDC_USING,LB_ADDSTRING,0,(LPARAM)String);
				SendDlgItemMessage(hDlg,IDC_USING,LB_SETITEMDATA,index,Data);
			}
			break;
		case IDC_REMOVE:
			{
				char String[100];
				int index, listCount, Data;

				index = SendMessage(GetDlgItem(hDlg,IDC_USING),LB_GETCURSEL,0,0);
				if (index < 0) { break; }
				SendMessage(GetDlgItem(hDlg,IDC_USING),LB_GETTEXT,index,(LPARAM)String);
				Data = SendMessage(GetDlgItem(hDlg,IDC_USING),LB_GETITEMDATA,index,0);
				SendDlgItemMessage(hDlg,IDC_USING,LB_DELETESTRING,index,0);
				listCount = SendDlgItemMessage(hDlg,IDC_USING,LB_GETCOUNT,0,0);
				if (index >= listCount) { index -= 1;}
				SendDlgItemMessage(hDlg,IDC_USING,LB_SETCURSEL,index,0);
				index = SendDlgItemMessage(hDlg,IDC_AVALIABLE,LB_ADDSTRING,0,(LPARAM)String);
				SendDlgItemMessage(hDlg,IDC_AVALIABLE,LB_SETITEMDATA,index,Data);
			}
			break;
		case IDC_UP:
			{
				char String[100];
				int index, Data;

				index = SendMessage(GetDlgItem(hDlg,IDC_USING),LB_GETCURSEL,0,0);
				if (index <= 0) { break; }
				SendMessage(GetDlgItem(hDlg,IDC_USING),LB_GETTEXT,index,(LPARAM)String);
				Data = SendMessage(GetDlgItem(hDlg,IDC_USING),LB_GETITEMDATA,index,0);
				SendDlgItemMessage(hDlg,IDC_USING,LB_DELETESTRING,index,0);
				index = SendDlgItemMessage(hDlg,IDC_USING,LB_INSERTSTRING,index - 1,(LPARAM)String);
				SendDlgItemMessage(hDlg,IDC_USING,LB_SETCURSEL,index,0);
				SendDlgItemMessage(hDlg,IDC_USING,LB_SETITEMDATA,index,Data);
			}
			break;
		case IDC_DOWN:
			{
				char String[100];
				int index,listCount,Data;

				index = SendMessage(GetDlgItem(hDlg,IDC_USING),LB_GETCURSEL,0,0);
				listCount = SendDlgItemMessage(hDlg,IDC_USING,LB_GETCOUNT,0,0);
				if ((index + 1) == listCount) { break; }
				SendMessage(GetDlgItem(hDlg,IDC_USING),LB_GETTEXT,index,(LPARAM)String);
				Data = SendMessage(GetDlgItem(hDlg,IDC_USING),LB_GETITEMDATA,index,0);
				SendDlgItemMessage(hDlg,IDC_USING,LB_DELETESTRING,index,0);
				index = SendDlgItemMessage(hDlg,IDC_USING,LB_INSERTSTRING,index + 1,(LPARAM)String);
				SendDlgItemMessage(hDlg,IDC_USING,LB_SETCURSEL,index,0);
				SendDlgItemMessage(hDlg,IDC_USING,LB_SETITEMDATA,index,Data);
			}
			break;
		}
		break;
		
	case WM_NOTIFY:
		if (((NMHDR FAR *) lParam)->code == PSN_APPLY) { 
			char String[200], szIndex[10];
			int index, listCount, Pos;
			DWORD Disposition = 0;
			HKEY hKeyResults = 0;
			long lResult;

			RomBrowser = SendMessage(GetDlgItem(hDlg,IDC_USE_ROMBROWSER),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;
			Rercursion = SendMessage(GetDlgItem(hDlg,IDC_RECURSION),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;

			sprintf(String,"Software\\N64 Emulation\\%s",AppName);
			lResult = RegCreateKeyEx( HKEY_CURRENT_USER, String,0,"", REG_OPTION_NON_VOLATILE,
				KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
			if (lResult == ERROR_SUCCESS) {
				RegSetValueEx(hKeyResults,"Use Rom Browser",0,REG_DWORD,(BYTE *)&RomBrowser,sizeof(DWORD));
				RegSetValueEx(hKeyResults,"Use Recursion",0,REG_DWORD,(BYTE *)&Rercursion,sizeof(DWORD));
			}
									
			SaveRomBrowserColoumnInfo(); // Any coloumn width changes get saved
			listCount = SendDlgItemMessage(hDlg,IDC_USING,LB_GETCOUNT,0,0);
			for (Pos = 0; Pos < listCount; Pos ++ ){
				index = SendMessage(GetDlgItem(hDlg,IDC_USING),LB_GETITEMDATA,Pos,0);
				SaveRomBrowserColoumnPosition(index,Pos);
			}
			listCount = SendDlgItemMessage(hDlg,IDC_AVALIABLE,LB_GETCOUNT,0,0);
			strcpy(szIndex,"-1");
			for (Pos = 0; Pos < listCount; Pos ++ ){
				index = SendMessage(GetDlgItem(hDlg,IDC_AVALIABLE),LB_GETITEMDATA,Pos,0);
				SaveRomBrowserColoumnPosition(index,-1);
			}			
			LoadRomBrowserColoumnInfo();
			ResetRomBrowserColomuns();
			if (RomBrowser) { ShowRomList(hMainWindow); }
			if (!RomBrowser) { HideRomBrowser(); }

			RemoveRecentList(hMainWindow);
			RomsToRemember = GetDlgItemInt(hDlg,IDC_REMEMBER,NULL,FALSE);
			if (RomsToRemember < 0) { RomsToRemember = 0; }
			if (RomsToRemember > 10) { RomsToRemember = 10; }
			RegSetValueEx(hKeyResults,"Roms To Remember",0,REG_DWORD,(BYTE *)&RomsToRemember,sizeof(DWORD));

			RemoveRecentList(hMainWindow);
			RomDirsToRemember = GetDlgItemInt(hDlg,IDC_REMEMBERDIR,NULL,FALSE);
			if (RomDirsToRemember < 0) { RomDirsToRemember = 0; }
			if (RomDirsToRemember > 10) { RomDirsToRemember = 10; }
			RegSetValueEx(hKeyResults,"Rom Dirs To Remember",0,REG_DWORD,(BYTE *)&RomDirsToRemember,sizeof(DWORD));
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK RomNotesProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		SetDlgItemText(hDlg,IDC_STATUS_TEXT,GS(NOTE_STATUS));
		SetDlgItemText(hDlg,IDC_CORE,GS(NOTE_CORE));
		SetDlgItemText(hDlg,IDC_PLUGIN,GS(NOTE_PLUGIN));
		{
			char Identifier[100], *IniFile;
			sprintf(Identifier,"%08X-%08X-C:%X",*(DWORD *)(&RomHeader[0x10]),*(DWORD *)(&RomHeader[0x14]),RomHeader[0x3D]);
			IniFile = GetIniFileName();

			{
				char String[0x3000], RomStatus[100], Status[200], *p;
				int len, index;

				_GetPrivateProfileString(Identifier,"Status",Default_RomStatus,RomStatus,sizeof(RomStatus),IniFile);
				GetPrivateProfileSection("Rom Status",String,sizeof(String), IniFile);
				for (p = String; strlen(p) > 0; p += strlen(p) + 1) {
					strncpy(Status,p,sizeof(Status));
					if (strrchr(Status,'=') == NULL) { continue; }
					*(strrchr(Status,'=')) = 0;
					len = strlen(Status);
					if (len > 4 && _strnicmp(&Status[len-4],".Sel",4) == 0) { continue; }
					if (len > 8 && _strnicmp(&Status[len-8],".Seltext",8) == 0) { continue; }
					if (len > 15 && _strnicmp(&Status[len-15],".AutoFullScreen",15) == 0) { continue; }
					index = SendMessage(GetDlgItem(hDlg,IDC_STATUS),CB_ADDSTRING,0,(LPARAM)Status);
					if (strcmp(Status,RomStatus) == 0) { SendMessage(GetDlgItem(hDlg,IDC_STATUS),CB_SETCURSEL,index,0); }
					if (SendMessage(GetDlgItem(hDlg,IDC_STATUS),CB_GETCOUNT,0,0) == 0) { SendMessage(GetDlgItem(hDlg,IDC_STATUS),CB_SETCURSEL,0,0); }
				}
			}
			{
				char CoreNotes[800];
				_GetPrivateProfileString(Identifier,"Core Note","",CoreNotes,sizeof(CoreNotes),IniFile);
				SetDlgItemText(hDlg,IDC_CORE_NOTES,CoreNotes);
			}
			{
				char PluginNotes[800];
				_GetPrivateProfileString(Identifier,"Plugin Note","",PluginNotes,sizeof(PluginNotes),IniFile);
				SetDlgItemText(hDlg,IDC_PLUGIN_NOTE,PluginNotes);
			}
		}
		if (strlen(RomName) == 0) {
			EnableWindow(GetDlgItem(hDlg,IDC_STATUS_TEXT),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_STATUS),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_CORE),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_CORE_NOTES),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_PLUGIN),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_PLUGIN_NOTE),FALSE);
		}
		break;
	case WM_NOTIFY:
		if (((NMHDR FAR *) lParam)->code == PSN_APPLY) { 
			char Identifier[100], string[200], *IniFile;
			sprintf(Identifier,"%08X-%08X-C:%X",*(DWORD *)(&RomHeader[0x10]),*(DWORD *)(&RomHeader[0x14]),RomHeader[0x3D]);
			IniFile = GetIniFileName();

			GetWindowText(GetDlgItem(hDlg, IDC_STATUS), string, sizeof(string));
			if (strlen(string) == 0) { strcpy(string, Default_RomStatus); }
			_WritePrivateProfileString(Identifier,"Status",string,IniFile);
			GetWindowText(GetDlgItem(hDlg, IDC_CORE_NOTES), string, sizeof(string));
			_WritePrivateProfileString(Identifier,"Core Note",string,IniFile);
			GetWindowText(GetDlgItem(hDlg, IDC_PLUGIN_NOTE), string, sizeof(string));
			_WritePrivateProfileString(Identifier,"Plugin Note",string,IniFile);
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK RomSettingsProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	char String[256];
	int indx;

	switch (uMsg) {
	case WM_INITDIALOG:		
		ReadRomOptions();

		SetDlgItemText(hDlg,IDC_CPU_TYPE_TEXT,GS(ROM_CPU_STYLE));
		SetDlgItemText(hDlg,IDC_SELFMOD_TEXT,GS(ROM_SMCM));
		SetDlgItemText(hDlg,IDC_MEMORY_SIZE_TEXT,GS(ROM_MEM_SIZE));
		SetDlgItemText(hDlg,IDC_BLOCK_LINKING_TEXT,GS(ROM_ABL));
		SetDlgItemText(hDlg,IDC_SAVE_TYPE_TEXT,GS(ROM_SAVE_TYPE));
		SetDlgItemText(hDlg,IDC_COUNTFACT_TEXT,GS(ROM_COUNTER_FACTOR));

		AddDropDownItem(hDlg,IDC_CPU_TYPE,ROM_DEFAULT,CPU_Default,&RomCPUType);
		AddDropDownItem(hDlg,IDC_CPU_TYPE,CORE_INTERPTER,CPU_Interpreter,&RomCPUType);
		AddDropDownItem(hDlg,IDC_CPU_TYPE,CORE_RECOMPILER,CPU_Recompiler,&RomCPUType);
		if (HaveDebugger) { AddDropDownItem(hDlg,IDC_CPU_TYPE,CORE_SYNC,CPU_SyncCores,&RomCPUType); }

		AddDropDownItem(hDlg,IDC_SELFMOD,ROM_DEFAULT,ModCode_Default,&RomSelfMod);
		AddDropDownItem(hDlg,IDC_SELFMOD,SMCM_NONE,ModCode_None,&RomSelfMod);
		AddDropDownItem(hDlg,IDC_SELFMOD,SMCM_CACHE,ModCode_Cache,&RomSelfMod);
		AddDropDownItem(hDlg,IDC_SELFMOD,SMCM_PROECTED,ModCode_ProtectedMemory,&RomSelfMod);
		AddDropDownItem(hDlg,IDC_SELFMOD,SMCM_CHECK_MEM,ModCode_CheckMemoryCache,&RomSelfMod);
		AddDropDownItem(hDlg,IDC_SELFMOD,SMCM_CHANGE_MEM,ModCode_ChangeMemory,&RomSelfMod);
		AddDropDownItem(hDlg,IDC_SELFMOD,SMCM_CHECK_ADV,ModCode_CheckMemory2,&RomSelfMod);

		AddDropDownItem(hDlg,IDC_RDRAM_SIZE,ROM_DEFAULT,-1,&RomRamSize);
		AddDropDownItem(hDlg,IDC_RDRAM_SIZE,RDRAM_4MB,0x400000,&RomRamSize);
		AddDropDownItem(hDlg,IDC_RDRAM_SIZE,RDRAM_8MB,0x800000,&RomRamSize);

		AddDropDownItem(hDlg,IDC_BLOCK_LINKING,ROM_DEFAULT,-1,&RomUseLinking);
		AddDropDownItem(hDlg,IDC_BLOCK_LINKING,ABL_ON,0,&RomUseLinking);
		AddDropDownItem(hDlg,IDC_BLOCK_LINKING,ABL_OFF,1,&RomUseLinking);

		AddDropDownItem(hDlg,IDC_SAVE_TYPE,SAVE_FIRST_USED,Auto,&RomSaveUsing);
		AddDropDownItem(hDlg,IDC_SAVE_TYPE,SAVE_4K_EEPROM,Eeprom_4K,&RomSaveUsing);
		AddDropDownItem(hDlg,IDC_SAVE_TYPE,SAVE_16K_EEPROM,Eeprom_16K,&RomSaveUsing);
		AddDropDownItem(hDlg,IDC_SAVE_TYPE,SAVE_SRAM,Sram,&RomSaveUsing);
		AddDropDownItem(hDlg,IDC_SAVE_TYPE,SAVE_FLASHRAM,FlashRam,&RomSaveUsing);

		AddDropDownItem(hDlg,IDC_COUNTFACT,ROM_DEFAULT,-1,&RomCF);
		AddDropDownItem(hDlg,IDC_COUNTFACT,NUMBER_1,1,&RomCF);
		AddDropDownItem(hDlg,IDC_COUNTFACT,NUMBER_2,2,&RomCF);
		AddDropDownItem(hDlg,IDC_COUNTFACT,NUMBER_3,3,&RomCF);
		AddDropDownItem(hDlg,IDC_COUNTFACT,NUMBER_4,4,&RomCF);
		AddDropDownItem(hDlg,IDC_COUNTFACT,NUMBER_5,5,&RomCF);
		AddDropDownItem(hDlg,IDC_COUNTFACT,NUMBER_6,6,&RomCF);

		SetFlagControl(hDlg,&RomUseLargeBuffer, IDC_LARGE_COMPILE_BUFFER, ROM_LARGE_BUFFER);
		SetFlagControl(hDlg,&RomUseTlb, IDC_USE_TLB, ROM_USE_TLB);
		SetFlagControl(hDlg,&RomUseCache, IDC_ROM_REGCACHE, ROM_REG_CACHE);
		SetFlagControl(hDlg,&RomDelaySI, IDC_DELAY_SI, ROM_DELAY_SI);
		SetFlagControl(hDlg,&RomDelayRDP, IDC_DELAY_RDP, ROM_DELAY_RDP);
		SetFlagControl(hDlg,&RomDelayRSP, IDC_DELAY_RSP, ROM_DELAY_RSP);
		SetFlagControl(hDlg,&RomEmulateAI, IDC_EMULATE_AI, ROM_EMULATE_AI);
		SetFlagControl(hDlg,&RomAudioSignal, IDC_AUDIO_SIGNAL, ROM_AUDIO_SIGNAL);
		SetFlagControl(hDlg,&RomSPHack, IDC_ROM_SPHACK, ROM_SP_HACK);
				
		if (strlen(RomName) == 0) {
			EnableWindow(GetDlgItem(hDlg,IDC_MEMORY_SIZE_TEXT),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_RDRAM_SIZE),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_SAVE_TYPE_TEXT),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_SAVE_TYPE),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_COUNTFACT_TEXT),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_COUNTFACT),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_CPU_TYPE_TEXT),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_CPU_TYPE),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_SELFMOD_TEXT),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_SELFMOD),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_USE_TLB),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_DELAY_SI),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_DELAY_RDP),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_DELAY_RSP),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_EMULATE_AI),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_ROM_SPHACK),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_ROM_SPHACK),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_ROM_REGCACHE),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_BLOCK_LINKING_TEXT),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_BLOCK_LINKING),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_AUDIO_SIGNAL),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_LARGE_COMPILE_BUFFER),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_NOTES),FALSE);
		}
		break;
	case WM_NOTIFY:
		if (((NMHDR FAR *) lParam)->code == PSN_APPLY) { 
			LPSTR NotesIniFileName = GetNotesIniFileName();
			char Identifier[100];
			
			if (strlen(RomName) == 0) { break; }
			sprintf(Identifier,"%08X-%08X-C:%X",*(DWORD *)(&RomHeader[0x10]),*(DWORD *)(&RomHeader[0x14]),RomHeader[0x3D]);
			_WritePrivateProfileString(Identifier, "Game Name", RomName, NotesIniFileName);
			GetDlgItemText(hDlg,IDC_NOTES,String,sizeof(String));
			_WritePrivateProfileString(Identifier,"Note",String,NotesIniFileName);

			if (!UseIni) { break; }
			indx = SendMessage(GetDlgItem(hDlg,IDC_RDRAM_SIZE),CB_GETCURSEL,0,0); 
			RomRamSize = SendMessage(GetDlgItem(hDlg,IDC_RDRAM_SIZE),CB_GETITEMDATA,indx,0);
			indx = SendMessage(GetDlgItem(hDlg,IDC_SAVE_TYPE),CB_GETCURSEL,0,0); 
			RomSaveUsing = SendMessage(GetDlgItem(hDlg,IDC_SAVE_TYPE),CB_GETITEMDATA,indx,0);
			indx = SendMessage(GetDlgItem(hDlg,IDC_COUNTFACT),CB_GETCURSEL,0,0); 
			RomCF = SendMessage(GetDlgItem(hDlg,IDC_COUNTFACT),CB_GETITEMDATA,indx,0);
			indx = SendMessage(GetDlgItem(hDlg,IDC_CPU_TYPE),CB_GETCURSEL,0,0); 
			RomCPUType = SendMessage(GetDlgItem(hDlg,IDC_CPU_TYPE),CB_GETITEMDATA,indx,0);
			indx = SendMessage(GetDlgItem(hDlg,IDC_SELFMOD),CB_GETCURSEL,0,0); 
			RomSelfMod = SendMessage(GetDlgItem(hDlg,IDC_SELFMOD),CB_GETITEMDATA,indx,0);
			indx = SendMessage(GetDlgItem(hDlg,IDC_BLOCK_LINKING),CB_GETCURSEL,0,0); 
			RomUseLinking = SendMessage(GetDlgItem(hDlg,IDC_BLOCK_LINKING),CB_GETITEMDATA,indx,0);
			RomDelaySI = SendMessage(GetDlgItem(hDlg,IDC_DELAY_SI),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;
			RomDelayRDP = SendMessage(GetDlgItem(hDlg,IDC_DELAY_RDP),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;
			RomDelayRSP = SendMessage(GetDlgItem(hDlg,IDC_DELAY_RSP),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;
			RomEmulateAI = SendMessage(GetDlgItem(hDlg,IDC_EMULATE_AI),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;
			RomAudioSignal = SendMessage(GetDlgItem(hDlg,IDC_AUDIO_SIGNAL),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;
			RomSPHack = SendMessage(GetDlgItem(hDlg,IDC_ROM_SPHACK),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;
			RomUseTlb = SendMessage(GetDlgItem(hDlg,IDC_USE_TLB),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;
			RomUseCache = SendMessage(GetDlgItem(hDlg,IDC_ROM_REGCACHE),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;
			RomUseLargeBuffer = SendMessage(GetDlgItem(hDlg,IDC_LARGE_COMPILE_BUFFER),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE;			
			SaveRomOptions();
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK ShellIntegrationProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:		
		SetDlgItemText(hDlg,IDC_SHELL_INT_TEXT,GS(SHELL_TEXT));	
		if (TestExtensionRegistered(".v64")) { SendMessage(GetDlgItem(hDlg,IDC_V64),BM_SETCHECK, BST_CHECKED,0); }
		if (TestExtensionRegistered(".z64")) { SendMessage(GetDlgItem(hDlg,IDC_Z64),BM_SETCHECK, BST_CHECKED,0); }
		if (TestExtensionRegistered(".n64")) { SendMessage(GetDlgItem(hDlg,IDC_N64),BM_SETCHECK, BST_CHECKED,0); }
		if (TestExtensionRegistered(".rom")) { SendMessage(GetDlgItem(hDlg,IDC_ROM),BM_SETCHECK, BST_CHECKED,0); }
		if (TestExtensionRegistered(".jap")) { SendMessage(GetDlgItem(hDlg,IDC_JAP),BM_SETCHECK, BST_CHECKED,0); }
		if (TestExtensionRegistered(".pal")) { SendMessage(GetDlgItem(hDlg,IDC_PAL),BM_SETCHECK, BST_CHECKED,0); }
		if (TestExtensionRegistered(".usa")) { SendMessage(GetDlgItem(hDlg,IDC_USA),BM_SETCHECK, BST_CHECKED,0); }
		break;
	case WM_NOTIFY:
		if (((NMHDR FAR *) lParam)->code == PSN_APPLY) { 
			RegisterExtension(".v64",SendMessage(GetDlgItem(hDlg,IDC_V64),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE);
			RegisterExtension(".z64",SendMessage(GetDlgItem(hDlg,IDC_Z64),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE);
			RegisterExtension(".n64",SendMessage(GetDlgItem(hDlg,IDC_N64),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE);
			RegisterExtension(".rom",SendMessage(GetDlgItem(hDlg,IDC_ROM),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE);
			RegisterExtension(".jap",SendMessage(GetDlgItem(hDlg,IDC_JAP),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE);
			RegisterExtension(".pal",SendMessage(GetDlgItem(hDlg,IDC_PAL),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE);
			RegisterExtension(".usa",SendMessage(GetDlgItem(hDlg,IDC_USA),BM_GETSTATE, 0,0) == BST_CHECKED?TRUE:FALSE);
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}