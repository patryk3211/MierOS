#include <tasking/syscalls/syscall.hpp>
#include <tasking/scheduler.hpp>
#include <arch/interrupts.h>
#include <memory/page/memoryfilepage.hpp>
#include <tasking/syscalls/map.hpp>
#include <fs/vfs.hpp>
#include <vector.hpp>
#include <elf.h>

using namespace kernel;

DEF_SYSCALL(exit, exitCode) {
    TRACE("(syscall) Process (pid = %d) exited with code %d\n", proc.main_thread()->pid(), exitCode);
    proc.die(exitCode);
    return 0;
}

DEF_SYSCALL(fork) {
    Process* child = proc.fork();

    Scheduler::schedule_process(*child);
    TRACE("(syscall) New process forked from pid %d (child pid = %d)\n", proc.main_thread()->pid(), child->main_thread()->pid());

    return child->main_thread()->pid();
}

DEF_SYSCALL(execve, filename, argv, envp) {
    const char* path = (const char*)filename;

    VNodePtr file = nullptr;
    if(path[0] == '/') {
        auto ret = VFS::instance()->get_file(nullptr, path, {});
        if(!ret) return -ret.errno();
        file = *ret;
    } else {
        auto ret = VFS::instance()->get_file(nullptr, (proc.cwd() + path).c_str(), {});
        if(!ret) return -ret.errno();
        file = *ret;
    }

    TRACE("(syscall) Process (pid = %d) trying to execute file '%s'\n", proc.main_thread()->pid(), path);
    return proc.execve(file, (char**)argv, (char**)envp);
}

Process* Process::fork() {
    f_lock.lock();
    Thread* caller = Thread::current();

    // Create a child process and clone the CPU state
    Process* child = new Process(CPUSTATE_IP(caller->f_syscall_state));
    memcpy(child->main_thread()->f_ksp, caller->f_syscall_state, sizeof(CPUState));
    CPUSTATE_RET(child->main_thread()->f_ksp) = 0;
    child->main_thread()->f_ksp->cr3 = child->f_pager->cr3();

    // Copy the memory space
    f_pager->lock();
    for(auto entry : f_memorymap) {
        switch(entry.value->type) {
            case MemoryEntry::MEMORY: {
                // Set all writable pages as copy-on-write
                PhysicalPage& page = *(PhysicalPage*)entry.value->page;
                if(page.flags().writable && !entry.value->shared) {
                    page.flags().writable = false;
                    page.copy_on_write() = true;
                    f_pager->map(page.addr(), entry.key, 1, page.flags());
                }

                // Map page to child
                child->map_page(entry.key, page, entry.value->shared);
                break;
            }
            case MemoryEntry::FILE:
            case MemoryEntry::EMPTY:
            case MemoryEntry::ANONYMOUS:
                // Copy the memory entries
                child->f_memorymap.insert({ entry.key, entry.value });
                break;
            case MemoryEntry::FILE_MEMORY: {
                MemoryFilePage& page = *(MemoryFilePage*)entry.value->page;
                if(page.f_flags.writable && !entry.value->shared) {
                    page.f_copy_on_write = true;
                    page.f_flags.writable = false;
                    f_pager->map(page.f_page.addr(), entry.key, 1, page.f_flags);
                }

                child->f_pager->lock();
                auto memoryEntry = std::make_shared<MemoryEntry>();
                memoryEntry->page = new MemoryFilePage(page);
                memoryEntry->type = MemoryEntry::FILE_MEMORY;
                memoryEntry->shared = entry.value->shared;
                child->f_memorymap.insert({ entry.key, memoryEntry });
                child->f_pager->map(page.f_page.addr(), entry.key, 1, page.f_flags);
                child->f_pager->unlock();
                break;
            }
        }
    }
    f_pager->unlock();

    // Clone streams
    child->f_next_fd = f_next_fd;
    for(auto entry : f_streams) {
        child->f_streams.insert({ entry.key, entry.value });
    }

    // Copy other properties of the process
    child->f_workingDirectory = f_workingDirectory;

    f_lock.unlock();

    return child;
}

