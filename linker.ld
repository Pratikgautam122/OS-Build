ENTRY(_start)

SECTIONS
{
    /* The kernel will be loaded at 1MB */
    . = 1M;

    /* We place the multiboot section and the text section first. */
    .text : ALIGN(4K)
    {
        *(.multiboot)
        *(.text)
    }

    /* Read-only data. */
    .rodata : ALIGN(4K)
    {
        *(.rodata)
    }

    /* Read-write data (initialized) */
    .data : ALIGN(4K)
    {
        *(.data)
    }

    /* Read-write data (uninitialized) and stack */
    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
        _kernel_end = .;
    }
}
