#pragma once
#include <types.h>
#include <atomic.hpp>

struct MountInfo;
class CacheBlock {
    MountInfo& f_mi;
    void* f_base;
    u32_t f_block_addr;
    std::Atomic<u32_t> f_ref_count;
    bool f_dirty;

public: 
    CacheBlock(MountInfo& mi, u32_t block_addr);
    ~CacheBlock();

    void sync();
    void free();

    void* ptr();
};
