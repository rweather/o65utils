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

#include <elf.h>
#include <libelf.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include "o65file.h"
#include "elfmos.h"

#define short_options "a:bdhl:o:s:"
static struct option long_options[] = {
    {"author-name",         required_argument,  0,  'a'},
    {"bss-zero",            no_argument,        0,  'b'},
    {"creation-date",       no_argument,        0,  'd'},
    {"hosted",              no_argument,        0,  'h'},
    {"linker-name",         required_argument,  0,  'l'},
    {"os-info",             required_argument,  0,  'o'},
    {"stack-size",          required_argument,  0,  's'},
    {0,                     0,                  0,    0},
};

/**
 * @brief Information about an image that is being converted to ".o65".
 */
typedef struct
{
    /** ELF file descriptor */
    Elf *elf;

    /** File descriptor for the underlying input file */
    int fd;

    /** General purpose flag for section callbacks */
    int flag;

    /** Name of the ELF file, for error reporting */
    const char *filename;

    /** File pointer for the output file */
    FILE *outfile;

    /** Header information for the final ".o65" file. */
    o65_header_t header;

    /** ELF machine option to add to the final file. */
    o65_option_t elf_machine;

    /** Identifier of the operating system as a header option. */
    o65_option_t os;

    /** Name of the author as a header option. */
    o65_option_t author;

    /** Name of the linker to add as a header option. */
    o65_option_t linker;

    /** Non-zero to add the creation date to the output file. */
    int add_creation_date;

    /** Creation date and time to add as a header option. */
    o65_option_t created;

    /** Entry point for the executable. */
    o65_size_t entry_point;

    /** Data for the .text segment. */
    uint8_t *text_segment;

    /** Address of the .text segment. */
    o65_size_t text_address;

    /** Size of the .text segment. */
    o65_size_t text_size;

    /** Data for the .data segment. */
    uint8_t *data_segment;

    /** Address of the .data segment. */
    o65_size_t data_address;

    /** Size of the .data segment. */
    o65_size_t data_size;

    /** Address of the .bss segment. */
    o65_size_t bss_address;

    /** Size of the .bss segment. */
    o65_size_t bss_size;

    /** Address of the .zp segment. */
    o65_size_t zeropage_address;

    /** Size of the .zp segment. */
    o65_size_t zeropage_size;

    /** Buffer containing all relocations in ".o65" format. */
    o65_reloc_t *reloc;

    /** Current size of the relocation buffer. */
    o65_size_t reloc_size;

    /** Allocation size of the relocation buffer. */
    o65_size_t reloc_alloc_size;

    /** Number of bytes of .text relocation data.  The rest is for .data. */
    o65_size_t text_reloc_size;

    /** Address of the last relocation that was performed. */
    o65_size_t last_reloc_address;

    /** Current segment that is being relocated. */
    uint8_t reloc_segment;

    /** Index of the section that contains the section header string table. */
    size_t hstrtab;

    /** Points to the data for the regular string table, for resolving
     *  the names of symbols. */
    Elf_Data *strtab;

    /** Points to the symbol table in the ELF file. */
    Elf32_Sym *symbols;

    /** Number of symbols in the ELF file. */
    size_t num_symbols;

    /** Array of ELF symbol name identifiers for undefined references. */
    Elf32_Word *undef_name_ids;

    /** Names of the ELF symbols for undefined references. */
    char **undef_names;

    /** Number of undefined names that have been added to ".o65" so far. */
    size_t num_undef_names;

    /** Maximum number of undefined names before reallocating the table. */
    size_t max_undef_names;

    /** Non-zero for "hosted" mode where the runtime loader provides the
     *  addresses of the llvm-mos imaginary registers. */
    int hosted;

} image_info_t;

static void usage(const char *progname);
static int set_os_option(image_info_t *info, const char *str);
static void free_image(image_info_t *info);
static int validate_elf(image_info_t *info);
static int load_segments(image_info_t *info);
static int convert_relocations(image_info_t *info);
static int write_o65(image_info_t *info, const char *filename);

