#ifndef _MIEROS_KERNEL_DMESG_H
#define _MIEROS_KERNEL_DMESG_H

#include <stdarg.h>
#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern void init_serial();

extern void dmesg(const char* format, ...);

extern _Noreturn void panic(const char* msg);

//#undef DEBUG
#ifdef DEBUG
#define TRACE(args...) dmesg("{T} " args)
#else
#define TRACE(args...)
#endif

#if defined(__cplusplus)
}
#endif

#endif
