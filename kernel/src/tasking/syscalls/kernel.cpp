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

DEF_SYSCALL(uevent_poll, eventPtr, flags) {
    UNUSED(proc);
    VALIDATE_PTR(eventPtr);

    bool block = flags & UEVENT_POLL_FLAG_NO_BLOCK;

    return EventManager::get().uevent_poll((UEvent*)eventPtr, block);
}

DEF_SYSCALL(uevent_complete, eventPtr, status) {
    UNUSED(proc);
    VALIDATE_PTR(eventPtr);

    EventManager::get().uevent_signal_complete((UEvent*)eventPtr, status);
    return 0;
}

