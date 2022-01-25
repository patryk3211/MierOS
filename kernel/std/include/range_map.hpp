#pragma once

#include <range.hpp>
#include <allocator.hpp>

namespace std {
    template<typename T, class Allocator = heap_allocator> class RangeMap {
    public:
        struct range_en {
            Range<T> rang;

            range_en* next;
            range_en* prev;
        };

        class iterator {
            range_en* value;

            iterator(range_en* value) : value(value) { }
        public:
            ~iterator() { }

            Range<T>& operator*() { return value->rang; }
            Range<T>* operator->() { return &value->rang; }

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

            friend class RangeMap;
        };
    private:
        Allocator alloc;

        range_en head;
        range_en tail;

        range_en* delete_range(range_en* r) {
            r->prev->next = r->next;
            r->next->prev = r->prev;
            range_en* n = r->next;
            //delete r;
            alloc.free(r);
            return n;
        }
    public:
        RangeMap() {
            head.prev = 0;
            head.next = &tail;
            tail.prev = &head;
            tail.next = 0;
        }

        ~RangeMap() {
            range_en* r = head.next;
            while(r != &tail) {
                range_en* n = r->next;
                //delete r;
                alloc.free(r);
                r = n;
            }
        }

        void add(T start, T end) {
            // Append the new range
            range_en* inserted = alloc.template alloc<range_en>();//new range_en();
            inserted->rang = { start, end };
            inserted->next = &tail;
            inserted->prev = tail.prev;
            tail.prev->next = inserted;
            tail.prev = inserted;
            
            // Loop through the entire list and check if any ranges can be merged
            auto resultRange = head.next;
            while(resultRange != &tail) {
                auto mergedRange = head.next;
                while(mergedRange != &tail) {
                    if(resultRange == mergedRange) { mergedRange = mergedRange->next; continue; } // Don't try to merge the range onto itself.
                    if(mergedRange->rang.start >= resultRange->rang.start && mergedRange->rang.start <= resultRange->rang.end) {
                        // Start of merged range is in bounds of the result range.
                        if(mergedRange->rang.end > resultRange->rang.end) {
                            // The merged range ends past the result range.
                            resultRange->rang.end = mergedRange->rang.end;
                        }
                        // If the end is also bound then the whole range fits into the result and we can remove it.
                        mergedRange = delete_range(mergedRange);
                    } else if(mergedRange->rang.end >= resultRange->rang.start && mergedRange->rang.end <= resultRange->rang.end) {
                        // If the end is bound and the start is not then we can set the start of result range to the start of merged range.
                        resultRange->rang.start = mergedRange->rang.start;
                        mergedRange = delete_range(mergedRange);
                    } else if(mergedRange->rang.start <= resultRange->rang.start && mergedRange->rang.end >= resultRange->rang.end) {
                        // If the merged range covers the entire result range we copy the start and end and remove the merged range.
                        resultRange->rang.start = mergedRange->rang.start;
                        resultRange->rang.end = mergedRange->rang.end;
                        mergedRange = delete_range(mergedRange);
                    } else mergedRange = mergedRange->next; // The ranges are unrelated, iterate to the next one.
                }
                resultRange = resultRange->next;
            }
        }

        void remove(T start, T end) {
            auto r = head.next;
            while(r != &tail) {
                if(start <= r->rang.start && end >= r->rang.end) {
                    // The unmap range covers the entire mapping, erase it.
                    r = delete_range(r);
                    continue;
                }
                bool startBound = start >= r->rang.start && start < r->rang.end;
                bool endBound = end >= r->rang.start && end < r->rang.end;
                if(startBound && !endBound) {
                    // The start is in bounds of this mapping but the end is not, we just shorten it.
                    r->rang.end = start;
                } else if(!startBound && endBound) {
                    // The end is in bounds of this mapping but the start is not, we start it later.
                    r->rang.start = end;
                } else if(startBound && endBound) {
                    // Unmapping in the middle
                    range_en* firstHalf = alloc.template alloc<range_en>();//new range_en();
                    firstHalf->rang.start = r->rang.start;
                    firstHalf->rang.end = start;
                    firstHalf->prev = r->prev;
                    firstHalf->next = r;
                    r->prev->next = firstHalf;
                    r->prev = firstHalf;
                    r->rang.start = end;
                    break;
                }
                r = r->next;
            }
        }

        void clear() {
            range_en* r = head.next;
            while(r != &tail) {
                range_en* n = r->next;
                //delete r;
                alloc.free(r);
                r = n;
            }
            head.next = &tail;
            tail.prev = &head;
        }

        bool is_bound(T value) {
            for(auto r = head.next; r != &tail; r = r->next)
                if(value >= r->rang.start && value < r->rang.end) return true;
            return false;
        }

        iterator begin() { return iterator(head.next); }
        iterator end() { return iterator(&tail); }
    };
}
