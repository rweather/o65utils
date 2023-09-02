o65utils
========

This repository contains utilities for working with relocatable 6502
binaries in the [o65 format](http://www.6502.org/users/andre/o65/).

Building
--------

The `elf2o65` utility requires `libelf`.  If the development version
of `libelf` is not installed, then `elf2o65` will not be built.
On Ubuntu and similar systems, you can usually do this to install it:

    sudo apt install libelf-dev

Then configure and build the code as follows:

    mkdir build
    cd build
    cmake ..
    make
    make install

Using
-----

### o65dump

The `o65dump` program will dump the contents of a `.o65` file to
standard output:

    o65dump hello.o65

It can also be used on multiple files:

    o65dump hello.o65 goodbye.o65 ...

If the `-d` option is supplied, the contents of the text segment will
be disassembled for CPU's in the 6502 family:

    o65dump -d hello.o65

If the CPU type cannot be disassembled, the contents of the text
segment will be dumped in hexadecimal instead.

### o65reloc

The `o65reloc` program can be used to convert a `.o65` file into a
`.bin` file that has been relocated to a fixed address:

    o65reloc -t 0x2000 hello.o65 hello.bin

By default, it is assumed that the `.text`, `.data`, and `.bss` segments
will end up in RAM at consecutive locations starting at the load address.
The segments will be aligned as required by the input file's options.

The `-d` option can be supplied to specify a different load address
for the `.data` segment, and the `-b` option can be supplied to
specify a different load address for the `.bss` segment.

If the `.data` segment is relocated to somewhere other than to the
end of the `.text` segment, you should provide a separate output file
for the `.data` segment:

    o65reloc -t 0x2000 hello.o65 hello-text.bin hello-data.bin

If the `-t` option is not provided, then the original address of
the `.text` segment in the input file will be used as the load address.

An error will occur if the load address is zero, or the input
file is aligned but the load address is not a multiple of the alignment.

If the file refers to external symbols, they can be provided with an
imports file:

    cat >imports.txt
    k_char_out 0xfded
    k_file_open 0xb700
    k_file_close 0xb703
    k_file_read 0xb706
    k_file_write 0xb709
    <EOF>
    o65reloc -t 0x2000 -i imports.txt hello.o65 hello.bin

Each line of the imports file consists of a symbol name and the
address to use for that symbol, separated by one or more whitespace
characters.  The addresses may be in decimal, octal, or hexadecimal.

If the input file contains multiple chained images, then only the first
image will be relocated.  The rest of the chained images will be ignored.

### elf2o65

The `elf2o65` utility converts ELF files that have been generated with
[llvm-mos](https://llvm-mos.org/) into `.o65` format.  Use it as follows:

    elf2o65 hello.elf hello.o65

The ELF file must have been created with a linker script that outputs the
sections in a form that `elf2o65` can understand.  Arbitrary ELF files
from llvm-mos will not work without modifications to the linker script.

The "link" directory in this repository contains an example linker
script called `o65reloc.ld`.  It may need some modification for specific
platforms.  The simplest method to convert a program's ELF file into
`.o65` format is:

    mos-sim-clang -Os -To65reloc.ld -Wl,-emit-relocs -o example example.c
    elf2o65 example.elf example.o65

If the progam has external references to functions in the operating
system, then add the `-Wl,--unresolved-symbols=ignore-all` option
and `elf2o65` will add the externals to the output file:

    mos-common-clang -Os -To65reloc.ld -Wl,-emit-relocs \
        -Wl,--unresolved-symbols=ignore-all -o example example.c
    elf2o65 example.elf example.o65

Extensions to the .o65 format
-----------------------------

### ELF Machine Type

The `elf2o65` utility adds an extension header option to the output file
with option number 69 (decimal), corresponding to a capital letter 'E'
in ASCII.  The option's payload consists of the ELF machine type and
processor-specific flags.

This option is needed because the CPU bits in the basic `.o65` header
are insufficient to represent the full set of CPU's that are supported
by llvm-mos.  The following is an example of an ELF machine option:

    08 45 66 19 3B 00 00 00

The first two bytes are the option length (8) and type (0x45 = 69 = 'E').
The next two bytes are the ELF machine type in little-endian byte order
(0x1966 = 6502).  The final four bytes are the ELF processor-specific flags
in little-endian byte order (0x0000003B = 6502 generic, BCD instructions,
65C02 extensions, R65C02 extensions, W65C02 extensions).

See the llvm-mos [ELF Specification](https://llvm-mos.org/wiki/ELF_specification)
for a full list of processor-specific flags for the 6502 machine family.
The traditional CPU bits in the `.o65` header should be set to the closest
historical match.

The original `.o65` format has some support for non-6502 CPU's such as
Z80, 6809, and so on.  Files that use those processor families could also
make use of the ELF machine option if they like.  Refer to the ELF
specification for the alternate processor family for the bits that
are required.

### Imaginary Registers

The [llvm-mos](https://llvm-mos.org/) compiler framework allocates 32
bytes of zero page memory for "imaginary registers".  These are used to
help the compiler's register allocator and for function parameter passing.

The `.o65` format already has a method for the operating system to
allocate a contiguous region of zero page memory to a program.
The imaginary registers can be placed in this region.  They will be
unique to the program.

If multiple programs are loaded by the operating system, they will
each get their own copy of the imaginary registers.  This is the easiest
option for operating systems that have no knowledge of llvm-mos
calling conventions.

However, the operating system may want to share the imaginary registers
between multiple programs, or the operating system itself may provide
system calls using the llvm-mos calling conventions.

When using the `--hosted` command-line option, the `.o65` file will
contain an external reference for `__IMAG_REGS` which the operating system
must replace with the base address of the 32 shared imaginary registers
in zero page memory.

It is assumed that the 32 imaginary registers are contiguous starting
at `__IMAG_REGS`.  This isn't the case on all platforms supported by llvm-mos.
But this method is simpler and more compact than requiring the `.o65` program
loader to handle separate external references for `__rc0`, `__rc1`, etc.

On systems with non-contiguous imaginary registers, the program loader
can add a special case to the handling of `LOW` relocations.
If the relocation refers to the `__IMAG_REGS` external reference, then
read the byte to be relocated, interpret it as an imaginary register
number, and replace the byte with the actual imaginary register address.

### Entry Point

Program binaries in `.o65` format that use
[lib6502](http://www.6502.org/users/andre/lib6502/lib6502.html)
export a global symbol called `main` for the entry point.
This entry point is assumed to take a pointer to the command-line
argument array in the A/Y register pair and to return a Unix-style
exit status code in A.

Note that the `main()` function in a C program may not actually be at
the entry point address.  There may be `crt0` logic that executes before
`main()`.  If such logic exists, the `main` global symbol in a `.o65`
file must be the `crt0` entry point, not the address of the `main()` function.

If the program's entry point is not compatible with lib6502's calling
conventions, then it should be called something other than `main`.

The `elf2o65` tool will export a symbol called `_start` if the
ELF entry point is not at the start of the text segment, and the
program is not compatible with lib6502.

Contact
-------

For more information on this code, to report bugs, or to suggest
improvements, please contact the author Rhys Weatherley via
[email](mailto:rhys.weatherley@gmail.com).
