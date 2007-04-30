UFRaw detailed processing description

$Id: README-processing.txt,v 1.2 2007/04/30 17:14:02 udifuchs Exp $

This document is a work in progress and may contain inaccurate information.


== Introduction

This document describes how UFRaw processes raw files to produce output
files.  The description is intended to be correct and precise, even at
the expense of readability.  This document is not a HOWTO.

This document assumes that the reader is generally familiar with computer
image processing, and understands the issues surrounding Bayer arrays,
white balance, and color management.

== Reading Raw files

The raw file is read and per-pixel sensor values extracted.
Each pixel is represented by integer number which is between
8 and 16 bits in size. 8 bits are used in some old cameras, while
16 bits are reserved for the more profesional cameras. Most cameras
today use 12-bits.

=== Canon raw file encoding

Canon is different.   TODO (See ufraw_develop.cc for details).

== TODO

input profiles

working color space, and what the gamma means on the input

white balance

clipping (highlights)

restore details

white balance

base curve

exposure compensation

corrections curve

black point compensation

saturation

display profile



== Writing output files

=== Output profile

=== Format

Output files are usually JPG or 16-bit TIFF.

Note that "color space" is a separable issue from encoding.  sRGB
implies both a set of primaries and a specific gamma encoding.  When
sRGB is written as 16-bit linear TIFF, the color space is the same but
the gamma encoding is omitted.

