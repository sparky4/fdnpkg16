call wattroot.bat
cd %WATT_ROOT%\src
::mkdir %WATT_ROOT%\src\build
::mkdir %WATT_ROOT%\build\watcom
::mkdir %WATT_ROOT%\build\watcom\large
configur.bat watcom
wmake -f watcom_l.mak
