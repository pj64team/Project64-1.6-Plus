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
extern DWORD RomFileSize, RomRamSize, RomSaveUsing, RomCPUType, RomSelfMod, 
	RomUseTlb, RomUseLinking, RomCF, RomUseLargeBuffer, RomUseCache,
	RomDelaySI, RomSPHack, RomAudioSignal, RomDelayRDP, RomDelayRSP, RomEmulateAI;
extern char CurrentFileName[MAX_PATH+1], RomName[MAX_PATH+1], RomHeader[0x1000];
extern char LastRoms[10][MAX_PATH+1], LastDirs[10][MAX_PATH+1];

void AddRecentFile           ( HWND hWnd, char * addition );
void ChangeRomOptionMemSize  ( DWORD NewSize );
void ChangeRomOptionSaveType ( enum SaveType type );
void GetRomDirectory         ( char * Directory );
BOOL LoadDataFromRomFile     ( char * FileName,BYTE * Data,int DataLen, int * RomSize );
BOOL LoadRomHeader           ( void );
void CreateRecentFileList    ( HMENU hMenu );
void CreateRecentDirList     ( HMENU hMenu );
void LoadRecentRom           ( DWORD Index );
void LoadRomOptions          ( void );
void OpenChosenFile	         ( void );
void ReadRomOptions          ( void );
void RemoveRecentList        ( HWND hWnd );
void OpenN64Image            ( void );
void SaveRecentDirs          ( void );
void SaveRecentFiles         ( void );
void SaveRomOptions          ( void );
void SetRecentRomDir         ( DWORD Index );
void SetRomDirectory         ( char * Directory );
void RecalculateCRCs         ( void );