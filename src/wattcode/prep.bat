cd ..
mkdir watt32
cd watt32
unzip -n ..\watt32.zip
cd ..\wattcode
xcopy /y /s util\*.* ..\watt32\util\
xcopy /y /s src\*.* ..\watt32\src\
rem mkdir ..\watt32\src\build
mkdir ..\watt32\src\build\watcom
rem mkdir ..\watt32\src\build\watcom\large
