#!/bin/sh

# $Id: autogen.sh,v 1.8 2013/10/06 05:00:15 nkbj Exp $

# Generate all autoconf/automake files, to prepare for running
# configure from only the contents of CVS.

# We silently assume that Automake 1.5 (released 2001-08-23) or higher is used.

aclocal
autoconf
autoheader
automake --foreign --add-missing --copy
