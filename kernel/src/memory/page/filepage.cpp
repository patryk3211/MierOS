#include <memory/page/filepage.hpp>

#include <assert.h>

using namespace kernel;

FilePage::FilePage(const VNodePtr& ptr, virtaddr_t start_addr, size_t offset, bool shared, bool write, bool execute)
    : f_file(ptr), f_start_addr(start_addr), f_offset(offset), f_shared(shared), f_write(write), f_execute(execute) {
    ASSERT_F(!(offset & 0xFFF), "FilePage offset must be 4096 byte aligned");
}

FilePage::~FilePage() {

}

PhysicalPage FilePage::resolve(virtaddr_t addr) {
    return f_file->filesystem()->resolve_mapping(*this, addr);
}
