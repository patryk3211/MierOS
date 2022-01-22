#pragma once

#include <types.h>
#include <functional.hpp>
#include <allocator.hpp>
#include <stdlib.h>
#include <pair.hpp>
#include <optional.hpp>

namespace std {
    template<typename K, typename V, class Hasher = std::hash<K>, class Pred = std::equal_to<K>, class Alloc = std::heap_allocator> class UnorderedMap {
        static constexpr size_t initial_bucket_size = 64;

        struct Entry {
            K key;
            V value;
            Entry* next;

            Entry();
            Entry(const K& key, const V& value) : key(key), value(value), next(0) { }
            ~Entry() = default;
        };

        Alloc allocator;

        Entry** bucket;
        size_t capacity;
        size_t size;
    public:
        UnorderedMap() : UnorderedMap(initial_bucket_size) { }
        UnorderedMap(size_t initial_capacity) : capacity(initial_capacity) {
            bucket = allocator.template alloc<Entry*>(capacity);
            memset(bucket, 0, sizeof(Entry*)*capacity);
            size = 0;
        }

        ~UnorderedMap() {
            
        }

        bool insert(Pair<K, V> value) {
            Entry* entry = allocator.template alloc<Entry>(value.key, value.value);

            size_t bucket_pos = Hasher{}(value.key) % capacity;

            if(bucket[bucket_pos] == 0) {
                bucket[bucket_pos] = entry;
            } else {
                Entry* last;
                for(Entry* entry = bucket[bucket_pos]; entry != 0; entry = entry->next) {
                    if(Pred{}(value.key, entry->key)) {
                        // Duplicate key
                        allocator.free(entry);
                        return false;
                    }
                    last = entry;
                }
                last->next = entry;
            }
            ++size;
            /// TODO: [22.01.2022] Implement rehashing.
            return true;
        }

        OptionalRef<V> at(const K& key) {
            size_t bucket_pos = Hasher{}(key) % capacity;
            
            if(bucket[bucket_pos] == 0) return {};
            for(Entry* entry = bucket[bucket_pos]; entry != 0; entry = entry->next) {
                if(Pred{}(key, entry->key)) {
                    // This is the value
                    return entry->value;
                }
            }
            return {};
        }

        void erase(const K& key) {
            size_t bucket_pos = Hasher{}(key) % capacity;

            if(bucket[bucket_pos] == 0) return;
            Entry* prev = 0;
            for(Entry* entry = bucket[bucket_pos]; entry != 0; entry = entry->next) {
                if(Pred{}(key, entry->key)) {
                    // Delete this
                    if(prev == 0) bucket[bucket_pos] = entry->next;
                    else prev->next = entry->next;
                    allocator.free(entry);
                    return;
                }
                prev = entry;
            }
        }
    };
}
