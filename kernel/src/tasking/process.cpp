#include <tasking/process.hpp>

using namespace kernel;

Process::Process(virtaddr_t entry_point, Pager* pager) : _pager(pager) {
    threads.push_back(new Thread(entry_point, true, *this));
}

Process* Process::construct_kernel_process(virtaddr_t entry_point) {
    return new Process(entry_point, &Pager::kernel());
}