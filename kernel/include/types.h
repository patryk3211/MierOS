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
    u8_t part1[4];
    u8_t part2[2];
    u8_t part3[2];
    u8_t part4[2];
    u8_t part5[6];

    uuid_t() = default;
    uuid_t(u32_t part1, u16_t part2, u16_t part3, u16_t part4, u64_t part5) {
        this->part1[0] = part1 >> 24;
        this->part1[1] = part1 >> 16;
        this->part1[2] = part1 >> 8;
        this->part1[3] = part1;

        this->part2[0] = part2 >> 8;
        this->part2[1] = part2;

        this->part3[0] = part3 >> 8;
        this->part3[1] = part3;

        this->part4[0] = part4 >> 8;
        this->part4[1] = part4;

        this->part5[0] = part5 >> 40;
        this->part5[1] = part5 >> 32;
        this->part5[2] = part5 >> 24;
        this->part5[3] = part5 >> 16;
        this->part5[4] = part5 >> 8;
        this->part5[5] = part5;
    }

    u32_t get_part1() { return (part1[0] << 24) | (part1[1] << 16) | (part1[2] << 8) | part1[3]; }
    u16_t get_part2() { return (part2[0] << 8) | part2[1]; }
    u16_t get_part3() { return (part3[0] << 8) | part3[1]; }
    u16_t get_part4() { return (part4[0] << 8) | part4[1]; }
    u64_t get_part5() { return ((u64_t)part5[0] << 40) | ((u64_t)part5[1] << 32) | ((u64_t)part5[2] << 24) | ((u64_t)part5[3] << 16) | ((u64_t)part5[4] << 8) | part5[5]; }
};
#endif

#endif
