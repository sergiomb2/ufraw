#!/bin/bash

# this uses astyle to standardize our indentation/braces etc

SOURCES=$(find . -name "*.c" -or -name "*.cc" -or -name "*.h" | grep -v dcraw.cc | grep -v wb_presets.c)

for i in $SOURCES
do
  astyle --style=k/r --indent=spaces=4 --indent-switches --pad-oper --pad-header --unpad-paren < $i > UFRawBeautifiStyle.tmp
  mv UFRawBeautifiStyle.tmp $i
done
