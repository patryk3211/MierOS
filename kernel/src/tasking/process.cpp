#include <arch/interrupts.h>
#include <errno.h>
#include <tasking/process.hpp>
#include <tasking/scheduler.hpp>
#include <tasking/syscalls/map.hpp>
#include <locking/locker.hpp>
#include <memory/page/anonymous_memory.hpp>
#include <asm/fcntl.h>
#include <asm/wait.h>

using namespace kernel;

Process::Process(Pager* pager)
    : f_pager(pager)
    , f_workingDirectory("/")
    , f_signal_actions(64)
    , f_terminal(nullptr) {
    f_next_fd = 0;
    f_first_free = MMAP_MIN_ADDR;

    f_uid = 1;
    f_gid = 1;
    f_euid = 1;
    f_egid = 1;

    f_parent = 0;

    reset_signals();
}

Process::Process(virtaddr_t entry_point, Pager* pager)
    : Process(pager) {
    f_threads.push_back(new Thread(entry_point, true, this));

    f_group = ProcessGroup::get_make_group(pid());
}

Process::Process(virtaddr_t entry_point)
    : Process(new Pager()) {
    f_threads.push_back(new Thread(entry_point, false, this));

    f_group = ProcessGroup::get_make_group(pid());
}

Process* Process::construct_kernel_process(virtaddr_t entry_point) {
    return new Process(entry_point, &Pager::kernel());
}

pid_t Process::pid() {
    return main_thread()->pid();
}

pid_t Process::ppid() {
    return f_parent->pid();
}

pid_t Process::pgid() {
    return f_group->id();
}

ProcessGroup& Process::group() {
    return *f_group;
}

ValueOrError<void> Process::set_group(pid_t id) {
    if(f_group->id() == id)
        return { };

    f_group = ProcessGroup::get_make_group(id);
    return { };
}

void Process::die(u32_t exitCode) {
    f_lock.lock();

    f_status = W_EXITCODE(exitCode, 0);

    // Reduce the memory footprint of this process
    bool suicide = minimize();

    if(suicide) {
        auto* thisThread = Thread::current();
        thisThread->change_state(DEAD);

        // We should not return from an exit syscall.
        f_lock.unlock();
        while(true) force_task_switch();
    }
}

bool Process::minimize() {
    auto* thisThread = Thread::current();

    bool suicide = false;

    for(auto* thread : f_threads) {
        if(thread == thisThread) {
            suicide = true;
            continue;
        }

        if(thread->f_state == RUNNING) {
            thread->f_state = DYING;
            send_task_switch_irq(thread->f_preferred_core);
        }

        Scheduler::remove_thread(thread);
        thread->change_state(DEAD);
    }

    // Get rid of memory mappings
    f_pager->lock();
    for(auto [addr, entry] : f_resolved_memory) {
        if(entry.f_file && entry.f_shared) {
            // File backed memory
            auto flags = f_pager->getFlags(addr);
            if(flags.writable && flags.dirty) {
                // Memory is dirty and shared, write back to file
                entry.f_file->filesystem()->sync_mapping(entry);
            }
        }

        entry.f_page.unref();
    }

    f_pager->unlock();
    f_resolved_memory.clear();
    f_resolvable_memory.clear();

    if(suicide) {
        if(main_thread() != thisThread) {
            // Swap the PIDs
            Thread* mainThread = main_thread();
            Thread::s_threads.erase(mainThread->f_pid);
            Thread::s_threads.erase(thisThread->f_pid);
            
            pid_t temp = thisThread->f_pid;
            thisThread->f_pid = mainThread->f_pid;
            mainThread->f_pid = temp;
        
            Thread::s_threads.insert({ mainThread->f_pid, mainThread });
            Thread::s_threads.insert({ thisThread->f_pid, thisThread });
        }

        // Keep only this thread for a PID reference
        for(auto* thread : f_threads) {
            if(thread != thisThread)
                delete thread;
        }

        f_threads.clear();
        f_threads.push_back(thisThread);

        // We can delete the pager but we need to activate the kernel pager
        enter_critical();
        Pager::kernel().enable();
        delete f_pager;
        f_pager = &Pager::kernel();
        leave_critical();
    } else {
        // Keep only the main thread for a PID reference
        for(auto* thread : f_threads) {
            if(thread != main_thread())
                delete thread;
        }

        auto* main = main_thread();
        f_threads.clear();
        f_threads.push_back(main);

        // If this is not a suicide we can delete the pager
        delete f_pager;
    }

    // Delete streams
    f_streams.clear();

    // Delete signal handler array
    f_signal_actions.clear();

    // Change the parent of children
    for(auto& child : f_children) {
        // New parent is the parent of this process
        child->f_parent = f_parent;
        f_parent->f_children.push_back(child);
    }
    f_children.clear();

    return suicide;
}

