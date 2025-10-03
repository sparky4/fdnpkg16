#
#  GNU make makefile for the MkMake/MkDep/MkLang/MkImp/bin2c
#  utilities used during Watt-32 developement
#
#  DOS versions:   Requires djgpp2
#  Win32 versions: Requires MinGW
#  Linux versions: Requires gcc (or clang)
#
#  MkMake additionally requires SLang 1.3+ library installed.
#

.SUFFIXES: .exe .l .y

SLANG_ROOT_DOS   ?= $(DJDIR)/contrib/slang.210
SLANG_LIB_DOS    ?= $(SLANG_ROOT_DOS)/src/djgobjs/libslang.a

SLANG_ROOT_WIN   ?= $(realpath $(MINGW32))/src/TUI/Slang
SLANG_LIB_WIN    ?= $(SLANG_ROOT_WIN)/src/gw32objs/libslang.a
SLANG_LIB_WIN_CL ?= $(SLANG_ROOT_WIN)/src/mw32objs/wslang32.lib

CC     = gcc
CFLAGS = -Wall -g # -s

all: mkmake.exe mkdep.exe mklang.exe mkimp.exe bin2c.exe

win32: $(addprefix win32/, mkmake.exe mkdep.exe mklang.exe bin2c.exe)

linux: $(addprefix linux/, mkmake mkdep mklang bin2c)

mkdep.exe: mkdep.c
	$(CC) $(CFLAGS) -o $@ $^

mklang.exe: mklang.c
	$(CC) $(CFLAGS) -o $@ $^

mkimp.exe: mkimp.c
	$(CC) $(CFLAGS) -o $@ $^

mkmake.exe: mkmake.c
	$(CC) $(CFLAGS) -I$(SLANG_ROOT_DOS)/src -o $@ $^ $(SLANG_LIB_DOS)

bin2c.exe: bin2c.c
	$(CC) $(CFLAGS) -o $@ $^

mkimp.c: mkimp.l
	flex -o$@ $^

#
# Win32 binaries:
#
win32/mkmake.exe: mkmake.c
	$(CC) -m32 $(CFLAGS) -I$(SLANG_ROOT_WIN)/src -o $*.exe $^ $(SLANG_LIB_WIN)

win32/mkdep.exe: mkdep.c
	$(CC) -m32 $(CFLAGS) -o $*.exe $^

win32/mklang.exe: mklang.c
	$(CC) -m32 $(CFLAGS) -o $*.exe $^

win32/bin2c.exe: bin2c.c
	$(CC) -m32 $(CFLAGS) -o $*.exe $^

#
# MSVC version for testing:
#
msvc/mkmake.exe: mkmake.c
	cl -nologo -MD -Zi -I../inc -Fe$@ -I$(SLANG_ROOT_WIN)/src $^ $(SLANG_LIB_WIN_CL)

#
# Linux binaries:
#
# S-lang comes with the distro. Some provide only slang/slang.h,
# some provide slang.h and symlink slang/slang.h to it. That is
# the reason for HAVE_SLANG_SLANG_H define.
#
linux/mkmake: mkmake.c
	$(CC) -DHAVE_SLANG_SLANG_H $(CFLAGS) -o $@ $^ -lslang

linux/mkdep: mkdep.c
	$(CC) $(CFLAGS) -o $@ $^

linux/mklang: mklang.c
	$(CC) $(CFLAGS) -o $@ $^

linux/bin2c: bin2c.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	@rm -f *.o mk*.exe bin2c.exe \
               linux/mk* linux/bin2c \
               win32/mk*.exe win32/bin2c.exe
#              mkimp.c mkimp_gr.c
