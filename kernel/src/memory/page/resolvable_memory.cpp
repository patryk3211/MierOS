#include <memory/page/resolvable_memory.hpp>

using namespace kernel;

std::Optional<ResolvedMemoryEntry> ResolvableMemoryEntry::resolve(virtaddr_t addr) {
    return f_file ?
                f_file->filesystem()->resolve_mapping(*this, addr) :
                std::Optional<ResolvedMemoryEntry>();
}

