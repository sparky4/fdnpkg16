#!/bin/sh
# this script just updates the freedos fdnpkg16 zip package's contents
echo "unzip"
cp -rp fdnpkg16.zip ../
mkdir fdnpkg16
mkdir .temp
mv fdnpkg16.zip fdnpkg16/
mv *.zip .temp/
mv *.7z .temp/
cd fdnpkg16
unzip fdnpkg16.zip
rm fdnpkg16.zip
cd ..
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
cd SOURCE/fdnpkg16/src/
if [ -e "WATT32" ]; then
  mv WATT32/INC WATT32INC
  mv WATT32/LIB WATT32LIB
  rm -rf WATT32
  mkdir watt32
  mv WATT32INC watt32/INC
  mv WATT32LIB watt32/LIB
elif [ -e "watt32" ]; then
  mv watt32/inc watt32inc
  mv watt32/lib watt32lib
  rm -rf watt32
  mkdir watt32
  mv watt32inc watt32/inc
  mv watt32lib watt32/lib
fi
cd ../../../
zip -r ../fdnpkg16.zip *
cd ..
mv .temp/* .
rmdir .temp
rm -r fdnpkg16/
