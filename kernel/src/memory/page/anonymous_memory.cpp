#include <memory/page/anonymous_memory.hpp>

using namespace kernel;

std::Optional<ResolvedMemoryEntry> AnonymousMemory::resolve(virtaddr_t) {
    ResolvedMemoryEntry entry(PhysicalPage {});

    entry.f_shared = false;
    entry.f_copy_on_write = false;

    auto& flags = entry.f_page.flags();
    flags.present = true;
    flags.writable = f_writable;
    flags.user_accesible = true;
    flags.executable = f_executable;

    entry.f_page_flags = flags;

    return entry;
}

SharedAnonymousMemory::SharedAnonymousMemory(virtaddr_t start, size_t length)
    : f_pages(length, { }) {
    f_start = start;
    f_length = length;
}

std::Optional<ResolvedMemoryEntry> SharedAnonymousMemory::resolve(virtaddr_t addr) {
    if(addr < f_start || addr >= (f_start + (f_length << 12)))
        return { };

    size_t pageOffset = (addr - f_start) >> 12;
    
    if(!f_pages[pageOffset]) {
        PhysicalPage page;

        auto& flags = page.flags();
        flags.present = true;
        flags.writable = f_writable;
        flags.user_accesible = true;
        flags.executable = f_executable;

        f_pages[pageOffset] = page;
    }

    ResolvedMemoryEntry entry(*f_pages[pageOffset]);
    entry.f_copy_on_write = false;

    return entry;
}

