#include <assert.h>
#include <memory/physical.h>
#include <tests/memory.hpp>
#include <memory/ppage.hpp>
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
    { // Test 2, check if PhysicalPage works
        size_t start_pages = allocated_ppage_count();

        PhysicalPage freed1;
        PhysicalPage freed2;

        {
            PhysicalPage page_ref;
            page_ref.ref();

            PhysicalPage page_ref_ref = page_ref;
            {
                PhysicalPage page;
                page_ref = page;
                page.ref();
            }

            ASSERT_F(page_ref.addr() != 0 && page_ref_ref.addr() != 0, "(PMM2) Pages were freed even thought they should not have been");

            freed1 = page_ref;
            freed2 = page_ref_ref;

            page_ref.unref();
            page_ref_ref.unref();
        }

        ASSERT_F(freed1.addr() == 0 && freed2.addr() == 0, "(PMM3) Pages were not freed");

        size_t end_pages = allocated_ppage_count();

        ASSERT_F(start_pages == end_pages, "(PMM4) Pages were leaked");
    }
    return true;
}
