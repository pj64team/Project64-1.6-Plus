#include <sys/stat.h>
#include <stdio.h>
#include <Windows.h>
#include "Settings Api_2.h"
#include "SectionIndexer.h"
#include "SectionBuffer.h"

FileIndexer *files = NULL;
int indexed_count = 0;

Buffer SectionBuffer;

int IndexNames(const char *FileName) {
#define buffer_size 0x1000
#define line_size 23
	FILE *fp = fopen(FileName, "rb");
	int i, index, amt_read, length;
	char chunk[buffer_size];
	char line[line_size];
	char *pos1 = NULL, *pos2 = NULL;
	struct stat st;
	FileIndexer fi;

	// Could not open the file for reading, bail.
	if (fp == NULL) return -1;

	stat(FileName, &st);

	// Check to see if the file is already indexed.
	for (i = 0, index = -1; i < indexed_count; i++) { 
		// File is indexed, check if it is up to date.
		if (strcmp(files[i].filename, FileName) == 0) {
			if (st.st_mtime == files[i].last_modified) {
				fclose(fp);
				return i;
			} else {
				FreeIndexEntry(&files[i]);
				index = i;
				break;
			}
		}
	}

	Init_Index(&fi);

	// Last modified time and the file name serve to see if the index should be used or not.
	fi.last_modified = st.st_mtime;

	if ((fi.filename = (char *)malloc(strlen(FileName) + 1)) == NULL) {
		fclose(fp);
		return -1;
	}
	strncpy(fi.filename, FileName, strlen(FileName) + 1);

	while (!feof(fp)) {
		amt_read = fread(chunk, sizeof(char), buffer_size, fp);

		for (i = 0; i < amt_read; i++) {
			
			// Find the '[' inside the chunk, if none exists get more data from the file.
			if ((pos1 = strchr(chunk + i, '[')) == NULL)
				break;

			// No ']' inside the current chunk but it may still be valid if we are at the end of the chunk
			//	so set the file position to where the [ is at.
			if ((pos2 = strchr(pos1, ']')) == NULL) {
				fseek(fp, ftell(fp) - amt_read + (int)(pos1 - chunk), SEEK_SET);
				break;
			}
			
			// An identifier is 21 or 22 characters long, continue if this is not the case.
			length = (int)(pos2 - pos1) - 1;
			if (length != 21 && length != 22) {
				i++;
				continue;
			}

			// Getting here means this may be an indentifier, check to make sure it is before continuing.
			if (*(pos1 + 9) == '-' && strncmp((pos1 + 18), "-C:", 3) == 0) {
				strncpy(line, pos1 + 1, length);
				line[length] = '\0';

				i = (int)(pos2 - chunk);
				AddID(&fi, line, (ftell(fp) - amt_read) + (int)(pos2 - chunk) + 1);
			}
		}
	}

	fclose(fp);

	// An index of -1 means the index is new, allocate memory for it's storage.
	if (index == -1) {
		FileIndexer *temp = (FileIndexer *)realloc(files, (indexed_count + 1) * sizeof(FileIndexer));

		// Failed to allocate memory, bail.
		if (temp == NULL)
			return -1;
		
		files = temp;

		index = indexed_count;
		indexed_count++;
	}

	files[index] = fi;

	return index;
}

void FillSection(const char *ID, const char *FileName) { 
	FILE *fp;
	struct stat tm;
	int index = 0, loc, length;
	long int position, position2;
	char *readbuffer;

	stat(FileName, &tm);

	// Create or fetch the index for this file.
	index = IndexNames(FileName);
	
	// Either the file does not exist or it could not be opened, reset the buffer and its variables.
	if (index == -1) {
		// Clear out the old entries in the buffer
		FreeBuffer(&SectionBuffer);
		return;
	}
	
	// Buffer is up to date, it is the same file, id, and last modified time.
	if (strcmp(files[index].filename, FileName) == 0 &&
		strcmp(SectionBuffer.identifier, ID) == 0 &&
		SectionBuffer.last_modified == tm.st_mtime)
		return;

	// Find the ID's location inside the index, this returns -1 if the ID is not found.
	loc = FindID(&files[index], ID);

	if (loc == -1) {
		FreeBuffer(&SectionBuffer);
		return;
	}

	// Need to fill the buffer with the lines.
	fp = fopen(FileName, "rb");

	// The last entry will simply read until the end.
	if (loc == files[index].count - 1) {
		fseek(fp, 0, SEEK_END);
		position2 = ftell(fp);
	} else {
		// Read up to the position of the next identifier, since this will include the indentifier remove it's length.
		// 2 is added to the identifier to take into account the brackets at the start and end [ ] in the file.
		position2 = files[index].locations[loc + 1] - (strlen(files[index].identifiers[loc + 1]) + 2);
	}
	
	position = files[index].locations[loc];
	fseek(fp, position, SEEK_SET);
	
	length = position2 - position;

	// Attempt to allocate enough memory to fit length + 1 characters
	if ((readbuffer = (char*)malloc(length + 1)) == NULL) {
		fclose(fp);
		FreeBuffer(&SectionBuffer);
		return;
	}

	fread(readbuffer, sizeof(char), length, fp);
	readbuffer[length] = '\0';
	fclose(fp);

	FillBuffer(readbuffer, &SectionBuffer);
	free(readbuffer);
	
	strcpy(SectionBuffer.filename, FileName);
	strcpy(SectionBuffer.identifier, ID);
	SectionBuffer.last_modified = tm.st_mtime;
}

void GetString (
  const char * lpAppName,        // points to section name
  const char * lpKeyName,        // points to key name
  const char * lpDefault,        // points to default string
  char *       lpReturnedString, // points to destination buffer
  unsigned int nSize,            // size of destination buffer
  const char * lpFileName        // points to initialization filename
  )
{
	char *pos1;
	int i, length;

	// Ensure the buffer is pointing to a valid entry, it will be NULL if the entry does not exist.
	FillSection(lpAppName, lpFileName);

	for (i = 0; i < SectionBuffer.rows; i++) {
		if (strncmp(SectionBuffer.buffer[i], lpKeyName, strlen(lpKeyName)) == 0) {
			pos1 = strchr(SectionBuffer.buffer[i], '=');

			if (pos1 == NULL || (strlen(pos1 + 1) == 0))
				break;

			length = strlen(pos1 + 1);

			strncpy(lpReturnedString, pos1 + 1, length);
			lpReturnedString[length] = '\0';
			return;
		}
	}

	// Either the file does not exist, could not be read, or the entry is missing.
	strncpy(lpReturnedString, lpDefault, nSize);
}

void FreeIndex() {
	FreeIndices(files, indexed_count);
	files = NULL;
	indexed_count = 0;
	FreeBuffer(&SectionBuffer);
}