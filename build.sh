#!/bin/sh
if [ -e "src/watt32/lib/WATTCPWL.LIB" ]; then
	mv src/watt32/lib/WATTCPWL.LIB src/watt32/lib/WATTCPW0.LIB
	mv src/watt32/lib/WATTCPW0.LIB src/watt32/lib/wattcpwl.lib
fi
wcl -0 -lr -ml -opnr -oe=24 -oil+ -outback -ohm -sg -wx -we -d0 -k24576 -i=src -i=src/zlib -i=src/watt32/inc -fe=fdnpkg16.exe src/*.c src/zlib/zlib_l.lib src/watt32/lib/wattcpwl.lib
upx --8086 -9 fdnpkg16.exe
wcl -0 -lr -ml -opnr -oe=24 -oil+ -outback -ohm -sg -wx -we -d0 -k24576 -fpi -DUSE_EXTERNAL_MTCP -i=src -i=src/zlib -fe=fdnpkg86.exe src/crc32.c src/fileexst.c src/kprintf0.c src/loadconf.c src/parsecmd.c src/pkginst.c src/readenv.c src/dbg.c src/getdelim.c src/inf.c src/libgz.c src/lsm.c src/parseurl.c src/pkgrem.c src/rtrim.c src/fdnpkg16.c src/helpers.c src/kitten.c src/libunzip.c src/pkgdb.c src/pkgsrch.c src/showinst.c src/zlib/zlib_l.lib
upx --8086 -9 fdnpkg86.exe
wcl -0 -lr -ml -opnr -oe=24 -oil+ -outback -ohm -sg -wx -we -d0 -k24576 -fpi -i=src -i=src/zlib -i=src/watt32/inc -fe=testhttp.exe testhttp.c src/net.c src/http.c src/parseurl.c src/helpers.c src/dbg.c src/zlib/zlib_l.lib src/watt32/lib/wattcpwl.lib
upx --8086 -9 testhttp.exe
if [ -e "fdnpkg16.err" ]; then
	cat *.err
fi
if [ -e "fdnpkg16.exe" ]; then
	ls *.exe
fi
