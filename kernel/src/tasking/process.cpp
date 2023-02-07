#include <arch/interrupts.h>
#include <errno.h>
#include <tasking/process.hpp>
#include <tasking/scheduler.hpp>
#include <tasking/syscalls/map.hpp>
#include <memory/page/anonpage.hpp>
#include <locking/locker.hpp>
#include <memory/page/memoryfilepage.hpp>

using namespace kernel;

Process::Process(virtaddr_t entry_point, Pager* pager)
    : f_pager(pager)
    , f_workingDirectory("/") {
    f_threads.push_back(new Thread(entry_point, true, *this));
    f_next_fd = 0;
    f_first_free = MMAP_MIN_ADDR;
}

Process::Process(virtaddr_t entry_point)
    : f_pager(new Pager())
    , f_workingDirectory("/") {
    f_threads.push_back(new Thread(entry_point, false, *this));
    f_next_fd = 0;
    f_first_free = MMAP_MIN_ADDR;
}

Process* Process::construct_kernel_process(virtaddr_t entry_point) {
    return new Process(entry_point, &Pager::kernel());
}

void Process::die(u32_t exitCode) {
    Thread* thisThread = Thread::current();

    bool suicide = false;

    f_lock.lock();
    for(auto* thread : f_threads) {
        if(thread == thisThread) {
            suicide = true;
            continue;
        }

        auto current_state = thread->f_state;
        thread->f_state = DYING;

        if(current_state == RUNNING)
            send_task_switch_irq(thread->f_preferred_core);

        Scheduler::remove_thread(thread);

        if(!thread->f_watched) thread->schedule_finalization();
    }

    f_exitCode = exitCode;

    if(suicide) {
        if(!thisThread->f_watched)
            thisThread->schedule_finalization();
        else
            thisThread->f_state = DYING;

        // We should not return from an exit syscall.
        f_lock.unlock();
        while(true) force_task_switch();
    }
}

fd_t Process::add_stream(Stream* stream) {
    f_lock.lock();
    fd_t fd = f_next_fd++;
    f_streams.insert({ fd, stream });

    f_lock.unlock();
    return fd;
}

void Process::close_stream(fd_t fd) {
    f_lock.lock();

    f_streams.erase(fd);

    f_lock.unlock();
}

Stream* Process::get_stream(fd_t fd) {
    f_lock.lock();

    auto val = f_streams.at(fd);

    f_lock.unlock();

    if(!val) return 0;
    return *val;
}

void Process::set_page_mapping(virtaddr_t addr, std::SharedPtr<MemoryEntry>& entry) {
    Locker lock(f_lock);

    auto currentEntryOpt = f_memorymap.at(addr);
    if(currentEntryOpt) {
        auto currentEntry = *currentEntryOpt;
        if(currentEntry->type == MemoryEntry::MEMORY) {
            f_pager->lock();
            f_pager->unmap(addr, 1);
            f_pager->unlock();
        }
    }
    f_memorymap[addr] = entry;
}

void Process::map_page(virtaddr_t addr, PhysicalPage& page, bool shared) {
    Locker lock(f_lock);
    f_pager->lock();

    page.ref();
    f_pager->map(page.addr(), addr, 1, page.flags());

    auto ptr = std::make_shared<MemoryEntry>();
    ptr->type = MemoryEntry::MEMORY;
    ptr->page = new PhysicalPage(page);
    ptr->shared = shared;

    f_pager->unlock();
    set_page_mapping(addr, ptr);

    if(f_first_free == addr)
        f_first_free = addr + 4096;
}

void Process::alloc_pages(virtaddr_t addr, size_t length, int flags, int prot) {
    Locker lock(f_lock);

    if((flags & MMAP_FLAG_SHARED) && prot != 0) {
        // Shared mapping
        for(size_t i = 0; i < length; ++i) {
            auto ptr = std::make_shared<MemoryEntry>();

            ptr->type = MemoryEntry::ANONYMOUS;
            ptr->shared = true;
            ptr->page = new SharedAnonymousPage(PageFlags(1, prot & MMAP_PROT_WRITE, 1, prot & MMAP_PROT_EXEC, 0, 0), flags & MMAP_FLAG_ZERO);

            set_page_mapping(addr + (i << 12), ptr);
        }
    } else {
        auto ptr = std::make_shared<MemoryEntry>();

        ptr->type = MemoryEntry::ANONYMOUS;
        ptr->shared = false;
        ptr->page = new AnonymousPage(PageFlags(1, prot & MMAP_PROT_WRITE, 1, prot & MMAP_PROT_EXEC, 0, 0), flags & MMAP_FLAG_ZERO);

        for(size_t i = 0; i < length; ++i)
            set_page_mapping(addr + (i << 12), ptr);
    }

    if(f_first_free == addr)
        f_first_free = addr + (length << 12);
}