int main(int argc, char *argv[])
{
    const char *progname = argv[0];
    image_info_t info = {
        .text_size = 0
    };
    const char *input_file;
    const char *output_file;
    char output_file_buf[BUFSIZ];
    int fd;
    int bsszero = 0;
    Elf *elf;

    /* Parse the command-line options */
    for (;;) {
        int opt = getopt_long(argc, argv, short_options, long_options, 0);
        if (opt < 0)
            break;
        switch (opt) {
        case 'a':
            o65_set_string_option
                (&(info.author), O65_OPT_AUTHOR, optarg, strlen(optarg));
            break;

        case 'b': bsszero = 1; break;
        case 'd': info.add_creation_date = 1; break;
        case 'h': info.hosted = 1; break;

        case 'l':
            o65_set_string_option
                (&(info.linker), O65_OPT_PROGRAM, optarg, strlen(optarg));
            break;

        case 'o':
            if (!set_os_option(&info, optarg)) {
                fprintf(stderr, "%s: invalid os information '%s'\n",
                        progname, optarg);
                return 1;
            }
            break;

        case 's':
            info.header.stack = strtoul(optarg, NULL, 0);
            break;

        default:
            usage(progname);
            return 1;
        }
    }

    /* Need one or two filenames */
    if ((argc - optind) < 1) {
        usage(progname);
        return 1;
    }
    input_file = argv[optind];
    if ((argc - optind) >= 2) {
        output_file = argv[optind + 1];
    } else {
        /* Synthesise an output filename by removing .elf from the input name,
         * or by adding .o65 if the input filename doesn't end in .elf. */
        size_t len = strlen(input_file);
        if (len > 4 && !strcmp(input_file + len - 4, ".elf")) {
            strncpy(output_file_buf, input_file, sizeof(output_file_buf));
            output_file_buf[len - 4] = '\0';
        } else {
            snprintf(output_file_buf, sizeof(output_file_buf),
                     "%s.o65", input_file);
        }
        output_file = output_file_buf;
    }

    /* Make sure that we are using the correct version of the ELF library */
    if (elf_version(EV_CURRENT) == EV_NONE) {
        fprintf(stderr, "%s is incompatible with the system libelf\n", argv[0]);
        return 1;
    }

    /* Open the input ELF file and fetch the header */
    fd = open(input_file, O_RDONLY, 0);
    if (fd < 0) {
        perror(input_file);
        return 1;
    }
    elf = elf_begin(fd, ELF_C_READ, NULL);
    if (!elf) {
        fprintf(stderr, "%s: %s\n", input_file, elf_errmsg(elf_errno()));
        close(fd);
        return 1;
    }

    /* Validate the ELF file for suitability to our purposes */
    info.elf = elf;
    info.filename = input_file;
    info.fd = fd;
    if (!validate_elf(&info)) {
        free_image(&info);
        return 1;
    }
    if (bsszero) {
        /* Force the .bss segment to be zero'ed */
        info.header.mode |= O65_MODE_BSSZERO;
    }

    /* Load the segments into memory and get their positions and sizes */
    if (!load_segments(&info)) {
        free_image(&info);
        return 1;
    }

    /* Convert the relocations into ".o65" form */
    if (!convert_relocations(&info)) {
        free_image(&info);
        return 1;
    }

    /* Write the output ".o65" file */
    if (!write_o65(&info, output_file)) {
        perror(output_file);
        free_image(&info);
        return 1;
    }

    /* Clean up and exit */
    free_image(&info);
    return 0;
}

/**
 * @brief Print usage information for the program.
 *
 * @param[in] progname Name of the program from argv[0].
 */
static void usage(const char *progname)
{
    fprintf(stderr, "Usage: %s [options] input.elf [output.o65]\n\n", progname);

    fprintf(stderr, "    --author-name AUTHOR, -a AUTHOR\n");
    fprintf(stderr, "        Set the name of the author in the header options.\n\n");

    fprintf(stderr, "    --bss-zero, -b\n");
    fprintf(stderr, "        Force the bss segment to be zeroed by the OS.\n\n");

    fprintf(stderr, "    --creation-date, -d\n");
    fprintf(stderr, "        Add the file creation date in the header options.\n\n");

    fprintf(stderr, "    --hosted, -h\n");
    fprintf(stderr, "        Hosted mode, where the runtime loader provides the\n");
    fprintf(stderr, "        addresses of the llvm-mos imaginary registers.\n\n");

    fprintf(stderr, "    --linker-name LINKER, -l LINKER\n");
    fprintf(stderr, "        Set the name of the linker in the header options.\n\n");

    fprintf(stderr, "    --os-info 'HEXBYTES', -o 'HEXBYTES'\n");
    fprintf(stderr, "        Sets the operating system header option.\n\n");

    fprintf(stderr, "    --stack-size NUM, -s NUM\n");
    fprintf(stderr, "        Declare the size of the stack to the operating system.\n\n");
}

/**
 * @brief Set the operating system option to a sequence of hex bytes.
 *
 * @param[in,out] info Information about the image we are converting.
 * @param[in] str Hex string to be converted.
 *
 * @return Non-zero if ok, or zero if there is something wrong with @a str.
 */
static int set_os_option(image_info_t *info, const char *str)
{
    int value = 0;
    int nibble = 0;
    info->os.len = 2;
    info->os.type = O65_OPT_OS;
    while (*str != '\0') {
        int ch = *str++;
        if (ch >= '0' && ch <= '9') {
            value = value * 16 + (ch - '0');
        } else if (ch >= 'A' && ch <= 'F') {
            value = value * 16 + (ch - 'A' + 10);
        } else if (ch >= 'a' && ch <= 'f') {
            value = value * 16 + (ch - 'a' + 10);
        } else if (ch == ' ') {
            if (!nibble)
                continue;
        } else {
            return 0;
        }
        nibble = !nibble;
        if (!nibble) {
            if (info->os.len >= O65_MAX_OPT_SIZE)
                return 0;
            info->os.data[info->os.len - 2] = (uint8_t)value;
            ++(info->os.len);
            value = 0;
        }
    }
    if (nibble) {
        if (info->os.len >= O65_MAX_OPT_SIZE)
            return 0;
        info->os.data[info->os.len - 2] = (uint8_t)value;
        ++(info->os.len);
    }
    return 1;
}

