#pragma once

#include <memory/page/unresolvedpage.hpp>
#include <fs/vnode.hpp>

namespace kernel {
    class FilePage : public UnresolvedPage {
        VNodePtr f_file;
        virtaddr_t f_start_addr;
        size_t f_offset;
        bool f_shared;
        bool f_write;
        bool f_execute;

    public:
        FilePage(const VNodePtr& ptr, virtaddr_t start_addr, size_t offset, bool shared, bool write, bool execute);
        virtual ~FilePage();

        const VNodePtr& file() const {
            return f_file;
        }

        virtaddr_t start_addr() const {
            return f_start_addr;
        }

        size_t offset() const {
            return f_offset;
        }

        bool shared() const {
            return f_shared;
        }

        bool writable() const {
            return f_write;
        }

        PageFlags flags() const {
            return PageFlags(1, f_write, 1, f_execute, 0, 0);
        }

        virtual PhysicalPage resolve(virtaddr_t addr);
    };
}
