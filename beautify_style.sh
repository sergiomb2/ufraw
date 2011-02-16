#!/bin/bash

# this uses astyle to standardize our indentation/braces etc

SOURCES=$(find . -name "*.c" -or -name "*.cc" -or -name "*.h" | grep -v dcraw.cc)

for i in $SOURCES
do
  astyle --style=k/r --indent=spaces=2 --indent-switches < $i > nkbj.c
  mv nkbj.c $i
done
