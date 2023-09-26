#ifndef ELFMOS_H
#define ELFMOS_H

/* This file is placed in the public domain */

/* Codes from here: https://llvm-mos.org/wiki/ELF_specification */

/* Machine types */
#ifndef EM_MOS
#define EM_MOS                  6502
#endif

/* Machine flags for MOS devices */
#ifndef EM_MOS_6502
#define EM_MOS_6502             0x00000001
#endif
#ifndef EM_MOS_6502_BCD
#define EM_MOS_6502_BCD         0x00000002
#endif
#ifndef EM_MOS_6502X
#define EM_MOS_6502X            0x00000004
#endif
#ifndef EM_MOS_65C02
#define EM_MOS_65C02            0x00000008
#endif
#ifndef EM_MOS_R65C02
#define EM_MOS_R65C02           0x00000010
#endif
#ifndef EM_MOS_W65C02
#define EM_MOS_W65C02           0x00000020
#endif
#ifndef EM_MOS_W65816
#define EM_MOS_W65816           0x00000100
#endif
#ifndef EM_MOS_65EL02
#define EM_MOS_65EL02           0x00000200
#endif
#ifndef EM_MOS_65CE02
#define EM_MOS_65CE02           0x00000400
#endif
#ifndef EM_MOS_HUC6280
#define EM_MOS_HUC6280          0x00000800
#endif
#ifndef EM_MOS_65DTV02
#define EM_MOS_65DTV02          0x00001000
#endif
#ifndef EM_MOS_4510
#define EM_MOS_4510             0x00002000
#endif
#ifndef EM_MOS_45GS02
#define EM_MOS_45GS02           0x00004000
#endif
#ifndef EM_MOS_SPC700
#define EM_MOS_SPC700           0x00020000
#endif

/* Section flags */
#ifndef SHF_MOS_ZEROPAGE
#define SHF_MOS_ZEROPAGE        0x10000000
#endif

/* Relocation types */
#ifndef R_MOS_NONE
#define R_MOS_NONE              0
#endif
#ifndef R_MOS_IMM8
#define R_MOS_IMM8              1
#endif
#ifndef R_MOS_ADDR8
#define R_MOS_ADDR8             2
#endif
#ifndef R_MOS_ADDR16
#define R_MOS_ADDR16            3
#endif
#ifndef R_MOS_ADDR16_LO
#define R_MOS_ADDR16_LO         4
#endif
#ifndef R_MOS_ADDR16_HI
#define R_MOS_ADDR16_HI         5
#endif
#ifndef R_MOS_PCREL_8
#define R_MOS_PCREL_8           6
#endif
#ifndef R_MOS_ADDR24
#define R_MOS_ADDR24            7
#endif
#ifndef R_MOS_ADDR24_BANK
#define R_MOS_ADDR24_BANK       8
#endif
#ifndef R_MOS_ADDR24_SEGMENT
#define R_MOS_ADDR24_SEGMENT    9
#endif
#ifndef R_MOS_ADDR24_SEGMENT_LO
#define R_MOS_ADDR24_SEGMENT_LO 10
#endif
#ifndef R_MOS_ADDR24_SEGMENT_HI
#define R_MOS_ADDR24_SEGMENT_HI 11
#endif
#ifndef R_MOS_PCREL_16
#define R_MOS_PCREL_16          12
#endif
#ifndef R_MOS_FK_DATA_4
#define R_MOS_FK_DATA_4         13
#endif
#ifndef R_MOS_FK_DATA_8
#define R_MOS_FK_DATA_8         14
#endif
#ifndef R_MOS_ADDR_ASCIZ
#define R_MOS_ADDR_ASCIZ        15
#endif
#ifndef R_MOS_IMM16
#define R_MOS_IMM16             16
#endif
#ifndef R_MOS_ADDR13
#define R_MOS_ADDR13            17
#endif

#endif
