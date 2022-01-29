#include <memory/physical.h>
#include <dmesg.h>
#include <memory/liballoc.h>
#include <stdlib.h>
#include <assert.h>
#include <defines.h>

#include <locking/spinlock.hpp>
#include <locking/locker.hpp>
#include <range_map.hpp>

using namespace kernel;

NO_EXPORT physaddr_t pmm_first_potential_page;

struct page_4gb_status_struct {
    u8_t used_bitmap[0x20000];
};

NO_EXPORT page_4gb_status_struct* status_pages;

void set_page_status(u64_t address, int status) {
    address >>= 12;
    u8_t* status_byte = &status_pages[address >> 20].used_bitmap[(address >> 3) & 0x1FFFFFFF];
    u8_t bit_offset = address & 7;
    *status_byte &= ~(1 << bit_offset);
    *status_byte |= status << bit_offset;
}

int is_page_used(u64_t address) {
    address >>= 12;
    return (status_pages[address >> 20].used_bitmap[(address >> 3) & 0x1FFFFFFF] >> (address & 7)) & 1;
}

NO_EXPORT SpinLock pmm_lock;

NO_EXPORT std::RangeMap<physaddr_t>* freeable_mem;

extern "C" TEXT_FREE_AFTER_INIT void init_pmm(stivale2_stag_memmap* memory_map) {
    ASSERT_F(memory_map != 0, "\033[1;37mmemory_map\033[0m is null");
    // Output the memory map on serial and count how much 4 GB pages there are.
    kprintf("[Kernel] Memory Map:\n[Kernel] Base             Length           Type    \n");
    physaddr_t max_address = 0;
    for(u64_t i = 0; i < memory_map->entry_count; ++i) {
        kprintf("[Kernel] %x16 %x16 %x8\n", memory_map->entries[i].base, memory_map->entries[i].length, memory_map->entries[i].type);
        if(max_address < memory_map->entries[i].base) max_address = memory_map->entries[i].base;
    }
    u32_t gb_page_count = (max_address >> 32) + ((max_address & 0xFFFFFFFF) == 0 ? 0 : 1);

    // Allocate the required status pages and set all memory as used.
    status_pages = new page_4gb_status_struct[gb_page_count];
    for(u32_t i = 0; i < gb_page_count; ++i) memset(status_pages[i].used_bitmap, 0xFF, sizeof(page_4gb_status_struct));

    freeable_mem = new std::RangeMap<physaddr_t>();

    u64_t free_mem = 0;
    // Look for usable memory regions and mark them as free.
    pmm_first_potential_page = ~0;
    for(u64_t i = 0; i < memory_map->entry_count; ++i) {
        if(memory_map->entries[i].type == STIVALE2_MEMMAP_TYPE_USABLE) {
            // Push back the first_potential_page pointer if a lower page is available.
            if(pmm_first_potential_page > memory_map->entries[i].base) pmm_first_potential_page = memory_map->entries[i].base; 
            // Set the memory block as free.
            for(u64_t page = 0; page < memory_map->entries[i].length; page += 4096) set_page_status(memory_map->entries[i].base+page, 0);

            free_mem += memory_map->entries[i].length;
        } else if(memory_map->entries[i].type == STIVALE2_MEMMAP_TYPE_BOOTLOADER_RECLAIMABLE) {
            // Add bootloader chunks to freeable range map.
            freeable_mem->add(memory_map->entries[i].base, memory_map->entries[i].base+memory_map->entries[i].length);
        }
    }

    // Mark the first MiB as used since it could be utilized in the startup of AP cores.
    for(u64_t page = 0; page < 1024*1024; page += 4096) set_page_status(page, 1);
    freeable_mem->add(0, 1024*1024);

    kprintf("[Kernel] Found %d KiB of free memory\n", free_mem/1024);

    pmm_lock = SpinLock();
}

extern "C" void pmm_release_bootloader_resources() {
    u64_t freed_mem = 0;
    for(auto iter = freeable_mem->begin(); iter != freeable_mem->end(); ++iter) {
        freed_mem += iter->end - iter->start;
        pfree(iter->start, (iter->end - iter->start) >> 12);
    }
    delete freeable_mem;
    kprintf("[Kernel] Freed %d KiB of bootloader memory\n", freed_mem/1024);
}

extern "C" physaddr_t palloc(size_t page_count) {
    Locker lock(pmm_lock);

    physaddr_t addr = pmm_first_potential_page;
    while(1) {
        // Find a big enough block of memory.
        int found_block = 1;
        for(size_t i = 0; i < page_count; ++i) {
            if(is_page_used(addr + (i << 12))) {
                found_block = 0;
                addr += (i + 1) << 12;
                break;
            }
        }
        if(!found_block) continue;

        // Mark the block as used.
        for(size_t i = 0; i < page_count; ++i) set_page_status(addr + (i << 12), 1);
        if(addr == pmm_first_potential_page) {
            // Bump address past this allocation.
            pmm_first_potential_page += page_count << 12;
        }
        return addr;
    }
}

/*extern "C" void preserve(physaddr_t addr, size_t page_count) {
    Locker lock(pmm_lock);

    // Mark the block as used.
    for(size_t i = 0; i < page_count; ++i) set_page_status(addr + (i << 12), 1);
    if(addr == pmm_first_potential_page) {
        // Bump address past this reservation.
        pmm_first_potential_page += page_count << 12;
    }
}*/

extern "C" void pfree(physaddr_t addr, size_t page_count) {
    addr &= ~0xFFF;
    Locker lock(pmm_lock);
    for(size_t i = 0; i < page_count; ++i) set_page_status(addr + (i << 12), 0);
    if(addr < pmm_first_potential_page) pmm_first_potential_page = addr;
}
