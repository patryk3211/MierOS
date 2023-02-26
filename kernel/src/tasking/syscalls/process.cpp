#include <asm/fcntl.h>
#include <tasking/syscalls/syscall.hpp>
#include <tasking/scheduler.hpp>
#include <arch/interrupts.h>
#include <tasking/syscalls/map.hpp>
#include <fs/vfs.hpp>
#include <vector.hpp>
#include <elf.h>
#include <asm/procid.h>

#include <util/profile.hpp>

using namespace kernel;

DEF_SYSCALL(exit, exitCode) {
    TRACE("(syscall) Process (pid = %d) exited with code %d", proc.main_thread()->pid(), exitCode);
    proc.die(exitCode);
    return 0;
}

DEF_SYSCALL(fork) {
    Process* child = proc.fork();

    Scheduler::schedule_process(*child);
    TRACE("(syscall) New process forked from pid %d (child pid = %d)", proc.main_thread()->pid(), child->main_thread()->pid());

    return child->pid();
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

    TRACE("(syscall) Process (pid = %d) trying to execute file '%s'", proc.main_thread()->pid(), path);
    return proc.execve(file, (char**)argv, (char**)envp);
}

DEF_SYSCALL(getid, id) {
    switch(id) {
        case PROCID_PID:
            return proc.pid();
        case PROCID_TID:
            return Thread::current()->pid();
        case PROCID_UID:
            return proc.uid();
        case PROCID_EUID:
            return proc.euid();
        case PROCID_GID:
            return proc.gid();
        case PROCID_EGID:
            return proc.egid();
        case PROCID_PARENT:
            return proc.ppid();
        default:
            return -EINVAL;
    }
}

#define WNOHANG 1
#define WUNTRACED 2
#define WSTOPPED 2
#define WEXITED 4
#define WCONTINUED 8

DEF_SYSCALL(waitpid, pid, status, options) {
    // TODO: [14.02.2023] We need to make sure that the process at pid is
    // a child of the calling process.
    auto* thread = Thread::get(pid);
    if(!thread)
        return ECHILD;

    if(!thread->is_main())
        return EINVAL;

    if(status)
        VALIDATE_PTR(status);

    int statusValue = 0;

    // TODO: [15.02.2023] We need to allow for groups of children to be handled.
    TRACE("(syscall) Wait pid called for to get state of pid = %d, caller pid = %d", pid, proc.pid());
    auto state = thread->get_state(!(options & WNOHANG));
    if(state == DEAD) {
        // Encode status data
        auto& proc = thread->parent();
        statusValue |= proc.exit_status() << 8;
    }

    if(status)
        *((int*)status) = statusValue;

    // TODO: [15.02.2023] Delete the thread if it died.

    return thread->pid();
}

DEF_SYSCALL(getcwd, ptr, maxLength) {
    VALIDATE_PTR(ptr);
    auto& cwd = proc.cwd();

    if(cwd.length() + 1 > maxLength)
        return -ERANGE;

    memcpy(reinterpret_cast<void*>(ptr), cwd.c_str(), cwd.length() + 1);
    return cwd.length();
}

