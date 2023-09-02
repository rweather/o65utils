/* Generated from instructions.txt; do not edit */

#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

/* Opcode modes (bits 0..4) and instruction lengths (bits 6..7). */
#define OP_ill          0x40
#define OP_imp          0x41
#define OP_imm          0x82
#define OP_abs          0xC3
#define OP_abs_X        0xC4
#define OP_abs_Y        0xC5
#define OP_X_ind        0x86
#define OP_ind_Y        0x87
#define OP_zpg          0x88
#define OP_zpg_X        0x89
#define OP_zpg_Y        0x8A
#define OP_rel          0x8B
#define OP_ind          0xCC
#define OP_ind_zpg      0x8D
#define OP_ind_abs_X    0xCE
#define OP_bit_zpg      0x8F
#define OP_zpg_rel      0xD0

/* Convert an opcode number into an index into the name table. */
unsigned char const op6502_to_name[256] = {
      0, /* brk  imp */
      3, /* ora  X,ind */
    210,
    210,
      6, /* tsb  zpg */
      3, /* ora  zpg */
      9, /* asl  zpg */
     12, /* rmb0 bit,zpg */
     15, /* php  imp */
      3, /* ora  imm */
      9, /* asl  imp */
    210,
      6, /* tsb  abs */
      3, /* ora  abs */
      9, /* asl  abs */
     18, /* bbr0 zpg,rel */
     21, /* bpl  rel */
      3, /* ora  ind,Y */
      3, /* ora  ind,zpg */
    210,
     24, /* trb  zpg */
      3, /* ora  zpg,X */
      9, /* asl  zpg,X */
     12, /* rmb1 bit,zpg */
     27, /* clc  imp */
      3, /* ora  abs,Y */
     30, /* inc  imp */
    210,
     24, /* trb  abs */
      3, /* ora  abs,X */
      9, /* asl  abs,X */
     18, /* bbr1 zpg,rel */
     33, /* jsr  abs */
     36, /* and  X,ind */
    210,
    210,
     39, /* bit  zpg */
     36, /* and  zpg */
     42, /* rol  zpg */
     12, /* rmb2 bit,zpg */
     45, /* plp  imp */
     36, /* and  imm */
     42, /* rol  imp */
    210,
     39, /* bit  abs */
     36, /* and  abs */
     42, /* rol  abs */
     18, /* bbr2 zpg,rel */
     48, /* bmi  rel */
     36, /* and  ind,Y */
     36, /* and  ind,zpg */
    210,
     39, /* bit  zpg,X */
     36, /* and  zpg,X */
     42, /* rol  zpg,X */
     12, /* rmb3 bit,zpg */
     51, /* sec  imp */
     36, /* and  abs,Y */
     54, /* dec  imp */
    210,
     39, /* bit  abs,X */
     36, /* and  abs,X */
     42, /* rol  abs,X */
     18, /* bbr3 zpg,rel */
     57, /* rti  imp */
     60, /* eor  X,ind */
    210,
    210,
    210,
     60, /* eor  zpg */
     63, /* lsr  zpg */
     12, /* rmb4 bit,zpg */
     66, /* pha  imp */
     60, /* eor  imm */
     63, /* lsr  imp */
    210,
     69, /* jmp  abs */
     60, /* eor  abs */
     63, /* lsr  abs */
     18, /* bbr4 zpg,rel */
     72, /* bvc  rel */
     60, /* eor  ind,Y */
     60, /* eor  ind,zpg */
    210,
    210,
     60, /* eor  zpg,X */
     63, /* lsr  zpg,X */
     12, /* rmb5 bit,zpg */
     75, /* cli  imp */
     60, /* eor  abs,Y */
     78, /* phy  imp */
    210,
    210,
     60, /* eor  abs,X */
     63, /* lsr  abs,X */
     18, /* bbr5 zpg,rel */
     81, /* rts  imp */
     84, /* adc  X,ind */
    210,
    210,
     87, /* stz  zpg */
     84, /* adc  zpg */
     90, /* ror  zpg */
     12, /* rmb6 bit,zpg */
     93, /* pla  imp */
     84, /* adc  imm */
     90, /* ror  imp */
    210,
     69, /* jmp  ind */
     84, /* adc  abs */
     90, /* ror  abs */
     18, /* bbr6 zpg,rel */
     96, /* bvs  rel */
     84, /* adc  ind,Y */
     84, /* adc  ind,zpg */
    210,
     87, /* stz  zpg,X */
     84, /* adc  zpg,X */
     90, /* ror  zpg,X */
     12, /* rmb7 bit,zpg */
     99, /* sei  imp */
     84, /* adc  abs,Y */
    102, /* ply  imp */
    210,
     69, /* jmp  ind,abs,X */
     84, /* adc  abs,X */
     90, /* ror  abs,X */
     18, /* bbr7 zpg,rel */
    105, /* bra  rel */
    108, /* sta  X,ind */
    210,
    210,
    111, /* sty  zpg */
    108, /* sta  zpg */
    114, /* stx  zpg */
    117, /* smb0 bit,zpg */
    120, /* dey  imp */
     39, /* bit  imm */
    123, /* txa  imp */
    210,
    111, /* sty  abs */
    108, /* sta  abs */
    114, /* stx  abs */
    126, /* bbs0 zpg,rel */
    129, /* bcc  rel */
    108, /* sta  ind,Y */
    108, /* sta  ind,zpg */
    210,
    111, /* sty  zpg,X */
    108, /* sta  zpg,X */
    114, /* stx  zpg,Y */
    117, /* smb1 bit,zpg */
    132, /* tya  imp */
    108, /* sta  abs,Y */
    135, /* txs  imp */
    210,
     87, /* stz  abs */
    108, /* sta  abs,X */
     87, /* stz  abs,X */
    126, /* bbs1 zpg,rel */
    138, /* ldy  imm */
    141, /* lda  X,ind */
    144, /* ldx  imm */
    210,
    138, /* ldy  zpg */
    141, /* lda  zpg */
    144, /* ldx  zpg */
    117, /* smb2 bit,zpg */
    147, /* tay  imp */
    141, /* lda  imm */
    150, /* tax  imp */
    210,
    138, /* ldy  abs */
    141, /* lda  abs */
    144, /* ldx  abs */
    126, /* bbs2 zpg,rel */
    153, /* bcs  rel */
    141, /* lda  ind,Y */
    141, /* lda  ind,zpg */
    210,
    138, /* ldy  zpg,X */
    141, /* lda  zpg,X */
    144, /* ldx  zpg,Y */
    117, /* smb3 bit,zpg */
    156, /* clv  imp */
    141, /* lda  abs,Y */
    159, /* tsx  imp */
    210,
    138, /* ldy  abs,X */
    141, /* lda  abs,X */
    144, /* ldx  abs,Y */
    126, /* bbs3 zpg,rel */
    162, /* cpy  imm */
    165, /* cmp  X,ind */
    210,
    210,
    162, /* cpy  zpg */
    165, /* cmp  zpg */
     54, /* dec  zpg */
    117, /* smb4 bit,zpg */
    168, /* iny  imp */
    165, /* cmp  imm */
    171, /* dex  imp */
    174, /* wai  imp */
    162, /* cpy  abs */
    165, /* cmp  abs */
     54, /* dec  abs */
    126, /* bbs4 zpg,rel */
    177, /* bne  rel */
    165, /* cmp  ind,Y */
    165, /* cmp  ind,zpg */
    210,
    210,
    165, /* cmp  zpg,X */
     54, /* dec  zpg,X */
    117, /* smb5 bit,zpg */
    180, /* cld  imp */
    165, /* cmp  abs,Y */
    183, /* phx  imp */
    186, /* stp  imp */
    210,
    165, /* cmp  abs,X */
     54, /* dec  abs,Y */
    126, /* bbs5 zpg,rel */
    189, /* cpx  imm */
    192, /* sbc  X,ind */
    210,
    210,
    189, /* cpx  zpg */
    192, /* sbc  zpg */
     30, /* inc  zpg */
    117, /* smb6 bit,zpg */
    195, /* inx  imp */
    192, /* sbc  imm */
    198, /* nop  imp */
    210,
    189, /* cpx  abs */
    192, /* sbc  abs */
     30, /* inc  abs */
    126, /* bbs6 zpg,rel */
    201, /* beq  rel */
    192, /* sbc  ind,Y */
    192, /* sbc  ind,zpg */
    210,
    210,
    192, /* sbc  zpg,X */
     30, /* inc  zpg,X */
    117, /* smb7 bit,zpg */
    204, /* sed  imp */
    192, /* sbc  abs,Y */
    207, /* plx  imp */
    210,
    210,
    192, /* sbc  abs,X */
     30, /* inc  abs,Y */
    126, /* bbs7 zpg,rel */
};

