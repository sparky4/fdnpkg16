.  ./wattroot.sh
cd %WATT_ROOT%/src
mkdir build
mkdir watcom
mkdir watcom/large
mkdir build/watcom
mkdir build/watcom/large
./configur.sh watcom
wmake -f watcom_l.mak
