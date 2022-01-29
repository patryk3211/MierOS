#include <dmesg.h>
#include <vector.hpp>

std::Vector<int> vec = std::Vector<int>(10, 1);

extern "C" int init() {
    dmesg("[Test Mod] Testing\n");
    vec.push_back(105);
    return vec[0] + vec[10];
}

extern "C" int destroy() {
    return 0;
}
