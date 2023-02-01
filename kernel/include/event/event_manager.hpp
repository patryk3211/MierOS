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
         * @brief Raise a userspace event
         *
         * This method raises a userspace event which can be polled by a
         * userspace task to get processed.
         *
         * @param uevent The event to be submitted
         */
        void raise_uevent(UEvent* uevent);

        /**
         * @brief Raise and wait for a userspace event
         *
         * This method raises a userspace event and waits for it
         * to be processed. If there are no userspace event handlers this
         * method will block until one start to process the events.
         *
         * @param uevent The event to be submitted
         * @return Value passed to the uevent_signal_complete method
         */
        u64_t raise_wait_uevent(UEvent* uevent);

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

        /**
         * @brief Poll for a userspace event
         *
         * This method will block (if block is true, otherwise it returns 0 when
         * no event was received) until it receives a userspace event. If the
         * event_out argument is null it will only return the size required to store
         * the received event but not take it off the queue, otherwise the uevent is
         * taken off the queue and copied into space pointed by event_out. Additionally
         * the method will only block when event_out is null, otherwise it behaves as if
         * block is false.
         *
         * @param event_out Pointer to store the uevent
         * @param block If true the method will block, otherwise returns 0 when event was not received
         * 
         * @return Size of the received event
         */
        size_t uevent_poll(UEvent* event_out = nullptr, bool block = true);

        /**
         * @brief Signal userspace event complete
         *
         * This method will signal the event manager that a userspace event has
         * been processed. It should be called with the event received from uevent_poll.
         * An additional status variable can be passed to all threads waiting for the
         * event to complete.
         * 
         * @param uevent Event that was processed
         * @param status Value to set processed_event->f_status to
         */
        void uevent_signal_complete(UEvent* uevent, u64_t status = 0);

        static EventManager& get();

    private:
        static void event_loop();

        static void sync_event_handler(Event& event);
        static void event_processed_handler(Event& event);
    };
}
