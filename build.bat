@echo off

rem wcl -0 -lr -ml -os -wx -we -d0 -i=..\ -i=..\zlib\ -DNOREPOS -DNOLZMA -fe=fdnpkg16.exe src\*.c ..\zlib\zlib_l.lib
wcl -0 -lr -ml -os -wx -we -d0 -k20480 -i=src/ -i=src/zlib/ -DNOREPOS -DNOLZMA -fe=fdnpkg16.exe src/*.c src/zlib/zlib_l.lib
if ERRORLEVEL 1 goto gameover

rem del src\*.c
rem rmdir src

upx --8086 -9 fdinst.exe

:gameover
