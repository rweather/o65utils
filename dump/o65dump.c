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
#include "elfmos.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int disassemble = 0;

static int dump_file(const char *filename);

int main(int argc, char *argv[])
{
    int arg;
    int first;
    int named;
    int exit_val = 0;

    /* Need at least one command-line argument other than "-d" */
    arg = 1;
    if (arg < argc && (!strcmp(argv[1], "-d") || !strcmp(argv[1], "--disassemble"))) {
        disassemble = 1;
        ++arg;
    }
    if (arg >= argc) {
        fprintf(stderr, "Usage: %s [-d|--disassemble] file1 ...\n", argv[0]);
        return 1;
    }

    /* Process each of the files in turn */
    first = 1;
    named = (argc - arg) > 1;
    for (; arg < argc; ++arg) {
        if (first)
            first = 0;
        else
            printf("\n");
        if (named)
            printf("%s:\n\n", argv[arg]);
        if (!dump_file(argv[arg]))
            exit_val = 1;
    }
    return exit_val;
}

static void file_error(FILE *file, const char *filename)
{
    if (feof(file))
        fprintf(stderr, "%s: unexpected EOF\n", filename);
    else
        perror(filename);
    fclose(file);
}

static void dump_string(const uint8_t *data, int len)
{
    while (len > 0) {
        int ch = *data++;
        if (ch >= ' ' && ch <= 0x7E)
            putc(ch, stdout);
        else if (ch != 0)
            printf("\\x%02x", ch);
        --len;
    }
}

static int dump_nul_string(FILE *file)
{
    int ch;
    for (;;) {
        ch = getc(file);
        if (ch < 0)
            return -1;
        else if (ch == 0)
            break;
        else if (ch >= ' ' && ch <= 0x7E)
            putc(ch, stdout);
        else if (ch != 0)
            printf("\\x%02x", ch);
    }
    return 1;
}

static void dump_hex(const uint8_t *data, int len)
{
    while (len > 0) {
        int ch = *data++;
         printf(" %02x", ch);
        --len;
    }
}

static void dump_hex_line
    (const o65_header_t *header, o65_size_t addr, const uint8_t *data, int len)
{
    if ((header->mode & O65_MODE_32BIT) != 0)
        printf("    %08lx:", (unsigned long)addr);
    else
        printf("    %04lx:", (unsigned long)addr);
    dump_hex(data, len);
    printf("\n");
}

