UFRaw detailed processing description

$Id: README-processing.txt,v 1.5 2007/12/02 20:48:29 lexort Exp $

This document is a work in progress and may contain inaccurate information.

== Introduction

This document describes how UFRaw processes raw files to produce output
files.  The description is intended to be correct and precise, even at
the expense of readability.  This document is not a HOWTO.

This document assumes that the reader is generally familiar with computer
image processing, and understands the issues surrounding Bayer arrays,
white balance, and color management.

== References

The following references may be helpful.

http://www.xs4all.nl/~tindeman/raw/color_reproduction.html

== Reading Raw files

The raw file is read and per-pixel sensor values extracted.
Each pixel is represented by integer number which is between
8 and 16 bits in size.
Most cameras (2007) use 12 bits, including Nikon and Canon amateur and
professional DSLRs.
Newer semi-professional and professional bodies use 14-bit A/D convertors.
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

== Processing Pipeline

=== Input color processing

ufraw, via dcraw, supports two methods of input color processing: ICC
profiles and color matrices.  These two methods solve the same
problem, and it is not sensible to use both of them at once.

Any input color processing method is intrinsically for a particular
illuminant, as cameras process color stimuli (light) rather than
object-color stimuli (objects that reflect an illuminant).  See the
section on white balance for more discussion.

TODO: figure out if this step is going to sRGB, or XYZ, and what the
working colorspace is.

==== Input profiles

ufraw can use an input profile, which specifies the relationship of
the camera's device color space to a standard color space.  Cameras
typically have R, G, and B filters, but these do not exactly
correspond to the primaries of sRGB.

==== Color matrices

ufraw can use a "color matrix", which are 3x3 matrices that transform
from the camera device colorspace to XYZ.
See dcraw.cc:adobe_coeff and dcraw.cc:cam_xyz_coeff.

The color matrix approach assumes that the camera sensor is perfectly linear

=== White balance

ufraw applies multipliers on the R G and B channels to cause white
objects to appear white for pictures taken under different lighting
conditions.

This white balance is an approximation and assumes that one can
decompose the camera's response into an input profile and a white
balance, rather than producing an input profile for every illuminant.

=== Gamma

Gamma processing is done to convert from the linear data present in
the RAW files to a gamma-encoded colorspace.  The exact curve that is
applied can be controlled by the "gamma" and "linearity" options.

The curve is

c * x              if x < linearity
(a * x + b) ^ g    if x >= linearity

where "x" is the input value in the [0,1] range and
g = gamma * (1 - linearity)/ (1 - gamma * linearity)
a = 1 / (1 + linearity * (g - 1))
b = linearity * (g - 1) * a
c = ((a * linearity + b)^g)/linearity

The default values are TODO.

This is apparently intended to convert from a linear representation to
the sRGB gamma represenation.  TODO: confirm.

TODO: Explain why this step makes sense.  If we are converting to
sRGB, then there is a single correct way to do this, described in the
sRGB specification.  Are we doing that?  Are we doing someting else?
Why would one set different  values?  If this transform is used for
another purpsose, should that purpose be more explicit?
== TODO

input profiles

working color space

film-like curves

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
