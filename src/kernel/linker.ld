    ENTRY(0x80002000)
    SECTIONS
    {
        .kernel : { *(.kernel) } AT(0x80002000)
        /* other sections */
    }