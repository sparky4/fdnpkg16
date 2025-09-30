@echo off
:start

if %1.==borlandc.  goto borlandc
if %1.==turboc.    goto turboc
if %1.==watcom.    goto watcom
if %1.==highc.     goto highc
if %1.==djgpp.     goto djgpp
if %1.==djgpp_dxe. goto djgpp_dxe
if %1.==digmars.   goto digmars
if %1.==quickc.    goto quickc
if %1.==visualc.   goto visualc
if %1.==mingw32.   goto mingw32
if %1.==pellesc.   goto pellesc
if %1.==ladsoft.   goto ladsoft
if %1.==lcc.       goto lcc
if %1.==all.       goto all
if %1.==clean.     goto clean
goto usage

::--------------------------------------------------------------------------
:borlandc
::
..\..\util\mkmake -o bcc_s.mak makefile.all BORLANDC SMALL
..\..\util\mkmake -o bcc_l.mak makefile.all BORLANDC LARGE
..\..\util\mkmake -o bcc_f.mak makefile.all BORLANDC FLAT
..\..\util\mkmake -o bcc_w.mak makefile.all BORLANDC WIN32
..\..\util\mkdep -s.obj -p$(OBJDIR)\ *.c > ..\borland\zlib.dep
goto next

::--------------------------------------------------------------------------
:turboc
::
..\..\util\mkmake -o tcc_s.mak makefile.all TURBOC SMALL
..\..\util\mkmake -o tcc_l.mak makefile.all TURBOC LARGE
..\..\util\mkdep -s.obj -p$(OBJDIR)\ *.c > ..\turboc\zlib.dep
goto next

::--------------------------------------------------------------------------
:watcom
::
..\..\util\mkmake -o watcom_s.mak makefile.all WATCOM SMALL
..\..\util\mkmake -o watcom_l.mak makefile.all WATCOM LARGE
..\..\util\mkmake -o watcom_f.mak makefile.all WATCOM FLAT
..\..\util\mkmake -o watcom_w.mak makefile.all WATCOM WIN32
..\..\util\mkdep -s.obj -p$(OBJDIR)\ *.c > ..\watcom\zlib.dep
goto next

::--------------------------------------------------------------------------
:highc
::
..\..\util\mkmake -o highc.mak makefile.all HIGHC
..\..\util\mkdep -s.obj -p$(OBJDIR)\ *.c > ..\highc\zlib.dep
goto next

::--------------------------------------------------------------------------
:ladsoft
::
..\..\util\mkmake -o ladsoft.mak makefile.all LADSOFT
..\..\util\mkdep -s.obj -p$(OBJDIR)\ *.c > ..\ladsoft\zlib.dep
goto next

::--------------------------------------------------------------------------
:lcc
::
..\..\util\mkmake -o lcc.mak makefile.all LCC WIN32
..\..\util\mkdep -s.obj -p$(OBJDIR)\ *.c > ..\lcc\zlib.dep
goto next

::--------------------------------------------------------------------------
:pellesc
::
..\..\util\mkmake -o pellesc.mak makefile.all PELLESC WIN32
..\..\util\mkdep -s.obj -p$(OBJDIR)\ *.c > ..\pellesc\zlib.dep
goto next

::--------------------------------------------------------------------------
:quickc
::
..\..\util\mkmake -o quickc_s.mak makefile.all QUICKC SMALL
..\..\util\mkmake -o quickc_l.mak makefile.all QUICKC LARGE
..\..\util\mkdep -s.obj -p$(OBJDIR)\ *.c > ..\quickc\zlib.dep
goto next

::--------------------------------------------------------------------------
:visualc
::
..\..\util\mkmake -o visualc-release.mak makefile.all VISUALC WIN32 RELEASE
..\..\util\mkmake -o visualc-debug.mak   makefile.all VISUALC WIN32 DEBUG
..\..\util\mkdep -s.obj -p$(OBJDIR)\ *.c > ..\visualc\zlib.dep
goto next

::--------------------------------------------------------------------------
:digmars
::
..\..\util\mkmake -o dmars_s.mak makefile.all DIGMARS SMALL
..\..\util\mkmake -o dmars_l.mak makefile.all DIGMARS LARGE
..\..\util\mkmake -o dmars_f.mak makefile.all DIGMARS FLAT
..\..\util\mkmake -o dmars_w.mak makefile.all DIGMARS WIN32
..\..\util\mkdep -s.obj -p$(OBJDIR)\ *.c > ..\digmars\zlib.dep
goto next

::--------------------------------------------------------------------------
:djgpp
::
..\..\util\mkmake -o djgpp.mak makefile.all DJGPP
..\..\util\mkdep -s.o -p$(OBJDIR)/ *.c > ..\djgpp\zlib.dep
goto next

::--------------------------------------------------------------------------
:djgpp_dxe
::
..\..\util\mkmake -o dj_dxe.mak makefile.all DJGPP_DXE
..\..\util\mkdep -s.o -p$(OBJDIR)/ *.c > ..\djgpp\zlib.dep
goto next

::--------------------------------------------------------------------------
:mingw32
::
..\..\util\mkmake -o mingw32.mak makefile.all MINGW32 WIN32
..\..\util\mkdep -s.o -p$(OBJDIR)/ *.c > ..\mingw32\zlib.dep
goto next

::--------------------------------------------------------------------------
:usage
::
echo Configuring Waterloo tcp/ip targets.
echo Usage %0 {borlandc, turboc, watcom, quickc, visualc, highc, ladsoft, pellesc, djgpp, djgpp_dxe, digmars, mingw32, lcc, all, clean}
goto quit

::--------------------------------------------------------------------------
:clean
::
del djgpp.mak
del dj_dxe.mak
del quickc_*.mak
del visualc-*.mak
del watcom_*.mak
del bcc_*.mak
del tcc_*.mak
del highc.mak
del dmars_*.mak
del mingw32.mak
del ladsoft.mak
del lcc.mak
del pellesc.mak

del ..\djgpp\zlib.dep
del ..\quickc\zlib.dep
del ..\visualc\zlib.dep
del ..\watcom\zlib.dep
del ..\borland\zlib.dep
del ..\turboc\zlib.dep
del ..\highc\zlib.dep
del ..\digmars\zlib.dep
del ..\mingw32\zlib.dep
del ..\ladsoft\zlib.dep
del ..\lcc\zlib.dep
del ..\pellesc\zlib.dep
goto next

::------------------------------------------------------------
:all
::
call %0 borlandc  %2
call %0 turboc    %2
call %0 watcom    %2
call %0 highc     %2
call %0 quickc    %2
call %0 visualc   %2
call %0 djgpp     %2
call %0 djgpp_dxe %2
call %0 digmars   %2
call %0 mingw32   %2
call %0 ladsoft   %2
call %0 lcc       %2
call %0 pellesc   %2

:next
shift
if not %1. == . goto start
:quit
