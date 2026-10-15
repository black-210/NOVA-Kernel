; NOVA OS - Next-Generation Microkernel Operating System
; File: boot/boot.asm
; Description: Multiboot entry point, x86_64 Long Mode transition, and early page tables.
; License: Apache-2.0

BITS 32

; Multiboot 1 constants
MB_MAGIC    equ 0x1BADB002
MB_FLAGS    equ 0x00000003 ; ALIGN | MEMINFO
MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS)

section .multiboot
align 4
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM

section .bss
align 4096
boot_pml4:
    resb 4096
boot_pdpt:
    resb 4096
boot_pd:
    resb 4096

align 16
stack_bottom:
    resb 16384 ; 16KB early stack
stack_top:

section .rodata
; 64-bit Global Descriptor Table
gdt64:
    dq 0 ; Null descriptor
.code: equ $ - gdt64
    ; Kernel 64-bit Code Segment: Present, Ring 0, Executable, Read/Write, 64-bit Long Mode
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53)
.data: equ $ - gdt64
    ; Kernel Data Segment: Present, Ring 0, Read/Write
    dq (1<<41) | (1<<44) | (1<<47)
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

section .text
global _start
extern kmain

_start:
    cli
    cld
    mov esp, stack_top

    ; Check if CPU supports CPUID
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    xor eax, ecx
    jz .no_cpuid

    ; Check for extended CPUID functions
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode

    ; Check for x86_64 Long Mode
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode

    ; Setup early identity page tables (First 1GB mapped)
    ; PML4[0] -> PDPT
    mov eax, boot_pdpt
    or eax, 0x03 ; Present | Writable
    mov [boot_pml4], eax

    ; PDPT[0] -> PD
    mov eax, boot_pd
    or eax, 0x03 ; Present | Writable
    mov [boot_pdpt], eax

    ; Map 512 entries in PD to 2MB huge pages (512 * 2MB = 1GB identity mapped)
    mov ecx, 0
.map_pd:
    mov eax, 0x200000 ; 2MB
    mul ecx
    or eax, 0x83      ; Present | Writable | Huge (2MB)
    mov [boot_pd + ecx * 8], eax
    inc ecx
    cmp ecx, 512
    jne .map_pd

    ; Load CR3 with boot PML4
    mov eax, boot_pml4
    mov cr3, eax

    ; Enable PAE (Physical Address Extension) in CR4
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Enable Long Mode (bit 8) and No-Execute Enable NXE (bit 11) in EFER MSR (0xC0000080)
    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8) | (1 << 11) ; LME | NXE
    wrmsr

    ; Enable Paging and Protected Mode in CR0
    mov eax, cr0
    or eax, (1 << 31) | (1 << 0)
    mov cr0, eax

    ; Load 64-bit GDT
    lgdt [gdt64.pointer]

    ; Far jump into 64-bit long mode
    jmp gdt64.code:long_mode_start

.no_cpuid:
    mov al, 'C'
    jmp .halt_error
.no_long_mode:
    mov al, 'L'
.halt_error:
    mov dx, 0x3F8 ; COM1
    out dx, al
.hang:
    hlt
    jmp .hang

BITS 64
long_mode_start:
    ; Reload data segment registers for 64-bit mode
    mov ax, gdt64.data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov rsp, stack_top

    ; Call 64-bit kernel main
    call kmain

.infinite_loop:
    cli
    hlt
    jmp .infinite_loop

section .note.GNU-stack noalloc noexec nowrite progbits
