OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(KeMain)

PHDRS
{
    text    PT_LOAD    FLAGS((1 << 0) | (1 << 2)) ; /* Execute + Read */
    rodata  PT_LOAD    FLAGS((1 << 2)) ;            /* Read only */
    data    PT_LOAD    FLAGS((1 << 1) | (1 << 2)) ; /* Write + Read */
}

SECTIONS
{
    . = 0xffffffff80000000;

    kKernelStart = .;

    kTextStart = .;
    .text : {
        *(.text .text.*)
    } :text
    kTextEnd = .;

    . = ALIGN(0x1000);

    kRodataStart = .;
    .rodata : {
        *(.rodata .rodata.*)
    } :rodata
    kRodataEnd = .;

    . = ALIGN(0x1000);

    kDataStart = .;
    .data : {
        *(.data .data.*)
    } :data
    kDataEnd = .;

    kKernelSize = . - kKernelStart;

    . = ALIGN(0x1000);

    .bss : {
        *(COMMON)
        *(.bss .bss.*)
    } :data

    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
    }
}