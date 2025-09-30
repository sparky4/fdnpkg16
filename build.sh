#!/bin/sh
if [ -e "src/watt32/lib/WATTCPWL.LIB" ]; then
	mv src/watt32/lib/WATTCPWL.LIB src/watt32/lib/WATTCPW0.LIB
	mv src/watt32/lib/WATTCPW0.LIB src/watt32/lib/wattcpwl.lib
fi
wcl -0 -lr -ml -opnr -oe=24 -oil+ -outback -ohm -sg -wx -we -d0 -k24576 -i=src -i=src/zlib -i=src/watt32/inc -fe=fdnpkg16.exe src/*.c src/zlib/zlib_l.lib src/watt32/lib/wattcpwl.lib
wcl -0 -lr -ml -opnr -oe=24 -oil+ -outback -ohm -sg -wx -we -d0 -k24576 -i=src -i=src/zlib -i=src/watt32/inc -fe=testhttp.exe testhttp.c src/net.c src/http.c src/parseurl.c src/helpers.c src/dbg.c src/zlib/zlib_l.lib src/watt32/lib/wattcpwl.lib
upx --8086 -9 fdnpkg16.exe
upx --8086 -9 testhttp.exe
