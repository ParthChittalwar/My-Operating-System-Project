/* linker.ld - Linker script for the NovaOS freestanding kernel */

OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(_start)

PHDRS
{
    text    PT_LOAD FLAGS(5); /* Read + Execute (RX) */
    rodata  PT_LOAD FLAGS(4); /* Read-only (R) */
    data    PT_LOAD FLAGS(6); /* Read + Write (RW) */
}

SECTIONS
{
    /* We load the kernel into the higher-half virtual address space */
    . = 0xffffffff80000000;

    /* Code section */
    .text : {
        *(.text .text.*)
    } :text

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    /* Read-only data section */
    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    /* Limine requests and responses need to be writable, so we map them to the data segment */
    .limine_requests : {
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests.*))
    } :data

    /* Data section */
    .data : {
        *(.data .data.*)
    } :data

    /* BSS section (uninitialized data) */
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data

    /* Discard unnecessary ELF sections */
    /DISCARD/ : {
        *(.eh_frame)
        *(.note.gnu.build-id)
    }
}