static void dump_option(const o65_option_t *option)
{
    printf("    ");
    switch (option->type) {
    case O65_OPT_FILENAME:
        printf("Filename: ");
        dump_string(option->data, option->len - 2);
        break;

    case O65_OPT_OS:
        printf("Operating System Information:");
        dump_hex(option->data, option->len - 2);
        break;

    case O65_OPT_PROGRAM:
        printf("Assembler/Linker: ");
        dump_string(option->data, option->len - 2);
        break;

    case O65_OPT_AUTHOR:
        printf("Author: ");
        dump_string(option->data, option->len - 2);
        break;

    case O65_OPT_CREATED:
        printf("Created: ");
        dump_string(option->data, option->len - 2);
        break;

    case O65_OPT_ELF_MACHINE:
        if (option->len >= 8 && option->data[0] == 0x66 &&
                option->data[1] == 0x19) {
            /* Dump the ELF MOS flags */
            struct elf_mos_flag
            {
                uint32_t flag;
                const char *name;
            };
            static struct elf_mos_flag const flags[] = {
                {EM_MOS_6502,       "mos6502"},
                {EM_MOS_6502_BCD,   "mos6502bcd"},
                {EM_MOS_6502X,      "mos6502x"},
                {EM_MOS_65C02,      "mos65c02"},
                {EM_MOS_R65C02,     "mosr65c02"},
                {EM_MOS_W65C02,     "mosw65c02"},
                {EM_MOS_W65816,     "mosw65816"},
                {EM_MOS_65EL02,     "mos65el02"},
                {EM_MOS_65CE02,     "mos65ce02"},
                {EM_MOS_HUC6280,    "moshuc6280"},
                {EM_MOS_65DTV02,    "mos65dtv02"},
                {EM_MOS_4510,       "mos4510"},
                {EM_MOS_45GS02,     "mos45gs02"},
                {0,                 0}
            };
            uint32_t elf_flags = o65_read_uint32(option->data + 2);
            int index;
            printf("ELF Machine: MOS Technologies\n");
            printf("    ELF Machine Flags: 0x%lx", (unsigned long)elf_flags);
            for (index = 0; flags[index].flag != 0; ++index) {
                /* Print all of the flags that we recognize */
                if (elf_flags & flags[index].flag) {
                    printf(", %s", flags[index].name);
                    elf_flags &= ~(flags[index].flag);
                }
            }
            if (elf_flags != 0) {
                /* We have some left-over flags; print them */
                printf(", Other: 0x%lx", (unsigned long)elf_flags);
            }
        } else {
            /* Not a 6502, so dump the options in hexadecimal */
            if (option->len == 8) {
                printf("ELF Machine: 0x%x\n", o65_read_uint16(option->data));
                printf("    ELF Machine Flags: 0x%lx",
                       (unsigned long)(o65_read_uint32(option->data + 2)));
            } else {
                printf("ELF Machine Option:");
                dump_hex(option->data, option->len - 2);
            }
        }
        break;

    default:
        printf("Option %d:", option->type);
        dump_hex(option->data, option->len - 2);
        break;
    }
    printf("\n");
}

#include "instructions.h"

static void disasseble_segment
    (const o65_header_t *header, o65_size_t addr,
     const uint8_t *data, o65_size_t len)
{
    uint8_t opcode;
    uint8_t opmode;
    uint8_t oplen;
    uint8_t posn;
    uint16_t target;
    const char *name;
    while (len > 0) {
        /* Fetch the next opcode */
        opcode = data[0];

        /* Find the name, length, and mode for the opcode */
        name = op6502_names + op6502_to_name[opcode];
        opmode = op6502_modes[opcode];
        oplen = opmode >> 6;

        /* Replace with illegal if there is insufficient data left */
        if (len < oplen) {
            name = "db ";
            opmode = OP_ill;
            oplen = 1;
        }

        /* Print the address */
        if (header->mode & O65_MODE_32BIT)
            printf("    %08lx:", (unsigned long)addr);
        else
            printf("    %04lx:", (unsigned long)addr);

        /* Print the bytes of the instruction */
        for (posn = 0; posn < oplen; ++posn) {
            printf(" %02x", data[posn]);
        }
        while (posn < 4) {
            printf("   ");
            ++posn;
        }

        /* Print the opcode name.  Special case the 4-character opcodes */
        if (opmode == OP_bit_zpg || opmode == OP_zpg_rel)
            printf("%c%c%c%d ", name[0], name[1], name[2], (opcode & 0x70) >> 4);
        else
            printf("%c%c%c ", name[0], name[1], name[2]);

        /* Print the operands */
        switch (opmode) {
        case OP_imp:
            /* Implict operand - nothing to do */
            break;

        case OP_imm:
            /* Immediate operand */
            printf("#$%02x", data[1]);
            break;

        case OP_abs:
            /* Absolute addressing mode */
            printf("$%04x", o65_read_uint16(data + 1));
            break;

        case OP_abs_X:
            /* Absolute addressing with X mode */
            printf("$%04x,x", o65_read_uint16(data + 1));
            break;

        case OP_abs_Y:
            /* Absolute addressing with Y mode */
            printf("$%04x,y", o65_read_uint16(data + 1));
            break;

        case OP_X_ind:
            /* Zero page indirect with X mode */
            printf("($%02x,x)", data[1]);
            break;

        case OP_ind_Y:
            /* Zero page indirect with Y mode */
            printf("($%02x),y", data[1]);
            break;

        case OP_zpg:
        case OP_bit_zpg:
        case OP_ill:
            /* Zero page addressing mode */
            printf("$%02x", data[1]);
            break;

        case OP_zpg_X:
            /* Zero page addressing with X mode */
            printf("$%02x,x", data[1]);
            break;

        case OP_zpg_Y:
            /* Zero page addressing with Y mode */
            printf("$%02x,y", data[1]);
            break;

        case OP_rel:
            /* Relative branch */
            target = (addr + 2) + (int16_t)(int8_t)(data[1]);
            printf("$%04x", target);
            break;

        case OP_ind:
            /* Absolute indirect addressing mode */
            printf("($%04x)", o65_read_uint16(data + 1));
            break;

        case OP_ind_zpg:
            /* Zero page indirect mode with no indexing */
            printf("($%02x)", data[1]);
            break;

        case OP_ind_abs_X:
            /* Absolute indirect addressing with X mode */
            printf("($%04x,x)", o65_read_uint16(data + 1));
            break;

        case OP_zpg_rel:
            /* Zero page addressing plus a branch */
            target = (addr + 3) + (int16_t)(int8_t)(data[2]);
            printf("$%02x,$%04x", data[1], target);
            break;

        default:
            printf("???");
            break;
        }
        printf("\n");

        /* Advance to the next opcode */
        addr += oplen;
        data += oplen;
        len -= oplen;
    }
}

