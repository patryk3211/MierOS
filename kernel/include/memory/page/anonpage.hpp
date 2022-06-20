#pragma once

#include <memory/page/unresolvedpage.hpp>
#include <memory/virtual.hpp>

namespace kernel {
    class AnonymousPage : public UnresolvedPage {
        PageFlags flags;
    public:
        AnonymousPage(const PageFlags& flags);
        ~AnonymousPage() = default;

        virtual PhysicalPage resolve(virtaddr_t addr);
    };
}
