if [ -d "fdnpkg16" ]; then
cd fdnpkg16/NLS

iconv -t cp437 -f utf8 fdnpkg16.en8 > fdnpkg16.en
iconv -t cp865 -f utf8 fdnpkg16.fi8 > fdnpkg16.fi
iconv -t cp850 -f utf8 fdnpkg16.es8 > fdnpkg16.es
iconv -t cp852 -f utf8 fdnpkg16.cs8 > fdnpkg16.cs
iconv -t cp850 -f utf8 fdnpkg16.da8 > fdnpkg16.da
iconv -t cp850 -f utf8 fdnpkg16.de8 > fdnpkg16.de
iconv -t cp858 -f utf8 fdnpkg16.eo8 > fdnpkg16.eo
iconv -t cp775 -f utf8 fdnpkg16.et8 > fdnpkg16.et
iconv -t cp850 -f utf8 fdnpkg16.fr8 > fdnpkg16.fr
iconv -t cp852 -f utf8 fdnpkg16.hu8 > fdnpkg16.hu
iconv -t cp861 -f utf8 fdnpkg16.is8 > fdnpkg16.is
iconv -t cp858 -f utf8 fdnpkg16.it8 > fdnpkg16.it
iconv -t cp775 -f utf8 fdnpkg16.lt8 > fdnpkg16.lt
iconv -t cp775 -f utf8 fdnpkg16.lv8 > fdnpkg16.lv
iconv -t cp858 -f utf8 fdnpkg16.nl8 > fdnpkg16.nl
iconv -t cp865 -f utf8 fdnpkg16.no8 > fdnpkg16.no
iconv -t cp860 -f utf8 fdnpkg16.pb8 > fdnpkg16.pb
iconv -t cp852 -f utf8 fdnpkg16.pl8 > fdnpkg16.pl
iconv -t cp860 -f utf8 fdnpkg16.pt8 > fdnpkg16.pt
iconv -t cp866 -f utf8 fdnpkg16.ru8 > fdnpkg16.ru
iconv -t cp852 -f utf8 fdnpkg16.si8 > fdnpkg16.si
iconv -t cp852 -f utf8 fdnpkg16.sk8 > fdnpkg16.sk
iconv -t cp852 -f utf8 fdnpkg16.sl8 > fdnpkg16.sl
iconv -t cp855 -f utf8 fdnpkg16.sr8 > fdnpkg16.sr
iconv -t cp858 -f utf8 fdnpkg16.sv8 > fdnpkg16.sv
iconv -t cp857 -f utf8 fdnpkg16.tr8 > fdnpkg16.tr
iconv -t cp855 -f utf8 fdnpkg16.uk8 > fdnpkg16.uk

rm fdnpkg16.*8
cd ../..
./pkgzip.sh
fi