/**
 * @brief Frees the memory that was used by an image.
 *
 * @param[in,out] info Information about the image we are converting.
 */
static void free_image(image_info_t *info)
{
    if (info->outfile)
        fclose(info->outfile);
    elf_end(info->elf);
    close(info->fd);
    if (info->text_segment)
        free(info->text_segment);
    if (info->reloc)
        free(info->reloc);
    if (info->undef_name_ids)
        free(info->undef_name_ids);
    if (info->undef_names)
        free(info->undef_names);
}

/**
 * @brief Callback for iterating over the list of all sections in an ELF file.
 *
 * @param[in,out] info Information about the image we are converting.
 * @param[in] scn ELF section control structure.
 * @param[in] shdr ELF section header structure.
 * @param[in] name Name of the section, or NULL if unknown.
 */
typedef void (*section_iterator_callback_t)
    (image_info_t *info, Elf_Scn *scn, Elf32_Shdr *shdr, const char *name);

/**
 * @brief Iterate over all sections in an ELF file.
 *
 * @param[in,out] info Information about the image we are converting.
 * @param[in] type The type of section to look for; e.g. SHT_NOBITS.
 * Set this to SHT_NULL to iterate over all sections regardless of type.
 * @param[in] callback Function to call for each section that is found.
 */
static void section_iterator
    (image_info_t *info, unsigned type, section_iterator_callback_t callback)
{
    Elf_Scn *scn;
    Elf32_Shdr *shdr;
    const char *name;
    for (scn = elf_nextscn(info->elf, NULL); scn != NULL;
            scn = elf_nextscn(info->elf, scn)) {
        shdr = elf32_getshdr(scn);
        if (type == SHT_NULL || shdr->sh_type == type) {
            name = elf_strptr(info->elf, info->hstrtab, shdr->sh_name);
            (*callback)(info, scn, shdr, name);
        }
    }
}

/**
 * @brief Determine if a section is located in the zero page.
 *
 * @param[in] shdr ELF section header structure.
 * @param[in] name Name of the section.
 *
 * @return Non-zero if the section is in the zero page, zero if not.
 *
 * Some .zp sections have the SHF_MOS_ZEROPAGE bit, but not all.
 * We use the .zp prefix to detect actual zero page sections.
 */
static int is_zp_section(const Elf32_Shdr *shdr, const char *name)
{
    if (shdr->sh_flags & SHF_MOS_ZEROPAGE)
        return 1;
    if (!name)
        return 0;
    if (!strcmp(name, ".zp"))
        return 1;
    if (!strncmp(name, ".zp.", 4))
        return 1;
    return 0;
}

/**
 * @brief Maps the CPU type from ELF to ".o65".
 *
 * @param[in] elf_flags Flags for the ELF CPU type.
 *
 * @return The ".o65" CPU type.
 *
 * The ".o65" format doesn't have as many CPU types as ELF.  We map to the
 * closest match and just make do.  A separate header option is used to
 * communicate the full set of ELF machine flags.
 */
static uint16_t map_cpu_type(uint32_t elf_flags)
{
    if (elf_flags & EM_MOS_W65816) {
        return (O65_MODE_CPU_65816 | O65_MODE_CPU_65C02);
    }
    if (elf_flags & EM_MOS_65CE02) {
        return O65_MODE_CPU_65CE02;
    }
    if (elf_flags & (EM_MOS_R65C02 | EM_MOS_W65C02)) {
        return O65_MODE_CPU_65SC02;
    }
    if (elf_flags & EM_MOS_65C02) {
        return O65_MODE_CPU_65C02;
    }
    if (elf_flags & EM_MOS_6502X) {
        return O65_MODE_CPU_UNDOC;
    }
    return O65_MODE_CPU_6502;
}

/**
 * @brief Callback for finding the regular string table.
 *
 * @param[in,out] info Information about the image we are converting.
 * @param[in] scn ELF section control structure.
 * @param[in] shdr ELF section header structure.
 * @param[in] name Name of the section, or NULL if unknown.
 */
static void section_callback_string_table
    (image_info_t *info, Elf_Scn *scn, Elf32_Shdr *shdr, const char *name)
{
    (void)shdr;
    (void)name;
    info->strtab = elf_getdata(scn, NULL);
}

/**
 * @brief Callback for finding the symbol table.
 *
 * @param[in,out] info Information about the image we are converting.
 * @param[in] scn ELF section control structure.
 * @param[in] shdr ELF section header structure.
 * @param[in] name Name of the section, or NULL if unknown.
 */
static void section_callback_symbol_table
    (image_info_t *info, Elf_Scn *scn, Elf32_Shdr *shdr, const char *name)
{
    Elf_Data *data = elf_getdata(scn, NULL);
    (void)shdr;
    (void)name;
    if (!data || data->d_size < sizeof(Elf32_Sym))
        return;
    info->symbols = (Elf32_Sym *)(data->d_buf);
    info->num_symbols = data->d_size / sizeof(Elf32_Sym);
}

