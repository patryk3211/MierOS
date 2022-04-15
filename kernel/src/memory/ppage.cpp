#include <memory/physical.h>
#include <memory/ppage.hpp>

#include <assert.h>

using namespace kernel;

PhysicalPage::PhysicalPage() {
    _addr = palloc(1);
}

PhysicalPage::~PhysicalPage() {
    if(_ref_count.load() == 0) pfree(_addr, 1);
}

void PhysicalPage::ref() {
    _ref_count.fetch_add(1);
}

void PhysicalPage::unref() {
    u32_t c = _ref_count.fetch_sub(1);
    ASSERT_F(c > 0, "Unreferencing a nonreferenced physical page");
}

u32_t PhysicalPage::ref_count() {
    return _ref_count.load();
}
