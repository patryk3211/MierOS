#include <event/event_manager.hpp>
#include <assert.h>
#include <locking/locker.hpp>
#include <tasking/scheduler.hpp>
#include <event/kernel_events.hpp>
#include <arch/interrupts.h>

using namespace kernel;

EventManager* EventManager::s_instance = 0;

EventManager::EventManager() {
    f_event_loop_proc = Process::construct_kernel_process((virtaddr_t)&event_loop);
    s_instance = this;
    f_next_eid = 1;

    register_handler(EVENT_SYNC, &sync_event_handler);

    f_processing_event = false;
    Scheduler::schedule_process(*f_event_loop_proc);
}

EventManager::~EventManager() { }

void EventManager::raise(Event* event) {
    Locker locker(f_lock);

    event->f_local_id = f_next_eid++;
    f_event_queue.push_back(event);
}

void EventManager::register_handler(u64_t identifier, event_handler_t* handler) {
    f_handlers[identifier].push_back(handler);
}

EventManager& EventManager::get() {
    ASSERT_F(s_instance != 0, "Trying to use the EventManager before it was initialized");
    return *s_instance;
}

/// TODO: [23.01.2023] Add the ability to process multiple events concurrently
void EventManager::event_loop() {
    // We will have to make this process wake up only when there are things to process on the event queue
    while(true) {
        while(!s_instance->f_event_queue.size()) {
            if(s_instance->f_processing_event) {
                s_instance->f_processing_event = false;
                auto* event = new Event(EVENT_QUEUE_EMPTY);
                event->set_flags(EVENT_FLAG_META_EVENT);
                s_instance->raise(event);
            } else {
                asm volatile("hlt"); // For now we halt if there are no events to process
            }
        }

        s_instance->f_lock.lock();
        auto* event = s_instance->f_event_queue.pop_front();
        s_instance->f_lock.unlock();

        // Meta-event don't trigger event manager events (So that we don't create an infinite event loop)
        if(!(event->get_flags() & EVENT_FLAG_META_EVENT)) {
            s_instance->f_processing_event = true;
        }

        // Fire all event handlers for the event id
        auto handlersRef = s_instance->f_handlers.at(event->f_identifier);
        if(handlersRef) {
            auto& handlers = *handlersRef;
            for(auto& handler : handlers) {
                event->reset_arg_ptr();
                handler(*event);

                if(event->is_consumed())
                    break;
            }
        }

        event->set_flags(EVENT_FLAG_PROCESSED);

        if(!(event->get_flags() & EVENT_FLAG_META_EVENT)) {
            auto* meta = new Event(EVENT_PROCESSED_EVENT, event->f_identifier, event->f_local_id);
            meta->set_flags(EVENT_FLAG_META_EVENT);
            s_instance->raise(meta);
        }

        if(event->should_clear())
            delete event;
    }
}

void EventManager::sync_event_handler(Event& event) {
    auto* cb = event.get_arg<void (*)(void*)>();
    auto* cbArg = event.get_arg<void*>();

    if(cb != 0)
        cb(cbArg);
}

void EventManager::wait(u64_t identifier, u64_t eid) {
    Wait wait(identifier, eid, Thread::current());
    f_lock.lock();

    // We need to make sure that the wait is on wait queue and
    // that the thread is transitioned into the wait state BEFORE
    // we do any task switches or else we could lose the thread
    // or possibly end up in a deadlock.
    f_event_waits.push_back(&wait);

    enter_critical();
    Thread::current()->sleep(false);
    f_lock.unlock();
    leave_critical();

    // We can now do a task switch, if the event was already processed we will
    // just switch a task and not accidentally lose the thread or anything else.
    force_task_switch();
}

void EventManager::raise_wait(Event* event) {
    f_lock.lock();

    // Assign an eid
    event->f_local_id = f_next_eid++;

    // Submit the wait before we submit the event
    Wait wait(event->f_identifier, event->f_local_id, Thread::current());
    f_event_waits.push_back(&wait);

    // Submit the event
    f_event_queue.push_back(event);

    // Enter the sleep state
    enter_critical();
    Thread::current()->sleep(false);
    f_lock.unlock();
    leave_critical();

    force_task_switch();
}

void EventManager::event_processed_handler(Event& event) {
    Locker lock(s_instance->f_lock);

    auto iter = s_instance->f_event_waits.begin();
    while(iter != s_instance->f_event_waits.end()) {
        auto* wait = *iter;

        if((!wait->f_identifier || wait->f_identifier == event.f_identifier) &&
           (!wait->f_eid || wait->f_eid == event.f_local_id)) {
            wait->f_thread->wakeup();
            iter = s_instance->f_event_waits.erase(iter);
        } else {
            ++iter;
        }
    }
}

void EventManager::raise_uevent(UEvent *uevent) {
    Locker lock(f_lock);

    UEvent* event_copy = reinterpret_cast<UEvent*>(new u8_t[uevent->size]);
    memcpy(event_copy, uevent, uevent->size);
    event_copy->id = f_next_eid++;

    f_uevent_queue.push_back(event_copy);
}

u64_t EventManager::raise_wait_uevent(UEvent *uevent) {
    f_lock.lock();

    UEvent* event_copy = reinterpret_cast<UEvent*>(new u8_t[uevent->size]);
    memcpy(event_copy, uevent, uevent->size);
    event_copy->id = f_next_eid++;

    Wait wait(EVENT_NULL, event_copy->id, Thread::current());
    f_event_waits.push_back(&wait);

    f_uevent_queue.push_back(event_copy);

    enter_critical();
    Thread::current()->sleep(false);
    f_lock.unlock();
    leave_critical();

    force_task_switch();
    return wait.f_processed_status;
}

size_t EventManager::uevent_poll(UEvent* event_out, bool block) {
    if(block && !event_out) {
        /// TODO: [01.02.2023] Improve the blocking by putting the thread to sleep
        while(!f_uevent_queue.size());
    } else {
        if(!f_uevent_queue.size())
            return 0;
    }

    Locker lock(f_lock);
    UEvent* event = f_uevent_queue.peek();

    if(event_out) {
        memcpy(event_out, event, event->size);
        f_uevent_queue.pop_front();
        delete event;
    }

    return event->size;
}

void EventManager::uevent_signal_complete(UEvent* uevent, u64_t status) {
    auto* meta = new Event(EVENT_PROCESSED_EVENT, (u64_t)EVENT_USERSPACE_EVENT, uevent->id);
    meta->set_flags(EVENT_FLAG_META_EVENT);
    meta->set_status(status);
    raise(meta);
}

