#include <arch/cpu.h>
#include <defines.h>
#include <types.h>

#include <arch/x86_64/acpi.h>
#include <arch/x86_64/cpu.h>

u64_t rdmsr(u32_t msr) {
    u32_t a, d;
    asm volatile("rdmsr"
                 : "=a"(a), "=d"(d)
                 : "c"(msr));
    return a | ((u64_t)d << 32);
}

void wrmsr(u32_t msr, u64_t value) {
    u32_t a = value & 0xFFFFFFFF;
    u32_t d = value >> 32;
    asm volatile("wrmsr"
                 :
                 : "a"(a), "d"(d), "c"(msr));
}

#define EFER_REG 0xC0000080

TEXT_FREE_AFTER_INIT void early_init_cpu() {
    u64_t EFER = rdmsr(EFER_REG);
    EFER |= (1 << 11); // Enable Execute Disable Bit
    wrmsr(EFER_REG, EFER);
}

void task_switched() {
    // We need to set the task switched bit for SSE
    // instructions to cause an exception and for us
    // to be able to restore the FPU state
    asm volatile("mov %%cr0, %%rax; or $0x08, %%rax; mov %%rax, %%cr0" ::: "rax");
}

void enter_critical() {
    asm volatile("cli");
}

void leave_critical() {
    asm volatile("sti");
}

