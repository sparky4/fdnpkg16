@echo off
rem echo "src\zlib\zlib_l.lib src\watt32\lib\wattcpwl.lib" > wlib.ar
wcl -0 -lr -ml -opnr -oil+ -outback -ohm -sg -d0 -k24576 -i=src -i=src\zlib -i=src\watt32\inc src\*.c src\zlib\zlib_l.lib src\watt32\lib\wattcpwl.lib
echo ===============================================================================
rem wcl -0 -lr -ml -os -wx -we -d0 -i=..\ -i=..\zlib\ -DNOREPOS -DNOLZMA -fe=fdnpkg16.exe src\*.c ..\zlib\zlib_l.lib
rem wcl -0 -lr -ml -os -wx -we -d0 -k40960 -i=src\ -i=src\zlib\ -DNOREPOS -DNOLZMA -fe=fdnpkg16.exe src\*.c src\zlib\zlib_l.lib
if ERRORLEVEL 1 goto gameover
rem upx --8086 -9 fdnpkg16.exe
rem upx --8086 -9 fdnpkg86.exe
rem upx --8086 -9 httpget.exe
rem upx --8086 -9 unzip16.exe

:gameover
