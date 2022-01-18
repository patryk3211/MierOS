#pragma once

namespace kernel {
    template<typename T> T&& move(T& t) {
        return static_cast<T&&>(t);
    }

    template<typename T> struct __remove_reference {
        using Type = T;
    };
    template<class T> struct __remove_reference<T&> {
        using Type = T;
    };
    template<class T> struct __remove_reference<T&&> {
        using Type = T;
    };

    template<typename T> using remove_reference = typename __remove_reference<T>::Type;

    template<typename T> T&& forward(remove_reference<T>& t) {
        return static_cast<T&&>(t);
    }

    template<typename T> T&& forward(remove_reference<T>&& t) {
        return static_cast<T&&>(t);
    }
}
