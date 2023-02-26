#pragma once

#include <fs/vnode.hpp>
#include <memory/page/resolved_memory.hpp>

namespace kernel {
    struct ResolvableMemoryEntry {
        bool f_shared;
        bool f_writable;
        bool f_executable;

        virtaddr_t f_start;
        size_t f_length;

        VNodePtr f_file;
        size_t f_file_offset;

        ResolvableMemoryEntry() = default;
        virtual ~ResolvableMemoryEntry() = default;

        virtual std::Optional<ResolvedMemoryEntry> resolve(virtaddr_t addr);
    };
}
