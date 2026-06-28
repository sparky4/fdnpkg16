#
# Generic make file for a C program with multiple targets
#

!ifdef __LINUX__ || __BSD__
#!ifdef UNIX
to_os_path=\=/
REMOVECOMMAND=rm -f
COPYCOMMAND=cp -f
XCOPYCOMMAND=cp -r
MOVECOMMAND=mv
LISTCOMMAND=ls
DIRSEP=/
OBJ=obj
DUMP=cat
!else		#DOS ^^
to_os_path=/=\
REMOVECOMMAND=del
COPYCOMMAND=copy /y
XCOPYCOMMAND=xcopy /s /y
MOVECOMMAND=move /y
LISTCOMMAND=dir
DIRSEP=\
OBJ=obj
DUMP=type
!endif

#!ifdef __WATCOMC__
!if $(__VERSION__) < 1300
GETDELIM = src$(DIRSEP)getdelim.c
!else
GETDELIM =
!endif
#!endif

# --- MACROS ---

# Compiler Options: Add desired options (e.g., debugging -g, optimization -O)
COMPILER_OPTIONS = -q -0 -lr -ml -opnr -oe=24 -oil+ -outback -ohm -sg -wx -we -d0 -k24576 -fpi -fo=.$(OBJ) -i=src
COMPILER_OPTIONS2 = -DNOREPOS -DNOLZMA

# Linker Options
LINKER_OPTIONS = src$(DIRSEP)zlib$(DIRSEP)zlib_l.lib -i=src$(DIRSEP)zlib
LINKER_OPTIONS_HTTPGET = src$(DIRSEP)watt32$(DIRSEP)lib$(DIRSEP)wattcpwl.lib -i=src$(DIRSEP)watt32$(DIRSEP)inc
LINKER_OPTIONS_FDINST = src$(DIRSEP)zlib$(DIRSEP)zlib_l.lib -i=src$(DIRSEP)zlib

# Source files
C_SOURCE = src$(DIRSEP)fdnpkg16.c src$(DIRSEP)crc32.c src$(DIRSEP)fileexst.c src$(DIRSEP)kprintf.c src$(DIRSEP)loadconf.c src$(DIRSEP)parsecmd.c src$(DIRSEP)pkginst.c src$(DIRSEP)readenv.c src$(DIRSEP)inf.c src$(DIRSEP)libgz.c src$(DIRSEP)lsm.c src$(DIRSEP)parseurl.c src$(DIRSEP)pkgrem.c src$(DIRSEP)rtrim.c src$(DIRSEP)helpers.c src$(DIRSEP)kitten.c src$(DIRSEP)libunzip.c src$(DIRSEP)pkgdb.c src$(DIRSEP)pkgsrch.c src$(DIRSEP)memcore.c src$(DIRSEP)showinst.c src$(DIRSEP)lzmadec.c $(GETDELIM)
C_SOURCE_HTTPGET = src$(DIRSEP)exec$(DIRSEP)httpget.c src$(DIRSEP)net.c src$(DIRSEP)http.c src$(DIRSEP)parseurl.c src$(DIRSEP)helpers.c src$(DIRSEP)memcore.c
C_SOURCE_FDINST = src$(DIRSEP)exec$(DIRSEP)fdinst16.c src$(DIRSEP)crc32.c src$(DIRSEP)fileexst.c src$(DIRSEP)helpers.c src$(DIRSEP)inf.c src$(DIRSEP)libunzip.c src$(DIRSEP)loadconf.c src$(DIRSEP)lsm.c src$(DIRSEP)parsecmd.c src$(DIRSEP)pkginst.c src$(DIRSEP)pkgrem.c src$(DIRSEP)readenv.c src$(DIRSEP)rtrim.c src$(DIRSEP)showinst.c src$(DIRSEP)kprintf0.c src$(DIRSEP)memcore.c $(GETDELIM)
C_SOURCE_FDINK = src$(DIRSEP)exec$(DIRSEP)fdink16.c src$(DIRSEP)crc32.c src$(DIRSEP)fileexst.c src$(DIRSEP)helpers.c src$(DIRSEP)inf.c src$(DIRSEP)libunzip.c src$(DIRSEP)loadconf.c src$(DIRSEP)lsm.c src$(DIRSEP)parsecmd.c src$(DIRSEP)pkginst.c src$(DIRSEP)pkgrem.c src$(DIRSEP)readenv.c src$(DIRSEP)rtrim.c src$(DIRSEP)showinst.c src$(DIRSEP)kprintf0.c src$(DIRSEP)memcore.c $(GETDELIM)

