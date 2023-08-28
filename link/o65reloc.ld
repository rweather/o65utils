/* Linker script to produce o65 relocatable binaries. This uses elf2o65
 * to convert the resulting absolute binary ELF files into relocatable
 * binaries.
 *
 * Based on the linker script for cpm65 from the llvm-mos-sdk project.
 *
 * © 2023 Rhys Weatherley
 * © 2022 David Given
 * This file is part of the llvm-mos-sdk project and is redistributable under
 * the terms of the Apache 2.0 license with the LLVM exceptions. See the LICENSE
 * file in the project root for the full text.
 */

MEMORY {
    zp : ORIGIN = 0x00, LENGTH = 0x100
    ram (rw) : ORIGIN = 0x200, LENGTH = 0xfe00
}

PHDRS {
	load PT_LOAD;
	init PT_LOAD;
}

SECTIONS {
	.zp (NOLOAD) : {
		/* Put the imaginary registers into the zeropage segment
		 * explicitly so that they will be relocated properly. */
		__rc0 = .;
		INCLUDE imag-regs.ld
		. = . + 32;
	} >zp :NONE

	.zp.bss (NOLOAD) : { INCLUDE zp-bss-sections.ld } >zp :NONE
	INCLUDE zp-bss-symbols.ld

	.text : {
	} >ram :load

	INCLUDE text.ld
	INCLUDE rodata.ld
	INCLUDE data.ld

	.zp.data : {
		__zp_data_start = .;
		*(.zp.data .zp.data.*)
		*(.zp.rodata .zp.rodata.*)
	} >zp AT>ram :init
	__zp_data_load_start = ADDR(.text) + 
		(LOADADDR(.zp.data) - LOADADDR(.text));
	__zp_data_size = SIZEOF(.zp.data);

	INCLUDE bss.ld
	INCLUDE noinit.ld

	INCLUDE zp-noinit.ld

	.noinit (NOLOAD) : {
		/* Create 256 bytes of stack space in .noinit memory */
		/* TODO: Should handle this within the crt0 for the platform */
		. = . + 256;
		__stack = .;
	}
}

/* The result of this is actually irrelevant --- the elf2o65 tool will
 * rewrite the output file and add relocation information. The only reason this
 * is here is to trick the linker into emitting the $FILENAME.elf file which
 * elf2o65 reads. */

OUTPUT_FORMAT {
    TRIM(ram)
}

