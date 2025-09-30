@echo off
if %WATT_ROOT%. ==. goto not_set
if not exist %WATT_ROOT%\src\makefile.all goto not_set
set src=*.[ch]
set src_wc=*.c

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
if %1.==ladsoft.   goto ladsoft
if %1.==lcc.       goto lcc
if %1.==pellesc.   goto pellesc
if %1.==all.       goto all
if %1.==clean.     goto clean
goto usage

::--------------------------------------------------------------------------
:borlandc
::
echo Generating Borland-C makefiles, directories, errnos and dependencies
..\util\mkmake  -o bcc_s.mak -d borland\small makefile.all BORLANDC SMALL
..\util\mkmake  -o bcc_l.mak -d borland\large makefile.all BORLANDC LARGE
..\util\mkmake  -o bcc_f.mak -d borland\flat  makefile.all BORLANDC FLAT
..\util\mkmake  -o bcc_w.mak -d borland\win32 makefile.all BORLANDC WIN32
..\util\mkdep -s.obj -p$(OBJDIR)\ %SRC% > borland\watt32.dep
..\util\bcc_err -s                      > borland\syserr.c
..\util\bcc_err -e                      > ..\inc\sys\borlandc.err
echo neterr.c: borland\syserr.c        >> borland\watt32.dep

echo Run make to make target(s):
echo   E.g. "maker -f bcc_l.mak" for large model
goto next

::--------------------------------------------------------------------------
:turboc
::
echo Generating Turbo-C makefiles, directories, errnos and dependencies
..\util\mkmake  -o tcc_s.mak -d turboc\small makefile.all TURBOC SMALL
..\util\mkmake  -o tcc_l.mak -d turboc\large makefile.all TURBOC LARGE
..\util\mkdep -s.obj -p$(OBJDIR)\ %SRC% > turboc\watt32.dep
..\util\tcc_err -s                      > turboc\syserr.c
..\util\tcc_err -e                      > ..\inc\sys\turboc.err
echo neterr.c: borland\syserr.c        >> turboc\watt32.dep

echo Run make to make target(s):
echo   E.g. "make -f tcc_l.mak" for large model
goto next

::--------------------------------------------------------------------------
:watcom
::
echo Generating Watcom makefiles, directories, errnos and dependencies
..\util\mkmake -o watcom_s.mak -d watcom\small makefile.all WATCOM SMALL
..\util\mkmake -o watcom_l.mak -d watcom\large makefile.all WATCOM LARGE
..\util\mkmake -o watcom_f.mak -d watcom\flat  makefile.all WATCOM FLAT
..\util\mkmake -o watcom_w.mak -d watcom\win32 makefile.all WATCOM WIN32
..\util\mkdep -s.obj -p$(OBJDIR)\ %SRC_WC% > watcom\watt32.dep
..\util\wc_err -s                          > watcom\syserr.c
..\util\wc_err -e                          > ..\inc\sys\watcom.err

echo Run wmake to make target(s):
echo   E.g. "wmake -f watcom_l.mak" for large model
goto next

::--------------------------------------------------------------------------
:highc
::
echo Generating Metaware High-C makefile, directory, errnos and dependencies
..\util\mkmake -o highc.mak -d highc makefile.all HIGHC
..\util\mkdep -s.obj -p$(OBJDIR)\ %SRC% > highc\watt32.dep
..\util\hc_err -s                       > highc\syserr.c
..\util\hc_err -e                       > ..\inc\sys\highc.err
echo neterr.c: highc\syserr.c          >> highc\watt32.dep

echo Run a Borland compatible make to make target:
echo   "maker -f highc.mak"
goto next

::--------------------------------------------------------------------------
:ladsoft
::
echo Generating LADsoft makefile, directory, errnos and dependencies
..\util\mkmake -o ladsoft.mak -d ladsoft makefile.all LADSOFT
..\util\mkdep -s.obj -p$(OBJDIR)\ %SRC% > ladsoft\watt32.dep
..\util\ls_err -s                       > ladsoft\syserr.c
..\util\ls_err -e                       > ..\inc\sys\ladsoft.err
echo neterr.c: ladsoft\syserr.c        >> ladsoft\watt32.dep

