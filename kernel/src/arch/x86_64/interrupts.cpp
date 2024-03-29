#include <arch/cpu.h>
#include <arch/interrupts.h>
#include <arch/x86_64/apic.h>
#include <arch/x86_64/cpu.h>
#include <arch/x86_64/int_handlers.h>
#include <defines.h>
#include <dmesg.h>
#include <stdlib.h>
#include <tasking/thread.hpp>
#include <trace.h>
#include <tasking/scheduler.hpp>

struct interrupt_descriptor {
    u16_t offset_015;
    u16_t segment;
    u8_t reserved1;
    u8_t conf;
    u16_t offset_1631;
    u32_t offset_3263;
    u32_t reserved2;
} PACKED;

NO_EXPORT interrupt_descriptor idt[256];

TEXT_FREE_AFTER_INIT void create_descriptor(interrupt_descriptor& entry, void (*handler)(), u8_t type, u8_t dpl) {
    entry.offset_015 = (u64_t)handler;
    entry.offset_1631 = (u64_t)handler >> 16;
    entry.offset_3263 = (u64_t)handler >> 32;

    entry.segment = 0x08;
    entry.reserved1 = 0;
    entry.reserved2 = 0;

    entry.conf = (type & 0xF) | ((dpl & 0x03) << 5) | (1 << 7);
}

struct idt_ptr {
    u16_t length;
    u64_t ptr;
} PACKED;

NO_EXPORT idt_ptr idtr;

struct handler_entry {
    void (*handler)();
    handler_entry* next;
};

NO_EXPORT handler_entry* handlers[256];
NO_EXPORT CPUState* (*_tsh)(CPUState* current_state);
NO_EXPORT syscall_arg_t (*_sh)(syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t);

extern "C" TEXT_FREE_AFTER_INIT void init_interrupts() {
    memset(handlers, 0, sizeof(handlers));
    _tsh = 0;

    for(int i = 0; i < 256; ++i) create_descriptor(idt[i], &int_ignore, 0xE, 0);

    create_descriptor(idt[0x00], &interrupt0x00, 0xE, 0);
    create_descriptor(idt[0x01], &interrupt0x01, 0xE, 0);
    create_descriptor(idt[0x02], &interrupt0x02, 0xE, 0);
    create_descriptor(idt[0x03], &interrupt0x03, 0xE, 0);
    create_descriptor(idt[0x04], &interrupt0x04, 0xE, 0);
    create_descriptor(idt[0x05], &interrupt0x05, 0xE, 0);
    create_descriptor(idt[0x06], &interrupt0x06, 0xE, 0);
    create_descriptor(idt[0x07], &interrupt0x07, 0xE, 0);
    create_descriptor(idt[0x08], &interrupt0x08, 0xE, 0);
    create_descriptor(idt[0x09], &interrupt0x09, 0xE, 0);
    create_descriptor(idt[0x0A], &interrupt0x0A, 0xE, 0);
    create_descriptor(idt[0x0B], &interrupt0x0B, 0xE, 0);
    create_descriptor(idt[0x0C], &interrupt0x0C, 0xE, 0);
    create_descriptor(idt[0x0D], &interrupt0x0D, 0xE, 0);
    create_descriptor(idt[0x0E], &interrupt0x0E, 0xE, 0);
    create_descriptor(idt[0x0F], &interrupt0x0F, 0xE, 0);
    create_descriptor(idt[0x10], &interrupt0x10, 0xE, 0);
    create_descriptor(idt[0x11], &interrupt0x11, 0xE, 0);
    create_descriptor(idt[0x12], &interrupt0x12, 0xE, 0);
    create_descriptor(idt[0x13], &interrupt0x13, 0xE, 0);
    create_descriptor(idt[0x14], &interrupt0x14, 0xE, 0);
    create_descriptor(idt[0x15], &interrupt0x15, 0xE, 0);
    create_descriptor(idt[0x16], &interrupt0x16, 0xE, 0);
    create_descriptor(idt[0x17], &interrupt0x17, 0xE, 0);
    create_descriptor(idt[0x18], &interrupt0x18, 0xE, 0);
    create_descriptor(idt[0x19], &interrupt0x19, 0xE, 0);
    create_descriptor(idt[0x1A], &interrupt0x1A, 0xE, 0);
    create_descriptor(idt[0x1B], &interrupt0x1B, 0xE, 0);
    create_descriptor(idt[0x1C], &interrupt0x1C, 0xE, 0);
    create_descriptor(idt[0x1D], &interrupt0x1D, 0xE, 0);
    create_descriptor(idt[0x1E], &interrupt0x1E, 0xE, 0);
    create_descriptor(idt[0x1F], &interrupt0x1F, 0xE, 0);

    create_descriptor(idt[0x20], &interrupt0x20, 0xE, 0);
    create_descriptor(idt[0x21], &interrupt0x21, 0xE, 0);
    create_descriptor(idt[0x22], &interrupt0x22, 0xE, 0);
    create_descriptor(idt[0x23], &interrupt0x23, 0xE, 0);
    create_descriptor(idt[0x24], &interrupt0x24, 0xE, 0);
    create_descriptor(idt[0x25], &interrupt0x25, 0xE, 0);
    create_descriptor(idt[0x26], &interrupt0x26, 0xE, 0);
    create_descriptor(idt[0x27], &interrupt0x27, 0xE, 0);
    create_descriptor(idt[0x28], &interrupt0x28, 0xE, 0);
    create_descriptor(idt[0x29], &interrupt0x29, 0xE, 0);
    create_descriptor(idt[0x2A], &interrupt0x2A, 0xE, 0);
    create_descriptor(idt[0x2B], &interrupt0x2B, 0xE, 0);
    create_descriptor(idt[0x2C], &interrupt0x2C, 0xE, 0);
    create_descriptor(idt[0x2D], &interrupt0x2D, 0xE, 0);
    create_descriptor(idt[0x2E], &interrupt0x2E, 0xE, 0);
    create_descriptor(idt[0x2F], &interrupt0x2F, 0xE, 0);

    create_descriptor(idt[0x8F], &interrupt0x8F, 0xE, 3);

    create_descriptor(idt[0xFE], &interrupt0xFE, 0xE, 0);

    idtr.length = sizeof(idt);
    idtr.ptr = (u64_t)idt;

    asm volatile("lidt %0; sti"
                 :
                 : "m"(idtr));
}

