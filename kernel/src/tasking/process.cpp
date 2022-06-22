#include <arch/interrupts.h>
#include <errno.h>
#include <tasking/process.hpp>
#include <tasking/scheduler.hpp>
#include <tasking/syscalls/map.hpp>
#include <memory/page/anonpage.hpp>

using namespace kernel;

Process::Process(virtaddr_t entry_point, Pager* pager)
    : f_pager(pager)
    , f_workingDirectory("/") {
    f_threads.push_back(new Thread(entry_point, true, *this));
    f_next_fd = 0;
}

Process::Process(virtaddr_t entry_point)
    : f_pager(new Pager())
    , f_workingDirectory("/") {
    f_threads.push_back(new Thread(entry_point, false, *this));
    f_next_fd = 0;
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

void Process::map_page(virtaddr_t addr, PhysicalPage& page, bool shared) {
    f_lock.lock();
    f_pager->lock();

    page.ref();
    f_pager->map(page.addr(), addr, 1, page.flags());

    auto ptr = std::make_shared<MemoryEntry>();
    ptr->type = MemoryEntry::MEMORY;
    ptr->page = new PhysicalPage(page);
    ptr->shared = shared;
    f_memorymap[addr] = ptr;

    f_pager->unlock();
    f_lock.unlock();
}

void Process::alloc_pages(virtaddr_t addr, size_t length, int flags, int prot) {
    f_lock.lock();

    if((flags & MMAP_FLAG_SHARED) && prot != 0) {
        // Shared mapping
        for(size_t i = 0; i < length; ++i) {
            auto ptr = std::make_shared<MemoryEntry>();

            ptr->type = MemoryEntry::ANONYMOUS;
            ptr->shared = true;
            ptr->page = new SharedAnonymousPage(PageFlags(1, prot & MMAP_PROT_WRITE, 1, prot & MMAP_PROT_EXEC, 0, 0));

            f_memorymap[addr + (i << 12)] = ptr;
        }
    } else {
        auto ptr = std::make_shared<MemoryEntry>();

        ptr->type = MemoryEntry::ANONYMOUS;
        ptr->shared = false;

        if(prot == 0) {
            // This page is not accessible
            ptr->page = nullptr;
        } else {
            // Use prot values
            ptr->page = new AnonymousPage(PageFlags(1, prot & MMAP_PROT_WRITE, 1, prot & MMAP_PROT_EXEC, 0, 0));
        }

        for(size_t i = 0; i < length; ++i)
            f_memorymap[addr + (i << 12)] = ptr;
    }

    f_lock.unlock();
}

void Process::handle_page_fault(virtaddr_t fault_address, u32_t code) {
    f_lock.lock();

    // Do the things...

    f_lock.unlock();
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
        }
    }
}
