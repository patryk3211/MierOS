#pragma once

#include <event/event.hpp>

namespace kernel {
    typedef bool event_handler_t(Event& event);

    class EventManager {
        static EventManager* s_instance;

    public:
        EventManager();
        ~EventManager();

        void raise(Event& event);
        void register_handler(u64_t identifier, event_handler_t* handler);

        static EventManager& get();
    };
}
