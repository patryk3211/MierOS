#include <tasking/syscalls/syscall.hpp>

#include <event/event_manager.hpp>
#include <event/kernel_events.hpp>
#include <tasking/syscalls/kernel.hpp>
#include <memory/virtual.hpp>

using namespace kernel;

DEF_SYSCALL(init_module_name, modName, argv) {
    UNUSED(proc);

    auto* event = new Event(EVENT_LOAD_MODULE, (char*)modName, nullptr, nullptr, (char**)argv);
    event->keep();

    EventManager::get().raise_wait(event);

    auto status = event->status();
    delete event;

    return status;
}

/// TODO: [30.01.2023] Implement a 'remove_module' syscall

DEF_SYSCALL(uevent_poll, eventPtr, flags) {
    UNUSED(proc);

    bool block = flags & UEVENT_POLL_FLAG_NO_BLOCK;

    if(eventPtr >= KERNEL_START)
        return -ERR_INVALID;

    return EventManager::get().uevent_poll((UEvent*)eventPtr, block);
}

DEF_SYSCALL(uevent_complete, eventPtr, status) {
    UNUSED(proc);

    // Validate the pointer
    if(!eventPtr || eventPtr >= KERNEL_START)
        return -ERR_INVALID;

    EventManager::get().uevent_signal_complete((UEvent*)eventPtr, status);
    return 0;
}

