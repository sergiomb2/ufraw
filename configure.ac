dnl $Id: configure.ac,v 1.185 2015/06/20 02:30:15 nkbj Exp $
AC_INIT([UFRaw],[0.23])
AC_PREREQ([2.69])

# Create host_os, host_cpu, host_alias variables.
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_HEADERS(config.h)
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_RANLIB

AC_CHECK_PROGS(POD2MAN, pod2man,
  [ echo pod2man is missing on this system; \
    echo you can ignore this problem by typing: touch ufraw.1; \
    echo cannot execute: pod2man], $PATH)
AC_FUNC_FSEEKO

# Ensure that getopt_long is available. It is included in GNU libc and
# in at least most BSD libcs. If not found, search for it in libgnugetopt.
AC_SEARCH_LIBS(getopt_long, gnugetopt, ,
  AC_MSG_ERROR(can not build UFRaw without getopt_long))

# Make sure that pow is available, trying libm if necessary.
AC_SEARCH_LIBS(pow, m)
AC_CHECK_FUNCS(canonicalize_file_name)
AC_CHECK_FUNCS(memmem)
AC_CHECK_FUNCS(strcasecmp)
AC_CHECK_FUNCS(strcasestr)

# For binary package creation, adjusting for the build CPU is not appropriate.
case $host_cpu in
  i686)
    UFRAW_MARCH="-march=i686"
    ;;
  * )
    UFRAW_MARCH=""
    ;;
esac

# Default CFLAGS, CXXFLAGS for GCC in case they were not set by the user.
if test "$GCC" = "yes"; then
  if test $ac_test_CFLAGS; then
    CFLAGS="-W -Wall -g -O3 -fomit-frame-pointer -D_FORTIFY_SOURCE=2 $UFRAW_MARCH"
  fi
  if test $ac_test_CXXFLAGS; then
    CXXFLAGS="-W -Wall -g -O3 -fomit-frame-pointer -D_FORTIFY_SOURCE=2 $UFRAW_MARCH"
  fi
fi

m4_ifndef([AC_OPENMP], [m4_include([ac_openmp.m4])])
AC_OPENMP
CFLAGS="$CFLAGS $OPENMP_CFLAGS"
CXXFLAGS="$CXXFLAGS $OPENMP_CFLAGS"

# Point to programs/libraries installed in a non-default place.
AC_ARG_WITH(prefix,
  [  --with-prefix=PREFIX    use also PREFIX as an input prefix for the build],
  [ with_prefix=$withval
    echo "Adding pkgconfig/cppflags/ldflags for $with_prefix."
    export PKG_CONFIG_PATH="$with_prefix/lib/pkgconfig$PATH_SEPARATOR$PKG_CONFIG_PATH"
    CPPFLAGS="$CPPFLAGS -I$with_prefix/include"
    LDFLAGS="$LDFLAGS -L$with_prefix/lib"
  ],
  [ with_prefix=NONE ] )

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.12 gthread-2.0)

PKG_CHECK_MODULES(LCMS2, lcms2)

AC_ARG_WITH([gtk],
  [AS_HELP_STRING([--with-gtk],
  [build the GTK GUI @<:@default=check@:>@])],
  [],
  [with_gtk=check])

have_gtk=no
AS_IF([test "x$with_gtk" != xno],
  [PKG_CHECK_MODULES(GTKBASE, gtk+-2.0 >= 2.12,
    [ have_gtk=yes ],
    [ have_gtk=no
      if test "x$with_gtk" != xcheck; then
        AC_MSG_FAILURE(
          [--with-gtk was given, but test for gtk failed])
      fi
    ] ) ] )

have_gimp=no
if test "$have_gtk" = "yes"; then
  PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.10.0)

