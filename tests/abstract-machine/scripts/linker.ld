ENTRY(_start)

SECTIONS {
    . = 0x80000000;
    .text : {
        *(entry)
        *(.text)
    }
    .rodata : {
        *(.rodata*)
    }
    .data : {
        *(.data)
    }
    .bss : {
        *(.bss*)
        *(.sbss*)
        *(.scommon)
    }

    _stack_top = ALIGN(0x1000);
    . = _stack_top + 0x8000;
    _stack_pointer = .;
}
