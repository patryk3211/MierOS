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

int strcmp(const char* a, const char* b) {
    while(*a == *b && *a != 0 && *b != 0) { a++; b++; }
    return *a - *b;
}

size_t strlen(const char* str) {
    size_t len;
    for(len = 0; str[len] != 0; ++len);
    return len;
}

int memcmp(const void* a, const void* b, size_t count) {
    for(size_t i = 0; i < count; ++i)
        if(*((u8_t*)a+i) != *((u8_t*)b+i)) return 0;
    return 1;
}

int strmatch(const char* wildcard, const char* str) {
    const char *text_backup = 0;
    const char *wild_backup = 0;
    while (*str != '\0')  {
        if (*wildcard == '*') {
            text_backup = str;
            wild_backup = ++wildcard;
        } else if (*wildcard == '?' || *wildcard == *str) {
            str++;
            wildcard++;
        } else {
            if (wild_backup == 0) return 0;
            str = ++text_backup;
            wildcard = wild_backup;
        }
    }
    while (*wildcard == '*') wildcard++;
    return *wildcard == '\0' ? 1 : 0;
}

char* strchr(const char* str, char c) {
    for(size_t i = 0; c == 0 || str[i] != 0; ++i)
        if(str[i] == c) return str+i;
    return 0;
}
