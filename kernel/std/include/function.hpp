#pragma once

#include <stdlib.h>
#include <unique_pointer.hpp>
#include <utility.hpp>
#include <assert.h>

namespace std {
    template<typename> class Function;
    template<typename Ret, typename... Args> class Function<Ret(Args...)> {
    public:
        template<typename T> Function(T func) {
            callable = make_unique<Callable<T>>(move(func));
        }

        Function(const Function& other) {
            callable = other.callable->copy();
        }
        /*template<typename T> Function& operator=(T func) {
            callable = make_unique(new Callable<T>(move(func)));
            return *this;
        }*/

        Ret operator()(Args... args) {
            return callable->invoke(args...);
        }

    private:
        class CallableBase {
        public:
            virtual ~CallableBase() = default;
            virtual Ret invoke(Args...) {
                ASSERT_NOT_REACHED("You should not be here");
                return Ret();
            }

            virtual UniquePtr<CallableBase> copy() const {
                ASSERT_NOT_REACHED("You should not be here");
                return UniquePtr<CallableBase>();
            }
        };

        template<typename Func> class Callable : public CallableBase {
            Func func;

        public:
            Callable(Func& func) : func(move(func)) { }
            Callable(const Callable& other) : func(other.func) { }

            ~Callable() override = default;

            virtual Ret invoke(Args... args) {
                return func(forward<Args>(args)...);
            }

            virtual UniquePtr<CallableBase> copy() const {
                return make_unique<Callable<Func>>(move(func));
            }
        };

        UniquePtr<CallableBase> callable;
    };
}
