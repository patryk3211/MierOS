#ifndef _MIEROS_KERNEL_SECTIONS_H
#define _MIEROS_KERNEL_SECTIONS_H

#define SECTION(s) __attribute__((section(s)))

#define FREE_AFTER_INIT __attribute__((section(".free_after_init"), visibility("hidden")))
#define TEXT_FREE_AFTER_INIT __attribute__((section(".text_free_after_init"), visibility("hidden")))

#define PACKED __attribute__((packed))

#define NO_EXPORT __attribute__((visibility("hidden")))
#define EXPORT __attribute__((visibility("default")))

#define MODULE_HEADER __attribute__((section(".modulehdr"), used))

#define USED __attribute__((used))

#define UNUSED(x) (void)(x)

#endif // _MIEROS_KERNEL_SECTIONS_H
