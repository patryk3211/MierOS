#pragma once

#include <list>
#include <string>

namespace udev {
    std::list<std::string> find_busses();
    void probe_bus(std::string bus);
}

