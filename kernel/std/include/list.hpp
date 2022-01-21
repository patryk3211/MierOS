#pragma once

#include <allocator.hpp>
#include <functional.hpp>

namespace std {
    template<typename T, class Allocator = std::heap_allocator> class List {
        struct Entry {
            Entry* next;
            Entry* prev;
            T value;

            Entry() { }
            Entry(T value) : value(value) { }
        };
    public:
        class iterator {
            Entry* value;

            iterator(Entry* value) : value(value) { }
        public:
            ~iterator() { }

            T& operator*() { return value->value; }
            T* operator->() { return &value->value; }

            iterator& operator++() { value = value->next; return *this; }
            iterator operator++(int) {
                iterator old = *this;
                value = value->next;
                return old;
            }

            iterator& operator--() { value = value->prev; return *this; }
            iterator operator--(int) {
                iterator old = *this;
                value = value->prev;
                return old;
            }

            bool operator==(const iterator& other) { return value == other.value; }
            bool operator!=(const iterator& other) { return value != other.value; }

            friend class List;
        };

        class reverse_iterator {
            Entry* value;

            reverse_iterator(Entry* value) : value(value) { }
        public:
            ~reverse_iterator() { }

            T& operator*() { return value->value; }
            T& operator->() { return value->value; }

            reverse_iterator& operator++() { value = value->prev; return *this; }
            reverse_iterator operator++(int) {
                reverse_iterator old = *this;
                value = value->prev;
                return old;
            }

            reverse_iterator& operator--() { value = value->next; return *this; }
            reverse_iterator operator--(int) {
                reverse_iterator old = *this;
                value = value->next;
                return old;
            }

            bool operator==(const iterator& other) { return value == other.value; }
            bool operator!=(const iterator& other) { return value != other.value; }
        };

        Allocator allocator;
    public:
        List() {
            head.prev = 0;
            head.next = &tail;
            tail.prev = &head;
            tail.next = 0;
            length = 0;
        }

        ~List() {
            Entry* e = head.next;
            while(e != &tail) {
                Entry* n = e->next;
                allocator.free(e);
                e = n;
            }
        }

        T& front() { return head.next->value; }
        T& back() { return tail.prev->value; }

        bool empty() { return length == 0; }
        unsigned int size() { return length; }

        void clear() {
            Entry* e = head.next;
            while(e != &tail) {
                Entry* n = e->next;
                allocator.free(e);
                e = n;
            }
            length = 0;

            head.next = &tail;
            tail.prev = &head;
        }

        iterator begin() { return iterator(head.next); }
        iterator end() { return iterator(&tail); }

        reverse_iterator rbegin() { return reverse_iterator(tail.prev); }
        reverse_iterator rend() { return reverse_iterator(&head); }

        iterator insert(iterator pos, const T& value) {
            Entry* cur = pos.value;
            Entry* en = allocator.template alloc<Entry>();
            en->value = value;
            en->next = cur;
            en->prev = cur->prev;

            cur->prev->next = en;
            cur->prev = en;

            length++;
            return iterator(en);
        }

        iterator erase(iterator pos) {
            Entry* cur = pos.value;
            if(cur == &tail) return iterator(&tail);

            cur->prev->next = cur->next;
            cur->next->prev = cur->prev;
            Entry* next = cur->next;

            length--;
            delete cur;
            return iterator(next);
        }

        void push_front(const T& value) { insert(begin(), value); }
        void push_back(const T& value) { insert(end(), value); }

        void pop_front() { erase(begin()); }
        void pop_back() { erase(--end()); }

        void sort() { sort(std::less<T>()); }
        template<class Comp> void sort(Comp comparator) {
            Entry* current = head.next;
            tail.prev->next = 0;
            head.next = &tail;
            tail.prev = &head;

            while(current != 0) {
                Entry* next = current->next;
                for(auto iter = iterator(head.next); iter != iterator(&tail); ++iter) {
                    if(comparator(current->value, *iter)) {
                        current->prev = iter.value->prev;
                        current->next = iter.value;
                        iter.value->prev->next = current;
                        iter.value->prev = current;
                        goto inserted;
                    }
                }
                current->prev = tail.prev;
                current->next = &tail;
                tail.prev->next = current;
                tail.prev = current;
            inserted:
                current = next;
            }
        }
    private:
        Entry head;
        Entry tail;
        size_t length;
    };
}