extern u32_t lapic_timer_ticks;

void trace_stack(void* base_pointer) {
    struct frame {
        frame* next_frame;
        u64_t ret_ip;
    };

    u64_t depth = 0;
    frame* f = (frame*)base_pointer;
    while(f != 0) {
        if(!kernel::Pager::active().getFlags((virtaddr_t)f).present) return;

        file_line_pair p = addr_to_line(f->ret_ip);
        dmesg("Stack frame 0x%016lx Ret: 0x%016lx %s:%d", f, f->ret_ip, p.name, p.line);
        f = f->next_frame;

        // Limit the stack trace to 32 frames
        if(depth++ > 32) {
            dmesg("...");
            break;
        }
    }
}

extern "C" void cpu_state_dump(CPUState* state) {
    u64_t cr0, cr2, cr3;
    asm volatile("mov %%cr0, %0" : "=a"(cr0));
    asm volatile("mov %%cr2, %0" : "=a"(cr2));
    asm volatile("mov %%cr3, %0" : "=a"(cr3));

    dmesg  ("\n+-------- CPU State -------+\n"
            "| rax = 0x%016lx |\n"
            "| rbx = 0x%016lx |\n"
            "| rcx = 0x%016lx |\n"
            "| rdx = 0x%016lx |\n"
            "| rsi = 0x%016lx |\n"
            "| rdi = 0x%016lx |\n"
            "| rbp = 0x%016lx |\n"
            "+--------------------------+\n"
            "| r8  = 0x%016lx |\n"
            "| r9  = 0x%016lx |\n"
            "| r10 = 0x%016lx |\n"
            "| r11 = 0x%016lx |\n"
            "| r12 = 0x%016lx |\n"
            "| r13 = 0x%016lx |\n"
            "| r14 = 0x%016lx |\n"
            "| r15 = 0x%016lx |\n"
            "+--------------------------+\n"
            "| rip = 0x%016lx |\n"
            "| cs  = 0x%016lx |\n"
            "| flg = 0x%016lx |\n"
            "| rsp = 0x%016lx |\n"
            "| ss  = 0x%016lx |\n"
            "+--------------------------+\n"
            "| cr0 = 0x%016lx |\n"
            "| cr2 = 0x%016lx |\n"
            "| cr3 = 0x%016lx |\n"
            "+--------------------------+\n"
            "| iec = 0x%016lx |\n"
            "| fs  = 0x%016lx |\n"
            "+--------------------------+\n"
            "state = 0x%016lx",
            state->rax, state->rbx, state->rcx, state->rdx, state->rsi, state->rdi, state->rbp, state->r8, state->r9, state->r10,
            state->r11, state->r12, state->r13, state->r14, state->r15, state->rip, state->cs, state->rflags, state->rsp, state->ss,
            cr0, cr2, cr3, state->err_code, state->fs, state);
}

