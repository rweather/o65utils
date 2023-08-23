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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>

#define short_options "t:d:b:z:i:"
static struct option long_options[] = {
    {"text-address",        required_argument,  0,  't'},
    {"data-address",        required_argument,  0,  'd'},
    {"bss-address",         required_argument,  0,  'b'},
    {"zeropage-address",    required_argument,  0,  'z'},
    {"imports",             required_argument,  0,  'i'},
    {0,                     0,                  0,    0},
};

/** Information about an imported symbol */
typedef struct import_info_s import_info_t;
struct import_info_s
{
    /** Name of the symbol */
    char *name;

    /** Value of the symbol */
    o65_size_t value;

    /** Next import in the list */
    import_info_t *next;
};

/** Information to use when relocating an image */
typedef struct
{
    /** Header that was loaded from the image */
    o65_header_t header;

    /** Address to load the .text segment to */
    o65_size_t load_text_address;

    /** Address to load the .data segment to, 0 for default location */
    o65_size_t load_data_address;

    /** Address to load the .bss segment to, 0 for default location */
    o65_size_t load_bss_address;

    /** Alignment to use on the image's .text, .data, and .bss segments */
    o65_size_t alignment;

    /** Address to load the .text segment to */
    o65_size_t text_address;

    /** Size of the .text segment after alignment */
    o65_size_t text_size;

    /** Contents of the .text segment */
    uint8_t *text_segment;

    /** Address to load the .data segment to */
    o65_size_t data_address;

    /** Size of the .data segment after alignment */
    o65_size_t data_size;

    /** Size of the .data segment plus .bss if .bss needs to be zeroed */
    o65_size_t data_plus_bss_size;

    /** Contents of the .data segment */
    uint8_t *data_segment;

    /** Address to load the .bss segment to */
    o65_size_t bss_address;

    /** Size of the .bss segment after alignment */
    o65_size_t bss_size;

    /** Address to load the zero page segment to */
    o65_size_t zeropage_address;

    /** Number of external references */
    o65_size_t num_externs;

    /** Resolved addresses for the external references */
    o65_size_t *externs;

    /** List of imported symbols to resolve external references */
    import_info_t *imports;

} reloc_info_t;

static void usage(const char *progname);
static void file_error(FILE *file, const char *filename);
static int load(reloc_info_t *info, FILE *file, const char *filename);
static int load_imports(reloc_info_t *info, const char *filename);
static void free_imports(reloc_info_t *info);

