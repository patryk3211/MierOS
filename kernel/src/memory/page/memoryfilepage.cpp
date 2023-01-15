#include <memory/page/memoryfilepage.hpp>

#include <assert.h>

using namespace kernel;

MemoryFilePage::MemoryFilePage(const VNodePtr& file, const PhysicalPage& page, size_t offset)
    : f_file(file), f_page(page), f_offset(offset) {
    f_dirty = false;
    f_copy_on_write = false;
    ASSERT_F(!(offset & 0xFFF), "MemoryFilePage offset must be 4096 byte aligned");
}

MemoryFilePage::~MemoryFilePage() {
    f_page.unref();
}
