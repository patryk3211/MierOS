#pragma once

#include <tasking/thread.hpp>

namespace kernel {
    class ThreadQueue {
        Thread* first;
        Thread* last;

        size_t length;
    public:
        class iterator {
            Thread* thread;
            Thread* prev;

            iterator(Thread* thread, Thread* prev) : thread(thread), prev(prev) { }
        public:
            ~iterator();

            iterator& operator++() {
                prev = thread;
                thread = thread->next;
                return *this;
            }

            iterator operator++(int) {
                iterator copy(thread, prev);
                prev = thread;
                thread = thread->next;
                return copy;
            }

            Thread* operator->() { return thread; }
            Thread& operator*() { return *thread; }

            friend class ThreadQueue;
        };

        ThreadQueue() {
            first  = 0;
            last   = 0;
            length = 0;
        }

        ~ThreadQueue() { }

        Thread* front() { return first; }
        Thread* back() { return last; }

        iterator begin() { return iterator(first, 0); }
        iterator end() { return iterator(0, last); }

        void push_front(Thread* thread) {
            thread->next = first;
            first = thread;
            if(last == 0) last = first;
        }

        void push_back(Thread* thread) {
            thread->next = 0;
            if(last != 0) {
                last->next = thread;
                last = thread;
            } else {
                first = thread;
                last = thread;
            }
            ++length;
        }

        void pop_front() {
            if(first == last) {
                first = 0;
                last = 0;
            } else first = first->next;
            --length;
        }

        iterator erase(const iterator& pos) {
            if(pos.prev == 0) {
                first = pos.thread->next;
                pos.thread->next = 0;
                return iterator(first, 0);
            }
            if(pos.thread->next == 0) last = pos.prev;
            pos.prev->next = pos.thread->next;
            pos.thread->next = 0;
            return iterator(pos.prev->next, pos.prev);
        }

        Thread* get_optimal_thread(int core) {
            Thread* best_thread = first;
            Thread* prev = 0;
            for(Thread* thread = first->next; thread != 0; thread = thread->next) {
                if(best_thread->preferred_core != core && thread->preferred_core == core) {
                    if(prev != 0) prev->next = best_thread->next;
                    else first = best_thread->next;

                    if(best_thread == last) last = prev;
                    best_thread->next = 0;

                    return thread;
                }
                if(best_thread->preferred_core != core && best_thread->preferred_core != -1 && thread->preferred_core == -1) {
                    best_thread = thread;
                }
                prev = thread;
            }
            if(prev != 0) prev->next = best_thread->next;
            else first = best_thread->next;

            if(best_thread == last) last = prev;
            best_thread->next = 0;

            return best_thread;
        }

        size_t size() const {
            return length;
        }
    };
}
