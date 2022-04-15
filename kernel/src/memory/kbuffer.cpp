#include <locking/locker.hpp>
#include <memory/kbuffer.hpp>
#include <memory/virtual.hpp>

using namespace kernel;

KBuffer::KBuffer(size_t size) {
    ref_count = new std::Atomic<u32_t>(1);

    if(size == 0) {
        raw_ptr = 0;
        page_size = 0;
        return;
    }

    page_size = (size >> 12) + ((size & 0xFFF) == 0 ? 0 : 1);

    auto& pager = Pager::active();
    Locker locker(pager);

    raw_ptr = (void*)pager.kalloc(page_size);
}

KBuffer::~KBuffer() {
    if(ref_count != 0 && ref_count->fetch_sub(1) == 1 && raw_ptr != 0) {
        auto& pager = Pager::active();
        Locker locker(pager);

        pager.free((virtaddr_t)raw_ptr, page_size);
    }
}

KBuffer::KBuffer(KBuffer&& other) {
    raw_ptr = other.raw_ptr;
    ref_count = other.ref_count;
    page_size = other.page_size;

    other.raw_ptr = 0;
    other.ref_count = 0;
}

KBuffer& KBuffer::operator=(KBuffer&& other) {
    if(ref_count != 0 && ref_count->fetch_sub(1) == 1 && raw_ptr != 0) {
        auto& pager = Pager::active();
        Locker locker(pager);

        pager.free((virtaddr_t)raw_ptr, page_size);
    }

    raw_ptr = other.raw_ptr;
    ref_count = other.ref_count;
    page_size = other.page_size;

    other.raw_ptr = 0;
    other.ref_count = 0;

    return *this;
}

KBuffer::KBuffer(const KBuffer& other) {
    raw_ptr = other.raw_ptr;
    ref_count = other.ref_count;
    page_size = other.page_size;

    ref_count->fetch_add(1);
}

KBuffer& KBuffer::operator=(const KBuffer& other) {
    if(ref_count != 0 && ref_count->fetch_sub(1) == 1 && raw_ptr != 0) {
        auto& pager = Pager::active();
        Locker locker(pager);

        pager.free((virtaddr_t)raw_ptr, page_size);
    }

    raw_ptr = other.raw_ptr;
    ref_count = other.ref_count;
    page_size = other.page_size;

    ref_count->fetch_add(1);

    return *this;
}

void KBuffer::resize(size_t new_size) {
    auto& pager = Pager::active();
    Locker locker(pager);

    size_t old_size = page_size;
    void* old_ptr = raw_ptr;

    page_size = (new_size >> 12) + ((new_size & 0xFFF) == 0 ? 0 : 1);
    raw_ptr = (void*)pager.kalloc(page_size);

    if(old_ptr != 0) {
        size_t copy_size = old_size > page_size ? page_size : old_size;
        memcpy(raw_ptr, old_ptr, copy_size * 4096);

        pager.free((virtaddr_t)old_ptr, old_size);
    }
}
