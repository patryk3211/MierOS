#pragma once

#include <atomic.hpp>
#include <cstddef.hpp>
#include <function.hpp>
#include <optional.hpp>
#include <types.h>

namespace std {
    struct GenericDataStorage {
        std::Atomic<size_t> f_ref_count;

        GenericDataStorage()
            : f_ref_count(1) { }
        
        virtual ~GenericDataStorage() { }
        virtual void* data() { return 0; }

        template<typename T> T& get() { return *static_cast<T*>(data()); }
    };

    template<typename T> class SharedPtr {
        struct TypedDataStorage : public GenericDataStorage {
            using DestructorOpt = std::Optional<std::Function<void(T&)>>;

            alignas(T) unsigned char f_storage[sizeof(T)];
            DestructorOpt f_destructor;

            TypedDataStorage(DestructorOpt destructor)
                : f_destructor(destructor) {
            }

            TypedDataStorage(const T& value, DestructorOpt destructor)
                : f_destructor(destructor) {
                new(f_storage) T(value);
            }

            virtual ~TypedDataStorage() {
                if(f_destructor)
                    (*f_destructor)(value());
                value().~T();
            }

            T& value() { return *reinterpret_cast<T*>(f_storage); }
            virtual void* data() { return f_storage; }
        };

        GenericDataStorage* f_data;

    public:
        SharedPtr() {
            f_data = 0;
        }

        SharedPtr(nullptr_t) {
            f_data = 0;
        }

        SharedPtr<T>& operator=(nullptr_t) {
            clear();
        }

        SharedPtr(const T& value, std::Optional<std::Function<void(T&)>> destructor = std::Optional<std::Function<void(T&)>>()) {
            f_data = new TypedDataStorage(value, destructor);
        }

        SharedPtr(const SharedPtr<T>& ptr)
            : f_data(ptr.f_data) {
            if(f_data)
                f_data->f_ref_count.fetch_add(1);
        }

        SharedPtr<T>& operator=(const SharedPtr<T>& ptr) {
            clear();

            f_data = ptr.f_data;
            if(f_data)
                f_data->f_ref_count.fetch_add(1);
            return *this;
        }

        SharedPtr(SharedPtr<T>&& ptr)
            : f_data(ptr.leak_ptr()) { }

        SharedPtr<T>& operator=(SharedPtr<T>&& ptr) {
            clear();

            f_data = ptr.leak_ptr();
            return *this;
        }

        template<typename Y> SharedPtr(const SharedPtr<Y>& other)
            : f_data(other.get_storage()) {
            if(f_data)
                f_data->f_ref_count.fetch_add(1);
        }

        template<typename Y> SharedPtr<T>& operator=(const SharedPtr<Y>& other) {
            clear();

            f_data = other.get_storage();
            if(f_data)
                f_data->f_ref_count.fetch_add(1);
            return *this;
        }

        ~SharedPtr() {
            clear();
        }

        GenericDataStorage* leak_ptr() {
            GenericDataStorage* ptr = f_data;
            f_data = 0;
            return ptr;
        }

        GenericDataStorage* get_storage() const {
            return f_data;
        }

        void clear() {
            if(f_data && f_data->f_ref_count.fetch_sub(1) == 1)
                delete f_data;
            f_data = 0;
        }

        T* ptr() { return &f_data->get<T>(); }
        const T* ptr() const { return &f_data->get<T>(); }

        T* operator->() { return &f_data->get<T>(); }
        const T* operator->() const { return &f_data->get<T>(); }

        T& operator*() { return f_data->get<T>(); }
        const T& operator*() const { return f_data->get<T>(); }

        operator bool() const { return f_data != 0; }
        bool operator!() const { return f_data == 0; }

        template<typename C, typename... Args> friend SharedPtr<C> make_shared(Args...);
    };

    template<typename C, typename... Args> SharedPtr<C> make_shared(Args... args) {
        auto ptr = SharedPtr<C>(nullptr);
        auto* typed = new typename SharedPtr<C>::TypedDataStorage({ });
        new(typed->f_storage) C(args...);
        ptr.f_data = typed;
        return ptr;
    }
}
