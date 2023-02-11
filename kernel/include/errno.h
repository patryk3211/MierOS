#ifndef _MIEROS_KERNEL_ERRNO_H
#define _MIEROS_KERNEL_ERRNO_H

#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef int err_t;

// Copied from mlibc errno

#define ENOSYS      	(err_t) 1
#define ENOENT      	(err_t) 2
#define ENOTDIR     	(err_t) 3
#define EEXIST      	(err_t) 4
#define ENODEV      	(err_t) 5
#define ENOEXEC     	(err_t) 6
#define EAGAIN      	(err_t) 7
#define EACCES      	(err_t) 8
#define EIO         	(err_t) 9
#define ESRCH       	(err_t) 10
#define ERANGE      	(err_t) 11
#define ENOMEM      	(err_t) 12
#define EINTR       	(err_t) 13
#define EPERM       	(err_t) 14
#define ETIMEDOUT   	(err_t) 15
#define EBUSY       	(err_t) 16
#define ENOTSUP     	(err_t) 17
#define ECHILD      	(err_t) 18
#define ECONNREFUSED 	(err_t) 19
#define EBADF       	(err_t) 20
#define EDEADLK     	(err_t) 21
#define EINVAL      	(err_t) 22
#define ECONNRESET  	(err_t) 23
#define ENOTCONN    	(err_t) 24
#define EPIPE       	(err_t) 25
#define EFAULT      	(err_t) 26
#define EOVERFLOW   	(err_t) 27
#define EISDIR      	(err_t) 28
#define ESPIPE      	(err_t) 29
#define ENXIO       	(err_t) 30
#define EDOM            (err_t) 31
#define EILSEQ          (err_t) 32
#define E2BIG           (err_t) 33
#define EADDRINUSE      (err_t) 34
#define ENOTSOCK        (err_t) 35
#define ENAMETOOLONG    (err_t) 36
#define EADDRNOTAVAIL   (err_t) 37
#define EALREADY        (err_t) 38
#define EBADMSG         (err_t) 39
#define ECANCELED       (err_t) 40
#define ECONNABORTED    (err_t) 41
#define EDESTADDRREQ    (err_t) 42
#define EDQUOT          (err_t) 43
#define EFBIG           (err_t) 44
#define EHOSTUNREACH    (err_t) 45
#define EIDRM           (err_t) 46
#define EINPROGRESS     (err_t) 47
#define EISCONN         (err_t) 48
#define EMFILE          (err_t) 49
#define EMLINK          (err_t) 50
#define EMSGSIZE        (err_t) 51
#define EMULTIHOP       (err_t) 52
#define ENETDOWN        (err_t) 53
#define ENETRESET       (err_t) 54
#define ENETUNREACH     (err_t) 55
#define ENFILE          (err_t) 56
#define ENOBUFS         (err_t) 57
#define ENOLINK         (err_t) 58
#define ENOMSG          (err_t) 59
#define ENOPROTOOPT     (err_t) 60
#define ENOTEMPTY       (err_t) 61
#define ENOTTY          (err_t) 62
#define ENOTRECOVERABLE (err_t) 63
#define EOPNOTSUPP      (err_t) 64
#define EOWNERDEAD      (err_t) 65
#define EPROTO          (err_t) 66
#define EPROTONOSUPPORT (err_t) 67
#define EPROTOTYPE      (err_t) 68
#define EROFS           (err_t) 69
#define ESTALE          (err_t) 70
#define ETXTBSY         (err_t) 71
#define EXDEV           (err_t) 72
#define ENODATA         (err_t) 73
#define ETIME           (err_t) 74
#define ENOKEY          (err_t) 75
#define ESHUTDOWN       (err_t) 76
#define EHOSTDOWN       (err_t) 77
#define EBADFD          (err_t) 78
#define ENOMEDIUM       (err_t) 79
#define ENOTBLK         (err_t) 80
#define ENONET          (err_t) 81
#define EPFNOSUPPORT    (err_t) 82
#define ESTRPIPE        (err_t) 83
#define ERFKILL         (err_t) 84
#define EREMOTEIO       (err_t) 85
#define EBADR           (err_t) 86
#define EMEDIUMTYPE     (err_t) 87
#define EUNATCH         (err_t) 88
#define EKEYREJECTED    (err_t) 89
#define EUCLEAN         (err_t) 90
#define EBADSLT         (err_t) 91
#define EREMOTE         (err_t) 92
#define ENOANO          (err_t) 93
#define ENOCSI          (err_t) 94
#define ENOSTR          (err_t) 95
#define ETOOMANYREFS    (err_t) 96
#define ENOPKG          (err_t) 97
#define EKEYREVOKED     (err_t) 98
#define EXFULL          (err_t) 99
#define ELNRNG          (err_t) 100
#define ENOTUNIQ        (err_t) 101
#define ERESTART        (err_t) 102
#define EUSERS          (err_t) 103
#define ELOOP           (err_t) 104
#define ENOLCK          (err_t) 105
#define ESOCKTNOSUPPORT (err_t) 106
#define EWOULDBLOCK     (err_t) 107
#define EAFNOSUPPORT    (err_t) 108
#define ENOSPC          (err_t) 109

#if defined(__cplusplus)
} // extern "C"

#include <new.hpp>
#include <utility.hpp>

namespace kernel {
    template<typename T> class ValueOrError {
        alignas(T) u8_t storage[sizeof(T)];
        int error;

    public:
        ValueOrError(const T& value)
            : error(0) { new(storage) T(value); }
        ValueOrError(err_t errno)
            : error(errno) { }

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
        ValueOrError()
            : error(0) { }
        ValueOrError(err_t errno)
            : error(errno) { }

        ValueOrError(const ValueOrError<void>& other)
            : error(other.error) { }
        ValueOrError(ValueOrError<void>&& other)
            : error(other.error) { }

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
