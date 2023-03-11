#include <fs/filesystem.hpp>
#include <fs/vnode.hpp>
#include <memory/page/resolved_memory.hpp>
#include <memory/page/resolvable_memory.hpp>

using namespace kernel;

ValueOrError<void> Filesystem::umount() {
    return ENOTSUP;
}

ValueOrError<VNodePtr> Filesystem::get_file(VNodePtr, const char*, FilesystemFlags) {
    return ENOTSUP;
}

ValueOrError<std::List<VNodePtr>> Filesystem::get_files(VNodePtr, FilesystemFlags) {
    return ENOTSUP;
}

ValueOrError<VNodePtr> Filesystem::resolve_link(VNodePtr, int) {
    return ENOTSUP;
}

ValueOrError<void> Filesystem::open(FileStream*, int) {
    return ENOTSUP;
}

ValueOrError<void> Filesystem::close(FileStream*) {
    return ENOTSUP;
}

ValueOrError<size_t> Filesystem::read(FileStream*, void*, size_t) {
    return ENOTSUP;
}

ValueOrError<size_t> Filesystem::write(FileStream*, const void*, size_t) {
    return ENOTSUP;
}

ValueOrError<size_t> Filesystem::seek(FileStream*, size_t, int) {
    return ENOTSUP;
}

std::Optional<ResolvedMemoryEntry> Filesystem::resolve_mapping(const ResolvableMemoryEntry&, virtaddr_t) {
    return { };
}

void Filesystem::sync_mapping(const ResolvedMemoryEntry&) {

}

ValueOrError<int> Filesystem::ioctl(FileStream*, u64_t, void*) {
    return ENOTSUP;
}

// ValueOrError<VNodePtr> Filesystem::link(VNodePtr, const char*, VNodePtr, bool) {
//     return ENOTSUP;
// }

ValueOrError<VNodePtr> Filesystem::mkdir(VNodePtr, const char*) {
    return ENOTSUP;
}

ValueOrError<VNodePtr> Filesystem::symlink(VNodePtr, const char*, const char*) {
    return ENOTSUP;
}

std::Optional<ResolvedMemoryEntry> Filesystem::mapping_resolve_helper(const ResolvableMemoryEntry& mapping, virtaddr_t addr, bool (*readBytes)(const VNodePtr& file, size_t offset, void* buffer, size_t length)) {
    size_t mappingOffset = (addr - mapping.f_start) & ~0xFFF;
    size_t fileOffset = mappingOffset + mapping.f_file_offset;

    auto vnode = mapping.f_file;

    auto pageOpt = vnode->f_shared_pages.at(fileOffset);
    if(pageOpt) {
        ResolvedMemoryEntry entry(*pageOpt);
        if(mapping.f_shared) {
            // We only set the entry as file backed if it is shared.
            // Private entries don't get to be file backed since they
            // don't affect the file contents.
            entry.f_file = mapping.f_file;
            entry.f_file_offset = fileOffset;
        }

        entry.f_shared = mapping.f_shared;
        entry.f_page_flags.executable = mapping.f_executable;
        entry.f_page_flags.writable = mapping.f_writable;
        if(!mapping.f_shared && mapping.f_writable) {
            entry.f_copy_on_write = true;
            entry.f_page_flags.writable = false;
        } else {
            entry.f_copy_on_write = false;
        }
        return entry;
    }

    PhysicalPage page;
    page.flags() = PageFlags(true, true, false, false, true, false);

    auto& pager = Pager::active();
    pager.lock();
    virtaddr_t ptr = pager.kmap(page.addr(), 1, page.flags());

    // Always read as much as possible into the page. The contents are
    // trimmed later to the desired length.
    ssize_t lengthLeft = vnode->size() - fileOffset;
    if(lengthLeft > 0) {
        if(lengthLeft > 4096)
            lengthLeft = 4096;
        if(!readBytes(vnode, fileOffset, (void*)ptr, lengthLeft))
            return { };
        if(lengthLeft < 4096)
            memset((u8_t*)ptr + lengthLeft, 0, 4096 - lengthLeft);
    } else {
        memset((u8_t*)ptr, 0, 4096);
    }

    pager.unmap(ptr, 1);
    pager.unlock();

    if(mapping.f_shared)
        vnode->f_shared_pages.insert({ fileOffset, page });

    ResolvedMemoryEntry entry(page);
    if(mapping.f_shared) {
        entry.f_file = mapping.f_file;
        entry.f_file_offset = fileOffset;
    }

    entry.f_shared = mapping.f_shared;
    entry.f_page_flags.writable = mapping.f_writable;
    entry.f_page_flags.executable = mapping.f_executable;
    // If the entry is not shared then we haven't put in into the vnodes pages
    // and we can freely write to it.
    entry.f_copy_on_write = false;
    return entry;
}


ValueOrError<void> Filesystem::stat(VNodePtr, mieros_stat*) {
    return ENOTSUP;
}

