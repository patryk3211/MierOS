#include <tests/test.hpp>
#include <tests/memory.hpp>
#include <tests/stdlib.hpp>

#include <dmesg.h>

using namespace kernel::tests;

void kernel::tests::do_tests() {
    dmesg("[Kernel] Performing kernel tests...\n");

    if(!pmm_test()) {
        dmesg("\033[1;31mTEST FAILED!\033[0m (PMM) Physical Memory Manager test has failed!\n");
        while(true);
    } else {
        dmesg("\033[1;32mTEST SUCCESSFUL!\033[0m (PMM) Physical Memory Manager test successful!\n");
    }

    if(!stdlib_test()) {
        dmesg("\033[1;31mTEST FAILED!\033[0m (SL) Standard Library test has failed!\n");
        while(true);
    } else {
        dmesg("\033[1;32mTEST SUCCESFUL!\033[0m (SL) Standard Library test succesful!\n");
    }
}
