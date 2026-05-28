if [ ! -d "fdnpkg16" ]; then
./pkgunzip.sh
cd fdnpkg16/NLS

iconv -f cp437 -t utf8 fdnpkg16.en > fdnpkg16.en8
iconv -f cp865 -t utf8 fdnpkg16.fi > fdnpkg16.fi8
iconv -f cp850 -t utf8 fdnpkg16.es > fdnpkg16.es8
iconv -f cp852 -t utf8 fdnpkg16.cs > fdnpkg16.cs8
iconv -f cp850 -t utf8 fdnpkg16.da > fdnpkg16.da8
iconv -f cp850 -t utf8 fdnpkg16.de > fdnpkg16.de8
iconv -f cp858 -t utf8 fdnpkg16.eo > fdnpkg16.eo8
iconv -f cp775 -t utf8 fdnpkg16.et > fdnpkg16.et8
iconv -f cp850 -t utf8 fdnpkg16.fr > fdnpkg16.fr8
iconv -f cp852 -t utf8 fdnpkg16.hu > fdnpkg16.hu8
iconv -f cp861 -t utf8 fdnpkg16.is > fdnpkg16.is8
iconv -f cp858 -t utf8 fdnpkg16.it > fdnpkg16.it8
iconv -f cp775 -t utf8 fdnpkg16.lt > fdnpkg16.lt8
iconv -f cp775 -t utf8 fdnpkg16.lv > fdnpkg16.lv8
iconv -f cp858 -t utf8 fdnpkg16.nl > fdnpkg16.nl8
iconv -f cp865 -t utf8 fdnpkg16.no > fdnpkg16.no8
iconv -f cp860 -t utf8 fdnpkg16.pb > fdnpkg16.pb8
iconv -f cp852 -t utf8 fdnpkg16.pl > fdnpkg16.pl8
iconv -f cp860 -t utf8 fdnpkg16.pt > fdnpkg16.pt8
iconv -f cp866 -t utf8 fdnpkg16.ru > fdnpkg16.ru8
iconv -f cp852 -t utf8 fdnpkg16.si > fdnpkg16.si8
iconv -f cp852 -t utf8 fdnpkg16.sk > fdnpkg16.sk8
iconv -f cp852 -t utf8 fdnpkg16.sl > fdnpkg16.sl8
iconv -f cp855 -t utf8 fdnpkg16.sr > fdnpkg16.sr8
iconv -f cp858 -t utf8 fdnpkg16.sv > fdnpkg16.sv8
iconv -f cp857 -t utf8 fdnpkg16.tr > fdnpkg16.tr8
iconv -f cp855 -t utf8 fdnpkg16.uk > fdnpkg16.uk8

cd ../..
fi
