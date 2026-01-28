#!/bin/sh
# this script just updates the freedos fdnpkg16 zip package's contents














echo "zip"
if [ -d "fdnpkg16" ]; then
#rm fdnpkg16/BIN/*.exe
wmake clean
cp -rp fdpget.bat fdnpkg16/BIN/
cp -rp fdnpkg16.cfg fdnpkg16/BIN/
cp -rp fdnpkg16.oem fdnpkg16/BIN/
cp -rp fdnpkg16.4 fdnpkg16/BIN/
cp -rp fdnpkg16.ico fdnpkg16/BIN/
#. ./build.sh
wmake
rm *.obj
mv fdnpkg16.exe fdnpkg16/BIN/
mv httpget.exe fdnpkg16/BIN/
mv fdinst16.exe fdnpkg16/BIN/
cp -p *.txt fdnpkg16/DOC/FDNPKG16/
mkdir fdnpkg16/SOURCE/fdnpkg16
cp -p * fdnpkg16/SOURCE/fdnpkg16/
rm -r fdnpkg16/SOURCE/fdnpkg16/src
mkdir fdnpkg16/SOURCE/fdnpkg16/src
cp -rp makefile fdnpkg16/SOURCE/fdnpkg16/
cp -rp src/* fdnpkg16/SOURCE/fdnpkg16/src/
cd fdnpkg16/SOURCE/fdnpkg16/src/
if [ -e "WATT32" ]; then
  mv WATT32/INC WATT32INC
  mv WATT32/LIB WATT32LIB
  rm -rf WATT32
  mkdir watt32
  mv WATT32INC watt32/inc
  mv WATT32LIB watt32/lib
elif [ -e "watt32" ]; then
  mv watt32/inc watt32inc
  mv watt32/lib watt32lib
  rm -rf watt32
  mkdir watt32
  mv watt32inc watt32/inc
  mv watt32lib watt32/lib
fi
cd ../
#cd ../../../
#cd fdnpkg16/SOURCE/fdnpkg16/
rm ../fdnpkg16.zip
zip -r ../fdnpkg16.zip *
cd ../../
#rsync -avr --delete src/ fdnpkg16/SOURCE/fdnpkg16/src/
#cp -rp fdinst/* fdnpkg16/SOURCE/fdnpkg16/fdinst/
#pwd

rm -r SOURCE/fdnpkg16
zip -r ../fdnpkg16.zip *
cd ..
mv .temp/* .
rmdir .temp
rm -r fdnpkg16/
fi
#./build.sh
wmake
rm *.obj
