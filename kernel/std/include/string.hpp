#pragma once

#include <allocator.hpp>
#include <stdlib.h>

namespace std {
    template<typename C = char, class Alloc = heap_allocator> class string {
        Alloc allocator;

        C* characters;

        size_t len;
    public:
        string() {
            characters = allocator.template alloc<C>(1);
            characters[0] = 0;
            len = 0;
        }

        string(const C* init_str) {
            len = strlen(init_str);
            characters = allocator.template alloc<C>(len+1);
            memcpy(characters, init_str, sizeof(C)*len);
            characters[len] = 0;
        }

        string(const string& init_str) {
            len = init_str.len;
            characters = allocator.template alloc<C>(len+1);
            memcpy(characters, init_str.characters, sizeof(C)*len);
            characters[len] = 0;
        }

        ~string() {
            allocator.free_array(characters);
        }

        string& operator+=(const string& other) {
            size_t newLen = len + other.len;
            C* newChars = allocator.template alloc<C>(newLen+1);
            memcpy(newChars, characters, sizeof(C)*len);
            memcpy(newChars+len, other.characters, sizeof(C)*other.len);
            newChars[newLen] = 0;
            allocator.free_array(characters);
            characters = newChars;
            len = newLen;
            return *this;
        }

        string& operator+=(const C* c_str) {
            size_t newLen = len + strlen(c_str);
            C* newChars = allocator.template alloc<C>(newLen+1);
            memcpy(newChars, characters, sizeof(C)*len);
            memcpy(newChars+len, c_str, sizeof(C)*(newLen-len));
            newChars[newLen] = 0;
            allocator.free_array(characters);
            characters = newChars;
            len = newLen;
            return *this;
        }

        string& operator+=(const C character) {
            size_t newLen = len + 1;
            C* newChars = allocator.template alloc<C>(newLen+1);
            memcpy(newChars, characters, sizeof(C)*len);
            newChars[newLen-1] = character;
            newChars[newLen] = 0;
            allocator.free_array(characters);
            characters = newChars;
            len = newLen;
            return *this;
        }

        string operator+(const string& other) const {
            string out = string(characters);
            out += other;
            return out;
        }

        string operator+(const C* c_str) const {
            string out = string(characters);
            out += c_str;
            return out;
        }

        string operator+(const C character) const {
            string out = string(characters);
            out += character;
            return out;
        }

        string& operator=(const C* c_str) {
            allocator.free_array(characters);
            len = strlen(c_str);
            characters = allocator.template alloc<C>(len+1);
            memcpy(characters, c_str, sizeof(C)*len);
            characters[len] = 0;
            return *this;
        }

        string& operator=(const string& str) {
            allocator.free_array(characters);
            len = str.len;
            characters = allocator.template alloc<C>(len+1);
            memcpy(characters, str.characters, sizeof(C)*len);
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

        bool operator==(const std::string<C>& other) const { 
            if(other.len != len) return false;
            for(size_t i = 0; i < len; ++i) if(characters[i] != other.characters[i]) return false;
            return true;
        }
        bool operator==(const C* c_str) const {
            size_t len;
            for(len = 0; c_str[len] != 0; ++len);
            if(this->len != len) return false;
            for(size_t i = 0; i < len; ++i) if(characters[i] != c_str[i]) return false;
            return true;
        }
    };
}
