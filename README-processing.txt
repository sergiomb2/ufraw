UFRaw detailed processing description

$Id: README-processing.txt,v 1.25 2014/08/25 13:00:18 nkbj Exp $

This document is a work in progress and may contain inaccurate information.

== Introduction

This document describes how UFRaw processes raw files to produce output
files.  The description is intended to be correct and precise, even at
the expense of readability.  This document is not a HOWTO.

This document assumes that the reader is generally familiar with computer
image processing, and understands the issues surrounding color filter arrays,
white balance, and color management.

== References

These are links that those wanting to understand raw conversion
details are likely to find interesting.

=== general

http://www.xs4all.nl/~tindeman/raw/color_reproduction.html

=== raw files

A lot of these references talk about "gamma correction", say that raw
data has a "linear gamma" (which makes little sense), or that it has a
gamma of 1.0.  They mean simply that the number correspond to
luminance, rather than some compressed function of luminance.

Then, many go on to say that such "linear gamma" images look dark, and
some even show screenshots.  Surely these screenshots are produced by
taking a file with a linear encoding and displaying it on an output
device which has the sRGB gamma encoding, which is 256 roughly equal
perceptual steps - nowhere near linear in luminance.  So while it
"looks dark", they've made a non-sensical transformation.

http://www.adobe.com/digitalimag/pdfs/understanding_digitalrawcapture.pdf
http://www.adobe.com/digitalimag/pdfs/linear_gamma.pdf
http://www.majid.info/mylos/weblog/2004/05/02-1.html
http://www.cambridgeincolour.com/tutorials/RAW-file-format.htm
http://regex.info/blog/photo-tech/nef-compression/
http://www.luminous-landscape.com/tutorials/understanding-series/u-raw-files.shtml

=== color spaces

http://regex.info/blog/2006-12-08/303
http://regex.info/blog/photo-tech/color-spaces-page1/

http://lists.freedesktop.org/archives/openicc/2006q3/000775.html
(an AdobeRGB-compatible profile)

=== other converters

Discussion of sensor saturation levels and highlight recovery in
dcraw:

http://www.guillermoluijk.com/tutorial/dcraw/index_en.htm

http://www.rawtherapee.com/data/RT-Internal-Workflow_2.3.pdf

http://dlraw.sourceforge.net/Processing.shtml

=== processing issues

http://www.4p8.com/eric.brasseur/gamma.html

http://www.21stcenturyshoebox.com/essays/color_reproduction.html

== Reading Raw files

The raw file is read and per-pixel sensor values extracted.
Each pixel is represented by integer number which is between
8 and 16 bits in size.
Most cameras (2007) use 12 bits, including Nikon and Canon amateur 
DSLRs.
Newer semi-professional and professional bodies use 14-bit A/D convertors.

The following is unconfirmed:
Some old cameras use 8 bits.
Some cameras, such as medium format digital backs, use 16 bits.

=== Canon raw file encoding

Canon's raw files are slightly, in that normal exposure uses the lower
half of the space, apparently to leave highlight headroom.  TODO (See
ufraw_develop.cc for details).

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

=== Camera Tone curves

==== Nikon

Udi wrote to the list in 2007:

The Nikon curve tag contains two curves. One is a set of points that
need to be interpolated, this is what you are talking about. The other
is a table mapping the 4096 CCD values to a number between 0 and 255,
this is what I am talking about. If the first curve is the default
linear curve, the second curve will contain a gamma curve which is
approximately like UFRaw's Gamma=0.45, Linearity=0.10 curve.

Then there is a third relevant curve, which is relevant when you want
to use the Nikon ICC profiles. These profiles assume that their input
is not the linear CCD input, but the CCD with a Gamma=0.45,
Linearity=0.00 applied to it. My guess is that Nikon did it so that
they could do all the CMS calculations in 8 bits. This should be
mostly useful for the in camera manipulations which are limited by the
camera's hardware.

=== Input color processing

ufraw, via dcraw, supports two methods of input color processing: ICC
profiles and color matrices.  These two methods solve the same
problem, and it is not sensible to use both of them at once.  (The UI
no longer allows mixing them.)

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

The lprof site has a discussion of how to create an input profile for
ufraw with an IT8.7 target.  This discussion includes comments about
setting the linearity and gamma values, but the key point is that the
same linearity and gamma values used for profile creation must be set
when the profile is used.

http://lprof.sourceforge.net/help/ufraw.html

==== Color matrices

ufraw can use a "color matrix", which are 3x3 matrices that transform
from the camera device colorspace to XYZ, as an alternative to an ICC
input profile.  See dcraw.cc:adobe_coeff and dcraw.cc:cam_xyz_coeff.

The color matrix approach assumes that the camera sensor is perfectly
linear.  CCDs are intrinsically quite linear, so this is a reasonable
assumption.

=== White balance