/**
 * @brief Validates an ELF file and loads basic information about it.
 *
 * @param[in,out] info Information about the image we are converting.
 *
 * @return Non-zero if the ELF file is suitable, or zero if not.
 */
static int validate_elf(image_info_t *info)
{
    Elf32_Ehdr *ehdr;
    Elf32_Phdr *phdr;
    size_t index, count;
    o65_size_t alignment;

    /* Get the ELF header and verify that it is suitable for MOS */
    ehdr = elf32_getehdr(info->elf);
    if (!ehdr) {
        fprintf(stderr, "%s: %s\n", info->filename, elf_errmsg(elf_errno()));
        return 0;
    }
    if (ehdr->e_machine != EM_MOS) {
        fprintf(stderr, "%s: ELF file is not suitable for MOS machines\n", info->filename);
        return 0;
    }

    /* Must be an executable, not an object file */
    if (ehdr->e_type != ET_EXEC) {
        fprintf(stderr, "%s: ELF file is not an executable\n", info->filename);
        return 0;
    }

    /* Map the CPU type to something ".o65" understands */
    info->header.mode = map_cpu_type(ehdr->e_flags);

    /* Set the ELF machine option if we don't have an exact CPU match */
    info->elf_machine.len = 8;
    info->elf_machine.type = O65_OPT_ELF_MACHINE;
    o65_write_uint16(info->elf_machine.data, ehdr->e_machine);
    o65_write_uint32(info->elf_machine.data + 2, ehdr->e_flags);

    /* Record some information from the ehdr for later */
    info->entry_point = ehdr->e_entry;

    /* Find the best alignment to use based on the loadable program headers */
    elf_getphdrnum(info->elf, &count);
    phdr = elf32_getphdr(info->elf);
    alignment = 1;
    for (index = 0; index < count; ++index) {
        if (phdr[index].p_type != PT_LOAD) {
            continue;
        }
        if (phdr[index].p_align > alignment) {
            alignment = phdr[index].p_align;
        }
    }

    /* Normalize the alignment to something allowed by ".o65".
     * The only allowable values are 1, 2, 4, and 256. .*/
    if (alignment > 4) {
        info->header.mode |= O65_MODE_ALIGN_256;
        info->header.mode |= O65_MODE_PAGED;
    } else if (alignment > 2) {
        info->header.mode |= O65_MODE_ALIGN_4;
    } else if (alignment > 1) {
        info->header.mode |= O65_MODE_ALIGN_2;
    } else {
        info->header.mode |= O65_MODE_ALIGN_1;
    }

    /* Find the section header string table */
    elf_getshdrstrndx(info->elf, &(info->hstrtab));

    /* Find the regular string table and the symbol table */
    section_iterator(info, SHT_STRTAB, section_callback_string_table);
    section_iterator(info, SHT_SYMTAB, section_callback_symbol_table);

    /* Done */
    return 1;
}

/**
 * @brief Callback for determining the size of .bss from all non-zp
 * NOBITS sections.
 *
 * @param[in,out] info Information about the image we are converting.
 * @param[in] scn ELF section control structure.
 * @param[in] shdr ELF section header structure.
 * @param[in] name Name of the section, or NULL if unknown.
 */
static void section_callback_bss_size
    (image_info_t *info, Elf_Scn *scn, Elf32_Shdr *shdr, const char *name)
{
    (void)scn;
    if (!is_zp_section(shdr, name)) {
        info->bss_size += shdr->sh_size;
    }
}

/**
 * @brief Callback for determining the size of the final zero page.
 *
 * @param[in,out] info Information about the image we are converting.
 * @param[in] scn ELF section control structure.
 * @param[in] shdr ELF section header structure.
 * @param[in] name Name of the section, or NULL if unknown.
 */
static void section_callback_zp_size
    (image_info_t *info, Elf_Scn *scn, Elf32_Shdr *shdr, const char *name)
{
    (void)scn;
    if (is_zp_section(shdr, name)) {
        info->zeropage_size += shdr->sh_size;
        if (shdr->sh_addr < info->zeropage_address)
            info->zeropage_address = shdr->sh_addr;
    }
}

/**
 * @brief Callback for determining the boundary between .text and .data.
 *
 * @param[in,out] info Information about the image we are converting.
 * @param[in] scn ELF section control structure.
 * @param[in] shdr ELF section header structure.
 * @param[in] name Name of the section, or NULL if unknown.
 */
static void section_callback_text_size
    (image_info_t *info, Elf_Scn *scn, Elf32_Shdr *shdr, const char *name)
{
    /* Figure out where the boundary between .text and .data lies.
     * We do this by looking for the first section with "WA" flags.
     * Earlier sections are assumed to be .text and .rodata. */
    (void)scn;
    (void)name;
    if (shdr->sh_flags == (SHF_WRITE | SHF_ALLOC) && !(info->flag)) {
        if (shdr->sh_addr >= info->text_address &&
                shdr->sh_addr <= (info->text_address + info->text_size)) {
            /* Reduce the .text segment in size and create .data */
            uint32_t offset = shdr->sh_addr - info->text_address;
            info->data_address = shdr->sh_addr;
            info->data_size = info->text_size - offset;
            info->text_size = offset;
        }
        info->flag = 1;
    }
}

