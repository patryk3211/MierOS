#pragma once

#include <new.hpp>
#include <utility.hpp>

namespace std {
    template<typename T> class Optional {
        alignas(T) unsigned char storage[sizeof(T)];
        bool exists;

    public:
        Optional()
            : exists(false) { }
        Optional(const T& value)
            : exists(true) { new(storage) T(value); }

        Optional(const Optional<T>& other)
            : exists(other.exists) {
            if(exists) new(storage) T(other.value());
        }

        Optional(Optional<T>&& other)
            : exists(other.exists) {
            if(exists) {
                new(storage) T(move(other.value()));
                other.exists = false;
            }
        }

        Optional<T>& operator=(const Optional<T>& other) {
            clear();
            exists = other.exists;
            if(exists) new(storage) T(other.value());
            return *this;
        }

        Optional<T>& operator=(Optional<T>&& other) {
            clear();
            exists = other.exists;
            if(exists) {
                new(storage) T(move(other.value()));
                other.exists = false;
            }
            return *this;
        }

        ~Optional() {
            clear();
        }

        void clear() {
            if(exists) {
                value().~T();
                exists = false;
            }
        }

        T& value() {
            return *reinterpret_cast<T*>(storage);
        }
        const T& value() const {
            return *reinterpret_cast<const T*>(storage);
        }

        T& operator*() { return value(); }
        T* operator->() { return &value(); }

        const T& operator*() const { return value(); }
        const T* operator->() const { return &value(); }

        operator bool() const { return exists; }
        bool operator!() const { return !exists; }
    };

    template<typename T> class OptionalRef {
        T* reference;

    public:
        OptionalRef()
            : reference(0) { }
        OptionalRef(T& value)
            : reference(&value) { }

        T& operator*() { return *reference; }
        T* operator->() { return reference; }

        operator bool() const { return !!reference; }
        bool operator!() const { return !reference; }

        T& resolve_or(const T& other) {
            return reference ? *reference : const_cast<T&>(other);
        }
    };
}
