#include <event/uevent.hpp>

#include <list.hpp>
#include <pair.hpp>

using namespace kernel;

UEvent::~UEvent() {
    delete buffer;
}

const char* UEvent::get_arg(const char* name) {
    for(size_t i = 0; i < argc; ++i) {
        if(!strcmp(name, argv[i].name))
            return argv[i].value;
    }
    return 0;
}

UEvent* kernel::parse_uevent(const void* data, size_t length) {
    char* dataCopy = new char[length + 1];
    memcpy(dataCopy, data, length);
    dataCopy[length] = 0;

    const char* eventName = 0;
    std::List<std::Pair<const char*, const char*>> eventArgs;

    size_t offset = 0;
    while(offset < length) {
        char* nextSeparator = strchr(dataCopy + offset, '\n');
        if(nextSeparator == 0)
            nextSeparator = dataCopy + length;
        *nextSeparator = 0;

        char* valueSeparator = strchr(dataCopy + offset, '=');
        if(!offset && !valueSeparator) {
            // Only the first line can be an event name
            eventName = dataCopy;
        } else {
            if(dataCopy[offset] != 0) {
                // This is not an empty line.
                if(valueSeparator != 0) {
                    // This line has a valid key=value pair
                    *valueSeparator = 0;
                    eventArgs.push_back({ dataCopy + offset, valueSeparator + 1 });
                }
            }
        }

        offset = (nextSeparator - dataCopy) + 1;
    }

    UEvent* uevent = (UEvent*)new u8_t[sizeof(UEvent) + sizeof(UEventArg) * eventArgs.size()];
    uevent->argc = eventArgs.size();
    uevent->buffer = dataCopy;
    uevent->event_name = eventName;

    size_t i = 0;
    for(auto& entry : eventArgs) {
        uevent->argv[i].name = entry.key;
        uevent->argv[i].value = entry.value;
        ++i;
    }

    return uevent;
}