Process* Process::fork() {
    f_lock.lock();
    Thread* caller = Thread::current();

    // Create a child process and clone the CPU state
    Process* child = new Process(CPUSTATE_IP(caller->f_syscall_state));
    memcpy(child->main_thread()->f_ksp, caller->f_syscall_state, sizeof(CPUState));
    CPUSTATE_RET(child->main_thread()->f_ksp) = 0;
    child->main_thread()->f_ksp->cr3 = child->f_pager->cr3();

    child->f_uid = f_uid;
    child->f_gid = f_gid;
    child->f_euid = f_euid;
    child->f_egid = f_egid;

    child->f_parent = pid();

    // Copy the memory space
    f_pager->lock();
    // Change private writable pages into copy-on-write and
    // add a reference to all physical pages
    for(auto [addr, entry] : f_resolved_memory) {
        if(entry.f_page_flags.writable && !entry.f_shared) {
            entry.f_page_flags.writable = false;
            entry.f_copy_on_write = true;
            f_pager->flags(addr, 1, entry.f_page_flags);
        }
        entry.f_page.ref();
        entry.f_page.flags() = entry.f_page_flags;
    }
    f_pager->unlock();

    // Copy the memory maps
    child->f_resolved_memory = f_resolved_memory;
    child->f_resolvable_memory = f_resolvable_memory;
    
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
    if(*stream.read(&header, sizeof(header)) != sizeof(header)) {
        // Unexpected EOF
        return ENOEXEC;
    }

    // Validate the header
    if(!memcmp(validElf, &header, 8)) {
        // Invalid header magic
        return ENOEXEC;
    }

    if(header.type != ET_EXEC && header.type != ET_DYN)
        return ENOEXEC;

    Elf64_Phdr programHeaders[header.phdr_entry_count];
    stream.seek(header.phdr_offset, SEEK_MODE_BEG);
    size_t readCount = header.phdr_entry_count * sizeof(Elf64_Phdr);
    if(*stream.read(programHeaders, readCount) != readCount) {
        // Unexpected EOF
        return ENOEXEC;
    }

    // No return part starts here
    f_lock.lock();
    auto* thisThread = Thread::current();
    pid_t mainPid = main_thread()->f_pid;

    ASSERT_F(f_threads.contains(thisThread), "Invalid current thread is trying to exec on a non parent process");

    // Stop all other threads
    for(auto* thread : f_threads) {
        if(thread == thisThread) continue;

        if(thread->f_state == RUNNING) {
            thread->f_state = DYING;
            send_task_switch_irq(thread->f_preferred_core);
        }

        Scheduler::remove_thread(thread);
        thread->change_state(DEAD);
        // TODO: [15.02.2023] Actually I think we should also delete the threads here
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
    for(auto [addr, entry] : f_resolved_memory) {
        if(entry.f_file && entry.f_shared) {
            // File backed memory
            auto flags = f_pager->getFlags(addr);
            if(flags.writable && flags.dirty) {
                // Memory is dirty and shared, write back to file
                entry.f_file->filesystem()->sync_mapping(entry);
            }
        }

        f_pager->unmap(addr, 1);
        entry.f_page.unref();
    }
    
    f_resolved_memory.clear();
    f_resolvable_memory.clear();

    virtaddr_t base = 0;
    if(header.type == ET_DYN) {
        // Find a suitable base address for a PIE.
        // For now we just set it to a hardcoded value
        // but in the future we might randomize this address.
        base = PIE_START;
    }

    VNodePtr interpFile = nullptr;

    virtaddr_t endAddr = 0;

    std::Vector<auxv_t> auxVectors;

    // Process headers
    for(int i = 0; i < header.phdr_entry_count; ++i) {
        auto& header = programHeaders[i];
        switch(header.type) {
            case PT_PHDR: {
                auxv_t vec;
                vec.a_type = AT_PHDR;
                vec.a_un.a_ptr = reinterpret_cast<void*>(base + header.vaddr);
                auxVectors.push_back(vec);

                vec.a_type = AT_PHENT;
                vec.a_un.a_val = sizeof(Elf64_Phdr);
                auxVectors.push_back(vec);

                vec.a_type = AT_PHNUM;
                vec.a_un.a_val = header.mem_size / sizeof(Elf64_Phdr);
                auxVectors.push_back(vec);
                break;
            }
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
                    file_pages(base + (header.vaddr & ~0xFFF), filePages, file, header.offset & ~0xFFF,
                               !(header.flags & 2), header.flags & 2, header.flags & 1);

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

                if(endAddr < header.vaddr + (pageCount << 12)) endAddr = header.vaddr + (pageCount << 12);

                break;
            }
            case PT_INTERP: {
                size_t nameSize = header.file_size;
                char* name = new char[nameSize + 1];
                // Just in case the file doesn't provide a null terminated string
                name[nameSize] = 0;

                stream.seek(header.offset, SEEK_MODE_BEG);
                stream.read(name, nameSize);
                
                auto result = VFS::instance()->get_file(nullptr, name, { .resolve_link = true, .follow_links = true });
                if(!result) // TODO: This should not be a kernel panic
                    panic("Could not find executable interpreter");
                interpFile = *result;
                delete[] name;
                break;
            }
            default: break;
        }
    }

    virtaddr_t entry = base + header.entry_point;
    
    if(interpFile) {
        // Load the file just like we loaded the executable
        FileStream interpStream(interpFile);
        if(!interpStream.open(FILE_OPEN_MODE_READ))
            panic("Failed to open the exectuable interpreter");

        interpStream.read(&header, sizeof(header));
        if(!memcmp(validElf, &header, 8)) {
            // Invalid header magic
            panic("Invalid header magic");
        }

        // Put the interpreter on the next page after the end of executable.
        virtaddr_t interpreterBase = ((base + endAddr) | 0xFFF) + 1;
        Elf64_Phdr interHeaders[header.phdr_entry_count];

        interpStream.seek(header.phdr_offset, SEEK_MODE_BEG);
        interpStream.read(interHeaders, header.phdr_entry_count * sizeof(Elf64_Phdr));

        for(size_t i = 0; i < header.phdr_entry_count; ++i) {
            Elf64_Phdr& header = interHeaders[i];
            if(header.type == PT_LOAD) {
                // Load header
                if((header.alignment & 0xFFF) != 0) {
                    ASSERT_NOT_REACHED("Alignment <4096 is currently not supported");
                }

                size_t alignedMemSize = header.mem_size + (header.vaddr & 0xFFF);
                size_t alignedFileSize = header.file_size + (header.offset & 0xFFF);

                size_t pageCount = (alignedMemSize >> 12) + ((alignedMemSize & 0xFFF) == 0 ? 0 : 1);
                size_t filePages = (alignedFileSize >> 12) + ((alignedFileSize & 0xFFF) == 0 ? 0 : 1);

                if(filePages > 0) {
                    // Map a file segment here
                    file_pages(interpreterBase + (header.vaddr & ~0xFFF), filePages, interpFile, header.offset & ~0xFFF,
                               !(header.flags & 2), header.flags & 2, header.flags & 1);

                    // We need this since the .bss section will likely intersect
                    // with some data and we need to guarantee that it is cleared
                    if(alignedMemSize > alignedFileSize && ((header.vaddr + header.file_size) & 0xFFF)) {
                        // We need to zero out some memory
                        memset((void*)(interpreterBase + header.vaddr + header.file_size), 0, 4096 - ((header.vaddr + header.file_size) & 0xFFF));
                    }
                }

                size_t pagesLeft = pageCount - filePages;
                if(pagesLeft > 0) {
                    // Map anonymous pages here
                    int prot = 1;
                    prot |= (header.flags & 2) ? MMAP_PROT_WRITE : 0;
                    prot |= (header.flags & 1) ? MMAP_PROT_EXEC : 0;
                    alloc_pages(interpreterBase + ((header.vaddr + (filePages << 12)) & ~0xFFF), pagesLeft, MMAP_FLAG_ZERO, prot);
                }
            }
        }

        auxv_t vec;
        vec.a_type = AT_BASE;
        vec.a_un.a_ptr = reinterpret_cast<void*>(interpreterBase);
        auxVectors.push_back(vec);

        vec.a_type = AT_ENTRY;
        vec.a_un.a_fnc = reinterpret_cast<void (*)()>(entry);
        auxVectors.push_back(vec);

        entry = interpreterBase + header.entry_point;
    }

    virtaddr_t stackStart = f_pager->getFreeRange(0x7FFF00000000 - PROCESS_STACK_SIZE, PROCESS_STACK_SIZE >> 12);
    alloc_pages(stackStart, PROCESS_STACK_SIZE >> 12, MMAP_FLAG_PRIVATE, MMAP_PROT_READ | MMAP_PROT_WRITE);
    virtaddr_t stackEnd = stackStart + PROCESS_STACK_SIZE;

    // Exec stack setup
    auxVectors.push_back({ 0, {0} }); // < Null vector

    size_t execStackSize = (3 + argStore.size() + envStore.size()) * sizeof(u64_t) + auxVectors.size() * sizeof(auxv_t);

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

    virtaddr_t infoBlock = (virtaddr_t)execStack + (3 + argStore.size() + envStore.size()) * sizeof(u64_t) + auxVectors.size() * sizeof(auxv_t);

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

    // Encode aux vectors
    memcpy(execStack + stackOffset, auxVectors.data(), auxVectors.size() * sizeof(auxv_t));

    thisThread->make_ks(entry, (virtaddr_t)execStack);
    f_first_free = MMAP_MIN_ADDR;

    f_pager->unlock();
    f_lock.unlock();

    // Close streams which require it
    std::List<fd_t> toClose;
    auto iter = f_streams.begin();
    while(iter != f_streams.end()) {
        if((*iter).value.base().flags() & O_CLOEXEC) {
            toClose.push_back((*iter).key);
        }
        ++iter;
    }
    for(auto& fd : toClose)
        close_stream(fd);

    // We will now return into our new state
    return { };
}