echo Run a Borland compatible make to make target:
echo   "maker -f ladsoft.mak"
goto next

::--------------------------------------------------------------------------
:quickc
::
echo Generating Microsoft Quick-C makefiles, directories, errnos and dependencies
..\util\mkmake -o quickc_s.mak -d quickc\small makefile.all QUICKC SMALL
..\util\mkmake -o quickc_l.mak -d quickc\large makefile.all QUICKC LARGE
..\util\mkdep -s.obj -p$(OBJDIR)\ %SRC% > quickc\watt32.dep
..\util\ms_err -s                       > quickc\syserr.c
..\util\ms_err -e                       > ..\inc\sys\quickc.err
echo neterr.c: quickc\syserr.c         >> quickc\watt32.dep

::
:: Must run nasm here because MS's nmake is a pmode program
::
..\util\nasm -f bin -l asmpkt.lst -o asmpkt.bin asmpkt.nas
..\util\bin2c asmpkt.bin > pkt_stub.h
echo Run nmake to make target(s):
echo   E.g. "nmake -f quickc_l.mak" for large model
goto next

::--------------------------------------------------------------------------
:visualc
::
echo Generating Microsoft Visual-C makefiles, directories, errnos and dependencies
set _lfn=%lfn
set lfn=y
..\util\mkmake -o visualc-release.mak -d visualc\release makefile.all VISUALC WIN32 RELEASE
..\util\mkmake -o visualc-debug.mak   -d visualc\debug   makefile.all VISUALC WIN32 DEBUG
..\util\mkdep -s.obj -p$(OBJDIR)\ %SRC% > visualc\watt32.dep
..\util\vc_err -s                       > visualc\syserr.c
..\util\vc_err -e                       > ..\inc\sys\visualc.err
echo neterr.c: visualc\syserr.c        >> visualc\watt32.dep
set lfn=%_lfn
set _lfn=

echo Run nmake to make target(s):
echo   E.g. "nmake -f visualc-release.mak"
goto next

::--------------------------------------------------------------------------
:lcc
::
echo Generating LCC-Win32 makefile, directory, errnos and dependencies
..\util\mkmake -o lcc.mak -d lcc makefile.all LCC WIN32
..\util\mkdep -s.obj -p$(OBJDIR)\ %SRC% > lcc\watt32.dep
..\util\lcc_err -s                      > lcc\syserr.c
..\util\lcc_err -e                      > ..\inc\sys\lcc.err
echo neterr.c: lcc\syserr.c            >> lcc\watt32.dep

echo Run make to make target:
echo   E.g. "make -f lcc.mak"
goto next

::--------------------------------------------------------------------------
:pellesc
::
echo Generating PellesC makefile, directory, errnos and dependencies
..\util\mkmake -o pelles.mak -d pellesc makefile.all PELLESC WIN32
..\util\mkdep -s.obj -p$(OBJDIR)\ %SRC% > pellesc\watt32.dep
..\util\po_err -s                       > pellesc\syserr.c
..\util\po_err -e                       > ..\inc\sys\pellesc.err
echo neterr.c: pellesc\syserr.c        >> pellesc\watt32.dep

echo Run pomake to make target:
echo   E.g. "pomake -f pellesc.mak"
goto next

::--------------------------------------------------------------------------
:digmars
::
echo Generating Digital Mars makefiles, directories, errnos and dependencies
..\util\mkmake -o dmars_s.mak -d digmars\small  makefile.all DIGMARS SMALL
..\util\mkmake -o dmars_l.mak -d digmars\large  makefile.all DIGMARS LARGE
..\util\mkmake -o dmars_f.mak -d digmars\flat   makefile.all DIGMARS FLAT
..\util\mkmake -o dmars_w.mak -d digmars\win32  makefile.all DIGMARS WIN32
..\util\mkdep -s.obj -p$(OBJDIR)\ %SRC% > digmars\watt32.dep
..\util\dm_err -s                       > digmars\syserr.c
..\util\dm_err -e                       > ..\inc\sys\digmars.err
echo neterr.c : digmars\syserr.c       >> digmars\watt32.dep

echo Run make to make target(s):
echo   E.g. "maker -f dmars_l.mak" for large model
goto next

