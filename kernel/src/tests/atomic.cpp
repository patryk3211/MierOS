#include <atomic.hpp>
#include <tests/atomic.hpp>
#include <tests/test.hpp>
#include <types.h>

bool kernel::tests::atomic_test() {
    {
        std::Atomic<u32_t> at_val;
        u32_t v1 = at_val.fetch_add(1);
        u32_t v2 = at_val.load();
        at_val.store(5);
        u32_t v3 = at_val.fetch_sub(2);
        u32_t v4 = at_val.load();

        TEST(v1 == 0, "(AT1.1) General atomic operations test failed!");
        TEST(v2 == 1, "(AT1.2) General atomic operations test failed!");
        TEST(v3 == 5, "(AT1.3) General atomic operations test failed!");
        TEST(v4 == 3, "(AT1.4) General atomic operations test failed!");
    }

    return true;
}