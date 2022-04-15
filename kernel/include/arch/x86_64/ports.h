#ifndef _MIEROS_KERNEL_ARCH_X86_64_PORTS_H
#define _MIEROS_KERNEL_ARCH_X86_64_PORTS_H

#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

inline void outb(u16_t port, u8_t data) {
    asm volatile("outb %0, %1"
                 :
                 : "a"(data), "Nd"(port));
}

inline u8_t inb(u16_t port) {
    u8_t data;
    asm volatile("inb %1, %0"
                 : "=a"(data)
                 : "Nd"(port));
    return data;
}

inline void outw(u16_t port, u16_t data) {
    asm volatile("outw %0, %1"
                 :
                 : "a"(data), "Nd"(port));
}

inline u16_t inw(u16_t port) {
    u16_t data;
    asm volatile("inw %1, %0"
                 : "=a"(data)
                 : "Nd"(port));
    return data;
}

inline void outl(u16_t port, u32_t data) {
    asm volatile("outl %0, %1"
                 :
                 : "a"(data), "Nd"(port));
}

inline u32_t inl(u16_t port) {
    u32_t data;
    asm volatile("inl %1, %0"
                 : "=a"(data)
                 : "Nd"(port));
    return data;
}

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_ARCH_X86_64_PORTS_H