void Process::null_pages(virtaddr_t addr, size_t length) {
    Locker lock(f_lock);

    auto ptr = std::make_shared<MemoryEntry>();

    ptr->type = MemoryEntry::EMPTY;
    ptr->shared = false;
    ptr->page = nullptr;

    for(size_t i = 0; i < length; ++i)
        set_page_mapping(addr + (i << 12), ptr);

    if(f_first_free == addr)
        f_first_free = addr + (length << 12);
}

void Process::file_pages(virtaddr_t addr, size_t length, FilePage* page) {
    Locker lock(f_lock);

    auto ptr = std::make_shared<MemoryEntry>();
    ptr->type = MemoryEntry::FILE;
    ptr->shared = page->shared();
    ptr->page = page;

    for(size_t i = 0; i < length; ++i)
        set_page_mapping(addr + (i << 12), ptr);

    if(f_first_free == addr)
        f_first_free = addr + (length << 12);
}

bool Process::handle_page_fault(virtaddr_t fault_address, u32_t code) {
    // We are accessing restricted memory
    if(fault_address >= KERNEL_START || fault_address == 0) return false;

    Locker locker(f_lock);

    // Get our mapping
    auto mappingOpt = f_memorymap.at(fault_address & ~0xFFF);

    // This is not mapped to anything
    if(!mappingOpt) return false;
    auto& mapping = *mappingOpt;

    switch(mapping->type) {
        case MemoryEntry::MEMORY: {
            auto* page = (PhysicalPage*)mapping->page;

            // Did we write a Copy-on-write page?
            if((code & 0x02) && page->copy_on_write()) {
                if(page->ref_count() > 1) {
                    // Copy the page
                    PhysicalPage new_page;
                    f_pager->lock();
                    auto vaddr = f_pager->kmap(new_page.addr(), 1, PageFlags(1, 1, 0, 0, 1, 0));

                    memcpy((void*)vaddr, (void*)(fault_address & ~0xFFF), 4096);

                    // Remap our new page
                    new_page.ref();
                    new_page.flags() = page->flags();
                    new_page.flags().writable = true;
                    f_pager->map(new_page.addr(), fault_address & ~0xFFF, 1, new_page.flags());

                    // Unmap the temp mapping
                    f_pager->unmap(vaddr, 1);
                    f_pager->unlock();

                    // Change the page in mappings
                    page->unref();
                    delete page;
                    mapping->page = new PhysicalPage(new_page);
                } else {
                    // We can just set the page as writable since it's the last reference
                    f_pager->lock();
                    page->copy_on_write() = false;
                    page->flags().writable = true;
                    f_pager->map(page->addr(), fault_address & ~0xFFF, 1, page->flags());
                    f_pager->unlock();
                }
                return true;
            }

            if(!(code & 0x01)) {
                // This page was not present when the fault happened but it is now.
                // Most likely another thread has caused a fault for this address
                // and it has been handled succesfully, so we don't have to
                // handle it again.
                return true;
            }

            // We did some bad stuff
            return false;
        }
        case MemoryEntry::EMPTY:
            // You don't mess with the null pages
            return false;
        case MemoryEntry::ANONYMOUS: {
            // Resolve the entry
            auto page = ((UnresolvedPage*)mapping->page)->resolve(fault_address);
            if(!page) return false;

            map_page(fault_address & ~0xFFF, page, mapping->shared);
            memset((void*)(fault_address & ~0xFFF), 0, 4096);
            return true;
        }
        case MemoryEntry::FILE: {
            // Resolve a file mapping
            auto* file_mapping = (FilePage*)mapping->page;
            auto page = file_mapping->resolve(fault_address);
            if(!page) return false;

            size_t offset = (fault_address - file_mapping->start_addr()) & ~0xFFF;
            MemoryFilePage* resolved_mapping = new MemoryFilePage(file_mapping->file(), page, offset);

            resolved_mapping->f_flags = file_mapping->flags();

            if(page.ref_count() != 0 && !file_mapping->shared() && file_mapping->writable()) {
                // Make a copy-on-write file mapping
                resolved_mapping->f_copy_on_write = true;
                resolved_mapping->f_flags.writable = false;
            }

            // Map the page
            auto ptr = std::make_shared<MemoryEntry>();
            ptr->type = MemoryEntry::FILE_MEMORY;
            ptr->shared = file_mapping->shared();
            ptr->page = resolved_mapping;

            f_pager->lock();
            page.ref();

            f_pager->map(page.addr(), fault_address & ~0xFFF, 1, resolved_mapping->f_flags);
            f_pager->unlock();

            set_page_mapping(fault_address & ~0xFFF, ptr);
            return true;
        }
        case MemoryEntry::FILE_MEMORY: {
            auto* file_mapping = (MemoryFilePage*)mapping->page;
            auto& page = file_mapping->f_page;

            if((code & 0x02) && file_mapping->f_copy_on_write) {
                if(page.ref_count() > 1) {
                    // Copy the page
                    PhysicalPage new_page;
                    f_pager->lock();
                    auto vaddr = f_pager->kmap(new_page.addr(), 1, PageFlags(1, 1, 0, 0, 1, 0));

                    memcpy((void*)vaddr, (void*)(fault_address & !0xFFF), 4096);

                    // Remap our new page
                    new_page.ref();
                    file_mapping->f_flags.writable = true;
                    f_pager->map(new_page.addr(), fault_address & ~0xFFF, 1, file_mapping->f_flags);

                    // Unmap the temp mapping
                    f_pager->unmap(vaddr, 1);
                    f_pager->unlock();

                    // Change the page in mappings
                    page.unref();
                    file_mapping->f_page = new_page;
                    file_mapping->f_copy_on_write = false;
                } else {
                    // We steal this page from vnode's shared pages
                    auto& vnode = file_mapping->f_file;
                    /// TODO: I don't know but this might cause concurrency issues if stuff happens at the wrong time
                    vnode->f_shared_pages.erase(file_mapping->f_offset);

                    // Remap
                    f_pager->lock();
                    file_mapping->f_flags.writable = true;
                    file_mapping->f_copy_on_write = false;
                    f_pager->map(page.addr(), fault_address & ~0xFFF, 1, file_mapping->f_flags);
                    f_pager->unlock();
                }
                return true;
            }

            // You are using your file mapping incorrectly, stop it.
            return false;
        }
    }

    return false;
}

