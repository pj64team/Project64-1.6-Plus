#ifndef SI_FILEINDEX
#define SI_FILEINDEX

typedef struct {
	// These are for the entries
	char **identifiers;
	long int *locations;
	int count;
	int max_allocated;

	// These identify what file the entries are for
	char *filename;
	time_t last_modified;
} FileIndexer;

void Init_Index(FileIndexer *fi);
void AddID(FileIndexer *fi, const char *id, long int location);
int FindID(FileIndexer *fi, const char *ID);

void FreeIndexEntry(FileIndexer *fi);
void FreeIndices(FileIndexer *files, int count);

#endif