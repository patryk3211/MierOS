#include <tests/stdlib.hpp>
#include <pointer.hpp>
#include <function.hpp>
#include <functional.hpp>
#include <assert.h>
#include <list.hpp>

using namespace std;

#define TEST(expr, msg) { bool temp = (expr); ASSERT_F(temp, msg); if(!temp) return false; }

bool kernel::tests::stdlib_test() {
    { // UniquePtr test
        UniquePtr<int> original = make_unique<int>(5271);
        UniquePtr<int> copy1 = original;
        *original = 10101;
        UniquePtr<int> copy2 = original;
        original.clear();

        TEST(!original && *copy1 == 5271 && *copy2 == 10101, "(SL1) UniquePtr test failed");
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

        TEST(i2 == 5 && i3 == 6, "(SL2) std::Function test failed");
    }

    { // Operator test
        TEST(less<int>{}(1, 2), "(SL3.1) std::less test failed");
        TEST(!less<int>{}(2, 1), "(SL3.2) std::less test failed");
        TEST(!less<int>{}(1, 1), "(SL3.3) std::less test failed");

        TEST(!greater<int>{}(1, 2), "(SL3.1) std::greater test failed");
        TEST(greater<int>{}(2, 1), "(SL3.2) std::greater test failed");
        TEST(!greater<int>{}(1, 1), "(SL3.3) std::greater test failed");
    }

    { // List test
        List<int> l = List<int>();
        l.push_back(5);
        l.push_back(2);
        l.push_front(1);
        l.push_back(4);
        l.push_back(8);

        int arr[5];
        int i = 0;
        for(auto& item : l) {
            arr[i++] = item;
        }

        TEST(arr[0] == 1 && arr[1] == 5 && arr[2] == 2 && arr[3] == 4 && arr[4] == 8, "(SL4.1) std::List test failed");

        l.sort();

        i = 0;
        for(auto& item : l) {
            arr[i++] = item;
        }

        TEST(arr[0] == 1 && arr[1] == 2 && arr[2] == 4 && arr[3] == 5 && arr[4] == 8, "(SL4.2) std::List sort test failed");
    }

    return true;
}
