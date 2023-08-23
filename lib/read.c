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

uint16_t o65_read_uint16(const uint8_t *buf)
{
    return buf[0] | (((uint16_t)(buf[1])) << 8);
}

uint32_t o65_read_uint24(const uint8_t *buf)
{
    return buf[0] | (((uint32_t)(buf[1])) << 8) |
           (((uint32_t)(buf[2])) << 16);
}

uint32_t o65_read_uint32(const uint8_t *buf)
{
    return buf[0] | (((uint32_t)(buf[1])) << 8) |
           (((uint32_t)(buf[2])) << 16) | (((uint32_t)(buf[3])) << 24);
}

int o65_read_header(FILE *file, o65_header_t *header)
{
    uint8_t buf[36];

    /* Read the first 8 bytes and byte-swap the mode field */
    if (fread(buf, 1, 8, file) != 8)
        return -1;
    header->marker[0] = buf[0];
    header->marker[1] = buf[1];
    header->magic[0]  = buf[2];
    header->magic[1]  = buf[3];
    header->magic[2]  = buf[4];
    header->version   = buf[5];
    header->mode      = o65_read_uint16(buf + 6);

    /* Verify the marker, magic number, and version fields */
    if (header->marker[0] != 0x01)
        return 0;
    if (header->marker[1] != 0x00)
        return 0;
    if (header->magic[0] != 0x6F)   /* o */
        return 0;
    if (header->magic[1] != 0x36)   /* 6 */
        return 0;
    if (header->magic[2] != 0x35)   /* 5 */
        return 0;
    if (header->version != 0)
        return 0;

    /* The rest of the header uses either 16-bit or 32-bit fields */
    if ((header->mode & O65_MODE_32BIT) == 0) {
        /* 16-bit fields */
        if (fread(buf, 1, 18, file) != 18)
            return -1;
        header->tbase = o65_read_uint16(buf);
        header->tlen  = o65_read_uint16(buf + 2);
        header->dbase = o65_read_uint16(buf + 4);
        header->dlen  = o65_read_uint16(buf + 6);
        header->bbase = o65_read_uint16(buf + 8);
        header->blen  = o65_read_uint16(buf + 10);
        header->zbase = o65_read_uint16(buf + 12);
        header->zlen  = o65_read_uint16(buf + 14);
        header->stack = o65_read_uint16(buf + 16);
    } else {
        /* 32-bit fields */
        if (fread(buf, 1, 36, file) != 36)
            return -1;
        header->tbase = o65_read_uint32(buf);
        header->tlen  = o65_read_uint32(buf + 4);
        header->dbase = o65_read_uint32(buf + 8);
        header->dlen  = o65_read_uint32(buf + 12);
        header->bbase = o65_read_uint32(buf + 16);
        header->blen  = o65_read_uint32(buf + 20);
        header->zbase = o65_read_uint32(buf + 24);
        header->zlen  = o65_read_uint32(buf + 28);
        header->stack = o65_read_uint32(buf + 32);
    }
    return 1;
}

int o65_read_option(FILE *file, o65_option_t *option)
{
    int ch;

    /* Clear the option details before we start */
    memset(option, 0, sizeof(o65_option_t));

    /* Get the length of the option data */
    if ((ch = fgetc(file)) == EOF)
        return -1;

    /* If the length is zero, then there are no more options */
    if (ch == 0)
        return 1;

    /* The length must be 2 or greater for a valid option */
    if (ch < 2)
        return 0;
    option->len = (uint8_t)ch;

    /* Read the option type byte */
    if ((ch = fgetc(file)) == EOF)
        return -1;
    option->type = (uint8_t)ch;

    /* Read the option data */
    ch = option->len - 2;
    if (ch > 0) {
        if (fread(option->data, 1, ch, file) != (size_t)ch)
            return -1;
    }
    return 1;
}

int o65_read_reloc
    (FILE *file, const o65_header_t *header, o65_reloc_t *reloc)
{
    int ch;

    /* Clear the relocation details in case of error */
    reloc->offset = 0;
    reloc->type = 0;
    reloc->extra = 0;
    reloc->undefid = 0;

    /* Read the relocation offset */
    if ((ch = fgetc(file)) == EOF)
        return -1;
    reloc->offset = (uint8_t)ch;

    /* Zero for the end of the table, 255 for a skip-ahead entry */
    if (ch == 0 || ch == 255)
        return 1;

    /* Read the type/segment byte */
    if ((ch = fgetc(file)) == EOF)
        return -1;
    reloc->type = (uint8_t)ch;

    /* Undefined relocations are followed by the index of the external symbol */
    if ((reloc->type & O65_RELOC_SEGID) == O65_SEGID_UNDEF) {
        int size = ((header->mode & O65_MODE_32BIT) == 0) ? 2 : 4;
        int bit = 0;
        while (size > 0) {
            if ((ch = fgetc(file)) == EOF)
                return -1;
            reloc->undefid |= (((uint32_t)(ch & 0xFF)) << bit);
            --size;
            bit += 8;
        }
    }

    /* Determine if we need to read any extra details */
    switch (reloc->type & O65_RELOC_TYPE) {
    case O65_RELOC_HIGH:
        if ((header->mode & O65_MODE_PAGED) == 0) {
            /* Need the low byte of the HIGH relocation from the file */
            if ((ch = fgetc(file)) == EOF)
                return -1;
            reloc->extra = (uint8_t)ch;
        }
        break;

    case O65_RELOC_SEG:
        /* Need the lower two bytes of the SEG relocation from the file */
        if ((ch = fgetc(file)) == EOF)
            return -1;
        reloc->extra = (uint8_t)ch;
        if ((ch = fgetc(file)) == EOF)
            return -1;
        reloc->extra |= (uint16_t)(ch << 8);
        break;

    default: break;
    }
    return 1;
}

int o65_read_segment(FILE *file, uint8_t **data, o65_size_t size)
{
    if (size) {
        *data = (uint8_t *)malloc(size);
        if (!(*data))
            return -1;
        if (fread(*data, 1, size, file) != size) {
            free(*data);
            return -1;
        }
        return 1;
    } else {
        *data = NULL;
        return 1;
    }
}

int o65_read_count(FILE *file, const o65_header_t *header, o65_size_t *count)
{
    uint8_t buf[4];
    if ((header->mode & O65_MODE_32BIT) == 0) {
        if (fread(buf, 1, 2, file) != 2)
            return -1;
        *count = o65_read_uint16(buf);
    } else {
        if (fread(buf, 1, 4, file) != 2)
            return -1;
        *count = o65_read_uint32(buf);
    }
    return 1;
}

int o65_read_string(FILE *file, char *str, size_t max_size)
{
    int ch;
    size_t posn = 0;
    int truncated = 0;
    for (;;) {
        ch = getc(file);
        if (ch == EOF) {
            return -1;
        } else if (ch == 0) {
            break;
        } else if ((posn + 1) < max_size) {
            str[posn++] = (char)ch;
        } else {
            truncated = 1;
        }
    }
    str[posn] = '\0';
    return truncated ? 0 : 1;
}
