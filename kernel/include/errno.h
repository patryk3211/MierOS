#ifndef _MIEROS_KERNEL_ERRNO_H
#define _MIEROS_KERNEL_ERRNO_H

#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef int err_t;

#define ERR_UNIMPLEMENTED (err_t)1
#define ERR_FILE_NOT_FOUND (err_t)2
#define ERR_NOT_A_DIRECTORY (err_t)3
#define ERR_FILE_EXISTS (err_t)4
#define ERR_LINK (err_t)5

#if defined(__cplusplus)
} // extern "C"

#include <new.hpp>
#include <utility.hpp>

namespace kernel {
    template<typename T> class ValueOrError {
        alignas(T) u8_t storage[sizeof(T)];
        int error;
    public:
        ValueOrError(const T& value) : error(0) { new(storage) T(value); }
        ValueOrError(err_t errno) : error(errno) { }

        ValueOrError(const ValueOrError<T>& other) {
            error = other.error;
            if(error == 0)
                new(storage) T(other.value());
        }

        ValueOrError(ValueOrError<T>&& other) {
            error = other.error;
            if(error == 0) {
                other.error = -1;
                new(storage) T(std::move(other.value()));
            }
        }

        ValueOrError& operator=(const ValueOrError<T>& other) {
            if(error == 0) value().~T();
            error = other.error;
            if(error == 0)
                new(storage) T(other.value());
            return *this;
        }

        ValueOrError& operator=(ValueOrError<T>&& other) {
            if(error == 0) value().~T();
            error = other.error;
            if(error == 0) {
                other.error = -1;
                new(storage) T(std::move(other.value()));
            }
            return *this;
        }

        ~ValueOrError() {
            if(error == 0) value().~T();
        }

        T& value() {
            return *reinterpret_cast<T*>(storage);
        }

        err_t errno() {
            return error;
        }

        T& operator*() {
            return value();
        }

        T* operator->() {
            return &value();
        }

        operator bool() {
            return error == 0;
        }
    };

    template<> class ValueOrError<void> {
        int error;
    public:
        ValueOrError() : error(0) { }
        ValueOrError(err_t errno) : error(errno) { }

        ValueOrError(const ValueOrError<void>& other) : error(other.error) { }
        ValueOrError(ValueOrError<void>&& other) : error(other.error) { }

        ValueOrError& operator=(const ValueOrError<void>& other) {
            error = other.error;
            return *this;
        }
        ValueOrError& operator=(ValueOrError<void>&& other) {
            error = other.error;
            return *this;
        }

        ~ValueOrError() { }

        void value() { }

        err_t errno() {
            return error;
        }

        operator bool() {
            return error == 0;
        }
    };
}

#endif

#endif // _MIEROS_KERNEL_ERRNO_H
