#include "cache.hpp"
#include "mount_info.hpp"
#include <fs/devicefs.hpp>
#include <memory/virtual.hpp>

using namespace kernel;

CacheBlock::CacheBlock(MountInfo& mi, u32_t block_addr) : f_mi(mi), f_block_addr(block_addr), f_ref_count(0) {
    f_base = 0;
    f_dirty = false;
}

CacheBlock::~CacheBlock() {
    free();
}

void CacheBlock::sync() {
    if(!f_dirty || f_base == 0) return;
    DeviceFilesystem::instance()->block_write(f_mi.fs_file, f_mi.get_lba(f_block_addr), f_mi.block_size / 512, f_base);
}

void CacheBlock::free() {
    if(f_base == 0) return;
    sync();
    
    auto& pager = Pager::active();
    pager.lock();
    pager.free((virtaddr_t)f_base, f_mi.block_size / 4096 + (f_mi.block_size % 4096 == 0 ? 0 : 1));
    pager.unlock();
}

void CacheBlock::ref() {
    f_ref_count.fetch_add(1);
}

void CacheBlock::unref() {
    f_ref_count.fetch_sub(1);
}

u32_t CacheBlock::ref_count() {
    return f_ref_count.load();
}

void* CacheBlock::ptr() {
    if(f_base == 0) {
        auto& pager = Pager::active();
        pager.lock();
        f_base = (void*)pager.kalloc(f_mi.block_size / 4096 + (f_mi.block_size % 4096 == 0 ? 0 : 1));
        pager.unlock();

        DeviceFilesystem::instance()->block_read(f_mi.fs_file, f_mi.get_lba(f_block_addr), f_mi.block_size / 512, f_base);
    }
    return f_base;
}
