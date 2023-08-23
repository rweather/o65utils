o65utils
========

This repository contains utilities for working with relocatable 6502
binaries in the [o65 format](http://www.6502.org/users/andre/o65/).

Building
--------

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

Contact
-------

For more information on this code, to report bugs, or to suggest
improvements, please contact the author Rhys Weatherley via
[email](mailto:rhys.weatherley@gmail.com).