/* List of all opcode names, compacted to save space */
char const op6502_names[] =
    "brkoratsbaslrmbphpbbrbpltrbclcincjsrandbitrolplpbmisecdecrtieor"
    "lsrphajmpbvccliphyrtsadcstzrorplabvsseiplybrastastystxsmbdeytxa"
    "bbsbcctyatxsldyldaldxtaytaxbcsclvtsxcpycmpinydexwaibnecldphxstp"
    "cpxsbcinxnopbeqsedplxdb "
;

/* Modes and instruction lengths for all opcodes */
unsigned char const op6502_modes[] = {
    OP_imp                 , /* brk imp */
    OP_X_ind               , /* ora X,ind */
    OP_ill,
    OP_ill,
    OP_zpg                 , /* tsb zpg */
    OP_zpg                 , /* ora zpg */
    OP_zpg                 , /* asl zpg */
    OP_bit_zpg             , /* rmb7 bit,zpg */
    OP_imp                 , /* php imp */
    OP_imm                 , /* ora imm */
    OP_imp                 , /* asl imp */
    OP_ill,
    OP_abs                 , /* tsb abs */
    OP_abs                 , /* ora abs */
    OP_abs                 , /* asl abs */
    OP_zpg_rel             , /* bbr7 zpg,rel */
    OP_rel                 , /* bpl rel */
    OP_ind_Y               , /* ora ind,Y */
    OP_ind_zpg             , /* ora ind,zpg */
    OP_ill,
    OP_zpg                 , /* trb zpg */
    OP_zpg_X               , /* ora zpg,X */
    OP_zpg_X               , /* asl zpg,X */
    OP_bit_zpg             , /* rmb7 bit,zpg */
    OP_imp                 , /* clc imp */
    OP_abs_Y               , /* ora abs,Y */
    OP_imp                 , /* inc imp */
    OP_ill,
    OP_abs                 , /* trb abs */
    OP_abs_X               , /* ora abs,X */
    OP_abs_X               , /* asl abs,X */
    OP_zpg_rel             , /* bbr7 zpg,rel */
    OP_abs                 , /* jsr abs */
    OP_X_ind               , /* and X,ind */
    OP_ill,
    OP_ill,
    OP_zpg                 , /* bit zpg */
    OP_zpg                 , /* and zpg */
    OP_zpg                 , /* rol zpg */
    OP_bit_zpg             , /* rmb7 bit,zpg */
    OP_imp                 , /* plp imp */
    OP_imm                 , /* and imm */
    OP_imp                 , /* rol imp */
    OP_ill,
    OP_abs                 , /* bit abs */
    OP_abs                 , /* and abs */
    OP_abs                 , /* rol abs */
    OP_zpg_rel             , /* bbr7 zpg,rel */
    OP_rel                 , /* bmi rel */
    OP_ind_Y               , /* and ind,Y */
    OP_ind_zpg             , /* and ind,zpg */
    OP_ill,
    OP_zpg_X               , /* bit zpg,X */
    OP_zpg_X               , /* and zpg,X */
    OP_zpg_X               , /* rol zpg,X */
    OP_bit_zpg             , /* rmb7 bit,zpg */
    OP_imp                 , /* sec imp */
    OP_abs_Y               , /* and abs,Y */
    OP_imp                 , /* dec imp */
    OP_ill,
    OP_abs_X               , /* bit abs,X */
    OP_abs_X               , /* and abs,X */
    OP_abs_X               , /* rol abs,X */
    OP_zpg_rel             , /* bbr7 zpg,rel */
    OP_imp                 , /* rti imp */
    OP_X_ind               , /* eor X,ind */
    OP_ill,
    OP_ill,
    OP_ill,
    OP_zpg                 , /* eor zpg */
    OP_zpg                 , /* lsr zpg */
    OP_bit_zpg             , /* rmb7 bit,zpg */
    OP_imp                 , /* pha imp */
    OP_imm                 , /* eor imm */
    OP_imp                 , /* lsr imp */
    OP_ill,
    OP_abs                 , /* jmp abs */
    OP_abs                 , /* eor abs */
    OP_abs                 , /* lsr abs */
    OP_zpg_rel             , /* bbr7 zpg,rel */
    OP_rel                 , /* bvc rel */
    OP_ind_Y               , /* eor ind,Y */
    OP_ind_zpg             , /* eor ind,zpg */
    OP_ill,
    OP_ill,
    OP_zpg_X               , /* eor zpg,X */
    OP_zpg_X               , /* lsr zpg,X */
    OP_bit_zpg             , /* rmb7 bit,zpg */
    OP_imp                 , /* cli imp */
    OP_abs_Y               , /* eor abs,Y */
    OP_imp                 , /* phy imp */
    OP_ill,
    OP_ill,
    OP_abs_X               , /* eor abs,X */
    OP_abs_X               , /* lsr abs,X */
    OP_zpg_rel             , /* bbr7 zpg,rel */
    OP_imp                 , /* rts imp */
    OP_X_ind               , /* adc X,ind */
    OP_ill,
    OP_ill,
    OP_zpg                 , /* stz zpg */
    OP_zpg                 , /* adc zpg */
    OP_zpg                 , /* ror zpg */
    OP_bit_zpg             , /* rmb7 bit,zpg */
    OP_imp                 , /* pla imp */
    OP_imm                 , /* adc imm */
    OP_imp                 , /* ror imp */
    OP_ill,
    OP_ind                 , /* jmp ind */
    OP_abs                 , /* adc abs */
    OP_abs                 , /* ror abs */
    OP_zpg_rel             , /* bbr7 zpg,rel */
    OP_rel                 , /* bvs rel */
    OP_ind_Y               , /* adc ind,Y */
    OP_ind_zpg             , /* adc ind,zpg */
    OP_ill,
    OP_zpg_X               , /* stz zpg,X */
    OP_zpg_X               , /* adc zpg,X */
    OP_zpg_X               , /* ror zpg,X */
    OP_bit_zpg             , /* rmb7 bit,zpg */
    OP_imp                 , /* sei imp */
    OP_abs_Y               , /* adc abs,Y */
    OP_imp                 , /* ply imp */
    OP_ill,
    OP_ind_abs_X           , /* jmp ind,abs,X */
    OP_abs_X               , /* adc abs,X */
    OP_abs_X               , /* ror abs,X */
    OP_zpg_rel             , /* bbr7 zpg,rel */
    OP_rel                 , /* bra rel */
    OP_X_ind               , /* sta X,ind */
    OP_ill,
    OP_ill,
    OP_zpg                 , /* sty zpg */
    OP_zpg                 , /* sta zpg */
    OP_zpg                 , /* stx zpg */
    OP_bit_zpg             , /* smb7 bit,zpg */
    OP_imp                 , /* dey imp */
    OP_imm                 , /* bit imm */
    OP_imp                 , /* txa imp */
    OP_ill,
    OP_abs                 , /* sty abs */
    OP_abs                 , /* sta abs */
    OP_abs                 , /* stx abs */
    OP_zpg_rel             , /* bbs7 zpg,rel */
    OP_rel                 , /* bcc rel */
    OP_ind_Y               , /* sta ind,Y */
    OP_ind_zpg             , /* sta ind,zpg */
    OP_ill,
    OP_zpg_X               , /* sty zpg,X */
    OP_zpg_X               , /* sta zpg,X */
    OP_zpg_Y               , /* stx zpg,Y */
    OP_bit_zpg             , /* smb7 bit,zpg */
    OP_imp                 , /* tya imp */
    OP_abs_Y               , /* sta abs,Y */
    OP_imp                 , /* txs imp */
    OP_ill,
    OP_abs                 , /* stz abs */
    OP_abs_X               , /* sta abs,X */
    OP_abs_X               , /* stz abs,X */
    OP_zpg_rel             , /* bbs7 zpg,rel */
    OP_imm                 , /* ldy imm */
    OP_X_ind               , /* lda X,ind */
    OP_imm                 , /* ldx imm */
    OP_ill,
    OP_zpg                 , /* ldy zpg */
    OP_zpg                 , /* lda zpg */
    OP_zpg                 , /* ldx zpg */
    OP_bit_zpg             , /* smb7 bit,zpg */
    OP_imp                 , /* tay imp */
    OP_imm                 , /* lda imm */
    OP_imp                 , /* tax imp */
    OP_ill,
    OP_abs                 , /* ldy abs */
    OP_abs                 , /* lda abs */
    OP_abs                 , /* ldx abs */
    OP_zpg_rel             , /* bbs7 zpg,rel */
    OP_rel                 , /* bcs rel */
    OP_ind_Y               , /* lda ind,Y */
    OP_ind_zpg             , /* lda ind,zpg */
    OP_ill,
    OP_zpg_X               , /* ldy zpg,X */
    OP_zpg_X               , /* lda zpg,X */
    OP_zpg_Y               , /* ldx zpg,Y */
    OP_bit_zpg             , /* smb7 bit,zpg */
    OP_imp                 , /* clv imp */
    OP_abs_Y               , /* lda abs,Y */
    OP_imp                 , /* tsx imp */
    OP_ill,
    OP_abs_X               , /* ldy abs,X */
    OP_abs_X               , /* lda abs,X */
    OP_abs_Y               , /* ldx abs,Y */
    OP_zpg_rel             , /* bbs7 zpg,rel */
    OP_imm                 , /* cpy imm */
    OP_X_ind               , /* cmp X,ind */
    OP_ill,
    OP_ill,
    OP_zpg                 , /* cpy zpg */
    OP_zpg                 , /* cmp zpg */
    OP_zpg                 , /* dec zpg */
    OP_bit_zpg             , /* smb7 bit,zpg */
    OP_imp                 , /* iny imp */
    OP_imm                 , /* cmp imm */
    OP_imp                 , /* dex imp */
    OP_imp                 , /* wai imp */
    OP_abs                 , /* cpy abs */
    OP_abs                 , /* cmp abs */
    OP_abs                 , /* dec abs */
    OP_zpg_rel             , /* bbs7 zpg,rel */
    OP_rel                 , /* bne rel */
    OP_ind_Y               , /* cmp ind,Y */
    OP_ind_zpg             , /* cmp ind,zpg */
    OP_ill,
    OP_ill,
    OP_zpg_X               , /* cmp zpg,X */
    OP_zpg_X               , /* dec zpg,X */
    OP_bit_zpg             , /* smb7 bit,zpg */
    OP_imp                 , /* cld imp */
    OP_abs_Y               , /* cmp abs,Y */
    OP_imp                 , /* phx imp */
    OP_imp                 , /* stp imp */
    OP_ill,
    OP_abs_X               , /* cmp abs,X */
    OP_abs_Y               , /* dec abs,Y */
    OP_zpg_rel             , /* bbs7 zpg,rel */
    OP_imm                 , /* cpx imm */
    OP_X_ind               , /* sbc X,ind */
    OP_ill,
    OP_ill,
    OP_zpg                 , /* cpx zpg */
    OP_zpg                 , /* sbc zpg */
    OP_zpg                 , /* inc zpg */
    OP_bit_zpg             , /* smb7 bit,zpg */
    OP_imp                 , /* inx imp */
    OP_imm                 , /* sbc imm */
    OP_imp                 , /* nop imp */
    OP_ill,
    OP_abs                 , /* cpx abs */
    OP_abs                 , /* sbc abs */
    OP_abs                 , /* inc abs */
    OP_zpg_rel             , /* bbs7 zpg,rel */
    OP_rel                 , /* beq rel */
    OP_ind_Y               , /* sbc ind,Y */
    OP_ind_zpg             , /* sbc ind,zpg */
    OP_ill,
    OP_ill,
    OP_zpg_X               , /* sbc zpg,X */
    OP_zpg_X               , /* inc zpg,X */
    OP_bit_zpg             , /* smb7 bit,zpg */
    OP_imp                 , /* sed imp */
    OP_abs_Y               , /* sbc abs,Y */
    OP_imp                 , /* plx imp */
    OP_ill,
    OP_ill,
    OP_abs_X               , /* sbc abs,X */
    OP_abs_Y               , /* inc abs,Y */
    OP_zpg_rel             , /* bbs7 zpg,rel */
};

#endif
