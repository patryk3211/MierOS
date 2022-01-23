#include <tasking/scheduler.hpp>

using namespace kernel;

ThreadQueue Scheduler::wait_queue = ThreadQueue();
ThreadQueue Scheduler::runnable_queue = ThreadQueue();
Scheduler* Scheduler::schedulers;

Scheduler::Scheduler() : idle_stack(4096) {
    current_thread = 0;
    idle_ksp = (CPUState*)((virtaddr_t)idle_stack.ptr()+4096-sizeof(CPUState));

    idle_ksp->rip = (u64_t)&idle;
    idle_ksp->rflags = 0x202;
}

Scheduler::~Scheduler() {

}

void Scheduler::init(int core_count) {
    schedulers = new Scheduler[core_count];
    for(int i = 0; i < core_count; ++i) schedulers[i].core_id = i;
}

void Scheduler::idle() {
    while(true) asm volatile("hlt");
}

Scheduler& Scheduler::scheduler(int core) {
    return schedulers[core];
}
