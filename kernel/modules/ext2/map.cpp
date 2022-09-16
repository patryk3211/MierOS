#include "fs_func.hpp"
#include "mount_info.hpp"
#include "data_storage.hpp"
#include <fs/devicefs.hpp>

using namespace kernel;

extern std::UnorderedMap<u16_t, MountInfo> mounted_filesystems;

PhysicalPage resolve_mapping(u16_t minor, const FilePage& mapping, virtaddr_t addr) {
    auto mi_opt = mounted_filesystems.at(minor);
    ASSERT_F(mi_opt, "No filesystem is mapped to this minor number");
    auto& mi = *mi_opt;

    ASSERT_F(mi.filesystem == mapping.file()->filesystem(), "Using a filestream from a different filesystem");

    size_t offset = ((addr - mapping.start_addr()) & ~0xFFF) +  mapping.offset();
    auto vnode = mapping.file();

    // Check if offset is in vnode shared pages
    auto pageOpt = vnode->f_shared_pages.at(offset);
    if(pageOpt) {
        // We have a page
        // The flags are handled in process resolve function
        return *pageOpt;
    }

    // We didn't find a page in memory, we have to read it
    PhysicalPage page;
    // We set the kernel access flags here, processes set their flags without looking at this
    page.flags() = PageFlags(1, 1, 0, 0, 1, 0);

    Ext2VNodeDataStorage* vnode_data = (Ext2VNodeDataStorage*)vnode->fs_data;

    auto& pager = Pager::active();
    pager.lock();
    virtaddr_t ptr = pager.kmap(page.addr(), 1, page.flags());

    //u32_t firstBlockIndex = offset >> (mi.superblock->blocks_size + 10);
    //u32_t block = get_inode_block(mi, vnode_data->inode, blockIndex);
    //u32_t blockOffset = offset & ((1 << (mi.superblock->blocks_size + 10)) - 1);

    // We need to read 8 sectors to fill up the page
    constexpr size_t sectorsToRead = (4096 >> 9);
    for(size_t i = 0; i < sectorsToRead; ++i) {
        size_t startI = i;
        u32_t chunkStartBlockIndex = offset >> (mi.superblock->blocks_size + 10);
        u32_t chunkStartBlock = get_inode_block(mi, vnode_data->inode, chunkStartBlockIndex);
        while(get_inode_block(mi, vnode_data->inode, (offset + (i++ << 9)) >> (mi.superblock->blocks_size + 10)) == chunkStartBlock && i < sectorsToRead);

        size_t continousSectors = i - startI;
        u32_t blockByteOffset = offset & ((1 << (mi.superblock->blocks_size + 10)) - 1);
        u32_t sectorOffset = blockByteOffset >> 9;

        pager.unlock();
        auto status = DeviceFilesystem::instance()->block_read(mi.fs_file, mi.get_lba(chunkStartBlock) + sectorOffset, continousSectors, (void*)(ptr + (startI << 9)));
        pager.lock();
    }

    pager.unmap(ptr, 1);
    pager.unlock();

    // Add page to vnode's shared pages if applicable
    if(mapping.shared()) {
        vnode->f_shared_pages.insert({ offset, page });
    }

    return page;
}
