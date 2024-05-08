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
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include "main.h"

#ifdef WIN32
char * LineFeed = "\r\n";
#else
char * LineFeed = "\n";
#endif

int AsciiToInt( char * String, int Maxlen) {
	int result = 0;

	for( ; Maxlen && *String == ' '; Maxlen--, String++);
    while(Maxlen--){
		if(isdigit(*String)) {
			result = result * 10 + (*String - '0');
		}
		String++;
   }
   return(result);
}

int fGetString(FILE * File,char * String, int MaxLen) {
	int count;

	if (String == NULL) { return -1; }
	if (MaxLen < 1) { return -1; }
	for (count = 0; count < (MaxLen - 1); count ++ ) {		
		int result;

		result = fgetc(File);
		if (result == EOF || result == 10) {
			String[count] = 0;
			if (result == EOF && count == 0) {
				return -1;
			}
			return count;
		} 
		String[count] = result;
	}
	String[MaxLen] = '\0'; 
	return MaxLen - 1;
}

int fGetString2(FILE * File, char **String, BYTE **Data, int * DataSize, int *Left ) {
#define BufferIncrease	1024
	int dwRead = BufferIncrease;

	if (*DataSize == 0) { 
		*DataSize = BufferIncrease;
		*Data = malloc(*DataSize);
		*Left = 0;
	}

	for (;;) {
		int count;

		for (count = 0; count < *Left; count ++) {
			if ((*Data)[count] == '\n') {
				if (*String != NULL) { 
					free(*String); 
					*String = NULL;
				}
				*String = malloc(count + 1);
				strncpy(*String,*Data,count);
				(*String)[count] = 0;
				*Left -= count + 1;
				if (*Left > 0) {
					memmove(&((*Data)[0]),&((*Data)[count + 1]),*Left);
				}
				return count + 1;
			}
		}
		
		if (dwRead == 0) { return -1; }
		if ((*DataSize - *Left) == 0) {
			*DataSize += BufferIncrease;
			*Data = (BYTE *)realloc(*Data, *DataSize);
			if (*Data == NULL) {
				DisplayError(GS(MSG_MEM_ALLOC_ERROR));
				return -1;
			}
		}
		dwRead = fread(&((*Data)[*Left]),1,*DataSize - *Left,File);
		*Left += dwRead;
	}
}

void fInsertSpaces(FILE * File,int Pos, int NoOfSpaces) {
#define fIS_MvSize 0x1000
	unsigned char Data[fIS_MvSize + 1];
	int SizeToRead, result;
	long end, WritePos;

	fseek(File,0,SEEK_END);
	end = ftell(File);
	
	if (NoOfSpaces > 0) {
		do {
			SizeToRead = end - Pos;
			if (SizeToRead > fIS_MvSize) { SizeToRead = fIS_MvSize; }
			if (SizeToRead > 0) {
				fseek(File,SizeToRead * -1,SEEK_CUR);
				WritePos = ftell(File);
				memset(Data,0,sizeof(Data));
				fread(Data,SizeToRead,1,File);
				fseek(File,WritePos,SEEK_SET);
				end = WritePos;
				fprintf(File,"%*c",NoOfSpaces,' ');
				result = strlen(Data);
				fwrite(Data,result,1,File);
				fseek(File,WritePos,SEEK_SET);						
			}
		} while (SizeToRead > 0);
	} if (NoOfSpaces < 0) {
		int ReadPos = Pos + (NoOfSpaces * -1);
		int WritePos = Pos;
		do {
			SizeToRead = end - ReadPos;
			if (SizeToRead > fIS_MvSize) { SizeToRead = fIS_MvSize; }
			fseek(File,ReadPos,SEEK_SET);
			fread(Data,SizeToRead,1,File);
			fseek(File,WritePos,SEEK_SET);
			fwrite(Data,SizeToRead,1,File);
			ReadPos += SizeToRead;
			WritePos += SizeToRead;
		} while (SizeToRead > 0);
		fseek(File,WritePos,SEEK_SET);
		fprintf(File,"%*c",(NoOfSpaces * -1),' ');
	}
}

