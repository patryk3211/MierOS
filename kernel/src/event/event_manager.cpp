#include <event/event_manager.hpp>
#include <assert.h>
#include <locking/locker.hpp>
#include <tasking/scheduler.hpp>
#include <event/kernel_events.hpp>

using namespace kernel;

EventManager* EventManager::s_instance = 0;

EventManager::EventManager() {
    f_event_loop_proc = Process::construct_kernel_process((virtaddr_t)&event_loop);
    s_instance = this;

    register_handler(EVENT_SYNC, &sync_event_handler);

    f_processing_event = false;
    Scheduler::schedule_process(*f_event_loop_proc);
}

EventManager::~EventManager() { }

void EventManager::raise(Event* event) {
    Locker locker(f_lock);

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
                s_instance->raise(new Event(EVENT_QUEUE_EMPTY));
            } else {
                asm volatile("hlt"); // For now we halt if there are no events to process
            }
        }

        s_instance->f_lock.lock();
        auto* event = s_instance->f_event_queue.pop_front();
        s_instance->f_lock.unlock();

        // We need this to prevent queue empty event from triggering itself
        if(event->identifier() != EVENT_QUEUE_EMPTY) {
            s_instance->f_processing_event = true;
        }

        // Signal all waits for the event id
        for(auto& wait : s_instance->f_event_waits) {
            if(wait->f_event_id == event->identifier()) {
                wait->f_semaphore.release();
            }
        }

        // Fire all event handlers for the event id
        auto handlersRef = s_instance->f_handlers.at(event->identifier());
        if(handlersRef) {
            auto& handlers = *handlersRef;
            for(auto& handler : handlers) {
                event->reset_arg_ptr();
                handler(*event);

                if(event->is_consumed())
                    break;
            }
        }

        delete event;
    }
}

void EventManager::sync_event_handler(Event& event) {
    auto* cb = event.get_arg<void (*)(void*)>();
    auto* cbArg = event.get_arg<void*>();

    if(cb != 0)
        cb(cbArg);
}

void EventManager::wait(u64_t identifier) {
    Wait wait(identifier);

    f_lock.lock();
    f_event_waits.push_back(&wait);
    f_lock.unlock();

    wait.f_semaphore.acquire();
}