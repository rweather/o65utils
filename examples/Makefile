
ELF2O65 = ../build/elf2o65/elf2o65
MOS_COMMON_CLANG = mos-common-clang
MOS_SIM_CLANG = mos-sim-clang
LINKER_SCRIPT = ../link/o65reloc.ld
CFLAGS = \
    -Os \
    -T$(LINKER_SCRIPT) \
    -Wl,-emit-relocs \
    -Wl,--unresolved-symbols=ignore-all

.PHONY: all clean

all: hello.o65 hello_hosted.o65

hello.o65: hello_exe
	$(ELF2O65) hello_exe.elf $@

hello_hosted.o65: hello_hosted_exe
	$(ELF2O65) --hosted hello_hosted_exe.elf $@

hello_exe: hello.c $(LINKER_SCRIPT) Makefile
	$(MOS_SIM_CLANG) $(CFLAGS) -o $@ hello.c

hello_hosted_exe: hello.c $(LINKER_SCRIPT) Makefile
	$(MOS_COMMON_CLANG) $(CFLAGS) -o $@ hello.c

clean:
	rm -rf hello_exe* hello_hosted_exe* *.o65
