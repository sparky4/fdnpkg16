CFLAGS = -std=gnu89 -march=i386 -O3 -Wall -Wextra -Werror -Wno-long-long -pedantic -Wformat-security -I watt32/inc
CLIBS = watt32/bin/386watt.a zlib/zlib_dj.a -lemu

#all: fdnpkg.exe test-ver.exe test-url.exe testhttp.exe test-unz.exe
all: fdnpkg.exe

# This target produces a fdnpkg0.exe that is not compressed and do not have cwsdpmi embedded
fdnpkg0.exe: fdnpkg.o crc32.o inf.o kprintf.o kittendj.o fileexst.o http.o libgz.o libunzip.o loadconf.o lsm.o net.o lzmadec.o pkgdb.o pkginst.o pkgsrch.o pkgrem.o readenv.o showinst.o getdelim.o rtrim.o parsecmd.o parseurl.o helpers.o
	gcc fdnpkg.o crc32.o inf.o kprintf.o kittendj.o fileexst.o http.o libgz.o libunzip.o loadconf.o lsm.o net.o lzmadec.o pkgdb.o pkginst.o pkgsrch.o pkgrem.o readenv.o showinst.o getdelim.o rtrim.o parsecmd.o parseurl.o helpers.o $(CLIBS) -o fdnpkg0.exe

# Here I strip fdnpkg0.exe, add the cwsdpmi stub and compress the whole thing with UPX
fdnpkg.exe: fdnpkg0.exe
	exe2coff fdnpkg0.exe
	copy /B cwsdstub.bin+fdnpkg0 fdnpkg.exe
	del fdnpkg0
	upx -9 fdnpkg.exe


# targets below are useful only for testing purposes
test-url.exe: test-url.c parseurl.o
	gcc test-url.c parseurl.o -o test-url.exe $(CFLAGS)

test-ver.exe: test-ver.c kprintf.o kittendj.o helpers.o lsm.o rtrim.o
	gcc test-ver.c kprintf.o kittendj.o helpers.o lsm.o rtrim.o -o test-ver.exe $(CFLAGS)

testhttp.exe: testhttp.c helpers.o http.o parseurl.o net.o
	gcc testhttp.c helpers.o http.o parseurl.o net.o watt32/bin/386watt.a -o testhttp.exe $(CFLAGS)

test-unz.exe: test-unz.c crc32.o kittendj.o kprintf.o libunzip.o lzmadec.o tinfl.o
	gcc -g test-unz.c crc32.o kittendj.o kprintf.o libunzip.o lzmadec.o tinfl.o -o test-unz.exe $(CFLAGS)

test-gz.exe: test-gz.c crc32.o libgz.o tinfl.o
	gcc -g test-gz.c crc32.o libgz.o tinfl.o -o test-gz.exe $(CFLAGS)

pkg: fdnpkg.exe fdinst\fdinst.exe
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

clean:
	del *.o
	del *.exe
