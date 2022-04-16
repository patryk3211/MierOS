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

            iterator(Thread* thread, Thread* prev)
                : thread(thread)
                , prev(prev) { }

        public:
            ~iterator();

            iterator& operator++() {
                prev = thread;
                thread = thread->f_next;
                return *this;
            }

            iterator operator++(int) {
                iterator copy(thread, prev);
                prev = thread;
                thread = thread->f_next;
                return copy;
            }

            Thread* operator->() { return thread; }
            Thread& operator*() { return *thread; }

            friend class ThreadQueue;
        };

        ThreadQueue() {
            first = 0;
            last = 0;
            length = 0;
        }

        ~ThreadQueue() { }

        Thread* front() { return first; }
        Thread* back() { return last; }

        iterator begin() { return iterator(first, 0); }
        iterator end() { return iterator(0, last); }

        void push_front(Thread* thread) {
            thread->f_next = first;
            first = thread;
            if(last == 0) last = first;
        }

        void push_back(Thread* thread) {
            thread->f_next = 0;
            if(last != 0) {
                last->f_next = thread;
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
            } else
                first = first->f_next;
            --length;
        }

        iterator erase(const iterator& pos) {
            if(pos.prev == 0) {
                first = pos.thread->f_next;
                pos.thread->f_next = 0;
                return iterator(first, 0);
            }
            if(pos.thread->f_next == 0) last = pos.prev;
            pos.prev->f_next = pos.thread->f_next;
            pos.thread->f_next = 0;
            return iterator(pos.prev->f_next, pos.prev);
        }

        Thread* get_optimal_thread(int core) {
            Thread* best_thread = first;
            Thread* prev = 0;
            for(Thread* thread = first; thread != 0; thread = thread->f_next) {
                if(best_thread->f_preferred_core != core && thread->f_preferred_core == core) {
                    if(prev != 0)
                        prev->f_next = best_thread->f_next;
                    else
                        first = best_thread->f_next;

                    if(best_thread == last) last = prev;
                    best_thread->f_next = 0;

                    return thread;
                }
                if(best_thread->f_preferred_core != core && best_thread->f_preferred_core != -1 && thread->f_preferred_core == -1) {
                    best_thread = thread;
                }
                prev = thread;
            }
            if(best_thread != 0) {
                if(best_thread == first) {
                    first = best_thread->f_next;
                    prev = 0;
                } else
                    prev->f_next = best_thread->f_next;

                if(best_thread == last) last = prev;
                best_thread->f_next = 0;
            }

            return best_thread;
        }

        size_t size() const {
            return length;
        }
    };
}