/**
 * @brief Loads the .text, .data, .bss, and .zp segments.
 *
 * @param[in,out] info Information about the image we are converting.
 *
 * @return Non-zero if the segments were loaded, or zero on error.
 */
static int load_segments(image_info_t *info)
{
    Elf32_Phdr *phdr;
    Elf_Data *raw;
    size_t index, count;
    int first;

    /* Concatenate all loadable program headers */
    elf_getphdrnum(info->elf, &count);
    phdr = elf32_getphdr(info->elf);
    first = 1;
    for (index = 0; index < count; ++index, ++phdr) {
        if (phdr->p_type != PT_LOAD) {
            continue;
        }
        raw = elf_getdata_rawchunk
            (info->elf, phdr->p_offset, phdr->p_filesz, ELF_T_BYTE);
        if (!raw) {
            fprintf(stderr, "%s: cannot map program header %u\n",
                    info->filename, (unsigned)index);
            return 0;
        }
        if (first) {
            /* This is the first loadable program header.  Assume that its
             * base address is the same as the base of the .text segment */
            info->text_address = phdr->p_vaddr;
            info->text_size = phdr->p_memsz;
            info->text_segment = calloc(phdr->p_memsz, 1);
            memcpy(info->text_segment, raw->d_buf, phdr->p_filesz);
            first = 0;
        } else {
            /* Concatencate this loadable segment to the previous one */
            if (phdr->p_vaddr < (info->text_address + info->text_size)) {
                /* Addresses went backwards.  Probably a load segment
                 * for zero page definitions.  No more .text sections */
                break;
            } else if (phdr->p_vaddr > (info->text_address + info->text_size)) {
                fprintf(stderr, "%s: program headers are not contiguous\n",
                        info->filename);
                return 0;
            }
            info->text_segment = realloc
                (info->text_segment, info->text_size + phdr->p_memsz);
            memset(info->text_segment + info->text_size, 0, phdr->p_memsz);
            memcpy(info->text_segment + info->text_size,
                   raw->d_buf, phdr->p_filesz);
            info->text_size += phdr->p_memsz;
        }
    }

    /* Count up the bytes in "NOBITS" sections to figure out how
     * big the .bss segment will be. */
    section_iterator(info, SHT_NOBITS, section_callback_bss_size);

    /* Count up the bytes in all zero page sections to figure
     * out how big the .o65 zero page will be.  This also finds
     * the lowest address that is used in the zero page. */
    info->zeropage_address = 0x100;
    section_iterator(info, SHT_NULL, section_callback_zp_size);
    if (info->zeropage_address >= 0x100) {
        /* We didn't find any zeropage sections */
        info->zeropage_address = 0;
    }

    /* Figure out where the boundary between .text and .data lies */
    info->data_address = info->text_address + info->text_size;
    info->data_size = 0;
    info->flag = 0;
    section_iterator(info, SHT_PROGBITS, section_callback_text_size);
    info->data_segment = info->text_segment + info->text_size;

    /* Set up the positions and sizes of the segments in the ".o65" header */
    info->header.tbase = info->text_address;
    info->header.tlen = info->text_size;
    info->header.dbase = info->data_address;
    info->header.dlen = info->data_size;
    info->bss_address = info->data_address + info->data_size;
    info->header.bbase = info->bss_address;
    info->header.blen = info->bss_size;
    info->header.zbase = info->zeropage_address;
    info->header.zlen = info->zeropage_size;
    return 1;
}

/**
 * @brief Adds a ".o65" relocation to the final image.
 *
 * @param[in,out] info Information about the image we are converting.
 * @param[in] reloc The relocation to be added.
 */
static void add_o65_relocation(image_info_t *info, const o65_reloc_t *reloc)
{
    if (info->reloc_size >= info->reloc_alloc_size) {
        info->reloc_alloc_size += 256;
        info->reloc = (o65_reloc_t *)realloc
            (info->reloc, sizeof(o65_reloc_t) * info->reloc_alloc_size);
        if (!info->reloc) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
    }
    info->reloc[(info->reloc_size)++] = *reloc;
}

/**
 * @brief Resolves an undefined symbol to an index in the external
 * references table of the final ".o65" file.
 *
 * @param[in,out] info Information about the image we are converting.
 * @param[in] name Symbol table index for the name of the undefined symbol.
 * @param[out] id Returns the identifier for the undefined symbol in ".o65".
 *
 * @return Non-zero if the symbol was resolved, zero on error.
 */