# It is not clear if we should install the GIMP plugin
# in the corresponding place in our prefix, or in GIMP's
# lib dir in its prefix. Currently, we install the
# plugin in our prefix. If the GIMP prefix and the
# configured UFRaw prefix are the same, this doesn't matter.
  pkg_prefix="$PKG_CONFIG"
  test $prefix = NONE || pkg_prefix="$pkg_prefix --define-variable=prefix=$prefix"
  test $exec_prefix = NONE || pkg_prefix="$pkg_prefix --define-variable=exec_prefix=$exec_prefix"

  AC_ARG_WITH([gimp],
    [AS_HELP_STRING([--with-gimp],
    [build gimp plugin @<:@default=check@:>@])],
    [],
    [with_gimp=check])

  AS_IF([test "x$with_gimp" != xno],
    [PKG_CHECK_MODULES(GIMP, gimpui-2.0 >= 2.2.0,
      [ have_gimp=yes
        GIMP_LIBDIR=`$pkg_prefix --variable=gimplibdir gimp-2.0` ],
      [ have_gimp=no
        GIMP_LIBDIR=
        if test "x$with_gimp" != xcheck; then
          AC_MSG_FAILURE(
            [--with-gimp was given, but test for gimp failed])
        fi
      ] ) ] )
  if test "$have_gimp" = "yes"; then
    PKG_CHECK_MODULES(GIMP_2_4, gimpui-2.0 >= 2.4.0,
      [ AC_DEFINE_UNQUOTED(HAVE_GIMP_2_4, 1, have Gimp 2.4 or later) ],
      [ AC_DEFINE_UNQUOTED(HAVE_GIMP_2_4, 0, have Gimp 2.4 or later) ])
    PKG_CHECK_MODULES(GIMP_2_9, gimpui-2.0 >= 2.9.0,
      [ AC_DEFINE_UNQUOTED(HAVE_GIMP_2_9, 1, have Gimp 2.9 or later) ],
      [ AC_DEFINE_UNQUOTED(HAVE_GIMP_2_9, 0, have Gimp 2.9 or later) ])
  fi

  # Disable deprecated functions on tested versions of GTK.
  if $PKG_CONFIG --max-version=2.18.99 gtk+-2.0; then
    AC_DEFINE(G_DISABLE_DEPRECATED, [], disable deprecated glib features)
    AC_DEFINE(GDK_DISABLE_DEPRECATED, [], disable deprecated gdk features)
    AC_DEFINE(GTK_DISABLE_DEPRECATED, [], disable deprecated gtk+ features)
    AC_DEFINE(GTK_DISABLE_SINGLE_INCLUDES, [], disable deprecated gtk headers)
    AC_DEFINE(GDK_PIXBUF_DISABLE_SINGLE_INCLUDES, [], disable deprecated gdk headers)
  fi
fi
AM_CONDITIONAL(MAKE_GTK, test $have_gtk = yes)
AC_SUBST(GTK_LIBS)

AM_CONDITIONAL(MAKE_GIMP, test $have_gimp = yes)
AC_SUBST(GIMP_CFLAGS)
AC_SUBST(GIMP_LIBS)
AC_SUBST(GIMP_LIBDIR)

# Check for zlib.
AC_CHECK_LIB(z, deflate)
have_zlib=${ac_cv_lib_z_deflate:-no}

# Check for libbz2.
AC_CHECK_LIB(bz2, BZ2_bzReadOpen)
have_libbz2=${ac_cv_lib_bz2_BZ2_bzReadOpen:-no}

# Check for jpeg headers and library.
AC_CHECK_HEADER(jpeglib.h,
  AC_CHECK_LIB(jpeg, jpeg_CreateCompress))
have_jpeg=${ac_cv_lib_jpeg_jpeg_CreateCompress:-no}

# Check for libjasper.
AC_ARG_ENABLE([jasper],
  AS_HELP_STRING([--enable-jasper], [enable JPEG2000 support]))

have_jasper=no
AS_IF([test "x$enable_jasper" = "xyes"], [
  AC_SEARCH_LIBS([jas_image_decode], [jasper], [have_jasper=yes], [
    AC_MSG_ERROR([unable to find the jas_image_decode() function])
  ])
])


# Check for tiff headers and library.
PKG_CHECK_MODULES(LIBTIFF, libtiff-4,
  [ have_tiff=yes
    AC_DEFINE(HAVE_LIBTIFF, 1, have the tiff library) ],
  [ have_tiff=no
    AC_MSG_RESULT($LIBTIFF_PKG_ERRORS) ] )
if test $have_tiff = no; then
  AC_CHECK_HEADER(tiffio.h,
    AC_CHECK_LIB(tiff, TIFFSetErrorHandler))
  have_tiff=${ac_cv_lib_tiff_TIFFSetErrorHandler:-no}
fi

PKG_CHECK_MODULES(LIBPNG, libpng >= 1.2,
  [ have_png=yes
    AC_DEFINE(HAVE_LIBPNG, 1, have the png library) ],
  [ have_png=no
    AC_MSG_RESULT($LIBPNG_PKG_ERRORS) ] )

AC_ARG_WITH([cfitsio],
  [AS_HELP_STRING([--with-cfitsio],
  [build fits support @<:@default=check@:>@])],
  [],
  [with_cfitsio=check])

have_cfitsio=no
AS_IF([test "x$with_cfitsio" != xno],
  [PKG_CHECK_MODULES(CFITSIO, cfitsio,
    [ have_cfitsio=yes
      AC_DEFINE(HAVE_LIBCFITSIO, 1, have the cfitsio library) ],
    [ have_cfitsio=no
      AC_MSG_RESULT($CFITSIO_PKG_ERRORS) ] ) ] )