Process::MemoryEntry::~MemoryEntry() {
    if(page != 0) {
        switch(type) {
            case MemoryEntry::ANONYMOUS:
                delete (AnonymousPage*)page;
                break;
            case MemoryEntry::MEMORY:
                delete (PhysicalPage*)page;
                break;
            case MemoryEntry::EMPTY:
                break;
            case MemoryEntry::FILE:
                delete (FilePage*)page;
                break;
            case MemoryEntry::FILE_MEMORY:
                delete (MemoryFilePage*)page;
                break;
        }
    }
}

virtaddr_t Process::get_free_addr(virtaddr_t hint, size_t length) {
    Locker locker(f_lock);

    virtaddr_t addr = hint != 0 ? hint : f_first_free;

    while(addr < KERNEL_START) {
        for(size_t i = 0; i < length; ++i) {
            auto val = f_memorymap.at(addr + (i << 12));
            if(val) {
                addr += 4096 * (i + 1);
                goto again;
            }
        }
        return addr;
    again:;
    }

    return 0;
}

void Process::unmap_pages(virtaddr_t addr, size_t length) {
    Locker lock(f_lock);
    Locker pager_lock(*f_pager);

    for(size_t i = 0; i < length; ++i) {
        virtaddr_t key = addr + (i << 12);
        auto entryOpt = f_memorymap.at(key);
        if(entryOpt) {
            auto& entry = *entryOpt;
            switch(entry->type) {
                case MemoryEntry::MEMORY: {
                    // Unmap a memory mapping
                    f_pager->unmap(key, 1);
                    break;
                } case MemoryEntry::FILE_MEMORY: {
                    // Unmap a file memory mapping
                    f_pager->unmap(key, 1);

                    // Sync the mapping to disk (if shared)
                    if(entry->shared) {
                        auto resolved_mapping = (MemoryFilePage*)entry->page;
                        resolved_mapping->f_file->filesystem()->sync_mapping(*resolved_mapping);
                    }
                    break;
                } default: break;
            }
            f_memorymap.erase(key);
        }
    }
}
