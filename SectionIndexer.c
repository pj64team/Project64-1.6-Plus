#include <Windows.h>
#include "SectionIndexer.h"

// Initialization for the FileIndexer struct, just setting things to 0 or null.
void Init_Index(FileIndexer *fi) {
	fi->count = 0;
	fi->max_allocated = 0;
	fi->identifiers = NULL;
	fi->locations = NULL;
	fi->filename = NULL;
}

void AddID(FileIndexer *fi, const char *id, long int location) {

	// Allocate more memory if necessary
	if (fi->count == fi->max_allocated) {
		char **junk;
		long int *junk2;

		// Growing by 20 entries
		fi->max_allocated += 25;

		junk = (char **)realloc(fi->identifiers, fi->max_allocated * (sizeof(char *)));
		junk2 = (long int *)realloc(fi->locations, fi->max_allocated * (sizeof(long int)));

		// Failed to reallocate, bail!
		if (junk == NULL || junk2 == NULL) {
			fi->max_allocated -= 25;
			return;
		}

		fi->identifiers = junk;
		fi->locations = junk2;
	}

	// Fill the entry
	if ((fi->identifiers[fi->count] = (char *)malloc(strlen(id) + 1)) == NULL)
		return;

	strncpy(fi->identifiers[fi->count], id, strlen(id) + 1);
	fi->locations[fi->count] = location;

	fi->count++;
}

int FindID(FileIndexer *fi, const char *ID) {
	int i;

	for(i = 0; i < fi->count; i++) {
		if (strcmp(fi->identifiers[i], ID) == 0)
			return i;
	}

	return -1;
}

void FreeIndexEntry(FileIndexer *fi) {
	int i;
	
	for (i = 0; i < fi->count; i++)
		free(fi->identifiers[i]);

	free(fi->identifiers);
	free(fi->locations);
	free(fi->filename);

	fi->count = 0;
	fi->max_allocated = 0;
	fi->last_modified = 0;
	fi = NULL;
}

void FreeIndices(FileIndexer *files, int count) {

	while (count != 0) {
		count--;
		FreeIndexEntry(&files[count]);
	}

	free(files);
	files = NULL;
}