unsigned int _GetPrivateProfileInt(
  const char * lpAppName,  // address of section name
  const char * lpKeyName,  // address of key name
  int nDefault,       // return value if key name is not found
  const char * lpFileName  // address of initialization filename
)
{ 
	char *Input = NULL, *Data = NULL, * Pos, CurrentSection[300];
	int DataLen = 0, DataLeft, result, count;
	static long Fpos = 0;
	FILE * fInput;

	fInput = fopen(lpFileName,"rb");
	if (fInput == NULL) { goto GetPrivateProfileString_ReturnDefault; }
	CurrentSection[0] = 0;
	
	//test to see if Fpos is same as current section;
	fseek(fInput,Fpos,SEEK_SET);
	result = fGetString2(fInput,&Input,&Data,&DataLen,&DataLeft);
	if (result > 1) {
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
		if (strlen(Input) > 1) {
			if (Input[0] == '[' && Input[strlen(Input) - 1] == ']') {
				strcpy(CurrentSection,&Input[1]);
				CurrentSection[strlen(CurrentSection) - 1] = 0;
			}
		}
	}
	if (strcmp(lpAppName,CurrentSection) != 0) { 
		DataLen = 0;
		DataLeft = 0;
		free(Data);
		Data = NULL;
		fseek(fInput,0,SEEK_SET);
	}

	do {
		if (strcmp(lpAppName,CurrentSection) != 0) { 
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
			strcpy(CurrentSection,&Input[1]);
			CurrentSection[strlen(CurrentSection) - 1] = 0;
			continue;
		}
		if (strcmp(lpAppName,CurrentSection) != 0) { 
			continue;
		}
		Pos = strchr(Input,'=');
		Pos[0] = 0;
		if (strcmp(Input,lpKeyName) != 0) { continue; }
		//strncpy(lpReturnedString,&Pos[1],nSize - 1);
		result = atoi(&Pos[1]);
		fclose(fInput);
		if (Input) { free(Input);  Input = NULL; }
		if (Data) {  free(Data);  Data = NULL; }
		return result;
	} while (result >= 0);
	fclose(fInput);
	if (Input) { free(Input);  Input = NULL; }
	if (Data) {  free(Data);  Data = NULL; }

GetPrivateProfileString_ReturnDefault:
	return nDefault;
}

unsigned int _GetPrivateProfileString(
  const char * lpAppName,        // points to section name
  const char * lpKeyName,        // points to key name
  const char * lpDefault,        // points to default string
  char *       lpReturnedString, // points to destination buffer
  unsigned int nSize,            // size of destination buffer
  const char * lpFileName        // points to initialization filename
) 
{	
	char *Input = NULL, *Data = NULL, * Pos, CurrentSection[300];
	int DataLen = 0, DataLeft, result, count;
	static long Fpos = 0;
	FILE * fInput;

	if (lpFileName == NULL) { return (unsigned int)&Fpos; }
	
	fInput = fopen(lpFileName,"rb");
	if (fInput == NULL) { goto GetPrivateProfileString_ReturnDefault; }
	CurrentSection[0] = 0;
	
	//test to see if Fpos is same as current section;
	fseek(fInput,Fpos,SEEK_SET);
	result = fGetString2(fInput,&Input,&Data,&DataLen,&DataLeft);
	if (result > 1) {
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
		if (strlen(Input) > 1) {
			if (Input[0] == '[' && Input[strlen(Input) - 1] == ']') {
				strcpy(CurrentSection,&Input[1]);
				CurrentSection[strlen(CurrentSection) - 1] = 0;
			}
		}
	}
	if (strcmp(lpAppName,CurrentSection) != 0) { 
		DataLen = 0;
		DataLeft = 0;
		free(Data);
		Data = NULL;
		fseek(fInput,0,SEEK_SET);
	}

	do {
		if (strcmp(lpAppName,CurrentSection) != 0) { 
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
			strcpy(CurrentSection,&Input[1]);
			CurrentSection[strlen(CurrentSection) - 1] = 0;
			continue;
		}
		if (strcmp(lpAppName,CurrentSection) != 0) { 
			continue;
		}
		Pos = strchr(Input,'=');
		if (Pos == NULL) { continue; }
		Pos[0] = 0;
		if (strcmp(Input,lpKeyName) != 0) { continue; }
		strncpy(lpReturnedString,&Pos[1],nSize - 1);
		lpReturnedString[nSize - 1] = 0;
		fclose(fInput);
		if (Input) { free(Input);  Input = NULL; }
		if (Data) {  free(Data);  Data = NULL; }
		return strlen(lpReturnedString);
	} while (result >= 0);
	fclose(fInput);
	if (Input) { free(Input);  Input = NULL; }
	if (Data) {  free(Data);  Data = NULL; }

GetPrivateProfileString_ReturnDefault:
	strncpy(lpReturnedString,lpDefault,nSize - 1);
	lpReturnedString[nSize - 1] = 0;
	return strlen(lpReturnedString);
	//return 0;
}

