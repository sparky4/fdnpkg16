@echo off
set ver=0.02d
rem set this to something for verboseness of error msg
set errorecho=
rem fdpget.bat by sparky4 uses HTGET from mTCP
REM SETTINGS START
if [%getter%] == [] set getter=htget
rem if exist %dosdir%\net\mtcp set getterlocation=%dosdir%\net\mtcp\
rem if exist %dosdir%\bin\%getter% set getterlocation=%dosdir%\bin\
set installer=fdinst
rem this is the patch for freecom
rem set patchforcommand=1
REM SETTINGS END
if NOT %OS_NAME% == FreeDOS goto notFreeDOS
if [%1%] == [] goto none
if [%2%] == [] goto none
if [%DOSDIR%] == [] goto none
if [%temp%] == [] goto none
set dest=%temp%\%2.zip
rem not nessesary! 
rem set cwdpee=%_CWD%
rem cdd %temp%

echo ==fdpget.bat== getting file!
if %getter%==htget goto htget
goto wget
:htget
if [%OS_VERSION%]==[] goto none
rem NO 1.0 REPO USE FULL CD
rem if not [%patchforcommand%]==[] goto nopatch
rem if not [%3]==[] goto useunofficial
rem :nopatch
rem if [%patchforcommand%]==[] goto skipnopatch
if not [%3]==[] %getter% -o %dest% http://dos.lod.bz/repos/current/%1/%2.zip
rem :skipnopatch
if [%3]==[] %getter% -o %dest% http://www.ibiblio.org/pub/micro/pc-stuff/freedos/files/repositories/%OS_VERSION%/%1/%2.zip

rem goto officialdone
rem :useunofficial
rem %getter% -o %dest% http://dos.lod.bz/repos/current/%1/%2.zip
rem :officialdone

if not [%errorecho%]==[] echo %ERRORLEVEL%
if not %ERRORLEVEL%==21 goto done
goto inst
:wget
if [%OS_VERSION%]==[] goto none
rem NO 1.0 REPO USE FULL CD
if not [%3]==[] %getter% http://dos.lod.bz/repos/current/%1/%2.zip
if [%3]==[] %getter% http://www.ibiblio.org/pub/micro/pc-stuff/freedos/files/repositories/%OS_VERSION%/%1/%2.zip

move %2.zip %dest%
if not [%errorecho%]==[] echo %ERRORLEVEL%
rem if not %ERRORLEVEL%==21 goto done
goto inst

:inst
if %ERRORLEVEL%==45 goto done
if not exist %dest% goto nofile
echo ==fdpget.bat== installing!
if %dest%==fdnpkg goto checkfdpkg
if not %installer%==fdinst goto checkfdpkg
if exist %dosdir%\bin\fdinst.exe goto fdinst
:checkfdpkg
if exist %dosdir%\bin\fdpkg.exe goto fdpkg
echo ==fdpget.bat== package manager not found!
goto nopkgm

:fdpkg
fdpkg /install %dest%
goto done

:removepkg
echo ==fdpget.bat== removing so we can update it!
fdinst remove %2
if not [%errorecho%]==[] echo %ERRORLEVEL%
if %ERRORLEVEL%==1 goto end

:fdinst
if %dest%==fdnpkg goto fdpkg
fdinst install %dest%
if not [%errorecho%]==[] echo %ERRORLEVEL%
if %ERRORLEVEL%==1 goto removepkg
goto done

:nofile
echo file not found!
goto end

:notFreeDOS
echo Use this with FreeDOS!
goto done

:nopkgm
echo ==fdpget.bat== getting sparky4's custom
echo fdpkg it works with FreeDOS 1.1+ packages
htget http://4ch.mooo.com/fdos/fdpkg.exe > %dosdir%\bin\fdpkg.exe

:none
echo fdpget.bat version %ver% uses %getter%
echo please type the 2+ arguments of fdpget "1" "2" "3"
echo 1 is repository (ex. util, net, etc.), 2 is package (no .zip)
echo 3 is optional for unofficial repository (You can use anything)
echo configured for %OS_VERSION% updates
echo copyleft sparky4 (Victoria Crenshaw) 2025 GNU GPL v3

:done
if exist %dest% del %dest%
:end
rem clean up
rem cd %cwdpee%
rem set cwdpee=
set getter=
set dest=
set installer=
set ver=
set errorecho=
rem set patchforcommand=
