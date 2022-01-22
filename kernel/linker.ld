ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

PHDRS {
	null 	PT_NULL 	FLAGS(0);					/* --- */
	text 	PT_LOAD 	FLAGS((1 << 2) | (1 << 0));	/* r-x */
	rodata 	PT_LOAD 	FLAGS((1 << 2)); 			/* r-- */
	data 	PT_LOAD		FLAGS((1 << 2) | (1 << 1));	/* rw- */
}

SECTIONS {
	. = 0xFFFFFFFF80000000;

	.text : ALIGN(4096) {
		*(.text)
		*(.text.*)
	} :text

	.text_free_after_init : ALIGN(4096) {
		*(.text_free_after_init)
		*(.text_free_after_init.*)
	} :text

	.stivale2hdr : ALIGN(4096) {
		KEEP(*(.stivale2hdr))
	} :rodata

	.rodata : {
		*(.rodata)
		*(.rodata.*)

		_global_constructor_start = .;
		*(.ctors)
		_global_constructor_end = .;
	} :rodata

	.data : ALIGN(4096) {
		*(.data)
		*(.data.*)
	} :data

	.bss (NOLOAD): {
		*(COMMON)
		*(.bss)
		*(.bss.*)

		. = ALIGN(4096);
		*(.heap)
	} :data

	.free_after_init (NOLOAD): ALIGN(4096) {
		*(.free_after_init)
	} :data

	. = ALIGN(4096);
	_kernel_end = .;

	/DISCARD/ : {
		*(.dtors)
	}
}
