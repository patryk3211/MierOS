.section .text
.extern interrupt_handle

.macro int_c num
.global interrupt\num
interrupt\num\():
    pushq $\num
    jmp handle_int
.endm

.macro int_nc num
.global interrupt\num
interrupt\num\():
    pushq $0
    pushq $\num
    jmp handle_int
.endm

int_nc 0x00
int_nc 0x01
int_nc 0x02
int_nc 0x03
int_nc 0x04
int_nc 0x05
int_nc 0x06
int_nc 0x07
int_c  0x08
int_nc 0x09
int_c  0x0A
int_c  0x0B
int_c  0x0C
int_c  0x0D
int_c  0x0E
int_nc 0x0F
int_nc 0x10
int_c  0x11
int_nc 0x12
int_nc 0x13
int_nc 0x14
int_c  0x15
int_nc 0x16
int_nc 0x17
int_nc 0x18
int_nc 0x19
int_nc 0x1A
int_nc 0x1B
int_nc 0x1C
int_c  0x1D
int_c  0x1E
int_nc 0x1F

int_nc 0xFE

handle_int:
    pushq %rbp
    
    pushq %r15
    pushq %r14
    pushq %r13
    pushq %r12
    pushq %r11
    pushq %r10
    pushq %r9
    pushq %r8

    pushq %rdi
    pushq %rsi
    
    pushq %rdx
    pushq %rcx
    pushq %rbx
    pushq %rax

    mov %cr3, %rax
    pushq %rax

    mov %rsp, %rdi
    call interrupt_handle
    mov %rax, %rsp

int_ret:
    popq %rax
    mov %rax, %cr3

    popq %rax
    popq %rbx
    popq %rcx
    popq %rdx

    popq %rsi
    popq %rdi

    popq %r8
    popq %r9
    popq %r10
    popq %r11
    popq %r12
    popq %r13
    popq %r14
    popq %r15

    popq %rbp

    add $16, %rsp

int_ignore:
    iret

.global int_ignore
