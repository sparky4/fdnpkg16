@echo off
wcl -0 -lr -ml -opnr -oe=24 -oil+ -outback -ohm -sg -wx -we -d0 -k32768 -i=src -i=src\zlib -fe=fdnpkg16.exe src\*.c src\zlib\zlib_l.lib
rem wcl -0 -lr -ml -os -wx -we -d0 -i=..\ -i=..\zlib\ -DNOREPOS -DNOLZMA -fe=fdnpkg16.exe src\*.c ..\zlib\zlib_l.lib
rem wcl -0 -lr -ml -os -wx -we -d0 -k40960 -i=src\ -i=src\zlib\ -DNOREPOS -DNOLZMA -fe=fdnpkg16.exe src\*.c src\zlib\zlib_l.lib
if ERRORLEVEL 1 goto gameover
upx --8086 -9 fdnpkg16.exe
:gameover
