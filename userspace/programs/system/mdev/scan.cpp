#include "scan.hpp"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>

std::list<std::string> udev::find_busses() {
    std::list<std::string> busList;

    auto busIter = std::filesystem::directory_iterator("/sys/bus");
    for(auto& entry : busIter) {
        if(entry.is_directory()) {
            busList.push_back(entry.path().filename());
        }
    }

    return busList;
}

void udev::probe_bus(std::string bus) {
    auto busIter = std::filesystem::directory_iterator("/sys/bus/" + bus + "/devices");

    for(auto& device : busIter) {
        std::cout << device.path().filename() << std::endl;
        std::ifstream ueventStream(device.path() / "uevent");

        if(ueventStream.is_open()) {
            std::string line;
            while(std::getline(ueventStream, line)) {
                std::cout << line << std::endl;
            }
        }
    }
}