void Process::reset_signals() {
    for(auto& entry : f_signal_actions) {
        entry.mask = 0;
        entry.flags = 0;
        entry.handler = 0;
    }
}

void Process::signal_lock() {
    f_signal_lock.lock();
}

void Process::signal_unlock() {
    f_signal_lock.unlock();
}

SignalAction& Process::action(int sigNum) {
    return f_signal_actions[sigNum];
}

void Process::signal(std::SharedPtr<siginfo_t> info, pid_t threadDelivery) {
    // Make sure we unlock this as fast as possible
    enter_critical();
    f_signal_lock.lock();
    f_signal_queue.push_back({ info, threadDelivery });
    f_signal_lock.unlock();
    leave_critical();
}

void Process::handle_signal(Thread* onThread) {
    if(onThread->f_in_kernel) {
        // We cannot handle signals for threads that are in the kernel.
        // TODO: [25.02.2023] We need to implement interruptable syscalls.
        return;
    }

    if(!f_signal_lock.try_lock()) {
        // We failed to acquire the signal lock, we will defer signal handling since
        // at the moment we are modifying the signal variables (raising a signal, changing a handler, etc.)
        return;
    }
    // Check for signals to handle
    if(!f_signal_queue.size()) {
        f_signal_lock.unlock();
        return;
    }

    // Look for a signal that isn't masked on the thread
    auto iter = f_signal_queue.begin();
    while(iter != f_signal_queue.end()) {
        if((!iter->deliver_to || iter->deliver_to == onThread->pid()) &&
           ((1 << iter->info->si_signo) & ~onThread->sigmask())) {
            // Handle this signal
            Signal sig = *iter;
            f_signal_queue.erase(iter);
            // We can release the lock once we grab the signal
            f_signal_lock.unlock();

            SignalAction& sigAct = action(sig.info->si_signo);

            auto* ucontext = onThread->save_signal_state();
            onThread->sigmask() |= sigAct.mask;
            if(!(sigAct.flags & SA_NODEFER))
                onThread->sigmask() |= (1 << sig.info->si_signo);

            CPUSTATE_IP(onThread->f_ksp) = reinterpret_cast<uintptr_t>(sigAct.handler);

            ucontext->uc_link = 0;
            ucontext->uc_sigmask = onThread->sigmask();
            ucontext->uc_stack.ss_sp = 0;
            ucontext->uc_stack.ss_size = 0;
            ucontext->uc_stack.ss_flags = 0;

            memcpy(&ucontext->uc_mcontext.info, sig.info.ptr(), sizeof(siginfo_t));

            // NOTE: [23.02.2023] This is ABI specific code, I don't know where it should go
            // (probably into the arch code) but it definitely is not here.
            onThread->f_ksp->rdi = sig.info->si_signo;
            if(sigAct.flags & SA_SIGINFO) {
                onThread->f_ksp->rsi = reinterpret_cast<uintptr_t>(&ucontext->uc_mcontext.info);
                onThread->f_ksp->rdx = reinterpret_cast<uintptr_t>(ucontext);
            }

            if(sigAct.flags & SA_RESETHAND)
                sigAct.handler = 0;
            
            if(sigAct.restorer) {
                // Put restorer return address on stack
                onThread->f_ksp->rsp -= sizeof(void(*)());
                *reinterpret_cast<void (**)()>(onThread->f_ksp->rsp) = sigAct.restorer;
            }

            return;
        }
        ++iter;
    }

    f_signal_lock.unlock();
}