static int resolve_undefined(image_info_t *info, Elf32_Word name, uint32_t *id)
{
    size_t index;
    char *symname;

    /* Do we already have a reference to this name? */
    for (index = 0; index < info->num_undef_names; ++index) {
        if (info->undef_name_ids[index] == name) {
            *id = index + (info->hosted ? 1 : 0);
            return 1;
        }
    }

    /* Get the name of the symbol from the ELF file */
    if (!(info->strtab) || name >= info->strtab->d_size) {
        fprintf(stderr, "%s: invalid string name offset %lu\n",
                info->filename, (unsigned long)name);
        *id = 0;
        return 0;
    }
    symname = info->strtab->d_buf + name;

    /* Add the symbol to the external reference table for ".o65" */
    if (info->num_undef_names >= info->max_undef_names) {
        info->max_undef_names += 32;
        info->undef_name_ids = (Elf32_Word *)
            realloc(info->undef_name_ids,
                    info->max_undef_names * sizeof(Elf32_Word));
        info->undef_names = (char **)
            realloc(info->undef_names,
                    info->max_undef_names * sizeof(char *));
        if (!(info->undef_name_ids) || !(info->undef_names)) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
    }
    info->undef_name_ids[info->num_undef_names] = name;
    info->undef_names[info->num_undef_names] = symname;
    *id = (info->num_undef_names)++ + (info->hosted ? 1 : 0);
    return 1;
}

/**
 * @brief Compares two relocations on ascending order of virtual address.
 *
 * @param[in] r1 Points to the first relocation.
 * @param[in] r2 Points to the second relocation.
 *
 * @return -1, 0, or 1 depending upon the relationship between @a r1 and @a r2.
 */
static int compare_rela(const void *r1, const void *r2)
{
    const Elf32_Rela *rel1 = (const Elf32_Rela *)r1;
    const Elf32_Rela *rel2 = (const Elf32_Rela *)r2;
    if (rel1->r_offset < rel2->r_offset)
        return -1;
    else if (rel1->r_offset > rel2->r_offset)
        return 1;
    else
        return 0;
}

/**
 * @brief Callback for processing the contents of a "RELA" section.
 *
 * @param[in,out] info Information about the image we are converting.
 * @param[in] scn ELF section control structure.
 * @param[in] shdr ELF section header structure.
 * @param[in] name Name of the section, or NULL if unknown.
 */
