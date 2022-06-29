#pragma once

#include <fs/vnode.hpp>
#include <memory/ppage.hpp>

namespace kernel {
    struct MemoryFilePage {
        VNodePtr f_file;
        PhysicalPage f_page;
        size_t f_offset;
        bool f_copy_on_write;
        bool f_dirty;

        MemoryFilePage(const VNodePtr& file, const PhysicalPage& page, size_t offset);
        ~MemoryFilePage();
    };
}
