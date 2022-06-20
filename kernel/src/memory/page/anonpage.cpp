#include <memory/page/anonpage.hpp>

using namespace kernel;

AnonymousPage::AnonymousPage(const PageFlags& flags)
    : flags(flags) {

}

PhysicalPage AnonymousPage::resolve(virtaddr_t) {
    PhysicalPage page;
    page.flags() = flags;
    return page;
}
