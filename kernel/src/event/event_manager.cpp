#include <event/event_manager.hpp>
#include <assert.h>
#include <locking/locker.hpp>
#include <tasking/scheduler.hpp>

using namespace kernel;

EventManager* EventManager::s_instance = 0;

EventManager::EventManager() {
    f_event_loop_proc = Process::construct_kernel_process((virtaddr_t)&event_loop);
    Scheduler::schedule_process(*f_event_loop_proc);

    s_instance = this;
}

EventManager::~EventManager() { }

void EventManager::raise(Event* event) {
    Locker locker(f_lock);

    f_event_queue.push_back(event);
}

void EventManager::register_handler(u64_t identifier, event_handler_t* handler) {
    Locker locker(f_lock);

    f_handlers[identifier].push_back(handler);
}

EventManager& EventManager::get() {
    ASSERT_F(s_instance != 0, "Trying to use the EventManager before it was initialized");
    return *s_instance;
}

void EventManager::event_loop() {
    // We will have to make this process wake up only when there are things to process on the event queue
    while(true) {
        while(!s_instance->f_event_queue.size())
            asm volatile("hlt"); // For now we halt if there are no events to process

        auto* event = s_instance->f_event_queue.pop_front();
        auto handlersRef = s_instance->f_handlers.at(event->identifier());

        if(handlersRef) {
            auto& handlers = *handlersRef;
            for(auto& handler : handlers) {
                handler(*event);

                if(event->is_consumed())
                    break;
            }
        }

        delete event;
    }
}
