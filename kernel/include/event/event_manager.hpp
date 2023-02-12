#pragma once

#include "tasking/thread.hpp"
#include <event/event.hpp>
#include <event/uevent.hpp>
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

        u64_t f_next_eid;
        std::Queue<Event*> f_event_queue;
        std::Queue<UEvent*> f_uevent_queue;

        bool f_processing_event;
        Process* f_event_loop_proc;

        struct Wait {
            u64_t f_identifier;
            u64_t f_eid;
            u64_t f_processed_status;
            Thread* f_thread;

            Wait(u64_t ident, u64_t eid, Thread* thread)
                : f_identifier(ident), f_eid(eid), f_thread(thread) { }
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
         * @param identifier Event identifier (0 means any event type)
         * @param eid Local event id (0 means any event of type)
         */
        void wait(u64_t identifier, u64_t eid);

        /**
         * @brief Raise and wait for an event
         *
         * As the name suggests, this method raises and event and immidiently
         * submits a wait for it so that it is not possible for the wait to get
         * lost (In case the event gets processed really quickly and the submit
         * is in the middle of a task switch for example). To achieve this, the
         * method simply maintains the lock for both of the operations.
         *
         * @param event The event to be submitted
         */
        void raise_wait(Event* event);

        /**
         * @brief Register a kernel event handler
         *
         * This method will registers an event handler for the given event
         * identifier.
         *
         * @param identifier Event identifier
         * @param handler Event handler
         */
        void register_handler(u64_t identifier, event_handler_t* handler);

        static EventManager& get();

    private:
        static void event_loop();

        static void sync_event_handler(Event& event);
        static void event_processed_handler(Event& event);
    };
}
