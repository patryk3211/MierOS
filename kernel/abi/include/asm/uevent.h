#ifndef _ASM_UEVENT_H
#define _ASM_UEVENT_H

#if defined(__cplusplus)
extern "C" {
#endif

#define UEVENT_NULL         0
#define UEVENT_LOAD_MODULE  1

#define UEVENT_ARGT_NULL    0
#define UEVENT_ARGT_STR     1

struct uevent_arg {
    unsigned char type;
    unsigned char reserved;
    unsigned short size;

    char name[64];

    char value[];
}__attribute__((packed));

struct uevent {
    unsigned long id;
    unsigned int type;
    unsigned int size;
    
    unsigned int argc;
    struct uevent_arg argv[];
}__attribute__((packed));

#if defined(__cplusplus)
}
#endif

#endif

