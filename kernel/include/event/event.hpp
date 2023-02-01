#pragma once

#include <types.h>
#include <assert.h>
#include <stdlib.h>
#include <utility.hpp>
#include <stdatomic.h>
#include <cstddef.hpp>

#define EVENT_FLAG_CONSUMED     1
#define EVENT_FLAG_CLEAR        2
#define EVENT_FLAG_PROCESSED    4
#define EVENT_FLAG_META_EVENT   8

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

    template<> struct EventArg<std::nullptr_t> {
        size_t encode(u8_t* buffer, std::nullptr_t) {
            memset(buffer, 0, sizeof(void*));
            return sizeof(void*);
        }

        std::nullptr_t decode(u8_t*& buffer) {
            buffer += sizeof(void*);
            return nullptr;
        }
    };

    template<> struct EventArg<char*> {
        size_t encode(u8_t* buffer, const char* value) {
            size_t len = strlen(value) + 1;
            memcpy(buffer, value, len);
            return len;
        }

        char* decode(u8_t*& buffer) {
            char* ptr = (char*)buffer;
            size_t len = strlen(ptr);
            buffer += len + 1;
            return ptr;
        }
    };

    template<> struct EventArg<const char*> : public EventArg<char*> { };

    template<> struct EventArg<char**> {
        size_t encode(u8_t* buffer, char** value) {
            u16_t* bytesTaken = (u16_t*)buffer;
            *bytesTaken += 2;
            buffer += 2;

            size_t argCount = 0;
            while(value[argCount]) ++argCount;
            *bytesTaken += sizeof(char**) * (argCount + 1);

            char** argPtrs = (char**)(buffer);
            char* dataBuffer = (char*)(buffer + (sizeof(char**) * (argCount + 1)));
            for(char** arg = value; *arg != 0; ++arg) {
                size_t argLen = strlen(*arg);
                memcpy(dataBuffer, *arg, argLen + 1);
                *argPtrs = dataBuffer;

                dataBuffer += argLen + 1;
                *bytesTaken += sizeof(char) * (argLen + 1);
                ++argPtrs;
            }
            *argPtrs = 0;
            return *bytesTaken;
        }

        char** decode(u8_t*& buffer) {
            u16_t skipBytes = *(u16_t*)buffer;
            char** ptr = (char**)(buffer + 2);
            buffer += skipBytes;
            return ptr;
        }
    };

    template<> struct EventArg<const char**> : public EventArg<char**> {
        size_t encode(u8_t* buffer, const char** value) {
            return EventArg<char**>().encode(buffer, const_cast<char**>(value));
        }

        const char** decode(u8_t*& buffer) {
            return (const char**)EventArg<char**>().decode(buffer);
        }
    };

    class Event {
        u64_t f_identifier;
        u64_t f_local_id;
        atomic_int f_flags;
        u64_t f_status;

        u8_t  f_arg_storage[1024];
        u8_t* f_arg_ptr;

    public:
        // Remember to correctly specify the types of arguments you are
        // pushing to the event object. If the order of decoding the args
        // is not exactly the same as the encoding order you will get
        // garbage instead of your arguments
        template<typename... Args> Event(u64_t id, Args... args) {
            f_identifier = id;
            f_arg_ptr = f_arg_storage;
            f_flags = EVENT_FLAG_CLEAR;
            f_status = 0;

            u8_t* buffer = f_arg_storage;
            encode_arg(buffer, args...);
        }

        /**
         * @brief Keep event
         *
         * Running this function will tell the event manager to not delete an event
         * after it has been processed. This can be useful in certain situations.
         */
        void keep() {
            clear_flags(EVENT_FLAG_CLEAR);
        }

        void consume() {
            set_flags(EVENT_FLAG_CONSUMED);
        }

        void set_status(u64_t status) {
            f_status = status;
        }


        void set_flags(u32_t flags) {
            atomic_fetch_or(&f_flags, flags);
        }

        void clear_flags(u32_t flags) {
            atomic_fetch_and(&f_flags, ~flags);
        }

        u32_t get_flags() const {
            return atomic_load(&f_flags);
        }


        bool is_consumed() const {
            return get_flags() & EVENT_FLAG_CONSUMED;
        }

        bool should_clear() const {
            return get_flags() & EVENT_FLAG_CLEAR;
        }

        bool is_processed() const {
            return get_flags() & EVENT_FLAG_PROCESSED;
        }


        u64_t identifier() const {
            return f_identifier;
        }

        u64_t eid() const {
            return f_local_id;
        }

        u64_t status() const {
            return f_status;
        }

        template<typename T> T get_arg() {
            return EventArg<T>().decode(f_arg_ptr);
        }

        void reset_arg_ptr() {
            f_arg_ptr = f_arg_storage;
        }

    private:
        // A bit of template code to encode variadic args passed to the Event constructor
        template<typename Arg, typename... TArgs> void encode_arg(u8_t*& buffer, Arg arg, TArgs... targs) {
            buffer += EventArg<Arg>().encode(buffer, arg);
            ASSERT_F(static_cast<size_t>(buffer - f_arg_storage) < sizeof(f_arg_storage), "Ran out of event arg storage space");
            encode_arg(buffer, targs...);
        }

        template<typename Arg> void encode_arg(u8_t*& buffer, Arg arg) {
            buffer += EventArg<Arg>().encode(buffer, arg);
            ASSERT_F(static_cast<size_t>(buffer - f_arg_storage) < sizeof(f_arg_storage), "Ran out of event arg storage space");
        }

        void encode_arg(u8_t*&) {
            // This gets "run" if there are no args passed in the Event constructor
        }

        friend class EventManager;
    };
}
