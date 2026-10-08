#ifndef CURLINC_MPRINTF_H
#define CURLINC_MPRINTF_H

#include <stdarg.h>
#include <stdio.h> 
#include "curl.h"  

#ifdef __cplusplus
extern "C"
{
#endif

    CURL_EXTERN int curl_mprintf(const char* format, ...);
    CURL_EXTERN int curl_mfprintf(FILE* fd, const char* format, ...);
    CURL_EXTERN int curl_msprintf(char* buffer, const char* format, ...);
    CURL_EXTERN int curl_msnprintf(char* buffer, size_t maxlength, const char* format, ...);
    CURL_EXTERN int curl_mvprintf(const char* format, va_list args);
    CURL_EXTERN int curl_mvfprintf(FILE* fd, const char* format, va_list args);
    CURL_EXTERN int curl_mvsprintf(char* buffer, const char* format, va_list args);
    CURL_EXTERN int curl_mvsnprintf(char* buffer, size_t maxlength, const char* format, va_list args);
    CURL_EXTERN char* curl_maprintf(const char* format, ...);
    CURL_EXTERN char* curl_mvaprintf(const char* format, va_list args);

#ifdef __cplusplus
}
#endif

#endif 