unsigned int _GetPrivateProfileString2(
  const char * lpAppName,        // points to section name
  const char * lpKeyName,        // points to key name
  const char * lpDefault,        // points to default string
  char **       lpReturnedString, // points to destination buffer
  const char * lpFileName        // points to initialization filename
  ) 
{	
	char *Input = NULL, *Data = NULL, * Pos, CurrentSection[300];
	int DataLen = 0, DataLeft, result, count, len;
	static long Fpos = 0;
	FILE * fInput;

	fInput = fopen(lpFileName,"rb");
	if (fInput == NULL) { goto GetPrivateProfileString_ReturnDefault2; }
	CurrentSection[0] = 0;
	
	//test to see if Fpos is same as current section;
	fseek(fInput,Fpos,SEEK_SET);
	result = fGetString2(fInput,&Input,&Data,&DataLen,&DataLeft);
	if (result > 1) {
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
		if (strlen(Input) > 1) {
			if (Input[0] == '[' && Input[strlen(Input) - 1] == ']') {
				strcpy(CurrentSection,&Input[1]);
				CurrentSection[strlen(CurrentSection) - 1] = 0;
			}
		}
	}
	if (strcmp(lpAppName,CurrentSection) != 0) { 
		DataLen = 0;
		DataLeft = 0;
		free(Data);
		Data = NULL;
		fseek(fInput,0,SEEK_SET);
	}

	do {
		if (strcmp(lpAppName,CurrentSection) != 0) { 
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
			strcpy(CurrentSection,&Input[1]);
			CurrentSection[strlen(CurrentSection) - 1] = 0;
			continue;
		}
		if (strcmp(lpAppName,CurrentSection) != 0) { 
			continue;
		}
		Pos = strchr(Input,'=');
		if (Pos == NULL) { continue; }
		Pos[0] = 0;
		if (strcmp(Input,lpKeyName) != 0) { continue; }
		len = strlen(&Pos[1]);
		if (*lpReturnedString) { free(*lpReturnedString); }
		*lpReturnedString = malloc(len + 1);
		strcpy(*lpReturnedString,&Pos[1]);
		fclose(fInput);
		if (Input) { free(Input);  Input = NULL; }
		if (Data) {  free(Data);  Data = NULL; }
		return len;
	} while (result >= 0);
	fclose(fInput);
	if (Input) { free(Input);  Input = NULL; }
	if (Data) {  free(Data);  Data = NULL; }

GetPrivateProfileString_ReturnDefault2:
	len = strlen(lpDefault);
	if (*lpReturnedString) { free(*lpReturnedString); }
	*lpReturnedString = malloc(len + 1);
	strcpy(*lpReturnedString,lpDefault);
	return len;
	//return 0;
}