static kernel::SpinLock s_except_log_lock;
NO_EXPORT void handle_exception(CPUState* state) {
    s_except_log_lock.lock();

    dmesg("Exception 0x%02x on core %d", state->int_num, current_core());
    cpu_state_dump(state);
    file_line_pair p = addr_to_line(state->rip);
    dmesg("Line: %s:%d", p.name, p.line);

    trace_stack((void*)state->rbp);

    s_except_log_lock.unlock();
    while(true) asm volatile("hlt");
}

extern "C" NO_EXPORT u64_t interrupt_handle(u64_t rsp) {
    CPUState* state = (CPUState*)rsp;
    u8_t intVec = state->int_num;

    switch(state->int_num) {
        case 0xFE: { // Task switch interrupt
            state = _tsh(state);

            u64_t timer_value = state->next_switch_time * lapic_timer_ticks / 1000;
            write_lapic(0x380, timer_value);
            break;
        } case 0x8F: { // Syscall interrupt
            kernel::Scheduler::pre_syscall(state);

            asm volatile("sti");
            syscall_arg_t rax = _sh(state->rax,
                state->rbx,
                state->rcx,
                state->rdx,
                state->rsi,
                state->rdi,
                state->r8);
            asm volatile("cli");

            state = kernel::Scheduler::post_syscall();
            state->rax = rax;
            break;
        } case 0x0E: { // Page Fault interrupt
            auto* thread = kernel::Thread::current();
            if(thread != 0) {
                u64_t cr2;
                asm volatile("mov %%cr2, %0" : "=a"(cr2));

                auto& proc = thread->parent();

                asm volatile("sti");
                bool status = proc.handle_page_fault(cr2, state->err_code);
                asm volatile("cli");

                set_kernel_stack(current_core(), (u64_t)state + sizeof(CPUState));
                if(status) return (u64_t)state;
                else handle_exception(state);
            }
            break;
        } case 0x07: {
            // Device not present (FPU), we need to restore the state
            // Clear CR0.TS bit for the FPU to become available.
            asm volatile("mov %%cr0, %%rax; and $0xFFFFFFFFFFFFFFF7, %%rax; mov %%rax, %%cr0" ::: "rax");
            kernel::Thread::current()->load_fpu_state();
            break;
        } default: { // Other interrupts
            if(state->int_num < 0x20) {
                // This is an exception interrupt
                handle_exception(state);
            } else {
                // Non exception interrupt, fire all registered handlers
                for(handler_entry* handler = handlers[state->int_num]; handler != 0; handler = handler->next)
                    handler->handler();
            }
            break;
        }
    }

    set_kernel_stack(current_core(), (u64_t)state + sizeof(CPUState));

    pic_eoi(intVec);
    return (u64_t)state;
}

extern "C" void register_handler(u8_t vector, void (*handler)()) {
    handler_entry* entry = new handler_entry();
    entry->next = 0;
    entry->handler = handler;

    if(handlers[vector] == 0)
        handlers[vector] = entry;
    else {
        handler_entry* last = handlers[vector];
        while(last->next != 0) last = last->next;
        last->next = entry;
    }
}

extern "C" void unregister_handler(u8_t vector, void (*handler)()) {
    handler_entry* prev = 0;
    for(handler_entry* en = handlers[vector]; en != 0; en = en->next) {
        if(en->handler == handler) {
            if(!prev)
                handlers[vector] = en->next;
            else
                prev->next = en->next;
            break;
        }
        prev = en;
    }
}

extern "C" void register_task_switch_handler(CPUState* (*handler)(CPUState* current_state)) {
    _tsh = handler;
}

extern "C" void register_syscall_handler(syscall_arg_t (*handler)(syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t)) {
    _sh = handler;
}

extern "C" void force_task_switch() {
    asm volatile("int $0xFE");
}