static void section_callback_reloc
    (image_info_t *info, Elf_Scn *scn, Elf32_Shdr *shdr, const char *name)
{
    Elf_Data *data;
    Elf32_Rela *rel_table;
    const Elf32_Rela *rel;
    const Elf32_Sym *sym;
    size_t count;
    o65_size_t address;
    o65_size_t symbol_address;
    o65_reloc_t out_rel;
    uint8_t from_segment;
    (void)shdr;
    (void)name;

    /* Get a pointer to the relocation entries in the section, plus a count */
    data = elf_getdata(scn, NULL);
    if (!data || data->d_size < sizeof(Elf32_Rela))
        return;
    count = data->d_size / sizeof(Elf32_Rela);
    rel_table = calloc(count, sizeof(Elf32_Rela));
    if (!rel_table) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    memcpy(rel_table, data->d_buf, count * sizeof(Elf32_Rela));

    /* Sort the relocation table on ascending order of address because the
     * ".o65" relocation system needs strict ordering to work properly. */
    qsort(rel_table, count, sizeof(Elf32_Rela), compare_rela);

    /* Process the relocations */
    for (rel = rel_table; count > 0; --count, ++rel) {
        /* Find the next address to be relocated, and which segment it is in */
        address = rel->r_offset;
        if (address >= info->text_address &&
                address < (info->text_address + info->text_size)) {
            from_segment = O65_SEGID_TEXT;
        } else if (address >= info->data_address &&
                   address < (info->data_address + info->data_size)) {
            from_segment = O65_SEGID_DATA;
        } else {
            fprintf(stderr, "%s: address 0x%lx is not in .text or .data\n",
                    info->filename, (unsigned long)address);
            info->flag = 0;
            continue;
        }
        if (from_segment != info->reloc_segment) {
            /* We have switched from .text to .data */
            if (from_segment != O65_SEGID_DATA) {
                fprintf(stderr, "%s: cannot relocate .text after .data\n",
                        info->filename);
                info->flag = 0;
                continue;
            }
            info->reloc_segment = from_segment;
            info->last_reloc_address = info->data_address - 1;
        }

        /* Relocation addresses cannot be repeated */
        if (address <= info->last_reloc_address) {
            fprintf(stderr, "%s: warning: duplicate relocation at 0x%lx\n",
                    info->filename, (unsigned long)address);
            continue;
        }

        /* Output "skip" relocations if the distance from the last
         * relocation is greater than 254 bytes. */
        while ((address - info->last_reloc_address) > 254) {
            out_rel.offset = 255; /* Special value that means "skip 254" */
            out_rel.type = 0;
            out_rel.extra = 0;
            out_rel.undefid = 0;
            add_o65_relocation(info, &out_rel);
            info->last_reloc_address += 254;
        }

        /* Clear the output relocation details, ready to fill them in */
        out_rel.offset = (uint8_t)(address - info->last_reloc_address);
        out_rel.extra = 0;
        out_rel.undefid = 0;

        /* Resolve the symbol name */
        symbol_address = ELF32_R_SYM(rel->r_info);
        if (symbol_address < info->num_symbols) {
            sym = info->symbols + symbol_address;
        } else {
            fprintf(stderr, "%s: symbol number %lu is invalid\n",
                    info->filename, (unsigned long)symbol_address);
            info->flag = 0;
            sym = info->symbols; /* Use the undefined symbol at index 0 */
        }

        /* Get the address of the symbol, including the addend */
        symbol_address = sym->st_value;
        symbol_address += rel->r_addend;

        /* Determine which segment the symbol lives in */
        if (sym->st_shndx == SHN_ABS) {
            /* If the symbol is absolute, then there is nothing to do.
             * We assume that the ELF linker already fixed up the value. */
            info->last_reloc_address = address;
            continue;
        } else if (sym->st_shndx == SHN_UNDEF) {
            /* Undefined symbol */
            if (resolve_undefined(info, sym->st_name, &(out_rel.undefid))) {
                out_rel.type = O65_SEGID_UNDEF;
                if (out_rel.undefid >= 0x10000U) {
                    /* We will need 32-bit offsets in the final ".o65" file */
                    info->header.mode |= O65_MODE_32BIT;
                }
            } else {
                info->flag = 0;
                info->last_reloc_address = address;
                continue;
            }
        } else if (symbol_address >= info->zeropage_address &&
                   symbol_address < (info->zeropage_address + info->zeropage_size)) {
            out_rel.type = O65_SEGID_ZEROPAGE;
            if (symbol_address < 32 && info->hosted) {
                /* We are in hosted mode.  Replace references to imaginary
                 * registers with offsets from the "__IMAG_REGS" symbol. */
                out_rel.type = O65_SEGID_UNDEF;
                out_rel.undefid = 0; /* Always the first external reference */
            }
        } else if (symbol_address >= info->text_address &&
                   symbol_address < (info->text_address + info->text_size)) {
            out_rel.type = O65_SEGID_TEXT;
        } else if (symbol_address >= info->data_address &&
                   symbol_address < (info->data_address + info->data_size)) {
            out_rel.type = O65_SEGID_DATA;
        } else if (symbol_address >= info->bss_address &&
                   symbol_address <= (info->bss_address + info->bss_size)) {
            out_rel.type = O65_SEGID_BSS;
        } else {
            fprintf(stderr, "%s: relocation address 0x%lx is not in a recognized section\n",
                    info->filename, (unsigned long)symbol_address);
            info->flag = 0;
            out_rel.type = O65_SEGID_TEXT;
        }

        /* Convert the relocation to ".o65" format.  We assume that the
         * ELF linker has already written the bytes of the virtual symbol
         * address to the .text and .data segments.  All we need to do
         * is adjust the base offset of the segments according to the
         * type of relocation. */
        switch (ELF32_R_TYPE(rel->r_info)) {
        case R_MOS_ADDR8:
        case R_MOS_ADDR16_LO:
        case R_MOS_ADDR24_SEGMENT_LO:
            out_rel.type |= O65_RELOC_LOW;
            break;

        case R_MOS_ADDR16:
        case R_MOS_ADDR24_SEGMENT:
            out_rel.type |= O65_RELOC_WORD;
            break;

        case R_MOS_ADDR16_HI:
        case R_MOS_ADDR24_SEGMENT_HI:
            out_rel.type |= O65_RELOC_HIGH;
            out_rel.extra = (uint8_t)symbol_address;
            break;

        case R_MOS_ADDR24:
            out_rel.type |= O65_RELOC_SEGADR;
            break;

        case R_MOS_ADDR24_BANK:
            out_rel.type |= O65_RELOC_SEG;
            out_rel.extra = (uint16_t)symbol_address;
            break;

        case R_MOS_NONE:
        case R_MOS_IMM8:
        case R_MOS_IMM16:
        case R_MOS_PCREL8:
        case R_MOS_FK_DATA_4:
        case R_MOS_FK_DATA_8:
        case R_MOS_ADDR_ASCIZ:
        default:
            fprintf(stderr, "%s: unsupported relocation type %d\n",
                    info->filename, (int)(ELF32_R_TYPE(rel->r_info)));
            info->flag = 0;
            info->last_reloc_address = address;
            continue;
        }

        /* Add the relocation to the image */
        add_o65_relocation(info, &out_rel);

        /* Update the length of the .text relocation table */
        if (from_segment == O65_SEGID_TEXT) {
            info->text_reloc_size = info->reloc_size;
        }

        /* Update the last address that was relocated */
        info->last_reloc_address = address;
    }
    free(rel_table);
}

/**
 * @brief Converts the relocations in an ELF file into ".o65" format.
 *
 * @param[in,out] info Information about the image we are converting.
 *
 * @return Non-zero if the relocations were converted, zero on error.
 */
static int convert_relocations(image_info_t *info)
{
    /* Find all "RELA" sections and convert the contents */
    info->flag = 1;
    info->last_reloc_address = info->text_address - 1;
    info->reloc_segment = O65_SEGID_TEXT;
    section_iterator(info, SHT_RELA, section_callback_reloc);
    return info->flag;
}

/**
 * @brief Populate the creation date header option in the ".o65" file.
 *
 * @param[in,out] info Information about the image we are converting.
 */
