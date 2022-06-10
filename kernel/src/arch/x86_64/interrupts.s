.section .text
.extern interrupt_handle

.macro int_c num
.global interrupt\num
interrupt\num\():
    pushq \num
    jmp handle_int
.endm

.macro int_nc num
.global interrupt\num
interrupt\num\():
    pushq 0
    pushq \num
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

int_nc 0x20
int_nc 0x21
int_nc 0x22
int_nc 0x23
int_nc 0x24
int_nc 0x25
int_nc 0x26
int_nc 0x27
int_nc 0x28
int_nc 0x29
int_nc 0x2A
int_nc 0x2B
int_nc 0x2C
int_nc 0x2D
int_nc 0x2E
int_nc 0x2F

int_nc 0x8F

int_nc 0xFE

handle_int:
    push rbp
    
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8

    push rdi
    push rsi
    
    push rdx
    push rcx
    push rbx
    push rax

    mov rax, cr3
    push rax

    sub rsp, 8

    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov rdi, rsp
    call interrupt_handle
    mov rsp, rax

int_ret:
    mov ax, [rsp + 160]
    cmp ax, 0x08
    je .kernel
.user:
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    jmp .post
.kernel:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
.post:
    add rsp, 8

    pop rax
    mov cr3, rax

    pop rax
    pop rbx
    pop rcx
    pop rdx

    pop rsi
    pop rdi

    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    pop rbp

    add rsp, 16

int_ignore:
    iretq

.global int_ignore
