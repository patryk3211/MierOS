#include <tests/atomic.hpp>
#include <tests/memory.hpp>
#include <tests/stdlib.hpp>
#include <tests/test.hpp>

#include <dmesg.h>

using namespace kernel::tests;

void kernel::tests::do_tests() {
    dmesg("(Kernel) Performing kernel tests...");

    if(!pmm_test()) {
        dmesg("\033[1;31mTEST FAILED!\033[0m (PMM) Physical Memory Manager test has failed!");
        while(true)
            ;
    } else {
        dmesg("\033[1;32mTEST SUCCESSFUL!\033[0m (PMM) Physical Memory Manager test successful!");
    }

    if(!stdlib_test()) {
        dmesg("\033[1;31mTEST FAILED!\033[0m (SL) Standard Library test has failed!");
        while(true)
            ;
    } else {
        dmesg("\033[1;32mTEST SUCCESSFUL!\033[0m (SL) Standard Library test succesful!");
    }

    if(!atomic_test()) {
        dmesg("\033[1;31mTEST FAILED!\033[0m (AT) Atomic test has failed!");
        while(true)
            ;
    } else {
        dmesg("\033[1;32mTEST SUCCESSFUL!\033[0m (AT) Atomic test succesful!");
    }
}
