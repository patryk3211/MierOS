#include <memory/kbuffer.hpp>
#include <memory/virtual.hpp>

using namespace kernel;

KBuffer::KBuffer(size_t size) {
    page_size = (size >> 12) + ((size & 0xFFF) == 0 ? 0 : 1);
    raw_ptr = (void*)Pager::active().kalloc(page_size);
    ref_count = new std::Atomic<u32_t>(1);
}

KBuffer::~KBuffer() {
    if(ref_count != 0 && ref_count->fetch_sub(1) == 1) {
        Pager::active().free((virtaddr_t)raw_ptr, page_size);
    }
}

KBuffer::KBuffer(KBuffer&& other)  {
    raw_ptr = other.raw_ptr;
    ref_count = other.ref_count;
    page_size = other.page_size;

    other.raw_ptr = 0;
    other.ref_count = 0;
}

KBuffer& KBuffer::operator=(KBuffer&& other) {
    if(ref_count != 0 && ref_count->fetch_sub(1) == 1) {
        Pager::active().free((virtaddr_t)raw_ptr, page_size);
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
    if(ref_count != 0 && ref_count->fetch_sub(1) == 1) {
        Pager::active().free((virtaddr_t)raw_ptr, page_size);
    }

    raw_ptr = other.raw_ptr;
    ref_count = other.ref_count;
    page_size = other.page_size;

    ref_count->fetch_add(1);

    return *this;
}
