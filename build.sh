#!/bin/sh
wcl -0 -lr -ml -ox -sg -wx -we -d0 -k40960 -i=src/ -i=src/zlib/ -DNOREPOS -DNOLZMA -fe=fdnpkg16.exe src/*.c src/zlib/zlib_l.lib
upx --8086 -9 fdnpkg16.exe
