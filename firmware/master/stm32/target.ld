ENTRY(_start)

/* Stack starts at the top of RAM */
_stack = 0x20000000 + (64*1024) - 4;

SECTIONS
{
    /* Flash */
    . = 0x08000000;

    .text : {
        *(.vectors);
        *(.text .text.*);
    }

    .rodata : {
        *(.rodata .rodata.*);
    }

    /* RAM */
    . = 0x20000000;

    .data : {
        *(.data .data.*)
    }

    .bss : {
        __bss_start = .;
        *(.bss .bss.*);
    }        

    _end = .;

    /DISCARD/ : {
        *(.note .note.* .comment .comment.*);
    }
}
