#include <memory/physical.h>
#include <memory/ppage.hpp>

#include <assert.h>

using namespace kernel;

PhysicalPage::PhysicalPage() {
    data = new PhysicalPage::Data();
    data->f_addr = palloc(1);
    data->f_ref_count.store(0);
    data->f_obj_ref_count.store(1);
}

PhysicalPage::~PhysicalPage() {
    if(data == 0) return;

    if(data->f_ref_count.load() == 0 && data->f_addr != 0) {
        pfree(data->f_addr, 1);
        data->f_addr = 0;
    }
    if(data->f_obj_ref_count.fetch_sub(1) == 1) delete data;
}

PhysicalPage::PhysicalPage(const PhysicalPage& other) : data(other.data) {
    data->f_obj_ref_count.fetch_add(1);
}

PhysicalPage::PhysicalPage(PhysicalPage&& other) : data(other.leak_ptr()) { }

PhysicalPage& PhysicalPage::operator=(const PhysicalPage& other) {
    this->~PhysicalPage();

    data = other.data;
    if(data != 0) data->f_obj_ref_count.fetch_add(1);

    return *this;
}

PhysicalPage& PhysicalPage::operator=(PhysicalPage&& other) {
    this->~PhysicalPage();

    data = other.leak_ptr();

    return *this;
}

void PhysicalPage::ref() {
    data->f_ref_count.fetch_add(1);
}

void PhysicalPage::unref() {
    u32_t c = data->f_ref_count.fetch_sub(1);
    ASSERT_F(c > 0, "Unreferencing a nonreferenced physical page");
}

u32_t PhysicalPage::ref_count() {
    return data->f_ref_count.load();
}

physaddr_t PhysicalPage::addr() {
    return data->f_addr;
}

PhysicalPage::Data* PhysicalPage::leak_ptr() {
    Data* ptr = data;
    data = 0;
    return ptr;
}
