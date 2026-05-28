#!/bin/bash
if [ -e "src/watt32/lib/WATTCPWL.LIB" ]; then
	mv src/watt32/lib/WATTCPWL.LIB src/watt32/lib/WATTCPW0.LIB
	mv src/watt32/lib/WATTCPW0.LIB src/watt32/lib/wattcpwl.lib
fi
#upx --8086 -9 fdinst16.exe
#upx --8086 -9 fdnpkg16.exe
#upx --8086 -9 httpget.exe
for file in *.err; do
  if [ -f "$file" ]; then
    cat *.err
  fi
done
for file in *.exe; do
  if [ -f "$file" ]; then
    ls $file
  fi
done