ufraw applies multipliers on the R G and B channels to cause white
objects to appear white for pictures taken under different lighting
conditions.

This white balance is an approximation and assumes that one can
decompose the camera's response into an input profile and a white
balance, rather than producing an input profile for every illuminant.
Surely this is not quite true but we are neglecting the errors.

=== Gamma

Gamma processing is done to convert from the linear data present in
the RAW files to a gamma-encoded colorspace.  Ufraw supports gamma
encoding with a  linear part, generalized from the sRGB rules.
Precisely

  c * x              if x < linearity
  (a * x + b) ^ g    if x >= linearity

where "x" is the input value in the [0,1] range and

  g = gamma * (1 - linearity)/ (1 - gamma * linearity)
  a = 1 / (1 + linearity * (g - 1))
  b = linearity * (g - 1) * a
  c = ((a * linearity + b)^g)/linearity

The default values depend on the input profile.  For a color matrix,
or no profile, the defaults are gamma = 0.45 (1/2.2) and linearity =
0.10.  For profiles, the defaults appear (TODO) to come from the
profile.

The gamma encoding specificed by sRGB can be specified by gamma 0.417
(1/2.4) and linearity 0.0031 (with an implicit scaling to 255):

  12.92 * x                    if x <= 0.0031308
  1.055 * x ^ (1/2.4) - 0.055  if x >  0.0031308

dcraw uses a different gamma encoding and primaries following ITU
Recommendation BT.709 because dcraw generates PPM files.  However, the
PPM man page notes that a common variation is to use the sRGB
colorspace.  (Probably those concerned at this level would use tiff,
so we will treat the precise details of ppm as unimportant.)
  r <= 0.018 ? r*4.5 : pow(r,0.45)*1.099-0.099 );
  http://netpbm.sourceforge.net/doc/ppm.html

==== Role of gamma and color mmnagement accuracy

See this thread for a summary:

http://sourceforge.net/mailarchive/message.php?msg_name=481D7313.10009%40telenet.be

On 2009-04-11, Martin Ling posted to ufraw-devel and hypothesized (or
pointed out his earlier hypothesis) that unpleasing fogginess when the
objectively correct gamma processing is done is due to fixed-point
quantization errors in lcms.

Argyll CMS uses LUTs that intend to be equally spaced perceptually and
linear encodings break this assumption:

http://www.freelists.org/post/argyllcms/icclink-G-and-source-gamuts-profiles,5
http://www.freelists.org/post/argyllcms/icclink-G-and-source-gamuts-profiles,9

Another semi-relevant message:

http://www.freelists.org/post/argyllcms/icclink-G-and-source-gamuts-profiles,1

==== Way Forward

There has been ongoing discussion on ufraw-devel about the validity of
using a gamma encoding different from the sRGB specification and then
assuming the pixels are in sRGB.  It is generally agreed that this is
not strictly correct, but that images produced this way are more
pleasing than images where the input linear encoding is passed to a
CMS directly, rather than being transformed to a perceptually uniform space.

As part of the discussion, a goal of some is to use lcms to apply the
input profile mapping even in the case of Color Matrices, and to
remove the gamma controls entirely.  The basic idea is to use the
RGB->XYZ matrix to create a profile from camera space to XYZ and then
link it to the sRGB profile to get a combination profile to go from
(linear) camera space to sRGB with the standard encoding.

Currently, the obstacles are a lack of spare time and that this is not
quite fully agreed on or understood.

=== Highlight processing

Highlight processing has two fundamentally different operations.

One is guessing at color when one of the sensor channels has
saturated.  In this case the color information is lost, and all that
is known for certain is that the region is very bright and in some
subset of the color space.

Another operation is mapping input levels which are within the sensor
range and thus known to representable values in the output space.

gdt thinks that both of these operations should be independent of EV
compensation.

Robert Krawitz has posted an image which results in incorrect colors
with the color matrix enabled:

  ftp://66.92.65.9/pub/crw_3888.crw

It that the matrix operations are somehow interfering with color
recovery in the sensor saturation case.

TODO: Explain whether ufraw does color recovery on pixels with sensor
values that are saturated, or something else, and why the scheme used
will get a reasonable outcome.

=== TODO for processing pipeline

working color space

film-like curves

clipping (highlights)

restore details

white balance

base curve

exposure compensation

corrections curve

black point compensation

contrast and saturation

display profile

== Writing output files

=== Output profile

=== Format

Output files are usually JPG or 16-bit TIFF.

Note that "color space" is a separable issue from encoding.  sRGB
implies both a set of primaries and a specific gamma encoding.  When
sRGB is written as 16-bit linear TIFF, the color space is the same but
the gamma encoding is omitted.

==== PPM

PPM output is specified to use the primaries and gamma encoding of
ITU-R Recommendation BT.709:
http://netpbm.sourceforge.net/doc/ppm.html
