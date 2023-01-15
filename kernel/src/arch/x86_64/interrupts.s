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

    # IA32_FS_BASE Model Specific Register
    mov ecx, 0xC0000100
    rdmsr

    mov ebx, edx
    shl rbx, 32
    mov ebx, eax

    push rbx

    # Make space for `next_switch_time`
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
    # REMINDER! When updating CPU State remember to change this offset
    # to point to the new location of the CS field
    mov ax, [rsp + 168]
    add ax, 8 # 0x08 -> 0x10, 0x1B -> 0x23

    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    # Skip past the `next_switch_time`
    add rsp, 8

    # Read fs base
    pop rbx
    # Lower 32 bits into eax
    mov eax, ebx
    # Upper 32 bits into edx
    shr rbx, 32
    mov edx, ebx
    # IA32_FS_BASE Model Specific Register
    mov ecx, 0xC0000100
    wrmsr

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

    # Skip past `int_num` and `err_code`
    add rsp, 16

int_ignore:
    iretq

.global int_ignore