PKG_CHECK_MODULES(EXIV2, exiv2 >= 0.20,
  [ have_exiv2=yes
    AC_DEFINE(HAVE_EXIV2, 1, have the exiv2 library) ],
  [ have_exiv2=no
    AC_MSG_RESULT($EXIV2_PKG_ERRORS) ] )

PKG_CHECK_MODULES(LENSFUN, lensfun >= 0.2.5,
  [ have_lensfun=yes
    AC_DEFINE(HAVE_LENSFUN, 1, have the lensfun library) ],
  [ have_lensfun=no
    AC_MSG_RESULT($LENSFUN_PKG_ERRORS) ] )

# Make sure MSGFMT_OPTS is defined for all combinations of autoconf and glib.
MSGFMT_OPTS=
ALL_LINGUAS="ca cs da de es fr it ja ko nb nl pl pt ru sr sr@latin sv zh_CN zh_TW"
AM_GNU_GETTEXT_VERSION([0.21])
AM_GNU_GETTEXT([external])
LIBS="$LIBS $INTLLIBS"
# The following might not work with some combinations of autoconf and glib due
# to a missing update of the glib-gettext.m4 file.
#AM_GLIB_DEFINE_LOCALEDIR(UFRAW_LOCALEDIR)
GETTEXT_PACKAGE=ufraw
AC_SUBST(GETTEXT_PACKAGE)
AC_SUBST(MSGFMT_OPTS)

# UFRAW_CPPFLAGS is added to the preprocessor flags AM_CPPFLAGS,
# affecting also the C and C++ compilers.
UFRAW_CPPFLAGS="$UFRAW_CPPFLAGS $EXIV2_CFLAGS $GTK_CFLAGS $GLIB_CFLAGS $LCMS2_CFLAGS $LENSFUN_CFLAGS $LIBTIFF_CFLAGS $LIBPNG_CFLAGS $CFITSIO_CFLAGS"
AC_SUBST(UFRAW_CPPFLAGS)

# UFRAW_LDADD is added to the linker flags LDADD.
UFRAW_LDADD="$UFRAW_LDADD $EXIV2_LIBS $GLIB_LIBS $LCMS2_LIBS $LENSFUN_LIBS $LIBTIFF_LIBS $LIBPNG_LIBS $CFITSIO_LIBS $CARBON_LIBS"
AC_SUBST(UFRAW_LDADD)

# Windows and Darwin (Mac OS X) will require some special attention.
case $host_os in
  *mingw* )
    ufraw_win32=yes
    ufraw_darwin=no
    ;;
  *darwin* )
    ufraw_win32=no
    ufraw_darwin=yes
    ;;
  * )
    ufraw_win32=no
    ufraw_darwin=no
    ;;
esac
AM_CONDITIONAL(UFRAW_WIN32, test x$ufraw_win32 = xyes)

if test $ufraw_win32 = yes; then
  CFLAGS="$CFLAGS -mwindows -mms-bitfields"
  CXXFLAGS="$CXXFLAGS -mwindows -mms-bitfields"
  CONSOLE="-mconsole"
  AC_MSG_CHECKING(whether to add the -mno-cygwin flag to CFLAGS and CXXFLAGS)
  AC_ARG_ENABLE(no_cygwin,
    [  --enable-no-cygwin      add the -mno-cygwin flag to CFLAGS and CXXFLAGS
                          (only in Windows) ],
    , enable_no_cygwin=no)
  AC_MSG_RESULT($enable_no_cygwin)
  if test $enable_no_cygwin = yes; then
    CFLAGS="$CFLAGS -mno-cygwin" 
    CXXFLAGS="$CXXFLAGS -mno-cygwin" 
  fi
else
  CONSOLE=
fi
AC_SUBST(CONSOLE)

# Check for the Apple Carbon framework (needed for GDK_WINDOWING_QUARTZ). 
CARBON_LIBS=
if test $ufraw_darwin = yes; then
  carbon_ok=no
  AC_MSG_CHECKING([for Mac OS X Carbon support])
  AC_PREPROC_IFELSE([AC_LANG_SOURCE([[
  #include <Carbon/Carbon.h>
  #include <ApplicationServices/ApplicationServices.h>
  #include <CoreServices/CoreServices.h>
  ]])],[carbon_ok=yes],[])
  AC_MSG_RESULT($carbon_ok)
  if test "x$carbon_ok" = "xyes"; then
    CARBON_LIBS="-framework Carbon -framework Cocoa"
  fi
fi
AC_SUBST(CARBON_LIBS)

