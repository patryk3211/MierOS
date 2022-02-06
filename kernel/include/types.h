#ifndef _MIEROS_KERNEL_TYPES_H
#define _MIEROS_KERNEL_TYPES_H

#if defined(__cplusplus)
extern "C" {
#endif

typedef unsigned char u8_t;
typedef unsigned short u16_t;
typedef unsigned int u32_t;
typedef unsigned long long u64_t;

typedef unsigned long size_t;

typedef u64_t physaddr_t;
typedef u64_t virtaddr_t;

typedef u32_t pid_t;

typedef u64_t time_t;

#if defined(__cplusplus)
}
#endif

#endif
