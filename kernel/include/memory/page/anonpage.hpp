#pragma once

#include <memory/page/unresolvedpage.hpp>
#include <memory/virtual.hpp>

namespace kernel {
    class AnonymousPage : public UnresolvedPage {
        PageFlags f_flags;
    public:
        AnonymousPage(const PageFlags& flags);
        virtual ~AnonymousPage() = default;

        virtual PhysicalPage resolve(virtaddr_t addr);
    };

    class SharedAnonymousPage : public AnonymousPage {
        PhysicalPage f_page;
    public:
        SharedAnonymousPage(const PageFlags& flags);
        virtual ~SharedAnonymousPage() = default;

        virtual PhysicalPage resolve(virtaddr_t addr);
    };
}
