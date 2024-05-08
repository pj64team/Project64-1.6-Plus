#include <Windows.h>
#include "SectionBuffer.h"

char* TrimString(char *start, char *end) {
	char *string, *comments;

	if (start >= end)
		return "";

	comments = strstr(start, "//");

	if (comments != NULL && end >= comments)
		end = comments;

	if (end == start)
		return "";

	while(end[-1] <= ' ')
		end--;

	if ((string = (char *)malloc((int)(end - start) + 1)) == NULL)
		return "";

	strncpy(string, start, (int)(end - start));
	string[(int)(end-start)] = '\0';

	return string;
}

void FillBuffer(char *string, Buffer *SectionBuffer) {
	BOOL finished = FALSE;
	const int increase_size = 10;
	char *pos1, *pos2, *hold;

	FreeBuffer(SectionBuffer);

	pos1 = string;

	while(pos1 != NULL && strlen(pos1) != 0) {

		// Skip any beginning white space such as \r, \n, and spaces.
		// This also includes most non-printable characters.
		if (pos1[0] <= ' ') {
			pos1++;
			continue;
		}

		// Allocate more memory for the buffer if it is needed
		if (SectionBuffer->rows == SectionBuffer->max_allocated) {
			char **increase = (char **)realloc(SectionBuffer->buffer, sizeof(char *) * (SectionBuffer->max_allocated + increase_size));

			if (increase == NULL)
				return;

			SectionBuffer->max_allocated += increase_size;
			SectionBuffer->buffer = increase;
		}

		pos2 = strchr(pos1, '\n');

		if (pos2 == NULL) 
			return;

		hold = TrimString(pos1, pos2);

		if (!strlen(hold) == 0) {
			SectionBuffer->buffer[SectionBuffer->rows] = hold;
			SectionBuffer->rows++;
		}

		pos1 = pos2;
	}
}

void FreeBuffer(Buffer *SectionBuffer) {

	while (SectionBuffer->rows != 0) {
		SectionBuffer->rows--;
		free(SectionBuffer->buffer[SectionBuffer->rows]);
		SectionBuffer->buffer[SectionBuffer->rows] = NULL;		
	}

	free(SectionBuffer->buffer);
	SectionBuffer->buffer = NULL;

	strcpy(SectionBuffer->filename, "");
	strcpy(SectionBuffer->identifier, "");

	SectionBuffer->max_allocated = 0;
	SectionBuffer->last_modified = 0;	
}