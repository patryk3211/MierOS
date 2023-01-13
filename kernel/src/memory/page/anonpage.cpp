#include <memory/page/anonpage.hpp>
#include <memory/virtual.hpp>

using namespace kernel;

AnonymousPage::AnonymousPage(const PageFlags& flags, bool zero)
    : f_flags(flags), f_zero(zero) {

}

PhysicalPage AnonymousPage::resolve(virtaddr_t) {
    PhysicalPage page;
    page.flags() = f_flags;


    if(f_zero) {
        // Zero fill this page
        auto& pager = Pager::active();
        pager.lock();
        virtaddr_t addr = pager.kmap(page.addr(), 1, PageFlags(true, true));

        memset((void*)addr, 0, 4096);

        pager.unmap(addr, 1);
        pager.unlock();
    }

    return page;
}

SharedAnonymousPage::SharedAnonymousPage(const PageFlags& flags, bool zero)
    : AnonymousPage(flags, zero), f_page(nullptr) {

}

PhysicalPage SharedAnonymousPage::resolve(virtaddr_t addr) {
    if(!f_page) {
        auto page = AnonymousPage::resolve(addr);
        f_page = page;
    }
    return f_page;
}
