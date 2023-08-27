
ELF2O65 = ../build/elf2o65/elf2o65
MOS_CLANG = mos-sim-clang -Os -Wl,-emit-relocs
LINKER_SCRIPT = ../link/link.ld

.PHONY: all clean

all: hello.o65

hello.o65: hello_exe
	$(ELF2O65) hello_exe.elf $@

hello_exe: hello.c $(LINKER_SCRIPT) Makefile
	$(MOS_CLANG) -T$(LINKER_SCRIPT) -o $@ hello.c

clean:
	rm -rf hello_exe* *.o65
