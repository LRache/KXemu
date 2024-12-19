ENTRY(_start)

SECTIONS {
    . = 0x80000000;
    .text : {
        *(entry)
        *(.text*)
    }
    .rodata : {
        *(.rodata*)
    }
    .data : {
        *(.data*)
    }
    .bss : {
        *(.bss*)
        *(.sbss*)
        *(.scommon)
    }
    __stack_top = ALIGN(0x1000);
    . = __stack_top + 0x8000; /* 32KB stack */
    __stack_pointer = .;
    __heap_start = ALIGN(0x1000);
}
