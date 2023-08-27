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

#include "o65file.h"
#include <string.h>
#include <stdlib.h>

void o65_write_uint16(uint8_t *buf, uint16_t value)
{
    buf[0] = (uint8_t)value;
    buf[1] = (uint8_t)(value >> 8);
}

void o65_write_uint24(uint8_t *buf, uint32_t value)
{
    buf[0] = (uint8_t)value;
    buf[1] = (uint8_t)(value >> 8);
    buf[2] = (uint8_t)(value >> 16);
}

void o65_write_uint32(uint8_t *buf, uint32_t value)
{
    buf[0] = (uint8_t)value;
    buf[1] = (uint8_t)(value >> 8);
    buf[2] = (uint8_t)(value >> 16);
    buf[3] = (uint8_t)(value >> 24);
}

int o65_write_header(FILE *file, o65_header_t *header)
{
    static uint8_t const magic[6] = {
        0x01, 0x00, 0x6F, 0x36, 0x35, 0x00
    };
    uint8_t buf[38];
    size_t size;

    /*
     * Page alignment can be specified in two different places.
     * Make sure that they are consistent.
     *
     * Technically we don't have to set O65_MODE_PAGED if alignment
     * is O65_MODE_ALIGN_256, only the other way around.  However,
     * setting O65_MODE_PAGED makes HIGH relocations more compact
     * so there is no downside in setting it.
     */
    if ((header->mode & O65_MODE_PAGED) != 0 ||
            (header->mode & O65_MODE_ALIGN) == O65_MODE_ALIGN_256) {
        header->mode |= O65_MODE_PAGED;
        header->mode = (header->mode & ~O65_MODE_ALIGN) | O65_MODE_ALIGN_256;
    }

    /* Force the use of 32-bit sizes if necessary.  We always force it for
     * 65816 and 80286 because there may be relocations with 24-bit values. */
    if ((header->tlen + header->dlen + header->blen) >= 0x10000U ||
            header->stack >= 0x10000U ||
            (header->mode & O65_MODE_CPU_65816) != 0 ||
            (header->mode & O65_MODE_CPU_BITS) == O65_MODE_CPU_80286) {
        header->mode |= O65_MODE_32BIT;
    }

    /* Does the layout appear to be simple? */
    if (header->dbase == (header->tbase + header->tlen) &&
            header->bbase == (header->dbase + header->dlen)) {
        header->mode |= O65_MODE_SIMPLE;
    } else {
        header->mode &= ~O65_MODE_SIMPLE;
    }

    /* Write the magic number and version information */
    if (fwrite(magic, 1, sizeof(magic), file) != sizeof(magic)) {
        return -1;
    }

    /* Encode the rest of the header and write it */
    if ((header->mode & O65_MODE_32BIT) == 0) {
        /* 16-bit size fields */
        o65_write_uint16(buf,      header->mode);
        o65_write_uint16(buf +  2, header->tbase);
        o65_write_uint16(buf +  4, header->tlen);
        o65_write_uint16(buf +  6, header->dbase);
        o65_write_uint16(buf +  8, header->dlen);
        o65_write_uint16(buf + 10, header->bbase);
        o65_write_uint16(buf + 12, header->blen);
        o65_write_uint16(buf + 14, header->zbase);
        o65_write_uint16(buf + 16, header->zlen);
        o65_write_uint16(buf + 18, header->stack);
        size = 20;
    } else {
        /* 32-bit size fields */
        o65_write_uint16(buf,      header->mode);
        o65_write_uint32(buf +  2, header->tbase);
        o65_write_uint32(buf +  6, header->tlen);
        o65_write_uint32(buf + 10, header->dbase);
        o65_write_uint32(buf + 14, header->dlen);
        o65_write_uint32(buf + 18, header->bbase);
        o65_write_uint32(buf + 22, header->blen);
        o65_write_uint32(buf + 26, header->zbase);
        o65_write_uint32(buf + 30, header->zlen);
        o65_write_uint32(buf + 34, header->stack);
        size = 38;
    }
    if (fwrite(buf, 1, size, file) != size) {
        return -1;
    }
    return 0;
}

int o65_write_option(FILE *file, const o65_option_t *option)
{
    if (!option || option->len == 0) {
        if (putc(0, file) < 0)
            return -1;
    } else {
        if (fwrite(&(option->len), 1, option->len, file) != option->len)
            return -1;
    }
    return 0;
}

void o65_set_string_option
    (o65_option_t *option, uint8_t type, const char *value, size_t len)
{
    if (len >= sizeof(option->data))
        len = sizeof(option->data) - 1;
    memcpy(option->data, value, len);
    option->data[len] = '\0';
    option->len = len + 3;
    option->type = type;
}

int o65_write_reloc
    (FILE *file, const o65_header_t *header, const o65_reloc_t *reloc)
{
    if (reloc->offset == 0 || reloc->offset == 255) {
        /* Special single-byte relocation */
        if (putc(reloc->offset, file) < 0) {
            return -1;
        }
    } else {
        /* Encode the relocation offset, type, and parameters */
        if (putc(reloc->offset, file) < 0) {
            return -1;
        }
        if (putc(reloc->type, file) < 0) {
            return -1;
        }
        if ((reloc->type & O65_RELOC_SEGID) == O65_SEGID_UNDEF) {
            /* Write the identifier of the external reference */
            if (o65_write_count(file, header, reloc->undefid) < 0)
                return -1;
        }
        switch (reloc->type & O65_RELOC_TYPE) {
        case O65_RELOC_HIGH:
            /* Include the low byte of the relocation address if not paged */
            if ((header->mode & O65_MODE_PAGED) == 0) {
                if (putc(reloc->extra & 0xFF, file) < 0) {
                    return -1;
                }
            }
            break;

        case O65_RELOC_SEG:
            /* Include the two low bytes of the relocation address */
            if (putc(reloc->extra & 0xFF, file) < 0) {
                return -1;
            }
            if (putc((reloc->extra >> 8) & 0xFF, file) < 0) {
                return -1;
            }
            break;

        default: break;
        }
    }
    return 0;
}

int o65_write_count(FILE *file, const o65_header_t *header, o65_size_t count)
{
    uint8_t buf[4];
    if ((header->mode & O65_MODE_32BIT) == 0) {
        o65_write_uint16(buf, count);
        if (fwrite(buf, 1, 2, file) != 2)
            return -1;
    } else {
        o65_write_uint32(buf, count);
        if (fwrite(buf, 1, 4, file) != 4)
            return -1;
    }
    return 0;
}

int o65_write_string(FILE *file, const char *str)
{
    size_t len;
    if (!str)
        str = "";
    len = strlen(str) + 1;
    if (fwrite(str, 1, len, file) != len)
        return -1;
    else
        return 0;
}

int o65_write_exported_symbol
    (FILE *file, const o65_header_t *header, const char *name,
     uint8_t segID, o65_size_t offset)
{
    if (o65_write_string(file, name) < 0)
        return -1;
    if (putc(segID, file) < 0)
        return -1;
    return o65_write_count(file, header, offset);
}
