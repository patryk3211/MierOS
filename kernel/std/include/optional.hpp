#pragma once

namespace std {
    template<typename T> class OptionalRef {
        T* reference;
    public:
        OptionalRef() : reference(0) { }
        OptionalRef(T& value) : reference(&value) { }

        T& operator*() { return *reference; }

        operator bool() { return !!reference; }
        bool operator!() { return !reference; }
    };
}