::--------------------------------------------------------------------------
:djgpp
::
echo Generating DJGPP makefile, directory, errnos and dependencies
..\util\mkmake -o djgpp.mak -d djgpp makefile.all  DJGPP
..\util\mkdep -s.o -p$(OBJDIR)/ %SRC% > djgpp\watt32.dep
..\util\dj_err -s                     > djgpp\syserr.c
..\util\dj_err -e                     > ..\inc\sys\djgpp.err
echo neterr.c: djgpp/syserr.c        >> djgpp\watt32.dep

echo Run GNU make to make target:
echo   make -f djgpp.mak
goto next

::--------------------------------------------------------------------------
:djgpp_dxe
::
echo Generating DJGPP (DXE) makefile, directory, errnos and dependencies
..\util\mkmake -o dj_dxe.mak -d djgpp\dxe makefile.all DJGPP_DXE
..\util\mkdep -s.o -p$(OBJDIR)/ %SRC% > djgpp\watt32.dep
..\util\dj_err -s                     > djgpp\syserr.c
..\util\dj_err -e                     > ..\inc\sys\djgpp.err
echo neterr.c: djgpp/syserr.c        >> djgpp\watt32.dep

:: to do
:: ..\util\mkimp --stub *.c > djgpp\dxe\stubs.inc
:: ..\util\mkimp --load *.c > djgpp\dxe\symbols.inc
:: ..\util\mkimp --dxe  *.c > djgpp\dxe\dxe_init.inc
echo Run GNU make to make targets:
echo   make -f dj_dxe.mak
goto next

::--------------------------------------------------------------------------
:mingw32
::
echo Generating MingW32 makefile, directory, errnos and dependencies
..\util\mkmake -o MingW32.mak -d MingW32 makefile.all MINGW32 WIN32
..\util\mkdep -s.o -p$(OBJDIR)/ %SRC% > MingW32\watt32.dep
..\util\mw_err -s                     > MingW32\syserr.c
..\util\mw_err -e                     > ..\inc\sys\mingw32.err
echo neterr.c: MingW32/syserr.c      >> Mingw32\watt32.dep

echo Run GNU make to make target:
echo   make -f MingW32.mak
goto next

::--------------------------------------------------------------------------
:usage
::
echo Configuring Waterloo tcp/ip targets.
echo Usage %0 {borlandc, turboc, watcom, quickc, visualc, highc, ladsoft, djgpp, djgpp_dxe, digmars, MingW32, lcc, pellesc, all, clean}
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
del MingW32.mak
del ladsoft.mak
del lcc.mak
del pellesc.mak

del djgpp\watt32.dep
del quickc\watt32.dep
del visualc\watt32.dep
del watcom\watt32.dep
del borland\watt32.dep
del turboc\watt32.dep
del highc\watt32.dep
del digmars\watt32.dep
del MingW32\watt32.dep
del ladsoft\watt32.dep
del lcc\watt32.dep
del pellesc\watt32.dep

del djgpp\syserr.c
del quickc\syserr.c
del visualc\syserr.c
del watcom\syserr.c
del turboc\syserr.c
del borland\syserr.c
del highc\syserr.c
del digmars\syserr.c
del MingW32\syserr.c
del ladsoft\syserr.c
del lcc\syserr.c
del pellesc\syserr.c

del ..\inc\sys\djgpp.err
del ..\inc\sys\quickc.err
del ..\inc\sys\visualc.err
del ..\inc\sys\watcom.err
del ..\inc\sys\turboc.err
del ..\inc\sys\borlandc.err
del ..\inc\sys\highc.err
del ..\inc\sys\digmars.err
del ..\inc\sys\mingw32.err
del ..\inc\sys\ladsoft.err
del ..\inc\sys\lcc.err
del ..\inc\sys\pellesc.err
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
call %0 MingW32   %2
call %0 ladsoft   %2
call %0 lcc       %2
call %0 pellesc   %2

:next
cd zlib
call configur.bat %1
cd ..

shift
echo.
if not %1. == . goto start
goto quit

:not_set
echo Environment variable WATT_ROOT not set (or incorrectly set).
echo Put this in your AUTOEXEC.BAT:
echo   e.g. "SET WATT_ROOT=C:\NET\WATT"

:quit
set src=
set src_wc=

