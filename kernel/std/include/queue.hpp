#pragma once

#include <utility.hpp>
#include <allocator.hpp>

namespace std {
    template<typename T, class Allocator = std::heap_allocator> class Queue {
        struct Entry {
            alignas(T) unsigned char storage[sizeof(T)];
            Entry* next;

            Entry(const T& value) {
                new(storage) T(value);
                next = 0;
            }

            Entry(const Entry& value) {
                new(storage) T(value.get());
                next = 0;
            }

            Entry(Entry&& value) {
                new(storage) T(move(value.get()));
                next = 0;
            }

            ~Entry() {
                get().~T();
            }

            T& get() {
                return *reinterpret_cast<T*>(storage);
            }
        };

        Allocator allocator;

        Entry* f_first;
        Entry* f_last;
        size_t f_size;

    public:
        Queue() {
            f_first = 0;
            f_last = 0;
            f_size = 0;
        }

        ~Queue() {
            clear();
        }

        void clear() {
            Entry* cur = f_first;
            while(cur != 0) {
                Entry* next = cur->next;
                allocator.free(cur);
                cur = next;
            }
            f_first = 0;
            f_last = 0;
            f_size = 0;
        }

        void push_back(const T& value) {
            push(new Entry(value));
        }

        T pop_front() {
            Entry* en = f_first;
            f_first = f_first->next;
            if(!f_first)
                f_last = 0;
            --f_size;

            T value = en->get();
            delete en;

            return value;
        }

        size_t size() const {
            return f_size;
        }

    private:
        void push(Entry* entry) {
            if(!f_last) {
                f_first = entry;
                f_last = entry;
            } else {
                f_last->next = entry;
                f_last = entry;
            }
            ++f_size;
        }
    };
}
