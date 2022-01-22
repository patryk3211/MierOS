#pragma once

namespace std {
    template<typename K, typename V> struct Pair {
        K key;
        V value;
    
        Pair(const K& key, const V& value) : key(key), value(value) { }
        ~Pair() = default;
    };
}