fd_t Process::add_stream(Stream* stream) {
    Locker lock(f_lock);

    fd_t fd = f_next_fd;
    while(f_streams.find(fd) != f_streams.end())
        ++fd;

    f_streams.insert({ fd, stream });
    if(fd == f_next_fd)
        f_next_fd = fd + 1;
    return fd;
}

ValueOrError<void> Process::close_stream(fd_t fd) {
    Locker lock(f_lock);

    if(f_streams.erase(fd)) {
        if(fd < f_next_fd)
            f_next_fd = fd;
        return { };
    } else {
        return EBADF;
    }
}

Stream* Process::get_stream(fd_t fd) {
    f_lock.lock();

    auto val = f_streams.at(fd);

    f_lock.unlock();

    if(!val) return 0;
    return &val->base();
}

ValueOrError<fd_t> Process::dup_stream(fd_t oldFd, fd_t newFd, int flags) {
    Locker lock(f_lock);

    auto val = f_streams.at(oldFd);
    if(!val)
        return EBADF;

    if(newFd != -1) {
        if(flags & DUP_SOFT) {
            while(f_streams.find(newFd) != f_streams.end())
                ++newFd;
            if(newFd == f_next_fd)
                f_next_fd = newFd + 1;
        } else {
            if(f_streams.at(newFd))
                f_streams.erase(newFd);
        }
    } else {
        newFd = f_next_fd;
        while(f_streams.find(newFd) != f_streams.end())
            ++newFd;
        if(newFd == f_next_fd)
            f_next_fd = newFd + 1;
    }

    f_streams.insert({ newFd, *val });
    if(flags & O_CLOEXEC) {
        auto& wrapper = *f_streams.at(newFd);
        wrapper.flags() |= O_CLOEXEC;
    }
    return newFd;
}

ValueOrError<StreamWrapper&> Process::get_stream_wrapper(fd_t fd) {
    auto refOpt = f_streams.at(fd);
    return refOpt ? ValueOrError<StreamWrapper&>(*refOpt) : EBADF;
}

void Process::map_page(virtaddr_t addr, PhysicalPage& page, bool shared, bool copyOnWrite) {
    Locker lock(f_lock);
    f_pager->lock();

    page.ref();
    f_pager->map(page.addr(), addr, 1, page.flags());
    f_pager->unlock();

    ResolvedMemoryEntry entry(page);
    entry.f_shared = shared;
    entry.f_copy_on_write = copyOnWrite;
    f_resolved_memory.insert({ addr, entry });
}

void Process::alloc_pages(virtaddr_t addr, size_t length, int flags, int prot) {
    ASSERT_F((addr & 0xFFF) == 0, "Address must be aligned to 4096 bytes");
    ASSERT_F(prot != 0, "Protection on anonymous pages must not be zero");

    Locker lock(f_lock);

    std::SharedPtr<ResolvableMemoryEntry> entry;

    bool shared = flags & MMAP_FLAG_SHARED;
    if(shared) {
        entry = std::make_shared<SharedAnonymousMemory>(addr, length);
    } else {
        entry = std::make_shared<AnonymousMemory>();
    }

    entry->f_shared = shared;
    entry->f_writable = prot & MMAP_PROT_WRITE;
    entry->f_executable = prot & MMAP_PROT_EXEC;

    f_resolvable_memory[addr] = entry;
    f_resolvable_memory.insert({ addr + (length << 12), nullptr });
}

