OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

PHDRS {
	null 	PT_NULL 	FLAGS(0);					/* --- */
	text 	PT_LOAD 	FLAGS((1 << 2) | (1 << 0));	/* r-x */
	rodata 	PT_LOAD 	FLAGS((1 << 2)); 			/* r-- */
	data 	PT_LOAD		FLAGS((1 << 2) | (1 << 1));	/* rw- */
	dynamic	PT_DYNAMIC	;
	modhdr	PT_NOTE		;
}

SECTIONS {
	.text : ALIGN(4096) {
		*(.text)
		*(.text.*)
	} :text

    . = ALIGN(4096);

	.note.gnu.build-id : {
		*(.note.gnu.build-id)
	} :rodata

	.rodata : {
		*(.rodata)
		*(.rodata.*)
	} :rodata

	.modulehdr : {
		*(.modulehdr)
	} :rodata :modhdr

    .ctors : {
        *(.ctors)
    } :rodata

    .dtors : {
        *(.dtors)
    } :rodata

    . = ALIGN(4096);
	.data : {
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
	} :data
}
