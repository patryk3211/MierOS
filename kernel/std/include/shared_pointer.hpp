#pragma once

#include <types.h>
#include <function.hpp>
#include <optional.hpp>
#include <atomic.hpp>

namespace std {
    template<typename T> class SharedPtr {
        struct _data {
            T value;
            std::Atomic<size_t> ref_count;
            std::Optional<std::Function<void(T&)>> destructor;
        } *data;
    public:
        SharedPtr(const T& value, std::Optional<std::Function<void(T&)>> destructor = std::Optional()) {
            data = new _data();
            data->value = value;
            data->ref_count->load(ref_count);
            data->destructor = destructor;
        }

        SharedPtr(const SharedPtr& ptr) : data(ptr.data) {
            this->data->ref_count->fetch_add(1);
        }

        SharedPtr(SharedPtr&& ptr) : data(ptr.leak_ptr()) { }

        ~SharedPtr() {
            if(this->data->ref_count->fetch_sub(1) == 1) {
                if(this->data->destructor) (*this->data->destructor)(this->data->value);
                delete data;
            }
        }

        _data* leak_ptr() {
            _data* ptr = data;
            data = 0;
            return ptr;
        }
    };
}