int main(int argc, char *argv[])
{
    const char *progname = argv[0];
    const char *input_file = 0;
    const char *output_file = 0;
    const char *data_output_file = 0;
    const char *imports_file = 0;
    reloc_info_t info = {
        .alignment = 1
    };
    FILE *infile;
    FILE *outfile;
    int result;

    /* Parse the command-line options */
    for (;;) {
        int opt = getopt_long(argc, argv, short_options, long_options, 0);
        if (opt < 0)
            break;
        switch (opt) {
        case 't':
            info.load_text_address = strtoul(optarg, NULL, 0);
            if (info.load_text_address == 0U) {
                fprintf(stderr, "%s: text load address cannot be zero\n", progname);
                return 1;
            }
            break;

        case 'd':
            info.load_data_address = strtoul(optarg, NULL, 0);
            break;

        case 'b':
            info.load_bss_address = strtoul(optarg, NULL, 0);
            break;

        case 'z':
            info.zeropage_address = strtoul(optarg, NULL, 0);
            if (info.zeropage_address >= 256U) {
                fprintf(stderr, "%s: invalid zero page address\n", progname);
                return 1;
            }
            break;

        case 'i': imports_file = optarg; break;

        default:
            usage(progname);
            return 1;
        }
    }

    /* Need two or three filenames */
    if ((argc - optind) < 2) {
        usage(progname);
        return 1;
    }
    input_file = argv[optind];
    output_file = argv[optind + 1];
    if ((argc - optind) >= 3) {
        data_output_file = argv[optind + 2];
    }

    /* Load the imports file */
    if (imports_file) {
        result = load_imports(&info, imports_file);
        if (result <= 0)
            return 1;
    }

    /* Open the input .o65 file and read the header */
    if ((infile = fopen(input_file, "rb")) == NULL) {
        perror(input_file);
        return 1;
    }
    result = o65_read_header(infile, &info.header);
    if (result < 0) {
        perror(input_file);
        fclose(infile);
        free_imports(&info);
        return 1;
    } else if (result == 0) {
        fprintf(stderr, "%s: not in .o65 format\n", input_file);
        fclose(infile);
        free_imports(&info);
        return 1;
    }

    /* Load the input file */
    result = load(&info, infile, input_file);
    if (result < 0)
        file_error(infile, input_file);
    else if (result == 0)
        fprintf(stderr, "%s: file is invalid\n", input_file);

    /* Write the relocated data to the output file(s) */
    if (result > 0) {
        if ((outfile = fopen(output_file, "wb")) == NULL) {
            perror(output_file);
            result = -1;
        } else {
            if (fwrite(info.text_segment, 1, info.text_size, outfile)
                    != info.text_size) {
                perror(output_file);
                result = -1;
                fclose(outfile);
            } else if (!data_output_file) {
                /* Write the .data segment to the same file as .text */
                if (fwrite(info.data_segment, 1, info.data_plus_bss_size, outfile)
                        != info.data_plus_bss_size) {
                    perror(output_file);
                    result = -1;
                }
                fclose(outfile);
            } else {
                /* Write the .data segment to a different file */
                fclose(outfile);
                if ((outfile = fopen(data_output_file, "wb")) == NULL) {
                    perror(data_output_file);
                    result = -1;
                } else {
                    if (fwrite(info.data_segment, 1, info.data_plus_bss_size, outfile)
                            != info.data_plus_bss_size) {
                        perror(data_output_file);
                        result = -1;
                    }
                    fclose(outfile);
                }
            }
        }
    }

    /* Clean up and exit */
    if (info.text_segment)
        free(info.text_segment);
    if (info.data_segment)
        free(info.data_segment);
    if (info.externs)
        free(info.externs);
    free_imports(&info);
    fclose(infile);
    return (result <= 0) ? 1 : 0;
}

/**
 * @brief Print usage information for the program.
 *
 * @param[in] progname Name of the program from argv[0].
 */
static void usage(const char *progname)
{
    fprintf(stderr, "Usage: %s [options] input.o65 output.bin [data-output.bin]\n\n", progname);

    fprintf(stderr, "    --text-address ADDRESS, -t ADDRESS\n");
    fprintf(stderr, "        Address to load the text segment to on the target system.\n");
    fprintf(stderr, "        Defaults to the text address from the input file.\n\n");

    fprintf(stderr, "    --data-address ADDRESS, -d ADDRESS\n");
    fprintf(stderr, "        Address to load the data segment to on the target system.\n");
    fprintf(stderr, "        Defaults to just after the text segment.\n\n");

    fprintf(stderr, "    --bss-address ADDRESS, -b ADDRESS\n");
    fprintf(stderr, "        Address to load the bss segment to on the target system.\n");
    fprintf(stderr, "        Defaults to just after the data segment.\n\n");

    fprintf(stderr, "    --zeropage-address ADDRESS, -z ADDRESS\n");
    fprintf(stderr, "        Address to load the zero page segment to; default is 0.\n\n");

    fprintf(stderr, "    --imports IMPFILE, -i IMPFILE\n");
    fprintf(stderr, "        File with a list of import addresses to resolve externals.\n\n");
}

/**
 * @brief Report an error from loading the input file.
 *
 * @param[in] file The file pointer.
 * @param[in] filename Name of the file.
 */
static void file_error(FILE *file, const char *filename)
{
    if (feof(file))
        fprintf(stderr, "%s: unexpected EOF\n", filename);
    else
        perror(filename);
    fclose(file);
}

/**
 * @brief Aligns a size value.
 *
 * @param[in] size Size value to be aligned.
 * @param[in] alignment Alignment to use, which must be a power of 2.
 *
 * @return The aligned size value.
 */
static o65_size_t align_size(o65_size_t size, o65_size_t alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}

/**
 * @brief Lay out the sections of the image into their final locations.
 *
 * @param[in,out] info Relocation information for the file.
 *
 * @return Non-zero if OK, or zero if out of memory.
 */
