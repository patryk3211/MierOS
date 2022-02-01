#pragma once

#include <types.h>
#include <string.hpp>

namespace std {
    template<typename T> struct less { bool operator()(const T& a, const T& b) const { return a < b; } };
    template<typename T> struct greater { bool operator()(const T& a, const T& b) const { return a > b; } };
    template<typename T> struct equal_to { bool operator()(const T& a, const T& b) const { return a == b; } };

    template<typename T> struct hash;
    template<> struct hash<const char*> { size_t operator()(const char* key) {
        const int p = 31;
        const int m = 1e9 + 9;
        
        size_t pp = 1;
        size_t hash = 0;
        for(size_t i = 0; key[i] != 0; i++) {
            hash = (hash + (key[i] - 'a' + 1) * pp) % m;
            pp = (pp * p) % m;
        }
        return (hash%m + m) % m;
    } };
    template<> struct hash<String<>> { size_t operator()(const String<>& key) {
        const int p = 31;
        const int m = 1e9 + 9;
        
        size_t pp = 1;
        size_t hash = 0;
        for(size_t i = 0; i < key.length(); i++) {
            hash = (hash + (key[i] - 'a' + 1) * pp) % m;
            pp = (pp * p) % m;
        }
        return (hash%m + m) % m;
    } };
    template<> struct hash<int> { size_t operator()(const int& key) {
        size_t x = ((key >> 16) ^ key) * 0x45d9f3b;
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = (x >> 16) ^ x;
        return x;
    } };
    template<> struct hash<short unsigned int> { size_t operator()(const short unsigned int& key) {
        size_t x = ((key >> 8) ^ key) * 0x45d9f3b;
        x = ((x >> 8) ^ x) * 0x45d9f3b;
        x = (x >> 8) ^ x;
        return x;
    } };
    template<> struct hash<unsigned int> { size_t operator()(const unsigned int& key) {
        size_t x = ((key >> 16) ^ key) * 0x45d9f3b;
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = (x >> 16) ^ x;
        return x;
    } };
}
