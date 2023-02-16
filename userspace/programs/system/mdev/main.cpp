#include <iostream>

#include "scan.hpp"

int main(int argc, char* argv[]) {
    std::cout << "Starting mdev daemon" << std::endl;

    auto busses = udev::find_busses();
    for(auto& bus : busses) {
        udev::probe_bus(bus);
    }

    return 0;
}

