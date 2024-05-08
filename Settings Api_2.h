#ifndef SETTINGS_API_2
#define SETTINGS_API_2

int IndexNames(const char *FileName);
void FillSection(const char *ID, const char *FileName);
void GetString (
  const char * lpAppName,        // points to section name
  const char * lpKeyName,        // points to key name
  const char * lpDefault,        // points to default string
  char *       lpReturnedString, // points to destination buffer
  unsigned int nSize,            // size of destination buffer
  const char * lpFileName        // points to initialization filename
  );

void FreeIndex();

#endif