/*int _WritePrivateProfileString(
  const char * lpAppName,  // pointer to section name
  const char * lpKeyName,  // pointer to key name
  const char * lpString,   // pointer to string to add
  const char * lpFileName  // pointer to initialization filename
) 
{
	char Input[300], CurrentSection[300];
	int result, count;
	long WritePos;
	FILE * fInput;

	//open the file for reading
	fInput = fopen(lpFileName,"r+b");
	if (fInput == NULL) { return 0; }
	fseek(fInput,0,SEEK_SET);
	CurrentSection[0] = 0;
	do {
		char * Pos;

		result = fGetString(fInput,Input,sizeof(Input));		
		if (result <= 1) { continue; }
		if (Input[strlen(Input) - 1] == '\r') { Input[strlen(Input) - 1] = 0; }

		Pos = Input;
		while (Pos != NULL) {
			Pos = strchr(Pos,'/');
			if (Pos != NULL) {
				if (Pos[1] == '/') { Pos[0] = 0; } else { Pos += 1; }
			}
		}
		
		for (count = strlen(&Input[0]) - 1; count >= 0; count --) {
			if (Input[count] != ' ') { break; }
			Input[count] = 0;
		}
		//stip leading spaces
		if (strlen(Input) <= 1) { continue; }
		if (Input[0] == '[') {
			if (Input[strlen(Input) - 1] != ']') { continue; }
			if (strcmp(lpAppName,CurrentSection) == 0) { 
				result = -1;
				continue;
			}
			strcpy(CurrentSection,&Input[1]);
			CurrentSection[strlen(CurrentSection) - 1] = 0;
			WritePos = ftell(fInput);
			continue;
		}
		if (strcmp(lpAppName,CurrentSection) != 0) { 
			continue;
		}
		Pos = strchr(Input,'=');
		if (Pos == NULL) { continue; }
		Pos[0] = 0;
		if (strcmp(Input,lpKeyName) != 0) { 
			WritePos = ftell(fInput);
			continue;
		}
		{
			long OldLen = ftell(fInput) - WritePos;
			int Newlen = strlen(lpKeyName) + strlen(lpString) + strlen(LineFeed) + 1;
			
			if (OldLen != Newlen) {
				fInsertSpaces(fInput,WritePos,Newlen - OldLen);
			}

			fseek(fInput,WritePos,SEEK_SET);
			fprintf(fInput,"%s=%s%s",lpKeyName,lpString,LineFeed);
			if ((Newlen - OldLen < 0)) {
				//copy all file excluding size diff
			}
			fclose(fInput);
		}
		return 0;
	} while (result >= 0);
	if (strcmp(lpAppName,CurrentSection) == 0) {
		int len = strlen(lpKeyName) + strlen(lpString) + strlen(LineFeed) + 1;
		fInsertSpaces(fInput,WritePos,len);
		fseek(fInput,WritePos,SEEK_SET);
		fprintf(fInput,"%s=%s%s",lpKeyName,lpString,LineFeed);
		fclose(fInput);
	} else {
		fseek(fInput,0,SEEK_END);
		fprintf(fInput,"%s[%s]%s%s=%s%s",LineFeed,lpAppName,LineFeed,
			lpKeyName,lpString,LineFeed);
		fclose(fInput);
	}
	return 0;
}
*/ 
int _DeletePrivateProfileString(
  const char * lpAppName,  // pointer to section name
  const char * lpKeyName,  // pointer to key name
  const char * lpFileName  // pointer to initialization filename
) 
{
	char *Input = NULL, *Data = NULL, * Pos, CurrentSection[300];
	int DataLen = 0, DataLeft, result, count;
	static long Fpos = 0;
	long WritePos;
	FILE * fInput;

	fInput = fopen(lpFileName,"r+b");
	if (fInput == NULL) { 
		fInput = fopen(lpFileName,"w+b");
		if (fInput == NULL) { return 0; }
	}
	CurrentSection[0] = 0;

	do {
		if (strcmp(lpAppName,CurrentSection) != 0) { 
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
			if (strcmp(lpAppName,CurrentSection) == 0) { 
				result = -1;
				continue;
			}
			strcpy(CurrentSection,&Input[1]);
			CurrentSection[strlen(CurrentSection) - 1] = 0;
			WritePos = ftell(fInput) - DataLeft;
			continue;
		}
		if (strcmp(lpAppName,CurrentSection) != 0) { 
			continue;
		}
		Pos = strchr(Input,'=');
		if (Pos == NULL) { continue; }
		Pos[0] = 0;
		if (strcmp(Input,lpKeyName) != 0) { 
			WritePos = ftell(fInput) - DataLeft;
			continue;
		}
		{
			long OldLen = (ftell(fInput) - DataLeft) - WritePos;
			int Newlen = 0;
			
			if (OldLen != Newlen) {
				fInsertSpaces(fInput,WritePos,Newlen - OldLen);
			}
			fclose(fInput);
		}
		if (Input) { free(Input);  Input = NULL; }
		if (Data) {  free(Data);  Data = NULL; }
		return 0;
	} while (result >= 0);
	fclose(fInput);
	if (Input) { free(Input);  Input = NULL; }
	if (Data) {  free(Data);  Data = NULL; }
	return 0;
}

