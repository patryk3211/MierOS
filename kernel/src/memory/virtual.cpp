#include <memory/virtual.hpp>
#include <sections.h>
#include <stdlib.h>

using namespace kernel;

union PageStructuresEntry {
    struct {
        u64_t present:1;
        u64_t write:1;
        u64_t user:1;
        u64_t write_through:1;
        u64_t cache_disabled:1;
        u64_t accesed:1;
        u64_t dirty:1;
        u64_t pat_ps:1;
        u64_t global:1;
        u64_t available:3;
        u64_t address:40;
        u64_t available2:7;
        u64_t protection_key:4;
        u64_t execute_disable:1;
    } structured;
    u64_t raw;
};

FREE_AFTER_INIT PageStructuresEntry init_pml4[512] __attribute__((aligned(4096)));
FREE_AFTER_INIT PageStructuresEntry init_pdpt[512] __attribute__((aligned(4096)));
FREE_AFTER_INIT PageStructuresEntry init_pd[512] __attribute__((aligned(4096)));
FREE_AFTER_INIT PageStructuresEntry init_pt[512] __attribute__((aligned(4096)));

Pager::Pager() {

}

Pager::~Pager() {

}

extern "C" void* _kernel_end;
void Pager::init(physaddr_t kernel_base_p, virtaddr_t kernel_base_v) {
    memset(init_pml4, 0, sizeof(PageStructuresEntry));
    memset(init_pdpt, 0, sizeof(PageStructuresEntry));
    memset(init_pd, 0, sizeof(PageStructuresEntry));
    memset(init_pt, 0, sizeof(PageStructuresEntry));

    /*constexpr int pt_addr = ((0xFFFFFFFF80000000) >> 12) & 0x1FF;
    constexpr int pd_addr = ((0xFFFFFFFF80000000) >> 21) & 0x1FF;
    constexpr int pdpt_addr = ((0xFFFFFFFF80000000) >> 30) & 0x1FF;
    constexpr int pml4_addr = ((0xFFFFFFFF80000000) >> 39) & 0x1FF;*/

    /*init_pml4[511].raw = ((virtaddr_t)&init_pdpt - kernel_base_v + kernel_base_p) | 0x3;
    init_pdpt[510].raw = ((virtaddr_t)&init_pd - kernel_base_v + kernel_base_p) | 0x3;
    init_pd[0].raw = ((virtaddr_t)&init_pt - kernel_base_v + kernel_base_p) | 0x3;
    
    int page_count = */
}