AC_ARG_WITH(dosprefix,
  [  --with-dosprefix=PREFIX PREFIX is the prefix in dos format
                          (needed only in Windows) ],
  [ with_dosprefix=$withval],
  [ with_dosprefix=NONE ] )

# The ws2_32 library is required for the ntohs symbol on win32.
if test $ufraw_win32 = yes; then
  LIBS="$LIBS -lws2_32"
fi

if test $ufraw_win32 = yes; then
  AC_CHECK_PROGS(WINDRES, $host_alias-windres $target_alias-windres windres, , $PATH)
  AC_SUBST(WINDRES)
  # For the Windows-installer make needs the location of the DLLs
  PREFIX=$with_prefix
  AC_SUBST(PREFIX)
  # and ISCC (ufraw-setup.iss.in) needs them in DOS format...
  DOSPREFIX=$with_dosprefix
  AC_SUBST(DOSPREFIX)
  ISCC="\"\$(PROGRAMFILES)\"/Inno\ Setup\ 5/ISCC.exe"
  AC_SUBST(ISCC)
  case $build_os in
    *cygwin* | *mingw* )
      WINE=""
      COMMENT_ICON=""
      ;;
    * )
      # Only needed if you plan to cross-build Windows-installer from Linux.
      test -z "$PROGRAMFILES" && PROGRAMFILES="C:\\Program Files"
      AC_SUBST(PROGRAMFILES)
      WINE="wine"
      COMMENT_ICON=";"
  esac
  AC_SUBST(WINE)
  AC_SUBST(COMMENT_ICON)
fi

AC_MSG_CHECKING(whether to install mime types)
AC_ARG_ENABLE(mime,
  [  --enable-mime           install mime files, see README for more information],
  , enable_mime=no)
AC_MSG_RESULT($enable_mime)
AM_CONDITIONAL(INSTALL_MIME, test $enable_mime = yes)

AC_MSG_CHECKING(whether to build extras)
AC_ARG_ENABLE(extras,
  [  --enable-extras         build extra (dcraw, nikon-curve) executables], ,
  enable_extras=no)
AC_MSG_RESULT($enable_extras)
AM_CONDITIONAL(MAKE_EXTRAS, test $enable_extras = yes)

AC_MSG_CHECKING(whether to enable DST correction for file timestamps)
AC_ARG_ENABLE(dst_correction,
  [  --enable-dst-correction enable DST correction for file timestamps],
  AC_DEFINE(LOCALTIME, 1, DST correction enabled),
  enable_dst_correction=no)
AC_MSG_RESULT($enable_dst_correction)

AC_MSG_CHECKING(whether to enable the contrast setting option)
AC_ARG_ENABLE(contrast,
  [  --enable-contrast       enable the contrast setting option],
  AC_DEFINE(UFRAW_CONTRAST, 1, Contrast setting option enabled),
  enable_contrast=no)
AC_MSG_RESULT($enable_contrast)

AC_MSG_CHECKING(whether to enable 'None' interpolation)
AC_ARG_ENABLE(interp_none,
  [  --enable-interp-none    enable 'None' interpolation (mostly for debugging)],
  AC_DEFINE(ENABLE_INTERP_NONE, 1, 'None' interpolation enabled),
  enable_interp_none=no)
AC_MSG_RESULT($enable_interp_none)

AC_MSG_CHECKING(whether to enable debugging with valgrind)
AC_ARG_ENABLE(valgrind,
  [  --enable-valgrind       enable debugging with valgrind],
  AC_DEFINE(UFRAW_VALGRIND, 1, Debugging with valgrind enabled),
  enable_valgrind=no)
AC_MSG_RESULT($enable_valgrind)

AC_CONFIG_FILES(Makefile)
AC_CONFIG_FILES(icons/Makefile)
AC_CONFIG_FILES(po/Makefile.in)
AC_CONFIG_FILES(ufraw-setup.iss)
AC_OUTPUT

AC_MSG_NOTICE(====================== summary =====================)
AC_MSG_NOTICE(build GTK GUI: $have_gtk)
AC_MSG_NOTICE(build GIMP plug-in: $have_gimp)
AC_MSG_NOTICE(EXIF support using exiv2: $have_exiv2)
AC_MSG_NOTICE(JPEG support: $have_jpeg)
AC_MSG_NOTICE(JPEG2000 (libjasper) support: $have_jasper)
AC_MSG_NOTICE(TIFF support: $have_tiff)
AC_MSG_NOTICE(PNG support: $have_png)
AC_MSG_NOTICE(FITS support: $have_cfitsio)
AC_MSG_NOTICE(gzip compressed raw support: $have_zlib)
AC_MSG_NOTICE(bzip2 compressed raw support: $have_libbz2)
AC_MSG_NOTICE(lens defects correction support using lensfun: $have_lensfun)
