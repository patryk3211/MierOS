ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

PHDRS {
	null 	PT_NULL 	FLAGS(0);					/* --- */
	text 	PT_LOAD 	FLAGS((1 << 2) | (1 << 0));	/* r-x */
	rodata 	PT_LOAD 	FLAGS((1 << 2)); 			/* r-- */
	data 	PT_LOAD		FLAGS((1 << 2) | (1 << 1));	/* rw- */
	dynamic	PT_DYNAMIC	;
}

SECTIONS {
	. = 0xFFFFFFFF80000000;

	.text : ALIGN(4096) {
		*(.text)
		*(.text.*)
	} :text

	.text_free_after_init : ALIGN(4096) {
		_init_text_start = .;
		*(.text_free_after_init)
		*(.text_free_after_init.*)
		_init_text_end = .;
	} :text

	.stivale2hdr : ALIGN(4096) {
		KEEP(*(.stivale2hdr))
	} :rodata

	.note.gnu.build-id : {
		*(.note.gnu.build-id)
	} :rodata

	.rodata : {
		*(.rodata)
		*(.rodata.*)

		. = ALIGN(8);
		_global_constructor_start = .;
		*(.ctors)
		_global_constructor_end = .;

		. = ALIGN(8);
		_init_array_start = .;
		*(.init_array)
		_init_array_end = .;
	} :rodata

	.dynsym : ALIGN(8) {
		_dynsym_start = .;
		*(.dynsym)
		_dynsym_end = .;
	} :rodata

	.dynstr : {
		_dynstr_start = .;
		*(.dynstr)
	} :rodata

	.data : ALIGN(4096) {
		*(.data)
		*(.data.*)
	} :data

	.dynamic : {
		*(.dynamic)
	} :data :dynamic

	.bss (NOLOAD): {
		*(COMMON)
		*(.bss)
		*(.bss.*)

		. = ALIGN(4096);
		*(.heap)
	} :data

	.free_after_init (NOLOAD): ALIGN(4096) {
		_init_data_start = .;
		*(.free_after_init)
		_init_data_end = .;
	} :data

	. = ALIGN(4096);
	_kernel_end = .;

	/DISCARD/ : {
		*(.dtors)
	}
}
