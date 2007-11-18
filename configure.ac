dnl $Id: configure.ac,v 1.104 2007/11/18 23:24:53 udifuchs Exp $
AC_INIT(UFRaw, 0.14)
AC_PREREQ(2.57)

# Create host_os, host_cpu variables
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_RANLIB

AC_CHECK_PROGS(POD2MAN, pod2man,
  [ echo pod2man is missing on this system; \
    echo you can ignore this problem by typing: touch ufraw.1; \
    echo cannot execute: pod2man], $PATH)
AC_FUNC_FSEEKO

# For binary package creation, adjusting for the build CPU is not appropriate.
case $host_cpu in
  i686)
    UFRAW_MARCH="-march=i686"
    ;;
  * )
    UFRAW_MARCH=""
    ;;
esac

# default CFLAGS, CXXFLAGS for GCC in case they were not set by the user.
if test "$GCC" = "yes"; then
  if test "$ac_test_CFLAGS" != "set"; then
    CFLAGS="-W -Wall -g -O3 -ffast-math -fomit-frame-pointer $UFRAW_MARCH"
  fi
  if test "$ac_test_CXXFLAGS" != "set"; then
    CXXFLAGS="-W -Wall -g -O3 -ffast-math -fomit-frame-pointer $UFRAW_MARCH"
  fi
fi

# windows will require some special attention
case $host_os in
  *mingw* )
    ufraw_win32=yes
    ;;
  * )
    ufraw_win32=no
    ;;
esac
AM_CONDITIONAL(UFRAW_WIN32, test $ufraw_win32 = yes)

if test $ufraw_win32 = yes; then
  CFLAGS="$CFLAGS -mno-cygwin -mwindows -mms-bitfields"
  CXXFLAGS="$CXXFLAGS -mno-cygwin -mwindows -mms-bitfields"
  CONSOLE="-mconsole"
else
  CONSOLE=
fi
AC_SUBST(CONSOLE)

# point to programs/libraries installed in a non-default place
AC_ARG_WITH(prefix,
  [  --with-prefix=PREFIX    use also PREFIX as an input prefix for the build],
  [ with_prefix=$withval
    echo "Adding pkgconfig/cppflags/ldflags for $with_prefix."
    export PKG_CONFIG_PATH="$with_prefix/lib/pkgconfig$PATH_SEPARATOR$PKG_CONFIG_PATH"
    CPPFLAGS="$CPPFLAGS -I$with_prefix/include"
    LDFLAGS="$LDFLAGS -L$with_prefix/lib"
  ],
  [ with_prefix=NONE ] )

AC_ARG_WITH(dosprefix,
  [  --with-dosprefix=PREFIX PREFIX is the prefix in dos format
                          (needed only in windows) ],
  [ with_dosprefix=$withval],
  [ with_dosprefix=NONE ] )

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.4)
PKG_CHECK_MODULES(GLIB, glib-2.0)
PKG_CHECK_MODULES(LCMS, lcms)
PKG_CHECK_MODULES(GTKIMAGEVIEW, gtkimageview >= 1.3,
  [ have_gtkimageview=yes
    GTK_LIBS=$GTKIMAGEVIEW_LIBS
    GTK_CFLAGS=$GTKIMAGEVIEW_CFLAGS
    AC_DEFINE(HAVE_GTKIMAGEVIEW, 1, have gtk image view) ],
  [ have_gtkimageview=no
    AC_MSG_RESULT($GIMP_PKG_ERRORS) ] )
AC_SUBST(GTK_LIBS)
PKG_CHECK_MODULES(gtk_image_view_damage_pixels, gtkimageview > 1.5.0,
  [ AC_DEFINE_UNQUOTED(GTK_IMAGE_VIEW_DAMAGE_PIXELS, 1, temporary) ],
  [ AC_DEFINE_UNQUOTED(GTK_IMAGE_VIEW_DAMAGE_PIXELS, 0, temporary) ])

# It is not clear if we should install the GIMP and Cinepaint plugins
# in the corresponding place in our prefix, or in GIMP's or
# Cinepaint's lib dir in their prefixes.  Currently, we install the
# plugin in our prefix.  (If the gimp/cinepaint prefix and the
# configured ufraw prefix are the same, this doesn't matter.)
pkg_prefix="$PKG_CONFIG"
test $prefix = NONE || pkg_prefix="$pkg_prefix --define-variable=prefix=$prefix"
test $exec_prefix = NONE || pkg_prefix="$pkg_prefix --define-variable=exec_prefix=$exec_prefix"

