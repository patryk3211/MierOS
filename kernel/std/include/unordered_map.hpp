#pragma once

#include <allocator.hpp>
#include <functional.hpp>
#include <optional.hpp>
#include <pair.hpp>
#include <stdlib.h>
#include <types.h>
#include <vector.hpp>

namespace std {
    template<typename K, typename V, class Hasher = std::hash<K>, class Pred = std::equal_to<K>, class Alloc = std::heap_allocator> class UnorderedMap {
        static constexpr size_t initial_bucket_size = 64;

        struct Entry {
            K key;
            alignas(V) u8_t valueStorage[sizeof(V)];
            Entry* next;

            Entry() = delete;
            Entry(const K& k)
                : key(k)
                , next(0) {
                new(valueStorage) V();
            }
            Entry(const K& k, const V& v)
                : key(k)
                , next(0) {
                new(valueStorage) V(v);
            }
            Entry(const Entry&) = delete;
            Entry(Entry&&) = delete;

            V& value() {
                return *reinterpret_cast<V*>(valueStorage);
            }

            ~Entry() {
                value().~V();
                next = 0;
            };
        };

        Alloc allocator;

        std::Vector<Entry*> f_bucket;
        size_t f_capacity;
        size_t f_size;

    public:
        class iterator {
            size_t index;
            Entry* entry;
            UnorderedMap& map;

            iterator(size_t index, Entry* entry, UnorderedMap& map)
                : index(index)
                , entry(entry)
                , map(map) { }

        public:
            ~iterator() = default;

            iterator& operator++() {
                if(entry->next == 0) {
                    Entry* new_entry = 0;
                    ++index;
                    while(index < map.f_capacity && new_entry == 0)
                        new_entry = map.f_bucket[index++];
                    entry = new_entry;
                    if(entry != 0) --index;
                } else
                    entry = entry->next;
                return *this;
            }

            iterator operator++(int) {
                iterator copy = iterator(index, entry);
                ++this;
                return copy;
            }

            Pair<K&, V&> operator*() {
                return { entry->key, entry->value() };
            }

            Pair<K&, V&> operator->() {
                return { entry->key, entry->value() };
            }

            bool operator==(const iterator& other) const {
                return index == other.index && entry == other.entry;
            }

            bool operator!=(const iterator& other) const {
                return !(*this == other);
            }

            friend class UnorderedMap;
        };

        UnorderedMap()
            : UnorderedMap(initial_bucket_size) { }
        UnorderedMap(size_t initial_capacity)
            : f_bucket(initial_capacity, nullptr)
            , f_capacity(initial_capacity) {
            f_size = 0;
        }

        UnorderedMap(const UnorderedMap<K, V>& other)
            : f_bucket(other.f_capacity, nullptr)
            , f_capacity(other.f_capacity)
            , f_size(other.f_size) {
            for(size_t i = 0; i < f_capacity; ++i) {
                if(f_bucket[i] == 0) continue;

                auto* last = allocator.template alloc<Entry>(other.f_bucket[i]->key, other.f_bucket[i]->value());
                f_bucket[i] = last;

                for(auto* o_entry = other.f_bucket[i]->next; o_entry != 0; o_entry = o_entry->next) {
                    auto* entry = allocator.template alloc<Entry>(o_entry->key, o_entry->value());
                    last->next = entry;
                    last = entry;
                }
            }
        }
        UnorderedMap<K, V>& operator=(const UnorderedMap<K, V>& other) {
            clear();

            f_capacity = other.f_capacity;
            f_size = other.f_size;

            f_bucket.resize(f_capacity, nullptr);
            for(size_t i = 0; i < f_capacity; ++i) {
                if(f_bucket[i] == 0) continue;

                auto* last = allocator.template alloc<Entry>(other.f_bucket[i]->key, other.f_bucket[i]->value());
                f_bucket[i] = last;

                for(auto* o_entry = other.f_bucket[i]->next; o_entry != 0; o_entry = o_entry->next) {
                    auto* entry = allocator.template alloc<Entry>(o_entry->key, o_entry->value());
                    last->next = entry;
                    last = entry;
                }
            }
        }

        UnorderedMap(UnorderedMap<K, V>&& other)
            : f_bucket(std::move(other.f_bucket))
            , f_capacity(other.f_capacity)
            , f_size(other.f_size) { }

        UnorderedMap<K, V>& operator=(UnorderedMap<K, V>&& other) {
            clear();

            f_capacity = other.f_capacity;
            f_size = other.f_size;

            f_bucket = std::move(other.bucket);
        }

        ~UnorderedMap() {
            clear();
        }

        iterator begin() {
            for(size_t i = 0; i < f_capacity; ++i) {
                if(f_bucket[i] != 0) return iterator(i, f_bucket[i], *this);
            }
            return end();
        }

        iterator end() {
            return iterator(f_capacity, 0, *this);
        }

        iterator find(const K& key) {
            size_t bucket_pos = Hasher {}(key) % f_capacity;

            if(f_bucket[bucket_pos] != 0) {
                for(Entry* entry = f_bucket[bucket_pos]; entry != 0; entry = entry->next) {
                    if(Pred {}(key, entry->key)) {
                        return iterator(bucket_pos, entry, *this);
                    }
                }
            }
            return end();
        }

        bool insert(Pair<K, V> value) {
            Entry* entry = allocator.template alloc<Entry>(value.key, value.value);

            size_t bucket_pos = Hasher {}(value.key) % f_capacity;

            if(f_bucket[bucket_pos] == 0) {
                f_bucket[bucket_pos] = entry;
            } else {
                Entry* last;
                for(Entry* lentry = f_bucket[bucket_pos]; lentry != 0; lentry = lentry->next) {
                    if(Pred {}(value.key, lentry->key)) {
                        // Duplicate key
                        allocator.free(entry);
                        return false;
                    }
                    last = lentry;
                }
                last->next = entry;
            }
            ++f_size;
            /// TODO: [22.01.2022] Implement rehashing.
            return true;
        }

        OptionalRef<V> at(const K& key) {
            size_t bucket_pos = Hasher {}(key) % f_capacity;

            if(f_bucket[bucket_pos] == 0) return {};
            for(Entry* entry = f_bucket[bucket_pos]; entry != 0; entry = entry->next) {
                if(Pred {}(key, entry->key)) {
                    // This is the value
                    return entry->value();
                }
            }
            return {};
        }

        V& operator[](const K& key) {
            size_t bucket_pos = Hasher {}(key) % f_capacity;
            if(f_bucket[bucket_pos] == 0) {
                f_bucket[bucket_pos] = allocator.template alloc<Entry>(key);
                return f_bucket[bucket_pos]->value();
            }
            Entry* last = 0;
            for(Entry* entry = f_bucket[bucket_pos]; entry != 0; entry = entry->next) {
                if(Pred {}(key, entry->key)) {
                    return entry->value();
                }
                last = entry;
            }

            last->next = allocator.template alloc<Entry>(key);
            return last->next->value();
        }

        void erase(const K& key) {
            size_t bucket_pos = Hasher {}(key) % f_capacity;

            if(f_bucket[bucket_pos] == 0) return;
            Entry* prev = 0;
            for(Entry* entry = f_bucket[bucket_pos]; entry != 0; entry = entry->next) {
                if(Pred {}(key, entry->key)) {
                    // Delete this
                    if(prev == 0)
                        f_bucket[bucket_pos] = entry->next;
                    else
                        prev->next = entry->next;
                    allocator.free(entry);
                    --f_size;
                    return;
                }
                prev = entry;
            }
        }

        size_t size() const { return f_size; }

        void clear() {
            for(size_t i = 0; i < f_capacity; ++i) {
                Entry* e = f_bucket[i];
                while(e != 0) {
                    Entry* next = e->next;
                    allocator.free(e);
                    e = next;
                }
            }

            f_bucket.clear();
            f_bucket.resize(initial_bucket_size, nullptr);
            f_capacity = initial_bucket_size;
            f_size = 0;
        }
    };
}
