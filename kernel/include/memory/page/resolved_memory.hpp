#pragma once

#include <fs/vnode.hpp>
#include <memory/ppage.hpp>

namespace kernel {
    struct ResolvedMemoryEntry {
        PhysicalPage f_page;
        PageFlags f_page_flags;
        bool f_copy_on_write;
        bool f_shared;

        // File backed page
        VNodePtr f_file;
        size_t f_file_offset;

        ResolvedMemoryEntry(const PhysicalPage& page)
            : f_page(page)
            , f_page_flags(page.flags())
            , f_file(nullptr) {
            f_page_flags.user_accesible = true;
            f_page_flags.global = false;
        }
        ~ResolvedMemoryEntry() = default;
    };
}