#define PROCESS_STACK_SIZE 1024 * 1024

const u8_t validElf[] = { 0x7F, 'E', 'L', 'F', 2, 1, 1, 0 };
ValueOrError<void> Process::execve(const VNodePtr& file, char* argv[], char* envp[]) {
    auto stream = FileStream(file);
    auto status = stream.open(FILE_OPEN_MODE_READ);
    if(!status) return status;

    // We have opened the file, check if the elf is correct
    Elf64_Header header;
    if(stream.read(&header, sizeof(header)) != sizeof(header)) {
        // Unexpected EOF
        return ERR_NO_EXEC;
    }

    // Validate the header
    if(!memcmp(validElf, &header, 8)) {
        // Invalid header magic
        return ERR_NO_EXEC;
    }

    if(header.type != ET_EXEC && header.type != ET_DYN)
        return ERR_NO_EXEC;

    Elf64_Phdr programHeaders[header.phdr_entry_count];
    stream.seek(header.phdr_offset, SEEK_MODE_BEG);
    size_t readCount = header.phdr_entry_count * sizeof(Elf64_Phdr);
    if(stream.read(programHeaders, readCount) != readCount) {
        // Unexpected EOF
        return ERR_NO_EXEC;
    }

    // No return part starts here
    f_lock.lock();
    auto* thisThread = Thread::current();
    pid_t mainPid = main_thread()->f_pid;

    ASSERT_F(f_threads.contains(thisThread), "Invalid current thread is trying to exec on a non parent process");

    // Stop all other threads
    for(auto* thread : f_threads) {
        if(thread == thisThread) continue;

        auto current_state = thread->f_state;
        thread->f_state = DYING;

        if(current_state == RUNNING)
            send_task_switch_irq(thread->f_preferred_core);

        Scheduler::remove_thread(thread);

        if(!thread->f_watched) thread->schedule_finalization();
    }

    // Only this thread will be left here
    f_threads.clear();
    f_threads.push_back(thisThread);

    // Change pid, if this thread was not main, it now is
    Thread::s_threads.erase(thisThread->f_pid);
    thisThread->f_pid = mainPid;
    Thread::s_threads.insert({ mainPid, thisThread });

    std::List<std::String<>> argStore;
    std::List<std::String<>> envStore;

    {
        if(argv) {
            char** argPtr = argv;
            while(*argPtr) {
                argStore.push_back(std::String<>(*argPtr++));
            }
        }

        if(envp) {
            char** envPtr = envp;
            while(*envPtr) {
                envStore.push_back(std::String<>(*envPtr++));
            }
        }
    }

    ASSERT_F(f_pager == &Pager::active(), "This thread is using a different pager than it's parent process");
    if(f_pager == &Pager::kernel()) {
        // We are transforming a kernel task into a userspace task
        f_pager = new Pager();
        f_pager->enable();
    }

    f_pager->lock();
    // Unmap all pages
    for(auto [key, entry] : f_memorymap) {
        if(entry->type == MemoryEntry::MEMORY || entry->type == MemoryEntry::FILE_MEMORY) {
            // Unmap page
            f_pager->unmap(key, 1);
        }
        if(entry->type == MemoryEntry::FILE_MEMORY && entry->shared) {
            // Flush to file
            auto resolved_mapping = (MemoryFilePage*)entry->page;
            resolved_mapping->f_file->filesystem()->sync_mapping(*resolved_mapping);
        }
    }
    f_memorymap.clear();

    virtaddr_t base = 0;
    if(header.type == ET_DYN) {
        // Find a suitable base address for a PIE.
        // For now we just set it to a hardcoded value
        // but in the future we might randomize this address.
        base = PIE_START;
    }

    // Process headers
    for(int i = 0; i < header.phdr_entry_count; ++i) {
        auto& header = programHeaders[i];
        switch(header.type) {
            case PT_LOAD: {
                if((header.alignment & 0xFFF) != 0) {
                    ASSERT_NOT_REACHED("Alignment <4096 is currently not supported");
                }

                size_t alignedMemSize = header.mem_size + (header.vaddr & 0xFFF);
                size_t alignedFileSize = header.file_size + (header.offset & 0xFFF);

                size_t pageCount = (alignedMemSize >> 12) + ((alignedMemSize & 0xFFF) == 0 ? 0 : 1);
                size_t filePages = (alignedFileSize >> 12) + ((alignedFileSize & 0xFFF) == 0 ? 0 : 1);

                if(filePages > 0) {
                    // Map a file segment here
                    FilePage* page = new FilePage(file, base + (header.vaddr & ~0xFFF), header.offset & ~0xFFF, !(header.flags & 2), header.flags & 2, header.flags & 1);

                    file_pages(base + (header.vaddr & ~0xFFF), filePages, page);

                    // We need this since the .bss section will likely intersect
                    // with some data and we need to guarantee that it is cleared
                    if(alignedMemSize > alignedFileSize && ((header.vaddr + header.file_size) & 0xFFF)) {
                        // We need to zero out some memory
                        memset((void*)(base + header.vaddr + header.file_size), 0, 4096 - ((header.vaddr + header.file_size) & 0xFFF));
                    }
                }

                size_t pagesLeft = pageCount - filePages;
                if(pagesLeft > 0) {
                    // Map anonymous pages here
                    int prot = 1;
                    prot |= (header.flags & 2) ? MMAP_PROT_WRITE : 0;
                    prot |= (header.flags & 1) ? MMAP_PROT_EXEC : 0;
                    alloc_pages(base + ((header.vaddr + (filePages << 12)) & ~0xFFF), pagesLeft, MMAP_FLAG_ZERO, prot);
                }

                break;
            }
            default: break;
        }
    }

    virtaddr_t stackStart = f_pager->getFreeRange(0x7FFF00000000 - PROCESS_STACK_SIZE, PROCESS_STACK_SIZE >> 12);
    alloc_pages(stackStart, PROCESS_STACK_SIZE >> 12, MMAP_FLAG_PRIVATE, MMAP_PROT_READ | MMAP_PROT_WRITE);
    virtaddr_t stackEnd = stackStart + PROCESS_STACK_SIZE;

    // Exec stack setup

    /// TODO: [10.01.2023] Will also need to take aux vector size into account
    size_t execStackSize = (4 + argStore.size() + envStore.size()) * sizeof(u64_t);

    auto getArgStoreSize = [](std::List<std::String<>>& args) {
        size_t size = 0;
        for(auto& arg : args)
            size += arg.length() + 1;
        return size;
    };

    execStackSize += getArgStoreSize(argStore);
    execStackSize += getArgStoreSize(envStore);

    // Align up to 16 byte boundary
    if(execStackSize & 0xF) execStackSize = (execStackSize | 0xF) + 1;

    u64_t* execStack = (u64_t*)(stackEnd - execStackSize);
    size_t stackOffset = 0;

    // argc
    execStack[stackOffset++] = argStore.size();

    /// TODO: [10.01.2023] Will also need to take aux vector size into account
    virtaddr_t infoBlock = (virtaddr_t)execStack + (4 + argStore.size() + envStore.size()) * sizeof(u64_t);

    // argv
    for(auto& arg : argStore) {
        execStack[stackOffset++] = infoBlock;
        memcpy((void*)infoBlock, arg.c_str(), arg.length() + 1);
        infoBlock += arg.length() + 1;
    }
    execStack[stackOffset++] = 0;

    // envp
    for(auto& env : envStore) {
        execStack[stackOffset++] = infoBlock;
        memcpy((void*)infoBlock, env.c_str(), env.length() + 1);
        infoBlock += env.length() + 1;
    }
    execStack[stackOffset++] = 0;

    // null aux vector
    execStack[stackOffset++] = 0;

    thisThread->make_ks(base + header.entry_point, (virtaddr_t)execStack);
    f_first_free = MMAP_MIN_ADDR;

    f_pager->unlock();
    f_lock.unlock();

    // We will now return into our new state
    return 0;
}
