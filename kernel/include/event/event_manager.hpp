#pragma once

#include <event/event.hpp>
#include <unordered_map.hpp>
#include <list.hpp>
#include <queue.hpp>
#include <locking/spinlock.hpp>
#include <tasking/process.hpp>

namespace kernel {
    typedef void event_handler_t(Event& event);

    class EventManager {
        static EventManager* s_instance;

        std::UnorderedMap<u64_t, std::List<event_handler_t*>> f_handlers;
        std::Queue<Event*> f_event_queue;

        bool f_processing_event;
        Process* f_event_loop_proc;

        struct Wait {
            u64_t f_event_id;
            Semaphore f_semaphore;

            Wait(u64_t id)
                : f_event_id(id), f_semaphore(1, 0) { }
        };
        std::List<Wait*> f_event_waits;

        // This lock secures the event queue and wait list from concurrent modification
        SpinLock f_lock;

    public:
        EventManager();
        ~EventManager();

        // This function consumes the pointer and will take care
        // of it's destruction but it will not take care of the
        // cleanup of pointers passed on the event object
        void raise(Event* event);

        /**
         * @brief Wait for event
         *
         * This function waits until the specified event occurs
         *
         * @param identifier Event id
         */
        void wait(u64_t identifier);

        void register_handler(u64_t identifier, event_handler_t* handler);

        static EventManager& get();

    private:
        static void event_loop();

        static void sync_event_handler(Event& event);
    };
}
