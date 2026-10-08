#ifndef CURLINC_STDCHEADERS_H
#define CURLINC_STDCHEADERS_H

#include <sys/types.h>

size_t fread(void*, size_t, size_t, FILE*);
size_t fwrite(const void*, size_t, size_t, FILE*);

int strcasecmp(const char*, const char*);
int strncasecmp(const char*, const char*, size_t);

#endif 