# Object files (derived from source files, adjust extension as needed for your setup)
OBJECTS = $(C_SOURCE:..$(DIRSEP).c=.$(OBJ))
OBJ_HTTPGET = $(C_SOURCE_HTTPGET:..$(DIRSEP).c=.$(OBJ))
OBJ_FDINST = $(C_SOURCE_FDINST:..$(DIRSEP).c=.$(OBJ))
OBJ_FDINK  = $(C_SOURCE_FDINK:..$(DIRSEP).c=.$(OBJ))
EXEC = &
     fdnpkg16.exe &
     httpget.exe &
     fdinst16.exe

# --- TARGETS ---

all: $(EXEC)

fdnpkg16.exe: $(OBJECTS)
#    @echo Building for dos
    *wcl -bt=dos $(COMPILER_OPTIONS) $(OBJECTS) $(LINKER_OPTIONS)
    @upx --8086 -9 fdnpkg16.exe
    @wmake -h vomitchan
#    @echo Finished building fdnpkg16.exe

httpget.exe: $(OBJ_HTTPGET)
#    @echo Building for dos
    *wcl -bt=dos $(COMPILER_OPTIONS) $(OBJ_HTTPGET) $(LINKER_OPTIONS_HTTPGET)
    @upx --8086 -9 httpget.exe
    @wmake -h vomitchan
#    @echo Finished building httpget.exe

fdinst16.exe: $(OBJ_FDINST)
#    @echo Building for dos
    @$(REMOVECOMMAND) *.$(OBJ)
    *wcl -bt=dos $(COMPILER_OPTIONS) $(COMPILER_OPTIONS2) $(OBJ_FDINST) $(LINKER_OPTIONS_FDINST)
    @$(REMOVECOMMAND) *.$(OBJ)
    @upx --8086 -9 fdinst16.exe
    @wmake -h vomitchan
#    @echo Finished building fdinst16.exe

fdink16.exe: $(OBJ_FDINK)
#    @echo Building for dos
    @$(REMOVECOMMAND) *.$(OBJ)
    *wcl -bt=dos $(COMPILER_OPTIONS) $(COMPILER_OPTIONS2) $(OBJ_FDINK) $(LINKER_OPTIONS_FDINST)
    @$(REMOVECOMMAND) *.$(OBJ)
    @upx --8086 -9 fdink16.exe
    @wmake -h vomitchan
#    @echo Finished building fdink16.exe

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
    @echo ===============================================================================

vomitchan: .symbolic
    @if exist *.err @$(DUMP) *.err
    @if exist *.exe @$(LISTCOMMAND) *.exe
    @echo ===============================================================================

#DO NOT USE YET!! I AM WORKING ON THIS --sparky4
pkgfkjdsljflkdsajflkdsajflkdsaDONTUSE: fdnpkg16.exe httpget.exe fdinst16.exe
	mkdir appinfo
	mkdir bin
	mkdir doc
	mkdir doc\fdnpkg
	mkdir source
	mkdir source\fdnpkg
	mkdir source\fdnpkg\fdinst
	mkdir source\fdnpkg\zlib
	copy fdnpkg.lsm appinfo
	copy fdnpkg.exe bin
	copy fdinst\fdinst.exe bin
	copy fdinst\fdinst.txt doc\fdnpkg
	copy fdnpkg.cfg bin
	copy fdnpkg.txt doc\fdnpkg
	copy history.txt doc\fdnpkg
	copy *.c source\fdnpkg
	copy *.h source\fdnpkg
	copy *.asm source\fdnpkg
	copy *.txt source\fdnpkg
	copy fdinst\*.c source\fdnpkg\fdinst
	copy fdinst\*.h source\fdnpkg\fdinst
	copy fdinst\*.txt source\fdnpkg\fdinst
	copy fdinst\*.bat source\fdnpkg\fdinst
	copy zlib\*.* source\fdnpkg\zlib
	copy makefile source\fdnpkg
	copy *.cfg source\fdnpkg
	copy *.bin source\fdnpkg
	if exist fdnpkg.zip del fdnpkg.zip
	zip -r -k -9 fdnpkg.zip appinfo bin doc nls source
	deltree /Y appinfo
	deltree /Y bin
	deltree /Y doc
	deltree /Y source
	echo "fdnpkg.zip ready!"

pkgfkjdsljflkdsajflkdsajflkdsaDONOTUSE: fdnpkg16.exe httpget.exe fdinst16.exe
	mkdir fdnpkg16
	mkdir fdnpkg16/appinfo
	mkdir fdnpkg16/bin
	mkdir fdnpkg16/doc
	mkdir fdnpkg16/doc/fdnpkg16
	mkdir fdnpkg16/source
	mkdir fdnpkg16/source/fdnpkg16
	# sparky4: i need to work on this part
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
