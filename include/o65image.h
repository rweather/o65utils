/*
 * Copyright (C) 2023 Southern Storm Software, Pty Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef O65IMAGE_H
#define O65IMAGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Reference: http://www.6502.org/users/andre/o65/fileformat.html
 */

/**
 * @brief Type that is large enough to hold a ".size" value from a ".o65" file.
 */
typedef uint32_t o65_size_t;

/**
 * @brief Structure of the raw ".o65" file header with 16-bit fields.
 */
typedef struct
{
    uint8_t magic[6];       /**< Magic number string */
    uint16_t mode;          /**< Mode word */
    uint16_t tbase;         /**< Original address of the .text segment */
    uint16_t tlen;          /**< Length of the .text segment */
    uint16_t dbase;         /**< Original address of the .data segment */
    uint16_t dlen;          /**< Length of the .data segment */
    uint16_t bbase;         /**< Original address of the .bss segment */
    uint16_t blen;          /**< Length of the .bss segment */
    uint16_t zbase;         /**< Original address of the .zeropage segment */
    uint16_t zlen;          /**< Length of the .zeropage segment */
    uint16_t stack;         /**< Number of bytes of stack space required */

} o65_header_16_t;

/**
 * @brief Structure of the raw ".o65" file header with 32-bit fields.
 */
typedef struct
{
    uint8_t magic[6];       /**< Magic number string */
    uint16_t mode;          /**< Mode word */
    uint32_t tbase;         /**< Original address of the .text segment */
    uint32_t tlen;          /**< Length of the .text segment */
    uint32_t dbase;         /**< Original address of the .data segment */
    uint32_t dlen;          /**< Length of the .data segment */
    uint32_t bbase;         /**< Original address of the .bss segment */
    uint32_t blen;          /**< Length of the .bss segment */
    uint32_t zbase;         /**< Original address of the .zeropage segment */
    uint32_t zlen;          /**< Length of the .zeropage segment */
    uint32_t stack;         /**< Number of bytes of stack space required */

} o65_header_32_t;

/* Bytes in the "magic" field */
#define O65_MAGIC_1         0x01    /**< First magic string byte */
#define O65_MAGIC_2         0x00    /**< Second magic string byte */
#define O65_MAGIC_3         0x6F    /**< Third magic string byte */
#define O65_MAGIC_4         0x36    /**< Fourth magic string byte */
#define O65_MAGIC_5         0x35    /**< Fifth magic string byte */
#define O65_MAGIC_6         0x00    /**< Sixth magic string byte */

/* Bits in the "mode" field */
#define O65_MODE_PAGED      0x4000  /**< Set if page alignment is required */
#define O65_MODE_32BIT      0x2000  /**< Set if sizes in the file are 32 bits */
#define O65_MODE_OBJ        0x1000  /**< Set for object file, clear for exe */
#define O65_MODE_SIMPLE     0x0800  /**< Simple load address form */
#define O65_MODE_CHAIN      0x0400  /**< Multiple chained images present */
#define O65_MODE_BSSZERO    0x0200  /**< .bss segment must be zeroed */
#define O65_MODE_CPU_BITS   0x80F0  /**< Bits that make up the CPU type */
#define O65_MODE_CPU_6502   0x0000  /**< CPU is 6502 core, no undoc opcodes */
#define O65_MODE_CPU_65C02  0x0010  /**< CPU is 65C02 */
#define O65_MODE_CPU_65SC02 0x0020  /**< CPU is 65SC02 */
#define O65_MODE_CPU_65CE02 0x0030  /**< CPU is 65CE02 */
#define O65_MODE_CPU_UNDOC  0x0040  /**< CPU is NMOS 6502 with undoc opcodes */
#define O65_MODE_CPU_EMUL   0x0050  /**< CPU is 65816 in 6502 emulation mode */
#define O65_MODE_CPU_6809   0x0080  /**< CPU is 6809 */
#define O65_MODE_CPU_Z80    0x00A0  /**< CPU is Z80 */
#define O65_MODE_CPU_8086   0x00D0  /**< CPU is 8086 */
#define O65_MODE_CPU_80286  0x00E0  /**< CPU is 80286 */
#define O65_MODE_CPU_65816  0x8000  /**< CPU is 65816 in 16-bit mode */
#define O65_MODE_ALIGN      0x0003  /**< Bits that make up the alignment mode */
#define O65_MODE_ALIGN_1    0x0000  /**< Byte alignment */
#define O65_MODE_ALIGN_2    0x0001  /**< Word alignment */
#define O65_MODE_ALIGN_4    0x0002  /**< Long word alignment */
#define O65_MODE_ALIGN_256  0x0003  /**< Page alignment */

/** Maximum number of bytes in an option, excluding the length and type bytes */
#define O65_MAX_OPT_SIZE    255

/* Standard header options */
#define O65_OPT_FILENAME    0   /**< Name of the object file */
#define O65_OPT_OS          1   /**< Operating system information */
#define O65_OPT_PROGRAM     2   /**< Name of the assembler or linker */
#define O65_OPT_AUTHOR      3   /**< Name of the author */
#define O65_OPT_CREATED     4   /**< Date and time that the file was created */

/* Custom header options */
#define O65_OPT_ELF_MACHINE 'E' /**< ELF machine type and flags */

/* Operating system types */
#define O65_OS_OSA65        1   /**< OSA/65 */
#define O65_OS_LUNIX        2   /**< Lunix */
#define O65_OS_CC65         3   /**< CC65 generic module */
#define O65_OS_OPENCBM      4   /**< opencbm floppy module */

/* Relocation types */
#define O65_RELOC_TYPE      0xE0    /**< Bits that contain the reloc type */
#define O65_RELOC_SEGID     0x1F    /**< Bits that contain the segment ID */
#define O65_RELOC_WORD      0x80    /**< 16-bit word */
#define O65_RELOC_HIGH      0x40    /**< High byte of a 16-bit word */
#define O65_RELOC_LOW       0x20    /**< Low byte of a 16-bit word */
#define O65_RELOC_SEGADR    0xC0    /**< 24-bit segment address */
#define O65_RELOC_SEG       0xA0    /**< Segment byte of a 24-bit address */

/* Segment identifiers in relocation type bytes */
#define O65_SEGID_UNDEF     0   /**< From the undefined references list */
#define O65_SEGID_ABS       1   /**< Absolute value */
#define O65_SEGID_TEXT      2   /**< .text segment */
#define O65_SEGID_DATA      3   /**< .data segment */
#define O65_SEGID_BSS       4   /**< .bss segment */
#define O65_SEGID_ZEROPAGE  5   /**< .zp segment */

#ifdef __cplusplus
}
#endif

#endif
