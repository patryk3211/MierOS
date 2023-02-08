#include <assert.h>
#include <defines.h>
#include <locking/locker.hpp>
#include <locking/spinlock.hpp>
#include <memory/liballoc.h>
#include <memory/virtual.hpp>
#include <stdlib.h>

using namespace kernel;

NO_EXPORT SpinLock heap_lock;
extern "C" int liballoc_lock() {
    heap_lock.lock();
    return 0;
}

extern "C" int liballoc_unlock() {
    heap_lock.unlock();
    return 0;
}

#define HEAP_SIZE 1024 * 1024 * 16 // 16 MiB heap

#define HEAP_PAGE_SIZE HEAP_SIZE / 4096

NO_EXPORT u8_t heap_usage_bitmap[HEAP_PAGE_SIZE / 8];
NO_EXPORT u32_t heap_first_potential_page;
NO_EXPORT SECTION(".heap") u8_t initial_heap[HEAP_SIZE];

inline bool is_page_used(u32_t page_idx) {
    ASSERT_F(page_idx < HEAP_PAGE_SIZE, "\033[1;37mpage_idx\033[0m points outside of the \033[1;37minitial_heap\033[0m");
    u8_t usageByte = heap_usage_bitmap[page_idx >> 3];
    u8_t usageMask = 1 << (page_idx & 7);
    return usageByte & usageMask;
}

inline void set_page_used(u32_t page_idx, bool status) {
    ASSERT_F(page_idx < HEAP_PAGE_SIZE, "\033[1;37mpage_idx\033[0m points outside of the \033[1;37minitial_heap\033[0m");
    u32_t byte_idx = page_idx >> 3;
    u8_t bit_idx = page_idx & 0x7;
    heap_usage_bitmap[byte_idx] &= ~(1 << bit_idx);
    heap_usage_bitmap[byte_idx] |= status << bit_idx;
}

extern "C" TEXT_FREE_AFTER_INIT void init_heap() {
    heap_first_potential_page = 0;
    memset(heap_usage_bitmap, 0, sizeof(heap_usage_bitmap));
    heap_lock = SpinLock();
}

extern "C" void* liballoc_alloc(size_t page_count) {
    if(heap_first_potential_page < HEAP_PAGE_SIZE) {
        // Allocate from local heap.
        for(u32_t page = heap_first_potential_page; page < HEAP_PAGE_SIZE; ++page) {
            bool found_block = true;
            for(u32_t i = 0; i < page_count; ++i) {
                // We have exhausted the heap.
                if(i + page >= HEAP_PAGE_SIZE) {
                    page = HEAP_PAGE_SIZE;
                    found_block = false;
                    break;
                }
                // This block is used, skip to next possibly free block.
                if(is_page_used(page + i)) {
                    found_block = false;
                    page += i;
                    break;
                }
            }
            // Block not found, continue searching
            if(!found_block)
                continue;
            // Move first potentially free page
            if(heap_first_potential_page == page)
                heap_first_potential_page = page + page_count;
            // Mark pages as used
            for(u32_t i = 0; i < page_count; ++i)
                set_page_used(page + i, true);
            // Return the allocated pointer
            /// DEBUG FURTHER
            kprintf("Allocated 0x%x16, %d pages\n", page << 12, page_count);
            return initial_heap + (page << 12);
        }
    }
    Pager pager = Pager::active();
    Locker lock(pager);
    return (void*)pager.kalloc(page_count);
}

extern "C" int liballoc_free(void* ptr, size_t page_count) {
    if(ptr >= initial_heap && ptr < initial_heap + HEAP_SIZE) {
        // Free on local heap.
        u32_t page = ((u64_t)ptr - (u64_t)heap_usage_bitmap) >> 12;
        for(u32_t i = 0; i < page_count; ++i) set_page_used(page + i, 0);
        if(page < heap_first_potential_page) heap_first_potential_page = page;
        kprintf("Freed 0x%x16, %d pages\n", page << 12, page_count);
        return 0;
    }
    Pager pager = Pager::active();
    Locker lock(pager);
    pager.free((virtaddr_t)ptr, page_count);
    return 0;
}

void* operator new(size_t size) {
    return malloc(size);
}

void* operator new[](size_t size) {
    return malloc(size);
}

void operator delete(void* ptr) {
    free(ptr);
}

void operator delete[](void* ptr) {
    free(ptr);
}

void operator delete(void* ptr, size_t) {
    free(ptr);
}

void operator delete[](void* ptr, size_t) {
    free(ptr);
}

void* operator new(size_t, void* ptr) {
    return ptr;
}
