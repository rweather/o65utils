o65utils
========

This repository contains utilities for working with relocatable 6502
binaries in the [o65 format](http://www.6502.org/users/andre/o65/).

Building
--------

```
mkdir build
cd build
cmake ..
make
make install
```

Using
-----

The `o65dump` program will dump the contents of a `.o65` file to
standard output:

```
o65dump hello.o65
```

It can also be used on multiple files:

```
o65dump hello.o65 goodbye.o65 ...
```

Contact
-------

For more information on this code, to report bugs, or to suggest
improvements, please contact the author Rhys Weatherley via
[email](mailto:rhys.weatherley@gmail.com).
