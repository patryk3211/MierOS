#pragma once

#include <event/event.hpp>
#include <unordered_map.hpp>
#include <list.hpp>
#include <queue.hpp>
#include <locking/spinlock.hpp>
#include <tasking/process.hpp>

namespace kernel {
    typedef bool event_handler_t(Event& event);

    class EventManager {
        static EventManager* s_instance;

        std::UnorderedMap<u64_t, std::List<event_handler_t*>> f_handlers;
        std::Queue<Event*> f_event_queue;

        Process* f_event_loop_proc;

        SpinLock f_lock;

    public:
        EventManager();
        ~EventManager();

        void raise(Event* event);
        void register_handler(u64_t identifier, event_handler_t* handler);

        static EventManager& get();

    private:
        static void event_loop();
    };
}
