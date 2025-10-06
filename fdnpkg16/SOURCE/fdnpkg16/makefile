CFLAGS = -0 -lr -ml -opnr -oe=24 -oil+ -outback -ohm -sg -wx -we -d0 -k24576 -fpc -i=src -i=src/zlib -i=src/watt32/inc
CLIBS = src/zlib/zlib_l.lib src/watt32/lib/wattcpwl.lib

#all: fdnpkg.exe test-ver.exe test-url.exe testhttp.exe test-unz.exe
all: fdnpkg16.exe

# This target produces a fdnpkg0.exe that is not compressed and do not have cwsdpmi embedded
fdnpkg16.exe: fdnpkg16.obj crc32.obj inf.obj kprintf.obj kittendj.obj fileexst.obj http.obj libgz.obj libunzip.obj loadconf.obj lsm.obj net.obj lzmadec.obj pkgdb.obj pkginst.obj pkgsrch.obj pkgrem.obj readenv.obj showinst.obj getdelim.obj rtrim.obj parsecmd.obj parseurl.obj helpers.obj
	wcc fdnpkg.obj crc32.obj inf.obj kprintf.obj kittendj.obj fileexst.obj http.obj libgz.obj libunzip.obj loadconf.obj lsm.obj net.obj lzmadec.obj pkgdb.obj pkginst.obj pkgsrch.obj pkgrem.obj readenv.obj showinst.obj getdelim.obj rtrim.obj parsecmd.obj parseurl.obj helpers.obj $(CLIBS)

# targets below are useful only for testing purposes
test-url.exe: test-url.c parseurl.obj
	wcc test-url.c parseurl.obj -o test-url.exe $(CFLAGS)

test-ver.exe: test-ver.c kprintf.obj kittendj.obj helpers.obj lsm.obj rtrim.obj
	wcc test-ver.c kprintf.obj kittendj.obj helpers.obj lsm.obj rtrim.obj -o test-ver.exe $(CFLAGS)

testhttp.exe: testhttp.c helpers.obj http.obj parseurl.obj net.obj
	wcc testhttp.c helpers.obj http.obj parseurl.obj net.obj src/watt32/lib/wattcpwl.lib -o testhttp.exe $(CFLAGS)

test-unz.exe: test-unz.c crc32.obj kittendj.obj kprintf.obj libunzip.obj lzmadec.obj tinfl.obj
	wcc -g test-unz.c crc32.obj kittendj.obj kprintf.obj libunzip.obj lzmadec.obj tinfl.obj -o test-unz.exe $(CFLAGS)

test-gz.exe: test-gz.c crc32.obj libgz.obj tinfl.obj
	wcc -g test-gz.c crc32.obj libgz.obj tinfl.obj -o test-gz.exe $(CFLAGS)

pkg: fdnpkg.exe fdinst/fdinst.exe
	mkdir appinfo
	mkdir bin
	mkdir doc
	mkdir doc/fdnpkg
	mkdir source
	mkdir source/fdnpkg
	mkdir source/fdnpkg/fdinst
	mkdir source/fdnpkg/zlib
	copy fdnpkg.lsm appinfo
	copy fdnpkg.exe bin
	copy fdinst/fdinst.exe bin
	copy fdinst/fdinst.txt doc/fdnpkg
	copy fdnpkg.cfg bin
	copy fdnpkg.txt doc/fdnpkg
	copy history.txt doc/fdnpkg
	copy *.c source/fdnpkg
	copy *.h source/fdnpkg
	copy *.asm source/fdnpkg
	copy *.txt source/fdnpkg
	copy fdinst/*.c source/fdnpkg/fdinst
	copy fdinst/*.h source/fdnpkg/fdinst
	copy fdinst/*.txt source/fdnpkg/fdinst
	copy fdinst/*.bat source/fdnpkg/fdinst
	copy zlib/*.* source/fdnpkg/zlib
	copy makefile source/fdnpkg
	copy *.cfg source/fdnpkg
	copy *.bin source/fdnpkg
	if exist fdnpkg.zip del fdnpkg.zip
	zip -r -k -9 fdnpkg.zip appinfo bin doc nls source
	deltree /Y appinfo
	deltree /Y bin
	deltree /Y doc
	deltree /Y source
	echo "fdnpkg.zip ready!"

clean:
	del *.obj
	del *.exe