PKG_CHECK_MODULES(GIMP, gimpui-2.0,
  [ have_gimp=yes
    GIMP_LIBDIR=`$pkg_prefix --variable=gimplibdir gimp-2.0` ],
  [ have_gimp=no
    GIMP_LIBDIR=
    AC_MSG_RESULT($GIMP_PKG_ERRORS) ] )
AM_CONDITIONAL(MAKE_GIMP, test $have_gimp = yes)
AC_SUBST(GIMP_CFLAGS)
AC_SUBST(GIMP_LIBS)
AC_SUBST(GIMP_LIBDIR)

PKG_CHECK_MODULES(CINEPAINT, cinepaint-gtk >= 0.22,
  [ have_cinepaint=yes
    CINEPAINT_LIBDIR=`$pkg_prefix --variable=libdir cinepaint-gtk`
    CINEPAINT_PROGRAMPLUGINDIR=`$pkg_prefix --variable=programplugindir cinepaint-gtk` ],
  [ have_cinepaint=no
    CINEPAINT_LIBDIR=
    CINEPAINT_PROGRAMPLUGINDIR=
    AC_MSG_RESULT($CINEPAINT_PKG_ERRORS) ] )
AM_CONDITIONAL(MAKE_CINEPAINT, test $have_cinepaint = yes)
AC_SUBST(CINEPAINT_CFLAGS)
AC_SUBST(CINEPAINT_LIBS)
AC_SUBST(CINEPAINT_PROGRAMPLUGINDIR)

# Disable deprecated functions on tested versions of GTK
# unless using an old gimp-dev package.
if $PKG_CONFIG --atleast-version=2.2 gimpui-2.0; then
  if $PKG_CONFIG --max-version=2.12.99 gtk+-2.0; then
    AC_DEFINE(G_DISABLE_DEPRECATED, [], disable deprecated glib features)
    AC_DEFINE(GDK_DISABLE_DEPRECATED, [], disable deprecated gdk features)
    AC_DEFINE(GTK_DISABLE_DEPRECATED, [], disable deprecated gtk+ features)
  fi
fi

# Ensure that getopt_long is available.  It is included in GNU libc and
# in at least most BSD libcs.  If not found, search for it in libgnugetopt.
AC_SEARCH_LIBS(getopt_long, gnugetopt, ,
  AC_MSG_ERROR(can not build UFRaw without getopt_long))

# The ws2_32 library is required for the ntohs symbol on WIN32.
if test $ufraw_win32 = yes; then
  LIBS="$LIBS -lws2_32"
fi

# Make sure that pow is available, trying libm if necessary.
AC_SEARCH_LIBS(pow, m)
AC_CHECK_FUNCS(canonicalize_file_name)
AC_CHECK_FUNCS(memmem)

# Check for zlib.
AC_CHECK_LIB(z, deflate)
have_zlib=${ac_cv_lib_z_deflate:-no}

# Check for libbz2
AC_CHECK_LIB(bz2, BZ2_bzReadOpen)
have_libbz2=${ac_cv_lib_bz2_BZ2_bzReadOpen:-no}

# Check for jpeg headers and library.
AC_CHECK_HEADER(jpeglib.h,
  AC_CHECK_LIB(jpeg, jpeg_CreateCompress))
have_jpeg=${ac_cv_lib_jpeg_jpeg_CreateCompress:-no}

# Check for png headers and library.
AC_CHECK_HEADER(png.h,
  AC_CHECK_LIB(png, png_create_info_struct))
have_png=${ac_cv_lib_png_png_create_info_struct:-no}

# Check for tiff headers and library.
AC_CHECK_HEADER(tiffio.h,
  AC_CHECK_LIB(tiff, TIFFSetErrorHandler))
have_tiff=${ac_cv_lib_tiff_TIFFSetErrorHandler:-no}

# Check for fits headers and library.
## You need to un-comment the next two lines to enable FITS support.
## The reason for disabling this feature in hardcode is that the FITS
## output is not final yet. If you use this feature, you should be
## aware of the fact that future version of UFRaw would create
## different FITS files.
#AC_CHECK_HEADER(fitsio.h,
#  AC_CHECK_LIB(cfitsio, ffcmsg))
have_fits=${ac_cv_lib_cfitsio_ffcmsg:-no}

