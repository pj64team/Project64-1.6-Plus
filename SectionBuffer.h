#ifndef SI_FILEBUFFER
#define SI_FILEBUFFER

typedef struct {
	char **buffer;

	char filename[255];
	char identifier[23];

	int rows;
	int max_allocated;
	time_t last_modified;
} Buffer;

char* TrimString(char *start, char *end);
void FillBuffer(char *string, Buffer *SectionBuffer);
void FreeBuffer(Buffer *SectionBuffer);

#endif