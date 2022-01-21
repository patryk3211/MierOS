#pragma once

#include <stdlib.h>
#include <memory/liballoc.h>

namespace std {
    template<typename T> class SharedPtr {
        
    };

    template<typename T> class UniquePtr {
        T* value;
    public:
        UniquePtr() { value = 0; }
        UniquePtr(T* value) : value(value) { }

        UniquePtr(const UniquePtr<T>& ptr) : value(new T(*ptr.ptr())) { }
        UniquePtr<T>& operator=(const UniquePtr<T>& ptr) {
            delete value;
            value = new T(*ptr.ptr());
            return *this;
        }

        UniquePtr(UniquePtr<T>&& ptr) : value(ptr.leak_ptr()) { }
        UniquePtr<T>& operator=(UniquePtr<T>&& ptr) {
            delete value;
            value = ptr.leak_ptr();
            return *this;
        }

        template<typename U> UniquePtr(UniquePtr<U>&& ptr) : value(static_cast<T*>(ptr.leak_ptr())) { }
        template<typename U> UniquePtr<T>& operator=(UniquePtr<U>&& ptr) {
            delete value;
            value = ptr.leak_ptr();
            return *this;
        }
        /*UniquePtr(const UniquePtr& other) {
            value = (T*)malloc(sizeof(*other.value));
            memcpy(value, other.value, sizeof(*other.value));
        }*/
        /*template<typename U> UniquePtr(const UniquePtr<U>& other) {
            U* v = (U*)malloc(sizeof(U));
            memcpy(v, other.ptr(), sizeof(U));
            value = static_cast<T*>(v);
        }*/

        /*UniquePtr& operator=(const UniquePtr& other) {
            delete value;
            //value = new T(*other.value);
            value = (T*)malloc(sizeof(*other.value));
            memcpy(value, other.value, sizeof(*other.value));
            return *this;
        }*/

        /*template<typename U> UniquePtr& operator=(const UniquePtr<U>& other) {
            //delete value;
            //value = static_cast<T*>(new U(*other.ptr()));
            //return *this;
            delete value;
            U* v = (U*)malloc(sizeof(U));
            memcpy(v, other.ptr(), sizeof(U));
            value = static_cast<T*>(v);
            return *this;
        }*/

        ~UniquePtr() {
            delete value;
        }

        void clear() {
            delete value;
            value = 0;
        }

        void reset(T* new_val) {
            delete value;
            value = new_val;
        }

        T* ptr() { return value; }
        const T* ptr() const { return value; }

        T* operator->() { return value; }
        const T* operator->() const { return value; }

        T& operator*() { return *value; }
        const T& operator*() const { return *value; }

        operator bool() const { return !!value; }
        bool operator!() const { return !value; }

        T* leak_ptr() {
            T* ptr = value;
            value = 0;
            return ptr;
        }
    };

    template<typename C, typename... Args> inline UniquePtr<C> make_unique(Args... args) {
        return UniquePtr(new C(args...));
    }
}
