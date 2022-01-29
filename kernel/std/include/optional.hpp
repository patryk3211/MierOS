#pragma once

namespace std {
    template<typename T> class Optional {
        T value;
        bool exists;
    public:
        Optional() : value(), exists(false) { }
        Optional(const T& value) : value(value), exists(true) { }

        T operator*() {
            if(exists) return *this;
            else return T();
        }

        operator bool() { return exists; }
        bool operator!() { return !exists; }
    };

    template<typename T> class OptionalRef {
        T* reference;
    public:
        OptionalRef() : reference(0) { }
        OptionalRef(T& value) : reference(&value) { }

        T& operator*() { return *reference; }
        T* operator->() { return reference; }

        operator bool() { return !!reference; }
        bool operator!() { return !reference; }
    };
}
