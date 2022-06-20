#include <memory/page/unresolvedpage.hpp>

using namespace kernel;

PhysicalPage UnresolvedPage::resolve(virtaddr_t) {
    ASSERT_NOT_REACHED("Unresolved page children have to override the resolve function");
    return nullptr;
}
