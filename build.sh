#!/bin/sh
wcl -0 -lr -ml -opnr -oe=24 -oil+ -outback -ohm -sg -wx -we -d0 -k32768 -i=src -i=src/zlib -fe=fdnpkg16.exe src/*.c src/zlib/zlib_l.lib
upx --8086 -9 fdnpkg16.exe
