#include <memory/page/filepage.hpp>

using namespace kernel;

FilePage::FilePage(const VNodePtr& ptr, virtaddr_t start_addr, size_t offset, bool shared, bool write, bool execute)
    : f_file(ptr), f_start_addr(start_addr), f_offset(offset), f_shared(shared), f_write(write), f_execute(execute) {

}

FilePage::~FilePage() {

}

PhysicalPage FilePage::resolve(virtaddr_t addr) {
    return f_file->filesystem()->resolve_mapping(*this, addr);
}
