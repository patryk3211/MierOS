#ifndef _MIEROS_KERNEL_ARCH_X86_64_INT_HANDLERS_H
#define _MIEROS_KERNEL_ARCH_X86_64_INT_HANDLERS_H

#if defined(__cplusplus)
extern "C" {
#endif

extern void int_ignore();
extern void interrupt0x00();
extern void interrupt0x01();
extern void interrupt0x02();
extern void interrupt0x03();
extern void interrupt0x04();
extern void interrupt0x05();
extern void interrupt0x06();
extern void interrupt0x07();
extern void interrupt0x08();
extern void interrupt0x09();
extern void interrupt0x0A();
extern void interrupt0x0B();
extern void interrupt0x0C();
extern void interrupt0x0D();
extern void interrupt0x0E();
extern void interrupt0x0F();
extern void interrupt0x10();
extern void interrupt0x11();
extern void interrupt0x12();
extern void interrupt0x13();
extern void interrupt0x14();
extern void interrupt0x15();
extern void interrupt0x16();
extern void interrupt0x17();
extern void interrupt0x18();
extern void interrupt0x19();
extern void interrupt0x1A();
extern void interrupt0x1B();
extern void interrupt0x1C();
extern void interrupt0x1D();
extern void interrupt0x1E();
extern void interrupt0x1F();

extern void interrupt0x20();
extern void interrupt0x21();
extern void interrupt0x22();
extern void interrupt0x23();
extern void interrupt0x24();
extern void interrupt0x25();
extern void interrupt0x26();
extern void interrupt0x27();
extern void interrupt0x28();
extern void interrupt0x29();
extern void interrupt0x2A();
extern void interrupt0x2B();
extern void interrupt0x2C();
extern void interrupt0x2D();
extern void interrupt0x2E();
extern void interrupt0x2F();

extern void interrupt0x8F();

extern void interrupt0xFE();

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_ARCH_X86_64_INT_HANDLERS_H
