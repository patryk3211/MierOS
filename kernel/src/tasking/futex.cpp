#include <stdatomic.h>
#include <tasking/futex.hpp>
#include <arch/interrupts.h>

using namespace kernel;

std::List<Futex::FutexEntry> Futex::s_futexes;

Futex::Futex(u32_t* addr)
    : f_addr(addr) {
}

Futex::~Futex() {

}

ValueOrError<void> Futex::wait(u32_t value) {
    unsigned int expected = value;
    if(!atomic_compare_exchange_strong((atomic_uint*)f_addr, &expected, value + 1))
        return EAGAIN;

    // TODO: [14.02.2023] We need to make sure that this won't
    // cause a deadlock and in case it does return an error.

    f_sleep.sleep();
    return { };
}

ValueOrError<u32_t> Futex::wake(u32_t value) {
    return static_cast<u32_t>(*f_sleep.wakeup(value));
}

bool Futex::empty() const {
    return f_sleep.empty();
}

Futex* Futex::get(u32_t* addr) {
    auto& pager = Pager::active();
    pager.lock();
    physaddr_t paddr = pager.getPhysicalAddress((virtaddr_t)addr);
    pager.unlock();

    for(auto& entry : s_futexes) {
        if(entry.f_addr == paddr)
            return entry.f_futex;
    }

    return 0;
}

Futex* Futex::make(u32_t* addr) {
    Futex* futex = get(addr);
    if(!futex) {
        auto& pager = Pager::active();
        pager.lock();
        physaddr_t paddr = pager.getPhysicalAddress((virtaddr_t)addr);
        pager.unlock();

        futex = new Futex(addr);
        s_futexes.push_back({ paddr, futex });
    }
    return futex;
}

