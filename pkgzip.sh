#!/bin/sh
# this script just updates the freedos fdnpkg16 zip package's contents











echo "zip"
rm fdnpkg16/BIN/*.exe
cp -rp fdpget.bat fdnpkg16/BIN/
cp -rp fdnpkg16.cfg fdnpkg16/BIN/
. ./build.sh
rm *.o
mv fdnpkg16.exe fdnpkg16/BIN/
cp -p * fdnpkg16/SOURCE/fdnpkg16/
cp -rp src/* fdnpkg16/SOURCE/fdnpkg16/src/
cd fdnpkg16
zip -r ../fdnpkg16.zip *
cd ..
mv .temp/* .
rmdir .temp
rm -r fdnpkg16/