static int can_disassemble(const o65_header_t *header)
{
    /* Determine if we can disassemble this CPU type */
    switch (header->mode & O65_MODE_CPU_BITS) {
    case O65_MODE_CPU_6502:
    case O65_MODE_CPU_65C02:
    case O65_MODE_CPU_65SC02:
    case O65_MODE_CPU_EMUL:
        return 1;
    default:
        return 0;
    }
}

static int dump_segment
    (FILE *file, const char *name, const o65_header_t *header,
     o65_size_t base, o65_size_t len, int is_text)
{
    uint8_t *data = NULL;
    o65_size_t posn;

    /* Print the name and size of the segment */
    printf("\n%s: %lu bytes\n", name, (unsigned long)len);

    /* Read the segment data */
    if (o65_read_segment(file, &data, len) < 0)
        return -1;

    /* Dump the contents of the segment */
    if (is_text && disassemble && can_disassemble(header)) {
        disasseble_segment(header, base, data, len);
    } else {
        posn = 0;
        while ((len - posn) >= 16U) {
            dump_hex_line(header, base, data + posn, 16);
            base += 16;
            posn += 16;
        }
        if (posn < len) {
            dump_hex_line(header, base, data + posn, len - posn);
        }
    }
    free(data);
    return 1;
}

static int dump_undefined_symbols(FILE *file, const o65_header_t *header)
{
    o65_size_t index;
    o65_size_t count;
    int result;

    /* Read the number of undefined symbols */
    if (o65_read_count(file, header, &count) < 0)
        return -1;

    /* This is easy if there are no undefined symbols */
    if (count == 0) {
        printf("\nUndefined Symbols: none\n");
        return 1;
    }

    /* Dump the names of the undefined symbols */
    printf("\nUndefined Symbols:\n");
    for (index = 0; index < count; ++index) {
        printf("    %lu: ", (unsigned long)index);
        result = dump_nul_string(file);
        if (result <= 0)
            return result;
        printf("\n");
    }
    return 1;
}