static int layout_image(reloc_info_t *info)
{
    /* Set the address and size of the .text segment */
    info->text_address = info->load_text_address;
    info->text_size = align_size(info->header.tlen, info->alignment);

    /* Set the address and size of the .data segment */
    if (info->load_data_address) {
        info->data_address = info->load_data_address;
    } else {
        info->data_address = info->text_address + info->text_size;
    }
    info->data_size = align_size(info->header.dlen, info->alignment);
    info->data_plus_bss_size = info->data_size;

    /* Set the address and size of the .bss segment.  We ignore the
     * load address override if the "bsszero" mode is set because we
     * need to clear that region with zeroes as part of the final
     * relocated image.  We cannot do that if .bss is located elsewhere. */
    info->bss_size = align_size(info->header.blen, info->alignment);
    if ((info->header.mode & O65_MODE_BSSZERO) != 0) {
        info->bss_address = info->data_address + info->data_size;
        info->data_plus_bss_size += info->bss_size;
    } else if (info->load_bss_address) {
        info->bss_address = info->load_bss_address;
    } else {
        info->bss_address = info->data_address + info->data_size;
    }

    /* Allocate memory for the segments, cleared to zeroes initially */
    info->text_segment = calloc(info->text_size ? info->text_size : 1, 1);
    info->data_segment = calloc
        (info->data_plus_bss_size ? info->data_plus_bss_size : 1, 1);
    if (info->text_segment && info->data_segment)
        return 1;
    else
        return 0;
}

/**
 * @brief Resolve external references.
 *
 * @param[in,out] info Relocation information for the file.
 * @param[in] file File to load from.
 * @param[in] filename Name of the file to load from, for error reporting.
 *
 * @return 1 on success, 0 if the file is invalid, and -1 on unexpected EOF
 * or a filesystem error.
 */
static int resolve_extern(reloc_info_t *info, FILE *file, const char *filename)
{
    char name[O65_STRING_MAX];
    o65_size_t index;
    import_info_t *import;
    int result;
    int ok;

    /* Read the number of external references in the file */
    result = o65_read_count(file, &(info->header), &(info->num_externs));
    if (result <= 0)
        return result;

    /* Nothing to do if the count is zero */
    if (info->num_externs == 0)
        return 1;

    /* Allocate a table to hold the resolved addresses */
    info->externs = calloc(info->num_externs, sizeof(o65_size_t));
    if (!(info->externs))
        return -1;

    /* Load the names of the externals and resolve them */
    ok = 1;
    for (index = 0; index < info->num_externs; ++index) {
        /* Read the next string from the file */
        result = o65_read_string(file, name, sizeof(name));
        if (result < 0) {
            break;
        } else if (result == 0) {
            fprintf(stderr, "%s: warning: symbol name '%s' was truncated\n",
                    filename, name);
        }

        /* Find the name in the imports list */
        import = info->imports;
        while (import != NULL) {
            if (!strcmp(import->name, name))
                break;
            import = import->next;
        }
        if (import != NULL) {
            info->externs[index] = import->value;
        } else {
            fprintf(stderr, "%s: unresolved external reference '%s'\n",
                    filename, name);
            ok = 0;
        }
    }
    return ok;
}

/**
 * @brief Resolve external references.
 *
 * @param[in,out] info Relocation information for the file.
 * @param[in] file File to load from.
 * @param[in] filename Name of the file to load from, for error reporting.
 * @param[in] data Points to the segment data to patch using relocations.
 * @param[in] size Size of the segment.
 *
 * @return 1 on success, 0 if the file is invalid, and -1 on unexpected EOF
 * or a filesystem error.
 */