int _WritePrivateProfileString(
  const char * lpAppName,  // pointer to section name
  const char * lpKeyName,  // pointer to key name
  const char * lpString,   // pointer to string to add
  const char * lpFileName  // pointer to initialization filename
) 
{
	char *Input = NULL, *Data = NULL, * Pos, CurrentSection[300];
	int DataLen = 0, DataLeft, result, count;
	static long Fpos = 0;
	long WritePos;
	FILE * fInput;

	fInput = fopen(lpFileName,"r+b");
	if (fInput == NULL) { 
		fInput = fopen(lpFileName,"w+b");
		if (fInput == NULL) { return 0; }
	}
	CurrentSection[0] = 0;
	
	//test to see if Fpos is same as current section;
/*	fseek(fInput,Fpos,SEEK_SET);
	result = fGetString2(fInput,&Input,&Data,&DataLen,&DataLeft);
	if (result > 1) {
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
		if (strlen(Input) > 1) {
			if (Input[0] == '[' && Input[strlen(Input) - 1] == ']') {
				strcpy(CurrentSection,&Input[1]);
				CurrentSection[strlen(CurrentSection) - 1] = 0;
			}
		}
	}
	if (strcmp(lpAppName,CurrentSection) != 0) { 
		DataLen = 0;
		DataLeft = 0;
		free(Data);
		Data = NULL;
		fseek(fInput,0,SEEK_SET);
	}*/

	do {
		if (strcmp(lpAppName,CurrentSection) != 0) { 
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
			if (strcmp(lpAppName,CurrentSection) == 0) { 
				result = -1;
				continue;
			}
			strcpy(CurrentSection,&Input[1]);
			CurrentSection[strlen(CurrentSection) - 1] = 0;
			WritePos = ftell(fInput) - DataLeft;
			continue;
		}
		if (strcmp(lpAppName,CurrentSection) != 0) { 
			continue;
		}
		Pos = strchr(Input,'=');
		if (Pos == NULL) { continue; }
		Pos[0] = 0;
		if (strcmp(Input,lpKeyName) != 0) { 
			WritePos = ftell(fInput) - DataLeft;
			continue;
		}
		{
			long OldLen = (ftell(fInput) - DataLeft) - WritePos;
			int Newlen = strlen(lpKeyName) + strlen(lpString) + strlen(LineFeed) + 1;
			
			if (OldLen != Newlen) {
				fInsertSpaces(fInput,WritePos,Newlen - OldLen);
			}

			fseek(fInput,WritePos,SEEK_SET);
			fprintf(fInput,"%s=%s%s",lpKeyName,lpString,LineFeed);
			fflush(fInput);
			if ((Newlen - OldLen < 0)) {
				//copy all file excluding size diff
			}
			fclose(fInput);
		}
		if (Input) { free(Input);  Input = NULL; }
		if (Data) {  free(Data);  Data = NULL; }
		return 0;
	} while (result >= 0);
	if (strcmp(lpAppName,CurrentSection) == 0) {
		int len = strlen(lpKeyName) + strlen(lpString) + strlen(LineFeed) + 1;
		fInsertSpaces(fInput,WritePos,len);
		fseek(fInput,WritePos,SEEK_SET);
		fprintf(fInput,"%s=%s%s",lpKeyName,lpString,LineFeed);
		fflush(fInput);
		fclose(fInput);
	} else {
		fseek(fInput,0,SEEK_END);
		fprintf(fInput,"%s[%s]%s%s=%s%s",LineFeed,lpAppName,LineFeed,
			lpKeyName,lpString,LineFeed);
		fflush(fInput);
		fclose(fInput);
	}
	if (Input) { free(Input);  Input = NULL; }
	if (Data) {  free(Data);  Data = NULL; }
	return 0;
}
 
//_GetPrivateProfileSectionNames

unsigned int _GetPrivateProfileSectionNames(									//added by Witten
  const char * lpszReturnBuffer,	// address of return buffer
  DWORD nSize,						// size of destination buffer
  const char * lpFileName			// address of initialization filename
) 
{	

	static long Fpos = 0;
	FILE * fInput;
	char String[2048], Section[256];
	int pos = 0;
	char *buf;
	char *seekpos;

	if (lpFileName == NULL) { return (unsigned int)&Fpos; }
	
	fInput = fopen(lpFileName,"rb");
	if (fInput == NULL) { goto GetPrivateProfileSectionNames_ReturnDefault; }

	buf = (char *)lpszReturnBuffer;
	
	memset(buf,'\n',nSize);
	memset(String,'\0',sizeof(String));
	memset(Section,'\0',sizeof(Section));

	fseek(fInput,Fpos,SEEK_SET);
	while (fgets(String, 1024, fInput) != NULL) {
		if (String[0] == '[') {
			if(String[strlen(String)-3] == ']') {
				strncpy(buf+pos, String+1, strlen(String)-4);
				pos += strlen(String)-3;
			}
		}		
		memset(String,'\0',sizeof(String));
	}

	while (strchr(buf, '\n') != NULL) {	//replace all newline characters by null characters
		seekpos = strrchr(buf, '\n');
		strncpy(seekpos,"\0",1);

	}

GetPrivateProfileSectionNames_ReturnDefault:
	return 0;
}

