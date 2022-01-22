#ifndef _MIEROS_KERNEL_STDLIB_H
#define _MIEROS_KERNEL_STDLIB_H

#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern int atoi(const char* str);

extern void memset(void* ptr, int val, size_t count);
extern void memcpy(void* dst, const void* src, size_t count);

extern int strcmp(const char* a, const char* b);
extern size_t strlen(const char* str);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_STDLIB_H
