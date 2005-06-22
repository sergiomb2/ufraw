#!/bin/sh
# Generate all autoconf/automake files.
# First look for the heighest version of automake available.
AUTOMAKE=automake-1.9
ACLOCAL=aclocal-1.9
($AUTOMAKE --version) < /dev/null > /dev/null 2>&1 || {
    AUTOMAKE=automake-1.8
    ACLOCAL=aclocal-1.8
}
($AUTOMAKE --version) < /dev/null > /dev/null 2>&1 || {
    AUTOMAKE=automake-1.7
    ACLOCAL=aclocal-1.7
}
($AUTOMAKE --version) < /dev/null > /dev/null 2>&1 || {
    AUTOMAKE=automake-1.6
    ACLOCAL=aclocal-1.6
}
($AUTOMAKE --version) < /dev/null > /dev/null 2>&1 || {
    AUTOMAKE=automake-1.5
    ACLOCAL=aclocal-1.5
}
($AUTOMAKE --version) < /dev/null > /dev/null 2>&1 || {
    echo "You must have automake-1.5 or heigher"
    exit 1
}
$ACLOCAL
autoconf
autoheader
$AUTOMAKE --foreign --add-missing --copy