static int relocate_segment
    (reloc_info_t *info, FILE *file, const char *filename,
     uint8_t *data, o65_size_t size)
{
    o65_reloc_t reloc;
    o65_size_t addr;
    o65_size_t adjust;
    o65_size_t vector;
    int result;

    /* Relocations actually start at the segment base - 1 */
    addr = ~((o65_size_t)0);

    /* Read and apply all relocations for the segment */
    for (;;) {
        /* Read the next relocation entry */
        result = o65_read_reloc(file, &(info->header), &reloc);
        if (result <= 0)
            return result;
        else if (reloc.offset == 0)
            break;

        /* Process skip relocations which advance by 254 bytes only */
        if (reloc.offset == 255) {
            addr += 254;
            continue;
        }

        /* Find the address to apply the relocation at */
        addr += reloc.offset;

        /* Get the adjustment to apply based on the segment ID */
        switch (reloc.type & O65_RELOC_SEGID) {
        case O65_SEGID_UNDEF:
            if (reloc.undefid < info->num_externs) {
                adjust = info->externs[reloc.undefid];
            } else {
                fprintf(stderr, "%s: invalid external reference %lu\n",
                        filename, (unsigned long)(reloc.undefid));
                return 0;
            }
            break;

        case O65_SEGID_TEXT:
            adjust = info->text_address - info->header.tbase;
            break;

        case O65_SEGID_DATA:
            adjust = info->data_address - info->header.dbase;
            break;

        case O65_SEGID_BSS:
            adjust = info->bss_address - info->header.bbase;
            break;

        case O65_SEGID_ZEROPAGE:
            adjust = info->zeropage_address - info->header.zbase;
            break;

        default:
            /* ABS and other segment ID's are not allowed in relocations */
            fprintf(stderr, "%s: invalid relocation segment ID %d\n",
                    filename, reloc.type & O65_RELOC_SEGID);
            return 0;
        }

        /* Apply the relocation.  See the ".o65" format spec for details:
         * http://www.6502.org/users/andre/o65/fileformat.html */
        if (addr >= size) {
            fprintf(stderr, "%s: relocation is out of range\n", filename);
            return 0;
        }
        switch (reloc.type & O65_RELOC_TYPE) {
        case O65_RELOC_WORD:
            /* 16-bit word address */
            if ((addr + 1) >= size) {
                fprintf(stderr, "%s: word relocation is out of range\n", filename);
                return 0;
            }
            vector = o65_read_uint16(data + addr);
            vector += adjust;
            o65_write_uint16(data + addr, (uint16_t)vector);
            break;

        case O65_RELOC_SEGADR:
            /* 24-bit segment address */
            if ((addr + 2) >= size) {
                fprintf(stderr, "%s: segment address relocation is out of range\n", filename);
                return 0;
            }
            vector = o65_read_uint24(data + addr);
            vector += adjust;
            o65_write_uint24(data + addr, vector);
            break;

        case O65_RELOC_HIGH:
            /* High byte from the code, low byte from the relocation */
            vector = (((uint16_t)(data[addr])) << 8) | reloc.extra;
            vector += adjust;
            data[addr] = (uint8_t)(vector >> 8);
            break;

        case O65_RELOC_LOW:
            /* Low byte from the code, high byte is irrelevant */
            vector = data[addr];
            vector += adjust;
            data[addr] = (uint8_t)vector;
            break;

        case O65_RELOC_SEG:
            /* Segment byte from the code, low 16 bits from the relocation */
            vector = (((uint32_t)(data[addr])) << 16) | reloc.extra;
            vector += adjust;
            data[addr] = (uint8_t)(vector >> 16);
            break;
        }
    }
    return 1;
}

/**
 * @brief Load the input file and relocate it.
 *
 * @param[in,out] info Relocation information for the file.
 * @param[in] file File to load from.
 * @param[in] filename Name of the file to load from, for error reporting.
 *
 * @return 1 on success, 0 if the file is invalid, and -1 on unexpected EOF
 * or a filesystem error.
 */
