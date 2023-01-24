#ifndef _MIEROS_KERNEL_STDLIB_H
#define _MIEROS_KERNEL_STDLIB_H

#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern int atoi(const char* str);

extern void memset(void* ptr, int val, size_t count);
extern void memcpy(void* dst, const void* src, size_t count);
extern int memcmp(const void* a, const void* b, size_t count);
extern void* memfind(void* ptr, const void* value, size_t valueLength, size_t ptrLength);

extern int strcmp(const char* a, const char* b);
extern int strncmp(const char* a, const char* b, size_t len);
extern size_t strlen(const char* str);
extern int strmatch(const char* wildcard, const char* str);
extern char* strchr(const char* str, char c);
extern char* strchrs(const char* str, const char* chars);
extern char* strnchrs(const char* str, const char* chars);
extern void strupper(char* str);
extern char* strfind(const char* str, const char* find);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_STDLIB_H
