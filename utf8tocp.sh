cd fdnpkg16/NLS

iconv -t cp437 -f utf8 fdnpkg16.en8 > fdnpkg16.en

rm fdnpkg16.*8
cd ../..
./pkgzip.sh

