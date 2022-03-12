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

            Entry() : next(0) { }
            Entry(const K& key, const V& value) : key(key), value(value), next(0) { }
            ~Entry() = default;
        };

        Alloc allocator;

        Entry** bucket;
        size_t capacity;
        size_t _size;
    public:
        class iterator {
            size_t index;
            Entry* entry;
            UnorderedMap& map;

            iterator(size_t index, Entry* entry, UnorderedMap& map) : index(index), entry(entry), map(map) { }
        public:
            ~iterator() = default;

            iterator& operator++() {
                if(entry->next == 0) {
                    Entry* new_entry = 0;
                    ++index;
                    while(index < map.capacity && new_entry == 0)
                        new_entry = map.bucket[index++];
                    entry = new_entry;
                    if(entry != 0) --index;
                } else entry = entry->next;
                return *this;
            }

            iterator operator++(int) {
                iterator copy = iterator(index, entry);
                ++this;
                return copy;
            }

            Pair<K&, V&> operator*() {
                return { entry->key, entry->value };
            }

            Pair<K&, V&> operator->() {
                return { entry->key, entry->value };
            }

            bool operator==(const iterator& other) const {
                return index == other.index && entry == other.entry;
            }

            bool operator!=(const iterator& other) const {
                return !(*this == other);
            }

            friend class UnorderedMap;
        };

        UnorderedMap() : UnorderedMap(initial_bucket_size) { }
        UnorderedMap(size_t initial_capacity) : capacity(initial_capacity) {
            bucket = allocator.template alloc<Entry*>(capacity);
            memset(bucket, 0, sizeof(Entry*) * capacity);
            _size = 0;
        }

        UnorderedMap(const UnorderedMap<K, V>& other) : capacity(other.capacity), _size(other._size) {
            bucket = allocator.template alloc<Entry*>(capacity);
            memset(bucket, 0, sizeof(Entry*) * capacity);
            for(size_t i = 0; i < capacity; ++i) {
                if(bucket[i] == 0) continue;

                auto* last = allocator.template alloc<Entry>(other.bucket[i]->key, other.bucket[i]->value);
                bucket[i] = last;

                for(auto* o_entry = other.bucket[i]->next; o_entry != 0; o_entry = o_entry->next) {
                    auto* entry = allocator.template alloc<Entry>(o_entry->key, o_entry->value);
                    last->next = entry;
                    last = entry;
                }
            }
        }
        UnorderedMap<K, V>& operator=(const UnorderedMap<K, V>& other) {
            for(size_t i = 0; i < capacity; ++i) {
                Entry* e = bucket[i];
                while(e != 0) {
                    Entry* next = e->next;
                    allocator.free(e);
                    e = next;
                }
            }
            allocator.free_array(bucket);

            capacity = other.capacity;
            _size = other._size;

            bucket = allocator.template alloc<Entry*>(capacity);
            memset(bucket, 0, sizeof(Entry*) * capacity);
            for(size_t i = 0; i < capacity; ++i) {
                if(bucket[i] == 0) continue;

                auto* last = allocator.template alloc<Entry>(other.bucket[i]->key, other.bucket[i]->value);
                bucket[i] = last;

                for(auto* o_entry = other.bucket[i]->next; o_entry != 0; o_entry = o_entry->next) {
                    auto* entry = allocator.template alloc<Entry>(o_entry->key, o_entry->value);
                    last->next = entry;
                    last = entry;
                }
            }
        }

        UnorderedMap(UnorderedMap<K, V>&& other) : capacity(other.capacity), _size(other._size) {
            bucket = other.bucket;
            other.bucket = 0;
        }

        UnorderedMap<K, V>& operator=(UnorderedMap<K, V>&& other) {
            for(size_t i = 0; i < capacity; ++i) {
                Entry* e = bucket[i];
                while(e != 0) {
                    Entry* next = e->next;
                    allocator.free(e);
                    e = next;
                }
            }
            allocator.free_array(bucket);

            capacity = other.capacity;
            _size = other._size;

            bucket = other.bucket;
            other.bucket = 0;
        }

        ~UnorderedMap() {
            if(bucket == 0) return;

            for(size_t i = 0; i < capacity; ++i) {
                Entry* e = bucket[i];
                while(e != 0) {
                    Entry* next = e->next;
                    allocator.free(e);
                    e = next;
                }
            }
            allocator.free_array(bucket);
        }

        iterator begin() {
            for(size_t i = 0; i < capacity; ++i) {
                if(bucket[i] != 0) return iterator(i, bucket[i], *this);
            }
            return end();
        }

        iterator end() {
            return iterator(capacity, 0, *this);
        }

        iterator find(const K& key) {
            size_t bucket_pos = Hasher{}(key) % capacity;

            if(bucket[bucket_pos] != 0) {
                for(Entry* entry = bucket[bucket_pos]; entry != 0; entry = entry->next) {
                    if(Pred{}(key, entry->key)) {
                        return iterator(bucket_pos, entry, *this);
                    }
                }
            }
            return end();
        }

        bool insert(Pair<K, V> value) {
            Entry* entry = allocator.template alloc<Entry>(value.key, value.value);
            entry->next = 0;

            size_t bucket_pos = Hasher{}(value.key) % capacity;

            if(bucket[bucket_pos] == 0) {
                bucket[bucket_pos] = entry;
            } else {
                Entry* last;
                for(Entry* lentry = bucket[bucket_pos]; lentry != 0; lentry = lentry->next) {
                    if(Pred{}(value.key, lentry->key)) {
                        // Duplicate key
                        allocator.free(entry);
                        return false;
                    }
                    last = lentry;
                }
                last->next = entry;
            }
            ++_size;
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

        V& operator[](const K& key) {
            size_t bucket_pos = Hasher{}(key) % capacity;
            if(bucket[bucket_pos] == 0) {
                bucket[bucket_pos] = new Entry();
                bucket[bucket_pos]->key = key;
                return bucket[bucket_pos]->value;
            }
            Entry* last = 0;
            for(Entry* entry = bucket[bucket_pos]; entry != 0; entry = entry->next) {
                if(Pred{}(key, entry->key)) {
                    return entry->value;
                }
                last = entry;
            }

            last->next = new Entry();
            last->next->key = key;
            return last->next->value;
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

        size_t size() const { return _size; }

        void clear() {
            for(size_t i = 0; i < capacity; ++i) {
                Entry* e = bucket[i];
                while(e != 0) {
                    Entry* next = e->next;
                    allocator.free(e);
                    e = next;
                }
            }
            allocator.free_array(bucket);

            bucket = allocator.template alloc<Entry*>(initial_bucket_size);
            capacity = initial_bucket_size;
            _size = 0;
        }
    };
}
