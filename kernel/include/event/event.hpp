#pragma once

#include <types.h>
#include <assert.h>

namespace kernel {
    template<typename T> struct EventArg {
        // Basic implementation will just utilize the copy constructor of given type
        size_t encode(u8_t* buffer, const T& value) {
            new(buffer) T(value);
            return sizeof(T);
        }

        T& decode(u8_t*& buffer) {
            T* ptr = reinterpret_cast<T*>(buffer);
            buffer += sizeof(T);
            return *ptr;
        }
    };

    class Event {
        u64_t f_identified;
        bool f_consumed;

        u8_t  f_arg_storage[512];
        u8_t* f_arg_ptr;

    public:
        // Remember to correctly specify the types of arguments you are
        // pushing to the event object. If the order of decoding the args
        // is not exactly the same as the encoding order you will get
        // garbage instead of your arguments
        template<typename... Args> Event(u64_t id, Args... args) {
            f_identified = id;
            f_consumed = false;
            f_arg_ptr = f_arg_storage;

            u8_t* buffer = f_arg_storage;
            encode_arg(buffer, args...);
        }

        void consume() {
            f_consumed = true;
        }

        bool is_consumed() const {
            return f_consumed;
        }

        u64_t identifier() const {
            return f_identified;
        }

        template<typename T> T& get_arg() {
            return EventArg<T>().decode(f_arg_ptr);
        }

    private:
        // A bit of template code to encode variadic args passed to the Event constructor
        template<typename Arg, typename... TArgs> void encode_arg(u8_t*& buffer, Arg arg, TArgs... targs) {
            buffer += EventArg<Arg>().encode(buffer, arg);
            ASSERT_F(static_cast<size_t>(buffer - f_arg_storage) > sizeof(f_arg_storage), "Ran out of event arg storage space");
            encode_arg(buffer, targs...);
        }

        template<typename Arg> void encode_arg(u8_t*& buffer, Arg arg) {
            buffer += EventArg<Arg>().encode(buffer, arg);
            ASSERT_F(static_cast<size_t>(buffer - f_arg_storage) > sizeof(f_arg_storage), "Ran out of event arg storage space");
        }

        void encode_arg(u8_t*&) {
            // This gets "run" if there are no args passed in the Event constructor
        }
    };
}
