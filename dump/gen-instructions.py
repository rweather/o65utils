#!/usr/bin/python
#
# Generate the instruction decoding table.

file = open('instructions.txt', 'r')
lines = file.readlines()
file.close()

# Parse the instruction information.
names = []
modes = []
opcodes = {}
for line in lines:
    fields = line.strip().split(';')
    opcode = int(fields[0], 16)
    name = fields[1].lower()
    mode = fields[2]
    if len(name) > 3:
        extra = name[3:]
        name = name[:3]
    else:
        extra = ''
    if name in names:
        name_index = names.index(name)
    else:
        name_index = len(names)
        names.append(name)
    opcodes[opcode] = {
        'opcode': opcode,
        'name': name,
        'extra': extra,
        'mode': mode,
        'index': name_index
    }
num_names = len(names)

# Print header information.
print("/* Generated from instructions.txt; do not edit */")
print("")
print("#ifndef INSTRUCTIONS_H")
print("#define INSTRUCTIONS_H")
print("")

# Dump definitions for the opcode modes.
print("/* Opcode modes (bits 0..4) and instruction lengths (bits 6..7). */");
print("#define OP_ill          0x40")
print("#define OP_imp          0x41")
print("#define OP_imm          0x82")
print("#define OP_abs          0xC3")
print("#define OP_abs_X        0xC4")
print("#define OP_abs_Y        0xC5")
print("#define OP_X_ind        0x86")
print("#define OP_ind_Y        0x87")
print("#define OP_zpg          0x88")
print("#define OP_zpg_X        0x89")
print("#define OP_zpg_Y        0x8A")
print("#define OP_rel          0x8B")
print("#define OP_ind          0xCC")
print("#define OP_ind_zpg      0x8D")
print("#define OP_ind_abs_X    0xCE")
print("#define OP_bit_zpg      0x8F")
print("#define OP_zpg_rel      0xD0")
print("")

# Dump the opcode to name table.
print("/* Convert an opcode number into an index into the name table. */")
print("unsigned char const op6502_to_name[256] = {")
for opcode in range(256):
    if opcode in opcodes:
        opc = opcodes[opcode]
        full_name = opc['name']
        if len(opc['extra']) > 0:
            full_name += opc['extra'] + " " + opc['mode']
        elif len(opc['mode']) > 0:
            full_name += "  " + opc['mode']
        print("    %3d, /* %s */" % (opc['index'] * 3, full_name))
    else:
        print("    %3d," % (num_names * 3))
print("};")
print("")

# Dump the opcode name table.
print('/* List of all opcode names, compacted to save space */')
print("char const op6502_names[] =")
name_list = ''.join(names) + 'db '
posn = 0;
while posn < len(name_list):
    print('    "%s"' % name_list[posn:posn+63])
    posn = posn + 63
print(";")
print("")

# Dump the opcode mode table.
print("/* Modes and instruction lengths for all opcodes */")
print("unsigned char const op6502_modes[] = {")
for opcode in range(256):
    if opcode in opcodes:
        opc = opcodes[opcode]
        full_name = opc['name']
        if len(opc['extra']) > 0:
            full_name += extra
        full_name += " " + opc['mode']
        mode = opc['mode'].replace(',', '_')
        print("    OP_%-20s, /* %s */" % (mode, full_name))
    else:
        print("    OP_ill,")
print("};")
print("")
print("#endif")
