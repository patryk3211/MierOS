#pragma once

#include <assert.h>
#define TEST(expr, msg) { bool temp = (expr); ASSERT_F(temp, msg); if(!temp) return false; }

namespace kernel::tests {
    void do_tests();
}
