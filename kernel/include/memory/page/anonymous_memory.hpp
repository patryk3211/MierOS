#pragma once

#include <memory/page/resolvable_memory.hpp>

namespace kernel {
    struct AnonymousMemory : public ResolvableMemoryEntry {
        virtual std::Optional<ResolvedMemoryEntry> resolve(virtaddr_t) override;
    };

    struct SharedAnonymousMemory : public ResolvableMemoryEntry {
        std::Vector<std::Optional<PhysicalPage>> f_pages;

        SharedAnonymousMemory(virtaddr_t start, size_t length);
        virtual ~SharedAnonymousMemory() = default;

        virtual std::Optional<ResolvedMemoryEntry> resolve(virtaddr_t) override;
    };
};

