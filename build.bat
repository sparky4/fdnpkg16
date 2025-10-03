@echo off
wcl -0 -lr -ml -opnr -oe=24 -oil+ -outback -ohm -sg -wx -we -d0 -k24576 -fpi -i=src -i=src\zlib -i=src\watt32\inc -fe=fdnpkg86.exe src\*.c src\zlib\zlib_l.lib src\watt32\lib\wattcpwl.lib
wcl -0 -lr -ml -opnr -oe=24 -oil+ -outback -ohm -sg -wx -we -d0 -k24576 -fpi -DUSE_EXTERNAL_MTCP -i=src -i=src\zlib -fe=fdnpkg16.exe src\crc32.c src\fileexst.c src\kprintf0.c src\loadconf.c src\parsecmd.c src\pkginst.c src\readenv.c src\dbg.c src\getdelim.c src\inf.c src\libgz.c src\lsm.c src\parseurl.c src\pkgrem.c src\rtrim.c src\fdnpkg16.c src\helpers.c src\kitten.c src\libunzip.c src\pkgdb.c src\pkgsrch.c src\showinst.c src/dbg.c src\zlib\zlib_l.lib
wcl -0 -lr -ml -opnr -oe=24 -oil+ -outback -ohm -sg -wx -we -d0 -k24576 -fpi -i=src -i=src\zlib -i=src\watt32\inc -fe=httpget.exe testhttp.c src\net.c src\http.c src\parseurl.c src\helpers.c src\dbg.c src\zlib\zlib_l.lib src\watt32\lib\wattcpwl.lib
rem wcl -0 -lr -ml -os -wx -we -d0 -i=..\ -i=..\zlib\ -DNOREPOS -DNOLZMA -fe=fdnpkg16.exe src\*.c ..\zlib\zlib_l.lib
rem wcl -0 -lr -ml -os -wx -we -d0 -k40960 -i=src\ -i=src\zlib\ -DNOREPOS -DNOLZMA -fe=fdnpkg16.exe src\*.c src\zlib\zlib_l.lib
if ERRORLEVEL 1 goto gameover
upx --8086 -9 fdnpkg16.exe
upx --8086 -9 fdnpkg86.exe
upx --8086 -9 httpget.exe
:gameover
