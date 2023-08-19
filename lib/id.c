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

typedef struct
{
    uint16_t id;
    char name[O65_NAME_MAX];

} o65_name_t;

static o65_name_t const cpu_names[] = {
    {O65_MODE_CPU_6502,     "6502"      },
    {O65_MODE_CPU_65C02,    "65C02"     },
    {O65_MODE_CPU_65SC02,   "65SC02"    },
    {O65_MODE_CPU_65CE02,   "65CE02"    },
    {O65_MODE_CPU_UNDOC,    "6502-undoc"},
    {O65_MODE_CPU_EMUL,     "65816-emul"},
    {O65_MODE_CPU_6809,     "6809"      },
    {O65_MODE_CPU_Z80,      "Z80"       },
    {O65_MODE_CPU_8086,     "8086"      },
    {O65_MODE_CPU_80286,    "80286"     },
    {O65_MODE_CPU_65816,    "65816"     },
    {0,                     ""          }
};

static o65_name_t const segment_names[] = {
    {O65_SEGID_UNDEF,       "undef"     },
    {O65_SEGID_ABS,         "abs"       },
    {O65_SEGID_TEXT,        ".text"     },
    {O65_SEGID_DATA,        ".data"     },
    {O65_SEGID_BSS,         ".bss"      },
    {O65_SEGID_ZEROPAGE,    ".zeropage" },
    {0,                     ""          }
};

int o65_get_cpu_name(uint16_t mode, char name[O65_NAME_MAX])
{
    unsigned index;

    /* Remove mode bits that are not required to identify the CPU */
    mode &= O65_MODE_CPU_BITS;

    /* Search for the CPU type */
    for (index = 0; cpu_names[index].name[0] != '\0'; ++index) {
        if (cpu_names[index].id == mode) {
            memcpy(name, cpu_names[index].name, O65_NAME_MAX);
            return 1;
        }
    }

    /* Unknown CPU, format it as 0x???? instead */
    snprintf(name, O65_NAME_MAX, "0x%04X", mode);
    return 0;
}

int o65_get_segment_name(uint8_t segid, char name[O65_NAME_MAX])
{
    unsigned index;

    /* Search for the segment identifier */
    for (index = 0; segment_names[index].name[0] != '\0'; ++index) {
        if (segment_names[index].id == segid) {
            memcpy(name, segment_names[index].name, O65_NAME_MAX);
            return 1;
        }
    }

    /* Unknown segment identifier, format it as "segment n" instead */
    snprintf(name, O65_NAME_MAX, "segment %d", segid);
    return 0;
}
