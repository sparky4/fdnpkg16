mkdir fdnpkg16
mkdir .temp
mv *.zip .temp/
mv *.7z .temp/
mv fdnpkg16.zip fdnpkg16/
cd fdnpkg16
unzip fdnpkg16.zip
rm fdnpkg16.zip
cd ..
cp -rp fdpget.bat fdnpkg16/BIN/
cp -rp fdnpkg16.cfg fdnpkg16/BIN/
. ./build.sh
rm *.o
mv fdnpkg16.exe fdnpkg16/BIN/
cp -p * fdnpkg16/SOURCE/fdnpkg16/
cp -p src/* fdnpkg16/SOURCE/fdnpkg16/src/
cd fdnpkg16
zip -r ../fdnpkg16.zip *
cd ..
mv .temp/* .
rmdir .temp
rm -rf fdnpkg16/
