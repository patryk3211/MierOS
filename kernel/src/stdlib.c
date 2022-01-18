#include <stdlib.h>

int atoi(const char* str) {
    int sign = 1;
    if(*str == '-') {
        sign = -1;
        ++str;
    } else if(*str == '+') ++str;
    int final_num = 0;
    while(*str >= '0' && *str <= '9') final_num = final_num*10+(*str++ - '0');
    return final_num*sign;
}

void memset(void* ptr, int val, size_t count) {
    for(size_t i = 0; i < count; ++i) ((u8_t*)ptr)[i] = val;
}

void memcpy(void* dst, const void* src, size_t count) {
    for(size_t i = 0; i < count; ++i) ((u8_t*)dst)[i] = ((u8_t*)src)[i];
}
