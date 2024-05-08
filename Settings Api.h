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
int _DeletePrivateProfileString(
  const char * lpAppName,  // pointer to section name
  const char * lpKeyName,  // pointer to key name
  const char * lpFileName  // pointer to initialization filename
);

unsigned int _GetPrivateProfileInt(
  const char * lpAppName,  // address of section name
  const char * lpKeyName,  // address of key name
  int nDefault,       // return value if key name is not found
  const char * lpFileName  // address of initialization filename
);

unsigned int _GetPrivateProfileString(
  const char * lpAppName,        // points to section name
  const char * lpKeyName,        // points to key name
  const char * lpDefault,        // points to default string
  char *       lpReturnedString, // points to destination buffer
  unsigned int nSize,            // size of destination buffer
  const char * lpFileName        // points to initialization filename
);

unsigned int _GetPrivateProfileString2(
  const char * lpAppName,        // points to section name
  const char * lpKeyName,        // points to key name
  const char * lpDefault,        // points to default string
  char **       lpReturnedString, // points to destination buffer
  const char * lpFileName        // points to initialization filename
);

int _WritePrivateProfileString(
  const char * lpAppName,  // pointer to section name
  const char * lpKeyName,  // pointer to key name
  const char * lpString,   // pointer to string to add
  const char * lpFileName  // pointer to initialization filename
);

unsigned int _GetPrivateProfileSectionNames(									//added by Witten
  const char * lpszReturnBuffer,	// address of return buffer
  DWORD nSize,						// size of destination buffer
  const char * lpFileName			// address of initialization filename
);


int fGetString2(FILE * File, char **String, BYTE **Data, int * DataSize, int *Left );
void fInsertSpaces(FILE * File,int Pos, int NoOfSpaces);
