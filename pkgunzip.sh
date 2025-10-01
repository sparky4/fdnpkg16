#!/bin/sh
# this script just updates the freedos fdnpkg16 zip package's contents
echo "unzip"
if [ ! -d "fdnpkg16" ]; then
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
fi
