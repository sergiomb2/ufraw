UFRaw detailed processing description

$Id: README-processing.txt,v 1.1 2007/04/30 16:56:40 lexort Exp $

This document is a work in progress and may contain inaccurate information.


== Introduction

This document describes how UFRaw processes raw files to produce output
files.  The description is intended to be correct and precise, even at
the expense of readability.  This document is not a HOWTO.

This document assumes that the reader is generally familiar with computer
image processing, and understands the issues surrounding Bayer arrays,
white balance, and color management.

== Reading Raw files

The raw file is read and per-pixel sensor values extracted.  All
supported cameras express these values in a 12-bit space (0-4095).

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

