#include "data_storage.hpp"
#include "fs_func.hpp"
#include "mount_info.hpp"
#include <fs/devicefs.hpp>
#include <streams/stream.hpp>

using namespace kernel;

extern std::UnorderedMap<u16_t, MountInfo> mounted_filesystems;

ValueOrError<size_t> read(u16_t minor, FileStream* filestream, void* buffer, size_t length) {
    auto mi_opt = mounted_filesystems.at(minor);
    ASSERT_F(mi_opt, "No filesystem is mapped to this minor number");
    auto& mi = *mi_opt;

    ASSERT_F(mi.filesystem == filestream->node()->filesystem(), "Using a filestream from a different filesystem");

    Ext2StreamDataStorage* file_data = (Ext2StreamDataStorage*)filestream->fs_data;

    // Check if file is long enought to read the requested amount, if not shorten the amount of data to read
    if(filestream->node()->f_size - file_data->position < length) length = filestream->node()->f_size - file_data->position;

    size_t leftToRead = length;
    u8_t* byteBuffer = (u8_t*)buffer;

    // Read until the file offset is aligned to filesystem block size
    size_t sizeToRead = mi.block_size - (file_data->position & (mi.block_size - 1));
    if(sizeToRead == mi.block_size) sizeToRead = 0;

    if(sizeToRead > leftToRead) sizeToRead = leftToRead;

    if(sizeToRead > 0) {
        memcpy(byteBuffer, (u8_t*)file_data->buffer.ptr() + (file_data->position & (mi.block_size - 1)), sizeToRead);
        leftToRead -= sizeToRead;
        byteBuffer += sizeToRead;
        file_data->position += sizeToRead;
    }

    if(leftToRead == 0) return length;

    Ext2VNodeDataStorage* vnode_data = (Ext2VNodeDataStorage*)filestream->node()->fs_data;

    // Read aligned blocks
    size_t blocksToRead = leftToRead >> (mi.superblock->blocks_size + 10);

    while(blocksToRead) {
        // Try to combine as much blocks into one read request as possible
        u32_t startBlock = file_data->position >> (mi.superblock->blocks_size + 10);
        u32_t block = get_inode_block(mi, vnode_data->inode, startBlock);

        size_t blkCount = 0;
        while(get_inode_block(mi, vnode_data->inode, startBlock + blkCount) == block + blkCount && blkCount < blocksToRead) ++blkCount;

        // Read data and return error value if something goes wrong
        auto ret = DeviceFilesystem::instance()->block_read(mi.fs_file, mi.get_lba(block), mi.block_size / 512 * blkCount, byteBuffer);
        if(!ret) return ret.errno();

        blocksToRead -= blkCount;
        leftToRead -= blkCount * mi.block_size;
        byteBuffer += blkCount * mi.block_size;
        file_data->position += blkCount * mi.block_size;
    }

    if(leftToRead == 0) return length;

    // Read unaligned tail into the buffer and copy requested data
    u32_t block = get_inode_block(mi, vnode_data->inode, file_data->position >> (mi.superblock->blocks_size + 10));

    auto ret = DeviceFilesystem::instance()->block_read(mi.fs_file, mi.get_lba(block), mi.block_size / 512, file_data->buffer.ptr());
    if(!ret) return ret.errno();

    memcpy(byteBuffer, file_data->buffer.ptr(), leftToRead);
    file_data->position += leftToRead;

    return length;
}

ValueOrError<size_t> seek(u16_t minor, FileStream* filestream, size_t position, int mode) {
    auto mi_opt = mounted_filesystems.at(minor);
    ASSERT_F(mi_opt, "No filesystem is mapped to this minor number");
    auto& mi = *mi_opt;

    ASSERT_F(mi.filesystem == filestream->node()->filesystem(), "Using a filestream from a different filesystem");

    Ext2StreamDataStorage* file_data = (Ext2StreamDataStorage*)filestream->fs_data;

    const size_t fileSize = filestream->node()->f_size;

    u64_t oldPosition = file_data->position;

    switch(mode) {
        case SEEK_MODE_CUR:
            file_data->position += position;
            break;
        case SEEK_MODE_BEG:
            file_data->position = position;
            break;
        case SEEK_MODE_END:
            file_data->position = fileSize + position;
            break;
    }

    // If the seek position left us at a position which is not a multiple of block size
    // we need to read the current block into the buffer to remain compatible
    // with our read interpretation. However we only need to do that if
    // we actually moved into a different block.
    if((file_data->position & (mi.block_size - 1)) && (oldPosition >> (mi.block_size + 10)) != (file_data->position >> (mi.block_size + 10))) {
        Ext2VNodeDataStorage* vnode_data = (Ext2VNodeDataStorage*)filestream->node()->fs_data;
        u32_t block = get_inode_block(mi, vnode_data->inode, file_data->position >> (mi.superblock->blocks_size + 10));

        auto ret = DeviceFilesystem::instance()->block_read(mi.fs_file, mi.get_lba(block), mi.block_size >> 9, file_data->buffer.ptr());
        if(!ret) return ret.errno();
    }

    /// TODO: [13.01.2023] Use `u64_t`?
    return (size_t)oldPosition;
}