static int load(reloc_info_t *info, FILE *file, const char *filename)
{
    o65_option_t option;
    int result;

    /* Skip any header options that are present */
    for (;;) {
        result = o65_read_option(file, &option);
        if (result <= 0)
            return result;
        if (option.len == 0)
            break;
    }

    /* Must be an executable, not an object file, to be able to relocate it */
    if (info->header.mode & O65_MODE_OBJ) {
        fprintf(stderr, "%s: cannot relocate object files\n", filename);
        return 0;
    }

    /* Pick a default load address for the .text segment */
    if (!info->load_text_address) {
        info->load_text_address = info->header.tbase;
        if (!info->load_text_address) {
            fprintf(stderr, "%s: text load address cannot be zero\n", filename);
            return 0;
        }
    }

    /* Select the segment alignment and validate the load address */
    switch (info->header.mode & O65_MODE_ALIGN) {
    case O65_MODE_ALIGN_1:   info->alignment = 1; break;
    case O65_MODE_ALIGN_2:   info->alignment = 2; break;
    case O65_MODE_ALIGN_4:   info->alignment = 4; break;
    case O65_MODE_ALIGN_256: info->alignment = 256; break;
    }
    if (info->header.mode & O65_MODE_PAGED) {
        /* Override the alignment value and force page alignment */
        info->alignment = 256;
    }
    if ((info->load_text_address & (~(info->alignment - 1))) != info->load_text_address) {
        fprintf(stderr, "%s: text load address 0x%lx is not aligned on a %d-byte boundary\n",
                filename, (unsigned long)info->load_text_address, (int)info->alignment);
        return 0;
    }

    /* Check the alignment of the .data and .bss segments */
    if ((info->load_data_address & (~(info->alignment - 1))) != info->load_data_address) {
        fprintf(stderr, "%s: data load address 0x%lx is not aligned on a %d-byte boundary\n",
                filename, (unsigned long)info->load_data_address, (int)info->alignment);
        return 0;
    }
    if ((info->load_bss_address & (~(info->alignment - 1))) != info->load_bss_address) {
        fprintf(stderr, "%s: bss load address 0x%lx is not aligned on a %d-byte boundary\n",
                filename, (unsigned long)info->load_bss_address, (int)info->alignment);
        return 0;
    }

    /* Lay out the segments into their final locations */
    if (!layout_image(info))
        return -1;

    /* Load the contents of the .text and .data segments from the file */
    if (info->header.tlen) {
        if (fread(info->text_segment, 1, info->header.tlen, file)
                != info->header.tlen) {
            return -1;
        }
    }
    if (info->header.dlen) {
        if (fread(info->data_segment, 1, info->header.dlen, file)
                != info->header.dlen) {
            return -1;
        }
    }

    /* Load and resolve the external references list */
    result = resolve_extern(info, file, filename);
    if (result <= 0)
        return result;

    /* Relocate the .text segment */
    result = relocate_segment
        (info, file, filename, info->text_segment, info->text_size);
    if (result <= 0)
        return result;

    /* Relocate the .data segment */
    result = relocate_segment
        (info, file, filename, info->data_segment, info->data_size);
    if (result <= 0)
        return result;

    /* The rest of the file contains exported symbols from this image.
     * Ignore because we cannot encode exported symbols in ".bin" format. */

    /* Done */
    return 1;
}

/**
 * @brief Loads the list of imports from a file.
 *
 * @param[in,out] info Relocation information to populate with the imports.
 * @param[in] filename Name of the imports file.
 *
 * @return 1 on success, 1 on a filesystem error.
 */
static int load_imports(reloc_info_t *info, const char *filename)
{
    char buf[BUFSIZ];
    FILE *file;
    size_t len;
    size_t posn;
    import_info_t *import;

    /* Open the imports file */
    if ((file = fopen(filename, "r")) == NULL) {
        perror(filename);
        return -1;
    }

    /* Read the contents of the imports file.  Each line should be
     * formatted as "name value".  Invalid lines are ignored. */
    while (fgets(buf, sizeof(buf), file)) {
        /* Strip whitespace from the end of the line */
        len = strlen(buf);
        while (len > 0 && isspace(buf[len - 1]))
            --len;
        buf[len] = '\0';

        /* If the line is empty or starts with '#', then it is a comment */
        if (buf[0] == '\0' || buf[0] == '#')
            continue;

        /* Split the line into name and value */
        posn = 0;
        while (buf[posn] != '\0' && !isspace(buf[posn]))
            ++posn;
        if (buf[posn] == '\0')
            continue; /* No value present; ignore this line */
        buf[posn++] = '\0';

        /* Allocate space for a new import definition */
        import = calloc(1, sizeof(import_info_t));
        import->name = strdup(buf);
        import->value = strtoul(buf + posn, NULL, 0);
        import->next = info->imports;
        info->imports = import;
    }

    /* Done */
    fclose(file);
    return 1;
}

/**
 * @brief Frees the list of imports.
 *
 * @param[in,out] info Relocation information containing the import list.
 */
static void free_imports(reloc_info_t *info)
{
    import_info_t *current = info->imports;
    import_info_t *next;
    while (current != 0) {
        next = current->next;
        free(current->name);
        free(current);
        current = next;
    }
    info->imports = NULL;
}
