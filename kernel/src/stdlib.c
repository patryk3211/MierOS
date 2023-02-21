#include <stdlib.h>

int atoi(const char* str) {
    int sign = 1;
    if(*str == '-') {
        sign = -1;
        ++str;
    } else if(*str == '+')
        ++str;
    int final_num = 0;
    while(*str >= '0' && *str <= '9') final_num = final_num * 10 + (*str++ - '0');
    return final_num * sign;
}

void memset(void* ptr, int val, size_t count) {
    for(size_t i = 0; i < count; ++i) ((u8_t*)ptr)[i] = val;
}

void memcpy(void* dst, const void* src, size_t count) {
    for(size_t i = 0; i < count; ++i) ((u8_t*)dst)[i] = ((u8_t*)src)[i];
}

int strcmp(const char* a, const char* b) {
    while(*a == *b && *a != 0 && *b != 0) {
        a++;
        b++;
    }
    return *a - *b;
}

size_t strlen(const char* str) {
    size_t len;
    for(len = 0; str[len] != 0; ++len)
        ;
    return len;
}

int memcmp(const void* a, const void* b, size_t count) {
    for(size_t i = 0; i < count; ++i)
        if(*((u8_t*)a + i) != *((u8_t*)b + i)) return 0;
    return 1;
}

int strmatch(const char* wildcard, const char* str) {
    const char* text_backup = 0;
    const char* wild_backup = 0;
    while(*str != '\0') {
        if(*wildcard == '*') {
            text_backup = str;
            wild_backup = ++wildcard;
        } else if(*wildcard == '?' || *wildcard == *str) {
            str++;
            wildcard++;
        } else {
            if(wild_backup == 0) return 0;
            str = ++text_backup;
            wildcard = wild_backup;
        }
    }
    while(*wildcard == '*') wildcard++;
    return *wildcard == '\0' ? 1 : 0;
}

char* strchr(const char* str, char c) {
    for(size_t i = 0; c == 0 || str[i] != 0; ++i)
        if(str[i] == c) return (char*)str + i;
    return 0;
}

char* strchrs(const char* str, const char* chars) {
    for(size_t i = 0; str[i] != 0; ++i) {
        for(size_t j = 0; chars[j] != 0; ++j) {
            if(str[i] == chars[j]) return (char*)str + i;
        }
    }
    return 0;
}

char* strnchrs(const char* str, const char* chars) {
    for(size_t i = 0; str[i] != 0; ++i) {
        int found = 0;
        for(size_t j = 0; chars[j] != 0; ++j) {
            if(str[i] == chars[j]) {
                found = 1;
                break;
            }
        }
        if(!found) return (char*)str + i;
    }
    return 0;
}

int strncmp(const char* a, const char* b, size_t len) {
    for(size_t i = 0; i < len; ++i)
        if(a[i] != b[i]) return a[i] - b[i];
    return 0;
}

void strupper(char* str) {
    while(*str != 0) {
        if(*str >= 'a' && *str <= 'z') *str &= ~0x20;
        ++str;
    }
}

char* strfind(const char* str, const char* find) {
    for(size_t i = 0; str[i] != 0; ++i) {
        int matched = 1;
        for(size_t j = 0; find[j] != 0; ++j) {
            if(str[i + j] != find[j]) {
                matched = 0;
                break;
            }
        }
        if(matched) return (char*)str + i;
    }
    return 0;
}

void atexit(void (*exit_handler)()) {
    (void)(exit_handler);
}

void* memfind(void* ptr, const void* value, size_t valueLength, size_t ptrLength) {
    for(size_t i = 0; i < ptrLength - valueLength; ++i) {
        if(memcmp((u8_t*)ptr + i, (const u8_t*)value, valueLength)) {
            return (u8_t*)ptr + i;
        }
    }

    return 0;
}

void strcpy(char* dst, const char* src) {
    while(*src != 0) {
        *dst++ = *src++;
    }
}

char* strlchr(const char* str, char c) {
    const char* ret = 0;
    while(*str) {
        if(*str == c)
            ret = str;
        ++str;
    }

    return (char*)ret;
}

