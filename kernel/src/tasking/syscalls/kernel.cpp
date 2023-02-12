#include <tasking/syscalls/syscall.hpp>

#include <event/event_manager.hpp>
#include <event/kernel_events.hpp>
#include <modules/module_manager.hpp>
#include <tasking/syscalls/kernel.hpp>
#include <memory/virtual.hpp>

using namespace kernel;

DEF_SYSCALL(init_module, modPtr, argv) {
    UNUSED(proc);
    VALIDATE_PTR(modPtr);

    auto status = ModuleManager::get().load_module((void*)modPtr, (const char**)argv);
    return status ? *status : -status.errno();
}

/// TODO: [30.01.2023] Implement a 'remove_module' syscall

DEF_SYSCALL(mount, source, target, fsType, flags, data) {
    VALIDATE_PTR(fsType);
    VALIDATE_PTR(target);

    const char* sourcePath = (const char*)source;

    VNodePtr file = nullptr;
    if(sourcePath != 0) {
        VALIDATE_PTR(source);
        if(sourcePath[0] == '/') {
            auto ret = VFS::instance()->get_file(nullptr, sourcePath, {});
            if(!ret) return -ret.errno();
            file = *ret;
        } else {
            auto ret = VFS::instance()->get_file(nullptr, (proc.cwd() + sourcePath).c_str(), {});
            if(!ret) return -ret.errno();
            file = *ret;
        }
    }

    // TODO: [12.02.2023] Use the flags and data arguments
    auto result = VFS::instance()->mount(file, (char*)fsType, (char*)target);
    if(result)
        TRACE("(syscall) Process (pid = %d) mounted filesystem '%s', location '%s'", proc.main_thread()->pid(), fsType, target);
    return result ? 0 : -result.errno();
}

DEF_SYSCALL(umount, target, flags) {
    VALIDATE_PTR(target);

    auto result = VFS::instance()->umount((char*)target);
    if(result)
        TRACE("(syscall) Process (pid = %d) unmounted filesystem at '%s'", proc.main_thread()->pid(), target);
    return result ? 0 : -result.errno();
}

