#include <memory/page/anonpage.hpp>

using namespace kernel;

AnonymousPage::AnonymousPage(const PageFlags& flags)
    : f_flags(flags) {

}

PhysicalPage AnonymousPage::resolve(virtaddr_t) {
    PhysicalPage page;
    page.flags() = f_flags;
    return page;
}

SharedAnonymousPage::SharedAnonymousPage(const PageFlags& flags)
    : AnonymousPage(flags), f_page(nullptr) {

}

PhysicalPage SharedAnonymousPage::resolve(virtaddr_t addr) {
    if(!f_page) {
        auto page = AnonymousPage::resolve(addr);
        f_page = page;
    }
    return f_page;
}
