#include <event/event_manager.hpp>
#include <assert.h>

using namespace kernel;

EventManager* EventManager::s_instance = 0;

EventManager::EventManager() {

    s_instance = this;
}

EventManager::~EventManager() { }

EventManager& EventManager::get() {
    ASSERT_F(s_instance != 0, "Trying to use the EventManager before it was initialized");
    return *s_instance;
}
