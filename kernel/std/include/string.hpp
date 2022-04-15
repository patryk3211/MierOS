#pragma once

#include <allocator.hpp>
#include <stdlib.h>

namespace std {
    template<typename C = char, class Alloc = heap_allocator> class String {
        Alloc allocator;

        C* characters;

        size_t len;

    public:
        String() {
            characters = allocator.template alloc<C>(1);
            characters[0] = 0;
            len = 0;
        }

        String(const C* init_str) {
            len = strlen(init_str);
            characters = allocator.template alloc<C>(len + 1);
            memcpy(characters, init_str, sizeof(C) * len);
            characters[len] = 0;
        }

        String(const String& init_str) {
            len = init_str.len;
            characters = allocator.template alloc<C>(len + 1);
            memcpy(characters, init_str.characters, sizeof(C) * len);
            characters[len] = 0;
        }

        ~String() {
            allocator.free_array(characters);
        }

        String& operator+=(const String& other) {
            size_t newLen = len + other.len;
            C* newChars = allocator.template alloc<C>(newLen + 1);
            memcpy(newChars, characters, sizeof(C) * len);
            memcpy(newChars + len, other.characters, sizeof(C) * other.len);
            newChars[newLen] = 0;
            allocator.free_array(characters);
            characters = newChars;
            len = newLen;
            return *this;
        }

        String& operator+=(const C* c_str) {
            size_t newLen = len + strlen(c_str);
            C* newChars = allocator.template alloc<C>(newLen + 1);
            memcpy(newChars, characters, sizeof(C) * len);
            memcpy(newChars + len, c_str, sizeof(C) * (newLen - len));
            newChars[newLen] = 0;
            allocator.free_array(characters);
            characters = newChars;
            len = newLen;
            return *this;
        }

        String& operator+=(const C character) {
            size_t newLen = len + 1;
            C* newChars = allocator.template alloc<C>(newLen + 1);
            memcpy(newChars, characters, sizeof(C) * len);
            newChars[newLen - 1] = character;
            newChars[newLen] = 0;
            allocator.free_array(characters);
            characters = newChars;
            len = newLen;
            return *this;
        }

        String operator+(const String& other) const {
            String out = String(characters);
            out += other;
            return out;
        }

        String operator+(const C* c_str) const {
            String out = String(characters);
            out += c_str;
            return out;
        }

        String operator+(const C character) const {
            String out = String(characters);
            out += character;
            return out;
        }

        String& operator=(const C* c_str) {
            allocator.free_array(characters);
            len = strlen(c_str);
            characters = allocator.template alloc<C>(len + 1);
            memcpy(characters, c_str, sizeof(C) * len);
            characters[len] = 0;
            return *this;
        }

        String& operator=(const String& str) {
            allocator.free_array(characters);
            len = str.len;
            characters = allocator.template alloc<C>(len + 1);
            memcpy(characters, str.characters, sizeof(C) * len);
            characters[len] = 0;
            return *this;
        }

        C& operator[](size_t idx) {
            return characters[idx];
        }

        const C& operator[](size_t idx) const {
            return characters[idx];
        }

        size_t length() const { return len; }
        const C* c_str() const { return characters; }
        bool empty() const { return len == 0; }

        bool operator==(const std::String<C>& other) const {
            if(other.len != len) return false;
            for(size_t i = 0; i < len; ++i)
                if(characters[i] != other.characters[i]) return false;
            return true;
        }
        bool operator==(const C* c_str) const {
            size_t len;
            for(len = 0; c_str[len] != 0; ++len)
                ;
            if(this->len != len) return false;
            for(size_t i = 0; i < len; ++i)
                if(characters[i] != c_str[i]) return false;
            return true;
        }
    };

    template<typename NumType> std::String<> num_to_string(NumType num) {
        int neg = num < 0;
        char buffer[80];
        int index = 0;
        do {
            int digit = num % 10;
            num /= 10;
            buffer[index++] = '0' + digit;
        } while(num > 0);
        char fin[index + neg + 1];
        for(int i = 0; i < index; i++) fin[i + neg] = buffer[index - i - 1];
        if(neg) fin[0] = '-';

        fin[index + neg] = 0;
        return String(fin);
    }

    inline std::String<> uuid_to_string(uuid_t uuid) {
        const char lookup[] = "0123456789abcdef";
        u8_t* as_bytes = (u8_t*)&uuid;

        char buffer[37];
        buffer[0] = lookup[as_bytes[0] >> 4];
        buffer[1] = lookup[as_bytes[0] & 15];
        buffer[2] = lookup[as_bytes[1] >> 4];
        buffer[3] = lookup[as_bytes[1] & 15];
        buffer[4] = lookup[as_bytes[2] >> 4];
        buffer[5] = lookup[as_bytes[2] & 15];
        buffer[6] = lookup[as_bytes[3] >> 4];
        buffer[7] = lookup[as_bytes[3] & 15];

        buffer[8] = '-';

        buffer[9] = lookup[as_bytes[4] >> 4];
        buffer[10] = lookup[as_bytes[4] & 15];
        buffer[11] = lookup[as_bytes[5] >> 4];
        buffer[12] = lookup[as_bytes[5] & 15];

        buffer[13] = '-';

        buffer[14] = lookup[as_bytes[6] >> 4];
        buffer[15] = lookup[as_bytes[6] & 15];
        buffer[16] = lookup[as_bytes[7] >> 4];
        buffer[17] = lookup[as_bytes[7] & 15];

        buffer[18] = '-';

        buffer[19] = lookup[as_bytes[8] >> 4];
        buffer[20] = lookup[as_bytes[8] & 15];
        buffer[21] = lookup[as_bytes[9] >> 4];
        buffer[22] = lookup[as_bytes[9] & 15];

        buffer[23] = '-';

        buffer[24] = lookup[as_bytes[10] >> 4];
        buffer[25] = lookup[as_bytes[10] & 15];
        buffer[26] = lookup[as_bytes[11] >> 4];
        buffer[27] = lookup[as_bytes[11] & 15];
        buffer[28] = lookup[as_bytes[12] >> 4];
        buffer[29] = lookup[as_bytes[12] & 15];
        buffer[30] = lookup[as_bytes[13] >> 4];
        buffer[31] = lookup[as_bytes[13] & 15];
        buffer[32] = lookup[as_bytes[14] >> 4];
        buffer[33] = lookup[as_bytes[14] & 15];
        buffer[34] = lookup[as_bytes[15] >> 4];
        buffer[35] = lookup[as_bytes[15] & 15];

        buffer[36] = 0;

        return std::String<>(buffer);
    }
}
