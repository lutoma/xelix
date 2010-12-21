ENTRY(mboot)
SECTIONS
{

    .text 0x100000 :
    {
        code = .; _code = .; __code = .;
        *(.text)
        . = ALIGN(16);
    }

    .data :
    {
        data = .; _data = .; __data = .;
        *(.data)
        *(.rodata)
        . = ALIGN(32);
    }

    .bss :
    {
        bss = .; _bss = .; __bss = .;
        *(.bss)
        . = ALIGN(32);
    }

    .GCC.command.line :
    {
        *(.GCC.command.line)
        . = ALIGN(0);
    }
    .comment :
    {
        *(.comment)
        . = ALIGN(0);
    }

    end = .; _end = .; __end = .;
}

