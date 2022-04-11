#pragma once

#include <types.h>
#include <function.hpp>
#include <optional.hpp>
#include <atomic.hpp>
#include <cstddef.hpp>

namespace std {
    template<typename T> class SharedPtr {
        struct DataStorage {
            alignas(T) unsigned char storage[sizeof(T)];
            std::Atomic<size_t> ref_count;
            std::Optional<std::Function<void(T&)>> destructor;

            T& value() { return *reinterpret_cast<T*>(storage); }
        } *data;
    public:
        SharedPtr() {
            data = 0;
        }

        SharedPtr(const T& value, std::Optional<std::Function<void(T&)>> destructor = std::Optional<std::Function<void(T&)>>()) {
            data = new DataStorage();
            new(data->storage) T(value);
            data->ref_count.store(1);
            data->destructor = destructor;
        }

        SharedPtr(nullptr_t) {
            data = 0;
        }

        SharedPtr<T>& operator=(nullptr_t) {
            clear();
        }

        SharedPtr(const SharedPtr<T>& ptr) : data(ptr.data) {
            if(this->data != 0) this->data->ref_count.fetch_add(1);
        }

        SharedPtr<T>& operator=(const SharedPtr<T>& ptr) {
            clear();

            this->data = ptr.data;
            if(this->data != 0) this->data->ref_count.fetch_add(1);
            return *this;
        }

        SharedPtr(SharedPtr<T>&& ptr) : data(ptr.leak_ptr()) { }

        SharedPtr<T>& operator=(SharedPtr<T>&& ptr) {
            clear();

            this->data = ptr.leak_ptr();
            return *this;
        }

        ~SharedPtr() {
            clear();
        }

        DataStorage* leak_ptr() {
            DataStorage* ptr = data;
            data = 0;
            return ptr;
        }

        void clear() {
            if(this->data && this->data->ref_count.fetch_sub(1) == 1) {
                if(this->data->destructor) (*this->data->destructor)(this->data->value());
                data->value().~T();
                delete data;
            }

            this->data = 0;
        }

        T* ptr() { return &data->value(); }
        const T* ptr() const { return &data->value(); }

        T* operator->() { return &data->value(); }
        const T* operator->() const { return &data->value(); }

        T& operator*() { return data->value(); }
        const T& operator*() const { return data->value(); }

        operator bool() { return data != 0; }
        bool operator!() { return data == 0; }

        template<typename C, typename... Args> friend SharedPtr<C> make_shared(Args...);
    };

    template<typename C, typename... Args> SharedPtr<C> make_shared(Args... args) {
        auto ptr = SharedPtr<C>();
        ptr.data = new typename SharedPtr<C>::DataStorage();
        new(ptr.data->storage) C(args...);
        ptr.data->ref_count.store(1);
        return ptr;
    }
}