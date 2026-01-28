#
# Generic make file for a C program with multiple targets
#

!ifdef __LINUX__ || __BSD__
#!ifdef UNIX
to_os_path=\=/
REMOVECOMMAND=rm -f
COPYCOMMAND=cp -f
MOVECOMMAND=mv
DIRSEP=/
OBJ=obj
DUMP=cat
!else		#DOS ^^
to_os_path=/=\
REMOVECOMMAND=del
COPYCOMMAND=copy /y
MOVECOMMAND=move /y
DIRSEP=\
OBJ=obj
DUMP=type
!endif

# --- MACROS ---

# Compiler Options: Add desired options (e.g., debugging -g, optimization -O)
# -bt=<system> is often passed via command line or inferred by wcl/wcl386
COMPILER_OPTIONS = -q -0 -lr -ml -opnr -oe=24 -oil+ -outback -ohm -sg -wx -we -d0 -k24576 -fpi -fo=.$(OBJ) -i=src
COMPILER_OPTIONS2 = -DNOREPOS -DNOLZMA

# Linker Options
LINKER_OPTIONS = src/zlib/zlib_l.lib -i=src/zlib
LINKER_OPTIONS_HTTPGET = src/watt32/lib/wattcpwl.lib -i=src/watt32/inc
LINKER_OPTIONS_FDINST = src/zlib/zlib_l.lib -i=src/zlib

# Source files
C_SOURCE = src/fdnpkg16.c src/crc32.c src/fileexst.c src/kprintf.c src/loadconf.c src/parsecmd.c src/pkginst.c src/readenv.c src/getdelim.c src/inf.c src/libgz.c src/lsm.c src/parseurl.c src/pkgrem.c src/rtrim.c src/helpers.c src/kitten.c src/libunzip.c src/pkgdb.c src/pkgsrch.c src/memcore.c src/showinst.c src/lzmadec.c
C_SOURCE_HTTPGET = src/exec/httpget.c src/net.c src/http.c src/parseurl.c src/helpers.c
C_SOURCE_FDINST = src/exec/fdinst16.c src/crc32.c src/fileexst.c src/getdelim.c src/helpers.c src/inf.c src/libunzip.c src/loadconf.c src/lsm.c src/parsecmd.c src/pkginst.c src/pkgrem.c src/readenv.c src/rtrim.c src/showinst.c src/exec/kprintf0.c

# Object files (derived from source files, adjust extension as needed for your setup)
OBJECTS = $(C_SOURCE:../.c=.$(OBJ))
OBJ_HTTPGET = $(C_SOURCE_HTTPGET:../.c=.$(OBJ))
OBJ_FDINST = $(C_SOURCE_FDINST:../.c=.$(OBJ))
EXEC = &
     fdnpkg16.exe &
     httpget.exe &
     fdinst16.exe

# --- TARGETS ---

all: $(EXEC)
fdnpkg16.exe: $(OBJECTS)
    @echo Building for dos
    *wcl -bt=dos $(COMPILER_OPTIONS) $(OBJECTS) $(LINKER_OPTIONS)
    @upx --8086 -9 fdnpkg16.exe
    @wmake -h vomitchan
    @echo Finished building fdnpkg16.exe

httpget.exe: $(OBJ_HTTPGET)
    @echo Building for dos
    *wcl -bt=dos $(COMPILER_OPTIONS) $(OBJ_HTTPGET) $(LINKER_OPTIONS_HTTPGET)
    @upx --8086 -9 httpget.exe
    @wmake -h vomitchan
    @echo Finished building httpget.exe

fdinst16.exe: $(OBJECTS)
    @echo Building for dos
    $(REMOVECOMMAND) *.$(OBJ)
    *wcl -bt=dos $(COMPILER_OPTIONS) $(COMPILER_OPTIONS2) $(OBJ_FDINST) $(LINKER_OPTIONS_FDINST)
    $(REMOVECOMMAND) *.$(OBJ)
    @upx --8086 -9 fdinst16.exe
    @wmake -h vomitchan
    @echo Finished building fdinst16.exe

# Generic rule to compile .c files into .$(OBJ) files
.c.$(OBJ) :
    *wcl $(COMPILER_OPTIONS) -c $[@

.$(OBJ).exe :
    *wcl $(COMPILER_OPTIONS) $(LINKER_OPTIONS) $< -fe=$@

.c : src$(DIRSEP);src$(DIRSEP)exec$(DIRSEP)

.$(OBJ) : .

.exe : .

# Clean target to remove built files
clean: .symbolic
    @echo Cleaning up...
    @$(REMOVECOMMAND) *.$(OBJ) *.exe

vomitchan: .symbolic
    @if exist *.err $(DUMP) *.err


pkg: fdnpkg16.exe httpget.exe fdinst16.exe
	mkdir appinfo
	mkdir bin
	mkdir doc
	mkdir doc/fdnpkg16
	mkdir source
	mkdir source/fdnpkg16
	mkdir source/fdnpkg16/fdinst
	mkdir source/fdnpkg16/zlib
	copy fdnpkg16.lsm appinfo
	copy fdnpkg16.exe bin
	copy fdinst16/fdinst16.exe bin
	copy fdinst16/fdinst16.txt doc/fdnpkg16
	copy fdnpkg16.cfg bin
	copy fdnpkg16.txt doc/fdnpkg16
	copy history.txt doc/fdnpkg16
	copy *.c source/fdnpkg16
	copy *.h source/fdnpkg16
	copy *.asm source/fdnpkg16
	copy *.txt source/fdnpkg16
	copy fdinst/*.c source/fdnpkg16/fdinst16
	copy fdinst/*.h source/fdnpkg16/fdinst16
	copy fdinst/*.txt source/fdnpkg16/fdinst16
	copy fdinst/*.bat source/fdnpkg16/fdinst16
	copy zlib/*.* source/fdnpkg16/zlib
	copy makefile source/fdnpkg16
	copy *.cfg source/fdnpkg16
	copy *.bin source/fdnpkg16
	if exist fdnpkg16.zip del fdnpkg16.zip
	zip -r -k -9 fdnpkg16.zip appinfo bin doc nls source
	deltree /Y appinfo
	deltree /Y bin
	deltree /Y doc
	deltree /Y source
	echo "fdnpkg16.zip ready!"
