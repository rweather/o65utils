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

#ifndef O65FILE_H
#define O65FILE_H

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Type that is large enough to hold a ".size" value from a ".o65" file.
 */
typedef uint32_t o65_size_t;

/**
 * @brief Structure of the of the ".o65" file header after it has been
 * read into memory and byte-swapped.
 *
 * http://www.6502.org/users/andre/o65/fileformat.html
 */
typedef struct
{
    uint8_t marker[2];      /**< $01, $00 to indicate "non-C64" */
    uint8_t magic[3];       /**< "o65" magic number */
    uint8_t version;        /**< Version of the format, currently 0 */
    uint16_t mode;          /**< Mode word */
    o65_size_t tbase;       /**< Original address of the .text segment */
    o65_size_t tlen;        /**< Length of the .text segment */
    o65_size_t dbase;       /**< Original address of the .data segment */
    o65_size_t dlen;        /**< Length of the .data segment */
    o65_size_t bbase;       /**< Original address of the .bss segment */
    o65_size_t blen;        /**< Length of the .bss segment */
    o65_size_t zbase;       /**< Original address of the .zeropage segment */
    o65_size_t zlen;        /**< Length of the .zeropage segment */
    o65_size_t stack;       /**< Number of bytes of stack space required */

} o65_header_t;

/* Bits in the "mode" field of o65_header_t. */
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

/**
 * @brief Information about a header option from a ".o65" file.
 */
typedef struct
{
    uint8_t len;    /**< Length of the option, zero for end options */
    uint8_t type;   /**< Type of option */
    uint8_t data[O65_MAX_OPT_SIZE - 2]; /**< Data for the option */

} o65_option_t;

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

/**
 * @brief Relocation entry that has been loaded from a ".o65" file.
 *
 * If "offset" is 255, then the relocation is a skip ahead by 254 bytes
 * and the other fields are not important.
 *
 * The "undefid" only makes sense if the segment ID is O65_SEGID_UNKNOWN.
 * Otherwise it is not used.
 *
 * The "extra" field is only useful for HIGH and SEG relocations which
 * need to know the low bytes of a 16-bit or 24-bit address to be able
 * to perform the relocation.
 */
typedef struct
{
    uint8_t offset;         /**< Offset to the relocation from the last one */
    uint8_t type;           /**< Relocation type and segment identifier */
    uint16_t extra;         /**< Extra value associated with the relocation */
    uint32_t undefid;       /**< Identifier for an undefined reference */

} o65_reloc_t;

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
#define O65_SEGID_ZEROPAGE  5   /**< .zeropage segment */

/** Maximum length of a CPU or segment name, including the terminating NUL. */
#define O65_NAME_MAX        16

/** Recommended maximum buffer length for the names of externals. */
#define O65_STRING_MAX      256

/**
 * @brief Reads a 16-bit value in little-endian byte order.
 *
 * @param[in] buf Points to the two bytes to convert.
 *
 * @return The 16-bit value.
 */
uint16_t o65_read_uint16(const uint8_t *buf);

/**
 * @brief Reads a 24-bit value in little-endian byte order.
 *
 * @param[in] buf Points to the three bytes to convert.
 *
 * @return The 24-bit value.
 */
uint32_t o65_read_uint24(const uint8_t *buf);

/**
 * @brief Reads a 32-bit value in little-endian byte order.
 *
 * @param[in] buf Points to the four bytes to convert.
 *
 * @return The 32-bit value.
 */
uint32_t o65_read_uint32(const uint8_t *buf);

/**
 * @brief Writes a 16-bit value in little-endian byte order.
 *
 * @param[in] buf Points to the buffer to write to.
 * @param[in] value The 16-bit value to write.
 */
void o65_write_uint16(uint8_t *buf, uint16_t value);

/**
 * @brief Writes a 24-bit value in little-endian byte order.
 *
 * @param[in] buf Points to the buffer to write to.
 * @param[in] value The 24-bit value to write.
 */
void o65_write_uint24(uint8_t *buf, uint32_t value);

/**
 * @brief Writes a 32-bit value in little-endian byte order.
 *
 * @param[in] buf Points to the buffer to write to.
 * @param[in] value The 32-bit value to write.
 */
void o65_write_uint32(uint8_t *buf, uint32_t value);

/**
 * @brief Reads the header from a ".o65" file.
 *
 * @param[in] file File pointer.
 * @param[out] header Returns the header details on success.
 *
 * @return 1 if the header was read, 0 if the header is invalid,
 * or -1 for unexpected EOF or a filesystem error.
 */
int o65_read_header(FILE *file, o65_header_t *header);

/**
 * @brief Writes a header to a ".o65" file.
 *
 * @param[in] file File pointer.
 * @param[in,out] header The header details.
 *
 * @return 0 if the header was written, or -1 for a filesystem error.
 *
 * The "marker", "magic", and "version" fields can be left unassigned.
 * This function will write the correct values for those bytes.
 *
 * The "mode" field may be modified to fix up section alignment,
 * and to force 32-bit mode if the section sizes or CPU require it.
 */
int o65_write_header(FILE *file, o65_header_t *header);