void Process::null_pages(virtaddr_t addr, size_t length) {
    Locker lock(f_lock);

    // Default implementation of ResolvableMemoryEntry never resolves into a page if it's not backed by a file
    f_resolvable_memory[addr] = std::make_shared<ResolvableMemoryEntry>();
    f_resolvable_memory.insert({ addr + (length << 12), nullptr });
}

void Process::file_pages(virtaddr_t addr, size_t length, VNodePtr file, size_t fileOffset, bool shared, bool write, bool execute) {
    ASSERT_F((addr & 0xFFF) == 0, "Address must be aligned to 4096 bytes");
    ASSERT_F((fileOffset & 0xFFF) == 0, "File offset must be aligned to 4096 bytes");

    Locker lock(f_lock);

    auto entry = std::make_shared<ResolvableMemoryEntry>();
    entry->f_file = file;
    entry->f_file_offset = fileOffset;

    entry->f_start = addr;
    entry->f_length = length;

    entry->f_shared = shared;
    entry->f_writable = write;
    entry->f_executable = execute;

    // Align to 4096 bytes
    if(length & 0xFFF)
        length = (length | 0xFFF) + 1;

    f_resolvable_memory[addr] = entry;
    f_resolvable_memory.insert({ addr + length, nullptr });
}

bool Process::handle_page_fault(virtaddr_t fault_address, u32_t code) {
    // We are accessing restricted memory
    if(fault_address >= KERNEL_START || fault_address == 0) return false;

    Locker locker(f_lock);

    if(code & 1) {
        // Page is present
        auto pageOpt = f_resolved_memory.at(fault_address & ~0xFFF);
        ASSERT_F(pageOpt, "Page is missing from the resolved memory map of the process");

        auto& page = *pageOpt;
        if((code & 2) && page.f_copy_on_write) {
            // Writing to a Copy-on-write page
            ASSERT_F(!page.f_shared, "A shared page cannot be copy-on-write");

            // Copy the page
            PhysicalPage new_page;
            f_pager->lock();
            auto vaddr = f_pager->kmap(new_page.addr(), 1, PageFlags(1, 1, 0, 0, 1, 0));

            memcpy((void*)vaddr, (void*)(fault_address & ~0xFFF), 4096);

            // Remap our new page
            new_page.ref();
            new_page.flags() = page.f_page_flags;
            new_page.flags().writable = true;
            f_pager->map(new_page.addr(), fault_address & ~0xFFF, 1, new_page.flags());

            // Unmap the temp mapping
            f_pager->unmap(vaddr, 1);
            f_pager->unlock();

            // Change the page in mappings
            page.f_page.unref();
            // Create a new entry
            auto entry = ResolvedMemoryEntry(new_page);
            entry.f_copy_on_write = false;
            entry.f_shared = false;
            page = entry;

            return true;
        }
    } else {
        if(auto opt = f_resolved_memory.at(fault_address & ~0xFFF); opt) {
            Locker pageLock(*f_pager);
            if(f_pager->getFlags(fault_address & ~0xFFF).present) {
                // This page was not present when the fault happened but it is now.
                // Most likely another thread has caused a fault for this address
                // and it has been handled succesfully, so we don't have to
                // handle it again.
                return true;
            }

            // This fault is a result of the fork syscall. The page is resolved
            // and ready to be used but it is not mapped in to save time.
            f_pager->map(opt->f_page.addr(), fault_address & ~0xFFF, 1, opt->f_page_flags);
            return true;
        }

        // Page is not present
        auto entryOpt = f_resolvable_memory.at_range(fault_address & ~0xFFF);
        auto& ptr = entryOpt.resolve_or(nullptr);

        if(!ptr)
            return false;

        auto resolved = ptr->resolve(fault_address & ~0xFFF);
        if(!resolved)
            return false;

        resolved->f_page.ref();
        
        f_pager->lock();
        f_pager->map(resolved->f_page.addr(), fault_address & ~0xFFF, 1, resolved->f_page_flags);
        f_resolved_memory.insert({ fault_address & ~0xFFF, *resolved });

        if(!ptr->f_shared && ptr->f_writable) {
            size_t mappingOffset = (fault_address & ~0xFFF) - ptr->f_start;
            ssize_t mappingLeft = ptr->f_length - mappingOffset;
            if(mappingLeft < 4096) {
                if(mappingLeft < 0)
                    mappingLeft = 0;
                memset((u8_t*)(fault_address & ~0xFFF) + mappingLeft, 0, 4096 - mappingLeft);
            }
        }
        f_pager->unlock();

        return true;
    }

    return false;
}

