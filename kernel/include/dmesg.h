#ifndef _MIEROS_KERNEL_DMESG_H
#define _MIEROS_KERNEL_DMESG_H

#include <stdarg.h>
#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern void init_serial();

extern void dmesg(const char* msg);
extern void dmesgl(const char* msg, size_t length);

extern void kprintf(const char* format, ...);
extern void va_kprintf(const char* format, va_list args);

extern void panic(const char* msg);

#if defined(__cplusplus)
}
#endif

#endif
