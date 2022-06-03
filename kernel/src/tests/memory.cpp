#include <assert.h>
#include <memory/physical.h>
#include <tests/memory.hpp>
#include <types.h>

bool kernel::tests::pmm_test() {
    { // Test 1, check if freed memory is reused
        physaddr_t address = palloc(256);
        for(int i = 0; i < 256; i += 2) pfree(address + (i << 12), 1);
        for(int i = 1; i < 256; i += 2) pfree(address + (i << 12), 1);
        physaddr_t address2 = palloc(256);
        ASSERT_F(address == address2, "(PMM1) \033[1;37maddress2\033[0m should be the same as \033[1;37maddress\033[0m because it has been freed");
        if(address != address2) return false;
        pfree(address2, 256);
    }
    return true;
}