virtaddr_t Process::get_free_addr(virtaddr_t hint, size_t length) {
    Locker locker(f_lock);

    virtaddr_t addr = hint != 0 ? hint : f_first_free;

    while(addr < KERNEL_START) {
        bool free = true;

        for(size_t i = 0; i < length; ++i) {
            // Check in both resolved and resolvable maps to see
            // if the address is already in use.
            if(f_resolved_memory.at(addr + (i << 12)) ||
               f_resolvable_memory.at_range(addr + (i << 12)).resolve_or(nullptr)) {
                if(f_first_free == addr)
                    f_first_free = addr + ((i + 1) << 12);
                addr += (i + 1) << 12;
                free = false;
                break;
            }
        }

        if(free)
            return addr;
    }
    return 0;
}

void Process::unmap_pages(virtaddr_t addr, size_t length) {
    Locker lock(f_lock);
    Locker pager_lock(*f_pager);

    /*for(size_t i = 0; i < length; ++i) {
        virtaddr_t key = addr + (i << 12);
        auto entryOpt = f_memory_map.at(key);
        if(entryOpt) {
            auto& entry = *entryOpt;
            switch(entry->type) {
                case MemoryEntry::MEMORY: {
                    // Unmap a memory mapping
                    f_pager->unmap(key, 1);
                    ((PhysicalPage*)entry->page)->unref();
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
            //f_memory_map.erase(key);
        }
    }*/

    panic("IMPLEMENT THIS");
}

ValueOrError<void> Process::wait_for_child(GroupSleepQueue::WaitInfo* info, bool sleep) {
    bool foundChild = false;
    for(auto& child : f_children) {
        auto status = child->main_thread()->get_state(false);
        if(status == DEAD) {
            info->f_waker_pid = child->pid();
            info->f_waker_status = child->f_status;
            return { };
        }
        foundChild = true;
    }

    if(!foundChild)
        return ESRCH;

    if(sleep) {
        f_children_wait.sleep(info, 0);
    } else {
        info->f_waker_status = 0;
        info->f_waker_pid = 0;
    }

    return { };
}

void Process::notify_state_change(ThreadState state) {
    if(state == DEAD) {
        GroupSleepQueue::WaitInfo info = { pid(), static_cast<int>(f_status) };
        f_group->notify_wait(info, ppid());
        f_parent->f_children_wait.wakeup(info, 0);
    }
}

uid_t Process::uid() {
    return f_uid;
}

uid_t Process::euid() {
    return f_euid;
}

gid_t Process::gid() {
    return f_gid;
}

gid_t Process::egid() {
    return f_egid;
}

void Process::cleanup_process(Process* proc) {
    proc->f_group->remove(proc);

    // Remove process from parent children list
    auto iter = proc->f_parent->f_children.begin();
    while(iter != proc->f_parent->f_children.end()) {
        if(*iter == proc) {
            proc->f_parent->f_children.erase(iter);
            break;
        }
        ++iter;
    }

    // Remove the main thread
    delete proc->main_thread();

    // Delete the process object itself
    delete proc;
}

