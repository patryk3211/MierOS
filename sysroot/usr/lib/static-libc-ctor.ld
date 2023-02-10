SECTIONS {
    .ctors :
    {
        *(SORT_BY_INIT_PRIORITY(.ctors*))
        *libc.a:*(SORT_BY_INIT_PRIORITY(.ctors*))
    }

    .init_array :
    {
        *libc.a:*(SORT_BY_INIT_PRIORITY(.init_array*))
        *(SORT_BY_INIT_PRIORITY(.init_array*))
    }
} INSERT AFTER .text

