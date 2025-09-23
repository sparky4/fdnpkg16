#!/bin/sh
wcl -0 -lr -ml -os -wx -we -d0 -i=pkglib/ -i=watt32/inc/ -i=nls_utf8/ -DNOREPOS -DNOLZMA -fe=pkgnet.exe *.c watt32/lib/wattcpwl.lib
upx --8086 -9 pkgnet.exe
cat *.err
