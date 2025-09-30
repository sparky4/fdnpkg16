#
#  GNU make makefile for the MkMake/MkDep/MkLang/MkImp utilities
#  used during WatTCP developement
#
#  Requires djgpp2 + SLang 1.3+ library installed
#

.SUFFIXES: .exe .l .y

#
# Change to suite
#
SLANG_ROOT = $(DJDIR)/contrib/slang.205/src

CC     = gcc
CFLAGS = -Wall -s

all: mkmake.exe mkdep.exe mklang.exe mkimp.exe dxegen.exe

mkdep.exe: mkdep.c
	$(CC) $(CFLAGS) -o $*.exe $^

mklang.exe: mklang.c
	$(CC) $(CFLAGS) -o $*.exe $^

mkimp.exe: mkimp.c
	$(CC) $(CFLAGS) -o $*.exe $^

mkmake.exe: mkmake.c
	$(CC) $(CFLAGS) -I$(SLANG_ROOT) -o $*.exe $^ \
	      $(SLANG_ROOT)/djgobjs/libslang.a

dxegen.exe: dxegen.c
	$(CC) $(CFLAGS) -o $*.exe $^

mkimp.c: mkimp.l
	flex -o$@ $^

clean:
	@rm -f *.o mkmake.exe mkdep.exe mklang.exe mkimp.exe dxegen.exe \
               mkimp.c mkimp_gr.c

