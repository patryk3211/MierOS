#include <util/uuid.h>
#include <stdlib.h>

extern "C" uuid_t str_to_uuid(const char* str) {
    if(strlen(str) < 36) return { };

    auto nextByte = [](const char* str) -> u16_t {
        auto decodeChar = [](char c) -> u8_t {
            if(c >= '0' && c <= '9') return c - '0';
            if(c >= 'a' && c <= 'f') return c - 'a' + 10;
            if(c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };

        u8_t c1 = decodeChar(str[0]);
        u8_t c2 = decodeChar(str[1]);
        if(c1 == -1 || c2 == -1) return -1;

        return (c1 << 4) | c2;
    };

    uuid_t uuid;

    size_t byte = 0;
    const char* current = str;
    for(size_t i = 0; i < 36; i += 2) {
        if(i == 8 || i == 13 || i == 18 || i == 23) ++i;

        u16_t value = nextByte(str + i);
        if(value > 255) return { };

        uuid.bytes[byte++] = value;
    }

    return uuid;
}
