#!/bin/sh
. ./clean.sh
. ./build.sh
cat *.err
rm *.o
rm *.err
