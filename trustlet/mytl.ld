/* mytl/mytl.ld — Kinibi trustlet memory layout.
 *
 *   VA 0x1000            text segment base (== file offset 0 for the loader)
 *   VA 0x1000..0x1100    reserved for the MCLF header + mclfTextHeader (elf2mclf fills)
 *   VA 0x1100..          code + rodata
 *   data: page-aligned above text (RX vs RW cannot share a page)
 *   bss:  immediately after data in VA; last 8 bytes = kernel-written TCI descriptor
 */
ENTRY(_start)

SECTIONS
{
    . = 0x1000;
    . = 0x1100;                         /* skip the 0x100-byte header region */

    .text : {
        *(.text) *(.text.*)
        *(.rodata) *(.rodata.*)
        . = ALIGN(8);
    }
    __text_end = .;

    . = ALIGN(0x1000);
    __data_start = .;
    .data : {
        *(.data) *(.data.*) *(.sdata) *(.sdata.*)
        . = ALIGN(8);
    }
    __data_end = .;

    .bss (NOLOAD) : {
        __bss_start = .;                /* == __data_end: bss follows data with no gap */
        *(.bss) *(.bss.*) *(COMMON)
        . = ALIGN(16);
        __stack_bottom = .;
        . = . + 0x2000;                 /* 8 KiB stack */
        __stack_top = .;
        . = ALIGN(8);
        __tci_desc = .;                 /* kernel writes {u32 ptr, u32 len} here */
        . = . + 8;
    }
    __bss_end = .;                      /* __tci_desc == __bss_end - 8 */

    /DISCARD/ : {
        *(.note.*) *(.comment) *(.eh_frame*) *(.ARM.attributes)
        *(.got) *(.got.plt) *(.plt*) *(.interp) *(.dynamic) *(.dynsym) *(.dynstr)
    }
}
