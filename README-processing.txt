UFRaw detailed processing description

$Id: README-processing.txt,v 1.3 2007/07/29 20:56:19 lexort Exp $

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
8 and 16 bits in size.
Most cameras (2007) use 12 bits, including Nikon and Canon amateur and
professional DSLRs.
Some old cameras use 8 bits.
Some cameras, such as medium format digital backs, use 16 bits.

=== Canon raw file encoding

Canon's raw files are different, in that only 11 of the 12 bits are
apparently used.  TODO (See ufraw_develop.cc for details).

== Comparison with in-camera processing

There are many ways to convert the raw data to an image, and it is
difficult to declare one of them to be the one true way.
However, it is useful to understand how a given processing method
differs from a canonical approach.
We therefore consider the in-camera creation of JPG as a reference for
discussion.
We choose as a reference conversion with all settings set to NORMAL
(rather than AUTO).
This reference is the benchmark for correctness; many choices may
produce a "better" image, but it should be understandable how this
arises.
Arguably, ufraw with default settings should produce a very similar
image to that produced in the camera.

TODO: discuss Nikon's Capture NX in light of the above.

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