static int dump_relocs
    (FILE *file, const char *name, const o65_header_t *header,
     o65_size_t addr)
{
    o65_reloc_t reloc;
    int result;

    /* Relocations actually start at the segment base - 1 */
    --addr;

    /* Read and dump all relocations for the segment */
    printf("\n%s.relocs:\n", name);
    for (;;) {
        /* Read the next relocation entry */
        result = o65_read_reloc(file, header, &reloc);
        if (result <= 0)
            return result;
        else if (reloc.offset == 0)
            break;

        /* Determine the next address to be relocated */
        if (reloc.offset == 255) {
            /* 255 indicates "skip ahead by 254 bytes" */
            addr += 254;
            continue;
        } else {
            addr += reloc.offset;
        }
        if ((header->mode & O65_MODE_32BIT) != 0)
            printf("    %08lx: ", (unsigned long)addr);
        else
            printf("    %04lx: ", (unsigned long)addr);

        /* Print the segment that the relocation destination points to */
        if ((reloc.type & O65_RELOC_SEGID) == O65_SEGID_UNDEF) {
            printf("undef %lu", (unsigned long)reloc.undefid);
        } else {
            char segname[O65_NAME_MAX];
            o65_get_segment_name(reloc.type & O65_RELOC_SEGID, segname);
            printf("%s", segname);
        }

        /* Print the relocation type plus any extra information */
        printf(", ");
        switch (reloc.type & O65_RELOC_TYPE) {
        case O65_RELOC_WORD:        printf("WORD"); break;
        case O65_RELOC_LOW:         printf("LOW"); break;
        case O65_RELOC_SEGADR:      printf("SEGADR"); break;

        case O65_RELOC_HIGH:
            if ((header->mode & O65_MODE_PAGED) == 0)
                printf("HIGH %02x", reloc.extra);
            else
                printf("HIGH");
            break;

        case O65_RELOC_SEG:
            printf("SEG %04x", reloc.extra);
            break;

        default:
            printf("RELOC-%02x", reloc.type & O65_RELOC_TYPE);
            break;
        }
        printf("\n");
    }
    return 1;
}

static int dump_exported_symbols(FILE *file, const o65_header_t *header)
{
    char segname[O65_NAME_MAX];
    o65_size_t index;
    o65_size_t count;
    o65_size_t value;
    int result;
    int ch;

    /* Read the number of exported symbols */
    if (o65_read_count(file, header, &count) < 0)
        return -1;

    /* This is easy if there are no undefined symbols */
    if (count == 0) {
        printf("\nExported Symbols: none\n");
        return 1;
    }

    /* Dump the names of the undefined symbols */
    printf("\nExported Symbols:\n");
    for (index = 0; index < count; ++index) {
        /* Dump the name of the symbol */
        printf("    ");
        result = dump_nul_string(file);
        if (result <= 0)
            return result;

        /* Dump the segment identifier for the symbol */
        if ((ch = getc(file)) == EOF)
            return -1;
        o65_get_segment_name(ch, segname);
        printf(", %s", segname);

        /* Dump the value for the symbol */
        if (o65_read_count(file, header, &value) < 0)
            return -1;
        if ((header->mode & O65_MODE_32BIT) == 0)
            printf(", 0x%04lx\n", (unsigned long)value);
        else
            printf(", 0x%08lx\n", (unsigned long)value);
    }
    return 1;
}