PKG_CHECK_MODULES(EXIV2, exiv2 >= 0.11,
  [ have_exiv2=yes
    AC_DEFINE(HAVE_EXIV2, 1, have exiv2) ],
  [ have_exiv2=no
    AC_MSG_RESULT($EXIV2_PKG_ERRORS) ] )

# UFRAW_CPPFLAGS is added to the preprocessor flags AM_CPPFLAGS,
# affecting also the C and C++ compilers.
UFRAW_CPPFLAGS="$UFRAW_CPPFLAGS $EXIV2_CFLAGS $GTK_CFLAGS $LCMS_CFLAGS"
AC_SUBST(UFRAW_CPPFLAGS)
# UFRAW_LDADD is added to the linker flags LDADD.
UFRAW_LDADD="$UFRAW_LDADD $EXIV2_LIBS $GLIB_LIBS $LCMS_LIBS"
AC_SUBST(UFRAW_LDADD)

if test $ufraw_win32 = yes; then
  AC_CHECK_PROGS(WINDRES, $target_alias-windres windres, , $PATH)
  AC_SUBST(WINDRES)
  # For the windows-intstaller make needs the location of the DLLs
  PREFIX=$with_prefix
  AC_SUBST(PREFIX)
  # and ISCC (ufraw-setup.iss.in) needs them in dos format...
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
      # Only needed if you plan to cross-build windows-installer from linux
      test -z "$PROGRAMFILES" && PROGRAMFILES="c:\\Program Files"
      AC_SUBST(PROGRAMFILES)
      WINE="wine"
      COMMENT_ICON=";"
  esac
  AC_SUBST(WINE)
  AC_SUBST(COMMENT_ICON)
fi

# Make sure MSGFMT_OPTS is defined for all combinations of autoconf and glib.
MSGFMT_OPTS=
ALL_LINGUAS="da es fr ja ko pl pt ru sv"
AM_GLIB_GNU_GETTEXT
LIBS="$LIBS $INTLLIBS"
# The following might not work with some combinations of autoconf and glib due
# to a missing update of the glib-gettext.m4 file.
#AM_GLIB_DEFINE_LOCALEDIR(UFRAW_LOCALEDIR)
GETTEXT_PACKAGE=ufraw
AC_SUBST(GETTEXT_PACKAGE)
AC_SUBST(MSGFMT_OPTS)

AC_MSG_CHECKING(whether to install mime types)
AC_ARG_ENABLE(mime,
  [  --enable-mime         install mime files, see README for more information],
  , enable_mime=no)
AC_MSG_RESULT($enable_mime)
AM_CONDITIONAL(INSTALL_MIME, test $enable_mime = yes)

AC_MSG_CHECKING(whether to build extras)
AC_ARG_ENABLE(extras,
  [  --enable-extras         build extra (dcraw, nikon-curve) executables], ,
  enable_extras=no)
AC_MSG_RESULT($enable_extras)
AM_CONDITIONAL(MAKE_EXTRAS, test $enable_extras = yes)

AC_CONFIG_FILES(Makefile)
AC_CONFIG_FILES(icons/Makefile)
AC_CONFIG_FILES(po/Makefile.in)
AC_CONFIG_FILES(ufraw-setup.iss)
AC_OUTPUT

AC_MSG_NOTICE(====================== summary =====================)
AC_MSG_NOTICE(build GIMP plug-in: $have_gimp)
AC_MSG_NOTICE(build CinePaint plug-in: $have_cinepaint)
AC_MSG_NOTICE(EXIF support using exiv2: $have_exiv2)
AC_MSG_NOTICE(JPEG support: $have_jpeg)
AC_MSG_NOTICE(PNG support: $have_png)
AC_MSG_NOTICE(FITS support: $have_fits)
AC_MSG_NOTICE(TIFF support: $have_tiff)
AC_MSG_NOTICE(gzip compressed raw support: $have_zlib)
AC_MSG_NOTICE(bzip2 compressed raw support: $have_libbz2)
AC_MSG_NOTICE(Scrolling in preview using GtkImageView: $have_gtkimageview)
