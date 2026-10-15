/*
 * NOVA OS - Linker Script for x86_64 Bare-Metal Microkernel
 * Places kernel at physical 1MB mark with strict page-aligned sections.
 */

ENTRY(_start)

SECTIONS
{
    . = 1M;

    .multiboot : ALIGN(4K)
    {
        KEEP(*(.multiboot))
    }

    .text : ALIGN(4K)
    {
        *(.text .text.*)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata .rodata.*)
    }

    .data : ALIGN(4K)
    {
        *(.data .data.*)
    }

    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss .bss.*)
    }

    . = ALIGN(4K);
    _kernel_end = .;
}
