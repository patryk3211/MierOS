#pragma once

#include <types.h>

class FIFO {
    const size_t f_capacity;

    u8_t* f_storage;
    size_t f_read_head;
    size_t f_write_head;

public:
    FIFO(size_t capacity) : f_capacity(capacity) {
        f_storage = new u8_t[capacity];
        f_read_head = 0;
        f_write_head = 0;
    }

    ~FIFO() {
        delete f_storage;
    }

    bool write(u8_t value, bool block) {
        if(block) {
            while((f_write_head + 1) == f_read_head);
        } else {
            if((f_write_head + 1) == f_read_head) return false;
        }

        f_storage[f_write_head] = value;
        f_write_head = (f_write_head + 1) % f_capacity;
        return true;
    }

    bool read(u8_t* out, bool block) {
        if(block) {
            while(f_read_head == f_write_head);
        } else {
            if(f_read_head == f_write_head) return false;
        }

        *out = f_storage[f_read_head];
        f_read_head = (f_read_head + 1) % f_capacity;
        return true;
    }
};

