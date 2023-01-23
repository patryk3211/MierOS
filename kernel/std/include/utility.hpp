#pragma once

namespace std {
    template<class T> struct __remove_reference { typedef T Type; };
    template<class T> struct __remove_reference<T&> { typedef T Type; };
    template<class T> struct __remove_reference<T&&> { typedef T Type; };

    template<typename T> using remove_reference = typename __remove_reference<T>::Type;

    template<typename T> remove_reference<T>&& move(T&& t) {
        return static_cast<typename __remove_reference<T>::Type&&>(t);
    }

    template<typename T> T&& forward(remove_reference<T>& t) {
        return static_cast<T&&>(t);
    }

    template<typename T> T&& forward(remove_reference<T>&& t) {
        return static_cast<T&&>(t);
    }


    template<class T> struct __remove_cv { typedef T Type; };
    template<class T> struct __remove_cv<const T> { typedef T Type; };
    template<class T> struct __remove_cv<volatile T> { typedef T Type; };

    template<typename T> using remove_cv = typename __remove_cv<T>::Type;
}
