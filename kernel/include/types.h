#ifndef _MIEROS_KERNEL_TYPES_H
#define _MIEROS_KERNEL_TYPES_H

#if defined(__cplusplus)
extern "C" {
#endif

typedef unsigned char       u8_t;
typedef unsigned short      u16_t;
typedef unsigned int        u32_t;
typedef unsigned long long  u64_t;

typedef signed char         s8_t;
typedef signed short        s16_t;
typedef signed int          s32_t;
typedef signed long long    s64_t;

typedef unsigned long size_t;
typedef signed long ssize_t;

typedef u64_t physaddr_t;
typedef u64_t virtaddr_t;

typedef u32_t pid_t;

typedef u64_t time_t;

typedef u32_t fd_t;

#if !defined(__cplusplus)
typedef struct uuid_t {
    u8_t part1[4];
    u8_t part2[2];
    u8_t part3[2];
    u8_t part4[2];
    u8_t part5[6];
} uuid_t;
#endif

#if defined(__cplusplus)
}

struct uuid_t {
    u8_t bytes[16];

    uuid_t() = default;
    uuid_t(u32_t part1, u16_t part2, u16_t part3, u16_t part4, u64_t part5) {
        this->bytes[0] = part1 >> 24;
        this->bytes[1] = part1 >> 16;
        this->bytes[2] = part1 >> 8;
        this->bytes[3] = part1;

        this->bytes[4] = part2 >> 8;
        this->bytes[5] = part2;

        this->bytes[6] = part3 >> 8;
        this->bytes[7] = part3;

        this->bytes[8] = part4 >> 8;
        this->bytes[9] = part4;

        this->bytes[10] = part5 >> 40;
        this->bytes[11] = part5 >> 32;
        this->bytes[12] = part5 >> 24;
        this->bytes[13] = part5 >> 16;
        this->bytes[14] = part5 >> 8;
        this->bytes[15] = part5;
    }

    u32_t get_part1() { return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3]; }
    u16_t get_part2() { return (bytes[4] << 8) | bytes[5]; }
    u16_t get_part3() { return (bytes[6] << 8) | bytes[7]; }
    u16_t get_part4() { return (bytes[8] << 8) | bytes[9]; }
    u64_t get_part5() { return ((u64_t)bytes[10] << 40) | ((u64_t)bytes[11] << 32) | ((u64_t)bytes[12] << 24) | ((u64_t)bytes[13] << 16) | ((u64_t)bytes[14] << 8) | bytes[15]; }
};
#endif

#endif
