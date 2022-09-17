#include <tasking/syscall.h>
#include <tasking/scheduler.hpp>
#include <arch/interrupts.h>
#include <memory/page/memoryfilepage.hpp>
#include <tasking/syscalls/map.hpp>

using namespace kernel;

syscall_arg_t syscall_exit(Process& proc, syscall_arg_t exitCode) {
    proc.die(exitCode);
    return 0;
}

syscall_arg_t syscall_fork(Process& proc) {
    Process* child = proc.fork();

    Scheduler::schedule_process(*child);

    return child->main_thread()->pid();
}

syscall_arg_t syscall_execve(Process& proc) {

}

Process* Process::fork() {
    f_lock.lock();
    Thread* caller = Thread::current();

    // Create a child process and clone the CPU state
    Process* child = new Process(CPUSTATE_IP(caller->f_syscall_state));
    memcpy(child->main_thread()->f_ksp, caller->f_syscall_state, sizeof(CPUState));
    CPUSTATE_RET(child->main_thread()->f_ksp) = 0;

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
            case MemoryEntry::EMPTY:
            case MemoryEntry::ANONYMOUS:
                // Copy the memory entries
                child->f_memorymap[entry.key] = entry.value;
                break;
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

    // Transfer argv and envp to a new page
    std::List<PhysicalPage> argumentPages;
    // We only need the envp offset since argv is always first
    size_t envpOffset = 0;

    {
        // Add 2 nullptrs
        size_t totalMoveSize = sizeof(char*) * 2;

        // First calculate the total required size for our arguments
        auto sizeCount = [&totalMoveSize](char** data) {
            size_t count = 0;
            for(char** ptr = data; *ptr != 0; ++ptr) {
                totalMoveSize += sizeof(char*);
                totalMoveSize += strlen(*ptr) + 1;
                ++count;
            }

            // Align to 8 bytes
            if(totalMoveSize & 7)
                totalMoveSize = (totalMoveSize | 7) + 1;

            return count;
        };

        size_t argCount = sizeCount(argv);
        size_t envCount = sizeCount(envp);

        auto& pager = Pager::active();
        pager.lock();

        // Then allocate our pages and map them continuously
        size_t pageCount = (totalMoveSize >> 12) + ((totalMoveSize & 0xFFF) == 0 ? 0 : 1);
        virtaddr_t startAddr = pager.getFreeRange(0, pageCount);

        for(size_t i = 0; i < pageCount; ++i) {
            PhysicalPage page;
            page.flags() = PageFlags(1, 1, 0, 0, 0, 0);

            argumentPages.push_back(page);
            pager.map(page.addr(), startAddr + (i << 12), 1, page.flags());
        }

        // Now move all the data to it's new location
        size_t currentOffset = 0;
        auto moveData = [&currentOffset, &startAddr](char** data, size_t count) {
            size_t start = currentOffset;
            size_t dataOffset = start + (count + 1) * sizeof(char*);

            for(size_t i = 0; i < count; ++i) {
                // Set offset of the argument in the pages
                ((uintptr_t*)(startAddr + start))[i] = dataOffset;

                // Copy the argument value
                size_t entryLen = strlen(data[i]) + 1;
                memcpy((void*)(startAddr + dataOffset), data[i], entryLen);
                dataOffset += entryLen;
            }

            // Align the data offset for next move
            if(dataOffset & 7)
                dataOffset = (dataOffset | 7) + 1;

            currentOffset = dataOffset;
            return start;
        };

        moveData(argv, argCount);
        envpOffset = moveData(envp, envCount);

        // Unmap the pages (They will be remapped when memory map of the process is established)
        pager.unmap(startAddr, pageCount);

        pager.unlock();
    }

    ASSERT_F(f_pager == &Pager::active(), "This thread is using a different pager than it's parent process");

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

    virtaddr_t start = ~0;
    virtaddr_t end = 0;

    // Process headers
    for(int i = 0; i < header.phdr_entry_count; ++i) {
        auto& header = programHeaders[i];
        switch(header.type) {
            case PT_LOAD: {
                if((header.alignment & 0xFFF) != 0) {
                    ASSERT_NOT_REACHED("Alignment <4096 is currently not supported");
                }

                size_t pageCount = (header.mem_size >> 12) + ((header.mem_size & 0xFFF) == 0 ? 0 : 1);
                size_t filePages = (header.file_size >> 12) + ((header.file_size & 0xFFF) == 0 ? 0 : 1);

                if(filePages > 0) {
                    // Map a file segment here
                    FilePage* page = new FilePage(file, header.vaddr, header.offset, !(header.flags & 2), header.flags & 2, header.flags & 1);

                    file_pages(header.vaddr, filePages, page);
                }

                size_t pagesLeft = pageCount - filePages;
                if(pagesLeft > 0) {
                    // Map anonymous pages here
                    int prot = 1;
                    prot |= (header.flags & 2) ? MMAP_PROT_WRITE : 0;
                    prot |= (header.flags & 1) ? MMAP_PROT_EXEC : 0;
                    alloc_pages(header.vaddr + (filePages << 12), pagesLeft, 0, prot);
                }

                if(start > header.vaddr) start = header.vaddr;
                if(end < header.vaddr + (pageCount << 12)) end = header.vaddr + (pageCount << 12);

                break;
            }
            default: break;
        }
    }

    // Map argv and envp and offset the addresses in them
    virtaddr_t argumentStart = f_pager->getFreeRange(end, argumentPages.size());

    {
        virtaddr_t address = argumentStart;
        for(auto& page : argumentPages) {
            map_page(address, page, false);
            address += (1 << 12);
        }

        auto offsetAddress = [](char** data, virtaddr_t offset) {
            for(char** ptr = data; *ptr != 0; ++ptr) {
                *((uintptr_t*)ptr) += offset;
            }
        };

        offsetAddress((char**)argumentStart, argumentStart);
        offsetAddress((char**)(argumentStart + envpOffset), argumentStart);
    }

    f_pager->unlock();

    // Make a new thread kernel stack
    thisThread->make_ks(header.entry_point);
    thisThread->f_ksp->rsi = argumentStart;
    thisThread->f_ksp->rdi = argumentStart + envpOffset;

    // We will now return into our new state
    return 0;
}