/**
 * @brief Reads a header option from a ".o65" file.
 *
 * @param[in] file File pointer.
 * @param[out] option Returns the option details on success.
 *
 * @return 1 if the option was read, 0 if the option data is invalid,
 * or -1 for unexpected EOF or a filesystem error.
 */
int o65_read_option(FILE *file, o65_option_t *option);

/**
 * @brief Writes a header option to a ".o65" file.
 *
 * @param[in] file File pointer.
 * @param[in[ option The option information to write, or NULL to
 * terminate the option list.
 *
 * @return 0 if the option was written, or -1 for a filesystem error.
 */
int o65_write_option(FILE *file, const o65_option_t *option);

/**
 * @brief Sets a header option to a string value.
 *
 * @param[out] option The header option to set.
 * @param[in] type Option type.
 * @param[in] value Points to the string value to set.
 * @param[in] len Length of the string value in bytes, excluding the
 * terminating NUL.
 */
void o65_set_string_option
    (o65_option_t *option, uint8_t type, const char *value, size_t len);

/**
 * @brief Reads a relocation declaration from a ".o65" file.
 *
 * @param[in] file File pointer.
 * @param[in] header File header, containing global relocation options.
 * @param[out] reloc Returns the relocation details on success.
 *
 * @return 1 if the relocation was read, 0 if the relocation data is invalid,
 * or -1 for unexpected EOF or a filesystem error.
 */
int o65_read_reloc
    (FILE *file, const o65_header_t *header, o65_reloc_t *reloc);

/**
 * @brief Writes a relocation declaration to a ".o65" file.
 *
 * @param[in] file File pointer.
 * @param[in] header File header, containing global relocation options.
 * @param[out] reloc The relocation details to write.
 *
 * @return 0 if the relocation was written, or -1 for a filesystem error.
 */
int o65_write_reloc
    (FILE *file, const o65_header_t *header, const o65_reloc_t *reloc);

/**
 * @brief Reads the contents of the .text or .data segment from a ".o65" file.
 *
 * @param[in] file File pointer.
 * @param[out] data Returns a pointer to the data, which must be freed
 * with free() when no longer required.
 * @param[in] size Number of bytes in the segment.
 *
 * @return 1 if the segment was read, or -1 for unexpected EOF or a
 * filesystem error.
 */
int o65_read_segment(FILE *file, uint8_t **data, o65_size_t size);

/**
 * @brief Reads a 16-bit or 32-bit count value from a ".o65" file.
 *
 * @param[in] file File pointer.
 * @param[in] header Points to the file header information.
 * @param[out] count Returns the count value that was read.
 *
 * @return 1 on success, or -1 for unexpected EOF or a filesystem error.
 */
int o65_read_count(FILE *file, const o65_header_t *header, o65_size_t *count);

/**
 * @brief Writes a 16-bit or 32-bit count value to a ".o65" file.
 *
 * @param[in] file File pointer.
 * @param[in] header Points to the file header information.
 * @param[in] count The count to write.
 *
 * @return 0 if the count was written, or -1 for a filesystem error.
 */
int o65_write_count(FILE *file, const o65_header_t *header, o65_size_t count);

/**
 * @brief Reads a NUL-terminated string from a ".o65" file.
 *
 * @param[in] file File pointer.
 * @param[in] str Points to the buffer to receive the string.
 * @param[in[ max_size Maximum size of the @a str buffer.
 *
 * @return 1 on success, 0 if the string was too long for @a str and
 * had to be truncated, or -1 for unexpected EOF or a filesystem error.
 */
int o65_read_string(FILE *file, char *str, size_t max_size);

/**
 * @brief Writes a NUL-terminated string to a ".o65" file.
 *
 * @param[in] file File pointer.
 * @param[in] str The string to write.
 *
 * @return 0 if the string was written, or -1 for a filesystem error.
 */
int o65_write_string(FILE *file, const char *str);

/**
 * @brief Writes an exported symbol definition to a ".o65" file.
 *
 * @param[in] file File pointer.
 * @param[in] header Points to the file header information.
 * @param[in] name The name of the symbol.
 * @param[in] segID The segment identifier; e.g. O65_SEGID_TEXT.
 * @param[in] offset The offset into the segment of the exported symbol.
 *
 * @return 0 if the symbol was written, or -1 for a filesystem error.
 */
int o65_write_exported_symbol
    (FILE *file, const o65_header_t *header, const char *name,
     uint8_t segID, o65_size_t offset);

/**
 * @brief Gets the name of a CPU from the header mode bits.
 *
 * @param[in]  mode Mode bits from the ".o65" header.
 * @param[out] name Returns the name of the CPU.
 *
 * @return Non-zero if the CPU name is recognized, or zero if unknown.
 * The @a name will always be populated, even for unknown CPU's.
 */
int o65_get_cpu_name(uint16_t mode, char name[O65_NAME_MAX]);

/**
 * @brief Gets the name of a segment from its identifier.
 *
 * @param[in] segid The segument identifier.
 * @param[out] name Returns the name of the CPU.
 *
 * @return Non-zero if the segment is recognized, or zero if unknown.
 * The @a name will always be populated, even for unknown segments.
 */
int o65_get_segment_name(uint8_t segid, char name[O65_NAME_MAX]);

#ifdef __cplusplus
}
#endif

#endif
