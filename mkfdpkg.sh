#!/bin/sh
# this script just updates the freedos fdnpkg16 zip package's contents
./pkgunzip.sh
read -p "Press Enter to continue..."
./pkgzip.sh
if id "sparky4" >/dev/null 2>&1; then
. ~/p/copyfdn.sh
fi
