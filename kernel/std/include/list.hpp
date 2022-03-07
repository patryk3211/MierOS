#pragma once

#include <allocator.hpp>
#include <functional.hpp>

namespace std {
    template<typename T, class Allocator = std::heap_allocator> class List {
        struct EntryBase {
            EntryBase* next;
            EntryBase* prev;
        };

        struct Entry : public EntryBase {
            T value;

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

            iterator& operator++() { value = static_cast<Entry*>(value->next); return *this; }
            iterator operator++(int) {
                iterator old = *this;
                value = static_cast<Entry*>(value->next);
                return old;
            }

            iterator& operator--() { value = static_cast<Entry*>(value->prev); return *this; }
            iterator operator--(int) {
                iterator old = *this;
                value = static_cast<Entry*>(value->prev);
                return old;
            }

            bool operator==(const iterator& other) { return value == other.value; }
            bool operator!=(const iterator& other) { return !(*this == other); }

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

        List(const List<T>& other) {
            head.prev = 0;
            head.next = &tail;
            tail.prev = &head;
            tail.next = 0;
            length = other.length;

            Entry* last = static_cast<Entry*>(&head);
            for(Entry* entry = static_cast<Entry*>(other.head.next); entry != &other.tail; entry = static_cast<Entry*>(entry->next)) {
                Entry* e = allocator.template alloc<Entry>(entry->value);
                
                e->next = last->next;
                e->prev = last;

                last->next->prev = e;
                last->next = e;
            }
        }

        List<T>& operator=(const List<T>& other) {
            Entry* e = head.next;
            while(e != &tail) {
                Entry* n = e->next;
                allocator.free(e);
                e = n;
            }

            head.prev = 0;
            head.next = &tail;
            tail.prev = &head;
            tail.next = 0;
            length = other.length;

            Entry* last = &head;
            for(Entry* entry = other.head.next; entry != &other.tail; entry = entry->next) {
                Entry* e = allocator.template alloc<Entry>(entry->value);
                
                e->next = last->next;
                e->prev = last;

                last->next->prev = e;
                last->next = e;
            }
        }

        List(List<T>&& other) {
            head.prev = 0;
            head.next = other.head.next;
            tail.prev = other.tail.prev;
            tail.next = 0;
            length = other.length;

            other.head.next = &other.tail;
            other.tail.prev = &other.head;
            other.length = 0;
        }

        List<T>& operator=(List<T>&& other) {
            Entry* e = head.next;
            while(e != &tail) {
                Entry* n = e->next;
                allocator.free(e);
                e = n;
            }

            head.prev = 0;
            head.next = other.head.next;
            tail.prev = other.tail.prev;
            tail.next = 0;
            length = other.length;

            other.head.next = &other.tail;
            other.tail.prev = &other.head;
            other.length = 0;

            return *this;
        }

        ~List() {
            Entry* e = static_cast<Entry*>(head.next);
            while(e != &tail) {
                Entry* n = static_cast<Entry*>(e->next);
                allocator.free(e);
                e = n;
            }
        }

        T& front() { return head.next->value; }
        T& back() { return tail.prev->value; }

        bool empty() { return length == 0; }
        unsigned int size() { return length; }

        void clear() {
            Entry* e = static_cast<Entry*>(head.next);
            while(e != &tail) {
                Entry* n = static_cast<Entry*>(e->next);
                allocator.free(e);
                e = n;
            }
            length = 0;

            head.next = &tail;
            tail.prev = &head;
        }

        iterator begin() { return iterator(static_cast<Entry*>(head.next)); }
        iterator end() { return iterator(static_cast<Entry*>(&tail)); }

        reverse_iterator rbegin() { return reverse_iterator(tail.prev); }
        reverse_iterator rend() { return reverse_iterator(&head); }

        iterator insert(iterator pos, const T& value) {
            Entry* cur = pos.value;
            Entry* en = allocator.template alloc<Entry>(value);
            en->next = cur;
            en->prev = cur->prev;

            cur->prev->next = en;
            cur->prev = en;

            length++;
            return iterator(en);
        }

        iterator erase(iterator pos) {
            Entry* cur = pos.value;
            if(cur == &tail) return iterator(static_cast<Entry*>(&tail));

            cur->prev->next = cur->next;
            cur->next->prev = cur->prev;
            Entry* next = static_cast<Entry*>(cur->next);

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
            Entry* current = static_cast<Entry*>(head.next);
            tail.prev->next = 0;
            head.next = &tail;
            tail.prev = &head;

            while(current != 0) {
                Entry* next = static_cast<Entry*>(current->next);
                for(auto iter = iterator(static_cast<Entry*>(head.next)); iter != iterator(static_cast<Entry*>(&tail)); ++iter) {
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
        EntryBase head;
        EntryBase tail;
        size_t length;
    };
}