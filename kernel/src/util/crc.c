#include <util/crc.h>

static inline u32_t reflect32(u32_t value) {
    u32_t ret = 0;
    for(int i = 0; i < 32; ++i) {
        ret = (ret << 1) | (value & 1);
        value >>= 1;
    }
    return ret;
}

static inline u8_t reflect8(u8_t value) {
    u8_t ret = 0;
    for(int i = 0; i < 8; ++i) {
        ret = (ret << 1) | (value & 1);
        value >>= 1;
    }
    return ret;
}

#define CRC32_POLYNOMIAL 0x04C11DB7
u32_t calc_crc32(const void* data, size_t length) {
    u32_t remainder = -1;

    const u8_t* data_byte = (const u8_t*)data;

    for(size_t i = 0; i < length; ++i) {
        remainder ^= reflect8(data_byte[i]) << 24;

        for(size_t j = 0; j < 8; ++j) {
            if(remainder & 0x80000000) remainder = (remainder << 1) ^ CRC32_POLYNOMIAL;
            else remainder <<= 1;
        }
    }

    return reflect32(~remainder);
}