static int dump_image(FILE *file, const o65_header_t *header)
{
    o65_option_t option;
    char cpu[O65_NAME_MAX];
    int result;
    int have_options;

    /* Dump the fields in the header */
    printf("Header:\n");
    printf("    mode  = 0x%04x (", header->mode);
    o65_get_cpu_name(header->mode, cpu);
    printf("%s", cpu);
    if (header->mode & O65_MODE_PAGED)
        printf(", pagewise relocation");
    if (header->mode & O65_MODE_32BIT)
        printf(", 32-bit addresses");
    else
        printf(", 16-bit addresses");
    if (header->mode & O65_MODE_OBJ)
        printf(", obj");
    else
        printf(", exe");
    if (header->mode & O65_MODE_SIMPLE)
        printf(", simple");
    if (header->mode & O65_MODE_CHAIN)
        printf(", chain");
    if (header->mode & O65_MODE_BSSZERO)
        printf(", bsszero");
    switch (header->mode & O65_MODE_ALIGN) {
    case O65_MODE_ALIGN_1:   printf(", byte alignment"); break;
    case O65_MODE_ALIGN_2:   printf(", word alignment"); break;
    case O65_MODE_ALIGN_4:   printf(", long alignment"); break;
    case O65_MODE_ALIGN_256: printf(", page alignment"); break;
    }
    printf(")\n");
    if ((header->mode & O65_MODE_32BIT) != 0) {
        printf("    tbase = 0x%08lx\n", (unsigned long)(header->tbase));
        printf("    tlen  = 0x%08lx\n", (unsigned long)(header->tlen));
        printf("    dbase = 0x%08lx\n", (unsigned long)(header->dbase));
        printf("    dlen  = 0x%08lx\n", (unsigned long)(header->dlen));
        printf("    bbase = 0x%08lx\n", (unsigned long)(header->bbase));
        printf("    blen  = 0x%08lx\n", (unsigned long)(header->blen));
        printf("    zbase = 0x%08lx\n", (unsigned long)(header->zbase));
        printf("    zlen  = 0x%08lx\n", (unsigned long)(header->zlen));
        printf("    stack = 0x%08lx\n", (unsigned long)(header->stack));
    } else {
        printf("    tbase = 0x%04x\n", header->tbase);
        printf("    tlen  = 0x%04x\n", header->tlen);
        printf("    dbase = 0x%04x\n", header->dbase);
        printf("    dlen  = 0x%04x\n", header->dlen);
        printf("    bbase = 0x%04x\n", header->bbase);
        printf("    blen  = 0x%04x\n", header->blen);
        printf("    zbase = 0x%04x\n", header->zbase);
        printf("    zlen  = 0x%04x\n", header->zlen);
        printf("    stack = 0x%04x\n", header->stack);
    }

    /* Read and dump the header options */
    have_options = 0;
    for (;;) {
        result = o65_read_option(file, &option);
        if (result <= 0)
            return result;
        if (option.len == 0)
            break;
        if (!have_options) {
            printf("\nOptions:\n");
            have_options = 1;
        }
        dump_option(&option);
    }

    /* Dump the contents of the text and data segments */
    result = dump_segment(file, ".text", header, header->tbase, header->tlen, 1);
    if (result <= 0)
        return result;
    result = dump_segment(file, ".data", header, header->dbase, header->dlen, 0);
    if (result <= 0)
        return result;

    /* Dump any undefined symbols */
    result = dump_undefined_symbols(file, header);
    if (result <= 0)
        return result;

    /* Dump the relocation tables for the text and data segments */
    result = dump_relocs(file, ".text", header, header->tbase);
    if (result <= 0)
        return result;
    result = dump_relocs(file, ".data", header, header->dbase);
    if (result <= 0)
        return result;

    /* Dump the list of exported symbols */
    return dump_exported_symbols(file, header);
}

static int dump_file(const char *filename)
{
    FILE *file;
    o65_header_t header;
    int result;

    /* Try to open the file */
    if ((file = fopen(filename, "rb")) == NULL) {
        perror(filename);
        return 0;
    }

    /* Dump the file's contents.  There may be multiple chained images. */
    do {
        /* Read and validate the ".o65" file header */
        result = o65_read_header(file, &header);
        if (result < 0) {
            file_error(file, filename);
            return 0;
        } else if (result == 0) {
            fprintf(stderr, "%s: not in .o65 format\n", filename);
            fclose(file);
            return 0;
        }

        /* Dump the contents of this image in the chain. */
        result = dump_image(file, &header);
        if (result < 0) {
            file_error(file, filename);
            return 0;
        } else if (result == 0) {
            fprintf(stderr, "%s: invalid format\n", filename);
            fclose(file);
            return 0;
        }

        /* Print a separator if there is another image in the chain. */
        if ((header.mode & O65_MODE_CHAIN) != 0) {
            printf("\n");
        }
    } while ((header.mode & O65_MODE_CHAIN) != 0);

    /* Done */
    fclose(file);
    return 1;
}
