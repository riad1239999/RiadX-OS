/* Advanced linker script for RiadX-OS kernel (x86, 32-bit) */
/* Adjust sections and addresses as needed for your kernel */

OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
    /* Kernel loaded at 1MB */
    KERNEL_BASE = 0x00100000;
    . = KERNEL_BASE;

    /* Multiboot header, if present */
    .multiboot ALIGN(4K) : {
        *(.multiboot)
    }

    /* Text/code section */
    .text ALIGN(4K) : AT (ADDR(.text)) {
        *(.text .text.*)
        *(.gnu.linkonce.t.*)
    }

    /* Read-only data */
    .rodata ALIGN(4K) : AT (ADDR(.rodata)) {
        *(.rodata .rodata.*)
        *(.gnu.linkonce.r.*)
    }

    /* Data section */
    .data ALIGN(4K) : AT (ADDR(.data)) {
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
    }

    /* Read-write, initialized data */
    .sdata ALIGN(4K) : AT (ADDR(.sdata)) {
        *(.sdata .sdata.*)
    }

    /* Uninitialized data (BSS) */
    .bss ALIGN(4K) (NOLOAD) : AT (ADDR(.bss)) {
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
    }

    /* Stack (optional, 16KB) */
    . = ALIGN(16K);
    PROVIDE(stack_bottom = .);
    .stack (NOLOAD) : {
        . = . + 16K;
    }
    PROVIDE(stack_top = .);

    /* Heap start/end (optional) */
    PROVIDE(heap_start = .);
    /* You can define heap_end elsewhere or dynamically */

    /* End of kernel image */
    _end = .;
}
