#!/bin/bash

# This uses astyle to standardize our indentation/braces etc.

SOURCES=$(find . -name "*.c" -or -name "*.cc" -or -name "*.h" | grep -v dcraw.cc | grep -v iccjpeg.c | grep -v iccjpeg.h | grep -v wb_presets.c)

for i in $SOURCES
do
  astyle --style=k/r --indent=spaces=4 --indent-switches --pad-oper --pad-header --unpad-paren < $i > UFRawBeautifyStyle.tmp
  mv UFRawBeautifyStyle.tmp $i
done

astyle --style=k/r --indent=spaces=2 --indent-switches < wb_presets.c > UFRawBeautifyStyle.tmp
mv UFRawBeautifyStyle.tmp wb_presets.c
