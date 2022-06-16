[bits 16]
[org 0x1000]
_start:
    mov eax, 1
    mov [0x2000], eax

    lgdt [gdtr]

    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8) | (1 << 11)
    wrmsr

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov eax, [0x2008]
    mov cr3, eax

    mov eax, cr0
    and eax, ~((1 << 30) | (1 << 29))
    or eax, 0x80010001
    mov cr0, eax

    jmp 0x08:start_64

init_gdt:
    dq 0
    dq 0x00AF9A000000FFFF
    dq 0x00AF92000000FFFF

gdtr:
    dw 0x0018
    dq init_gdt

[bits 64]
start_64:
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov fs, ax
    mov es, ax
    mov gs, ax

    mov rsp, 0x3000

    mov rcx, [0x2010]
    jmp rcx
