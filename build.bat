@echo off
wcl -0 -lr -ml -opnr -oe=24 -oil+ -outback -ohm -sg -wx -we -d0 -k24576 -fpi -i=src -i=src\zlib -i=src\watt32\inc -fe=fdnpkg16.exe src\*.c src\zlib\zlib_l.lib src\watt32\lib\wattcpwl.lib
wcl -0 -lr -ml -opnr -oe=24 -oil+ -outback -ohm -sg -wx -we -d0 -k24576 -fpi -i=src -i=src\zlib -i=src\watt32\inc -fe=testhttp.exe testhttp.c src\net.c src\http.c src\parseurl.c src\helpers.c src\dbg.c src\zlib\zlib_l.lib src\watt32\lib\wattcpwl.lib
rem wcl -0 -lr -ml -os -wx -we -d0 -i=..\ -i=..\zlib\ -DNOREPOS -DNOLZMA -fe=fdnpkg16.exe src\*.c ..\zlib\zlib_l.lib
rem wcl -0 -lr -ml -os -wx -we -d0 -k40960 -i=src\ -i=src\zlib\ -DNOREPOS -DNOLZMA -fe=fdnpkg16.exe src\*.c src\zlib\zlib_l.lib
if ERRORLEVEL 1 goto gameover
upx --8086 -9 fdnpkg16.exe
upx --8086 -9 testhttp.exe
:gameover
