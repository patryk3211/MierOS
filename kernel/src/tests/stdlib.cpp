#include <tests/stdlib.hpp>
#include <pointer.hpp>
#include <function.hpp>
#include <assert.h>

bool kernel::tests::stdlib_test() {
    { // UniquePtr test
        UniquePtr<int> original = make_unique<int>(5271);
        UniquePtr<int> copy1 = original;
        *original = 10101;
        UniquePtr<int> copy2 = original;
        original.clear();

        bool expr = !original && *copy1 == 5271 && *copy2 == 10101;
        ASSERT_F(expr, "(SL1) UniquePtr test failed");
        if(!expr) return false;
    }

    { // Function test
        int i = 5;
        Function<int(void)> func1 = [=]() mutable { 
            return i++;
        };

        auto func2 = func1;
        func1();
        auto func3 = func1;

        int i2 = func2();
        int i3 = func3();

        bool expr = i2 == 5 && i3 == 6;
        ASSERT_F(expr, "(SL2) Function test failed");
        if(!expr) return false;
    }

    return true;
}
