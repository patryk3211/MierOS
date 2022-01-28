#pragma once

#include <types.h>
#include <allocator.hpp>
#include <stdlib.h>

namespace std {
    template<typename T, class Allocator = heap_allocator> class Vector {
        T* elements;

        size_t f_size;
        size_t f_capacity;

        Allocator alloc;
    public:
        static constexpr size_t INITIAL_SIZE = 16;

        typedef T* iterator;

        Vector() : Vector(INITIAL_SIZE) { f_size = 0; }
        Vector(size_t size) {
            this->f_size = size;
            this->f_capacity = size;

            elements = alloc.template alloc<T>(this->f_size);
        }

        Vector(size_t size, const T& initial_value) {
            this->f_size = size;
            this->f_capacity = size;
            
            elements = alloc.template alloc<T>(size);
            for(size_t i = 0; i < size; ++i) elements[i] = initial_value;
        }

        Vector(const Vector<T>& vector) {
            this->f_size = vector.f_size;
            this->f_capacity = vector.f_capacity;

            this->elements = alloc.template alloc<T>(this->f_capacity);
            memcpy(this->elements, vector.elements, this->f_size);
        }

        Vector<T>& operator=(const Vector<T>& vector) {
            clear();
            this->f_size = vector.f_size;
            this->f_capacity = vector.f_capacity;

            this->elements = alloc.template alloc<T>(this->f_capacity);
            memcpy(this->elements, vector.elements, this->f_size);            
        }

        Vector(Vector<T>&& vector) {
            this->f_size = vector.f_size;
            this->f_capacity = vector.f_capacity;

            this->elements = vector.elements;
            vector.elements = 0;
        }

        Vector<T>& operator=(Vector<T>&& vector) {
            clear();
            this->f_size = vector.f_size;
            this->f_capacity = vector.f_capacity;

            this->elements = vector.elements;
            vector.elements = 0;
        }

        ~Vector() {
            alloc.free_array(elements);
        }

        T& operator[](size_t index) {
            return elements[index];
        }

        void reserve(size_t capacity) {
            T* new_buffer = alloc.template alloc<T>(capacity);

            size_t new_size = capacity < f_size ? capacity : f_size;

            if(elements != 0) memcpy(new_buffer, elements, new_size*sizeof(T));

            f_capacity = capacity;
            f_size = new_size;
            alloc.free_array(elements);
            elements = new_buffer;
        }

        void resize(size_t size) {
            reserve(size);
        }

        void resize(size_t size, const T& initial_value) {
            size_t old_size = f_size;
            reserve(size);

            for(size_t i = old_size; i < size; ++i)
                elements[i] = initial_value;
        }

        void clear() {
            alloc.free_array(elements);
            this->f_capacity = 0;
            this->f_size = 0;
            this->elements = 0;
        }

        void push_back(const T& value) {
            if(f_size == f_capacity) reserve(f_capacity+INITIAL_SIZE);
            this->elements[f_size++] = value;
        }

        void pop_back() {
            --f_size;
        }

        size_t size() const {
            return f_size;
        }

        size_t capacity() const {
            return f_capacity;
        }

        iterator begin() {
            return elements;
        }

        iterator end() {
            return elements + f_size;
        }

        T& front() {
            return *elements[0];
        }

        T& back() {
            return *elements[f_size];
        }
    };
}