static void set_creation_date(image_info_t *info)
{
    size_t len;
    time_t t;
    struct tm *tm;
    struct stat st;
    char *tstr;

    /* Bail out if we don't actually want the date */
    if (!(info->add_creation_date))
        return;

    /* Use the modification time on the .elf file, or the current time
     * if we cannot get the modification time. */
    if (fstat(info->fd, &st) >= 0)
        t = st.st_mtime;
    else
        time(&t);

    /* Format the date and time into the header option */
    tm = localtime(&t);
    tstr = (char *)(info->created.data);
    strftime(tstr, sizeof(info->created.data), "%a %b %d %H:%M:%S %Z %Y", tm);
    len = strlen(tstr);
    if (len >= sizeof(info->created.data))
        len = sizeof(info->created.data) - 1;
    if (len > 0 && tstr[len - 1] == '\n')
        --len;
    info->created.len = len + 3;
    info->created.type = O65_OPT_CREATED;
    info->created.data[len] = '\0';
}

/**
 * @brief Writes a list of ".o65" relocations to the output file.
 *
 * @param[in,out] info Information about the image we are converting.
 * @param[in] relocs Points to the array of relocations to write.
 * @param[in] count Number of relocations to write.
 *
 * @return Non-zero if the relocations were written, zero on filesystem error.
 */
static int write_relocations
    (image_info_t *info, const o65_reloc_t *relocs, o65_size_t count)
{
    o65_reloc_t end = { .offset = 0 };
    for (; count > 0; --count, ++relocs) {
        if (o65_write_reloc(info->outfile, &(info->header), relocs) < 0)
            return 0;
    }
    if (o65_write_reloc(info->outfile, &(info->header), &end) < 0) {
        return 0;
    }
    return 1;
}

/**
 * @brief Writes out the final ".o65" file.
 *
 * @param[in,out] info Information about the image we are converting.
 * @param[in] filename Name of the file to write to.
 *
 * @return Non-zero if the image was written, zero on filesystem error.
 */
static int write_o65(image_info_t *info, const char *filename)
{
    size_t index;

    /* Open the output file */
    if ((info->outfile = fopen(filename, "wb")) == NULL)
        return 0;

    /* Set the creation date header option */
    set_creation_date(info);

    /* If we are in hosted mode, then subtract the imaginary registers
     * from the front of the zeropage segment.  They will be provided
     * by the runtime loader instead. */
    if (info->hosted && info->header.zlen >= 32) {
        info->header.zbase += 32;
        info->header.zlen -= 32;
    }

    /* Write the header */
    if (o65_write_header(info->outfile, &(info->header)) < 0)
        return 0;

    /* Write the header options */
    if (info->os.len != 0) {
        if (o65_write_option(info->outfile, &(info->os)) < 0)
            return 0;
    }
    if (info->linker.len != 0) {
        if (o65_write_option(info->outfile, &(info->linker)) < 0)
            return 0;
    }
    if (info->author.len != 0) {
        if (o65_write_option(info->outfile, &(info->author)) < 0)
            return 0;
    }
    if (info->created.len != 0) {
        if (o65_write_option(info->outfile, &(info->created)) < 0)
            return 0;
    }
    if (info->elf_machine.len != 0) {
        if (o65_write_option(info->outfile, &(info->elf_machine)) < 0)
            return 0;
    }
    if (o65_write_option(info->outfile, NULL) < 0) {
        return 0;
    }

    /* Write the .text segment */
    if (info->text_size > 0) {
        if (fwrite(info->text_segment, 1, info->text_size, info->outfile)
                != info->text_size) {
            return 0;
        }
    }

    /* Write the .data segment */
    if (info->data_size > 0) {
        if (fwrite(info->data_segment, 1, info->data_size, info->outfile)
                != info->data_size) {
            return 0;
        }
    }

    /* Write the external references list */
    if (info->hosted) {
        /* We need an extra external for the imaginary register table */
        if (o65_write_count
                (info->outfile, &(info->header), info->num_undef_names + 1) < 0) {
            return 0;
        }
        if (o65_write_string(info->outfile, "__IMAG_REGS") < 0) {
            return 0;
        }
    } else {
        if (o65_write_count
                (info->outfile, &(info->header), info->num_undef_names) < 0) {
            return 0;
        }
    }
    for (index = 0; index < info->num_undef_names; ++index) {
        if (o65_write_string(info->outfile, info->undef_names[index]) < 0) {
            return 0;
        }
    }

    /* Write the relocation tables */
    if (!write_relocations(info, info->reloc, info->text_reloc_size)) {
        return 0;
    }
    if (!write_relocations(info, info->reloc + info->text_reloc_size,
                           info->reloc_size - info->text_reloc_size)) {
        return 0;
    }

    /* Write the exported globals.  Only one so far for the main entry point. */
    if (o65_write_count(info->outfile, &(info->header), 1) < 0)
        return 0;
    if (o65_write_exported_symbol
            (info->outfile, &(info->header), "main",
             O65_SEGID_TEXT, info->entry_point) < 0) {
        return 0;
    }

    /* Clean up and exit */
    fclose(info->outfile);
    info->outfile = NULL;
    return 1;
}
