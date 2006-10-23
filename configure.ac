dnl $Id: configure.ac,v 1.51 2006/10/23 21:38:10 udifuchs Exp $
AC_INIT(UFRaw, 0.10)
AC_PREREQ(2.57)

# Create host_os, host_cpu variables
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL

AC_CHECK_PROGS(POD2MAN, pod2man, , $PATH)

# nikon_curve makes some sizeof assumptions, requiring the following tests
# (could be avoided by using C99 standard types)
AC_CHECK_SIZEOF(short)
if test $ac_cv_sizeof_short -ne 2; then
    AC_MSG_ERROR([size of short must be 2])
fi
AC_CHECK_SIZEOF(int)
if test $ac_cv_sizeof_int -ne 4; then
    AC_MSG_ERROR([size of int must be 4])
fi
AC_CHECK_SIZEOF(float)
if test $ac_cv_sizeof_float -ne 4; then
    AC_MSG_ERROR([size of float must be 4])
fi
AC_CHECK_SIZEOF(double)
if test $ac_cv_sizeof_double -ne 8; then
    AC_MSG_ERROR([size of double must be 8])
fi

UFRAW_CFLAGS="-ffast-math -fomit-frame-pointer"
if test "x$GCC" = "xyes"; then
    UFRAW_CFLAGS="$UFRAW_CFLAGS -W -Wall -O3"
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
  UFRAW_CFLAGS="$UFRAW_CFLAGS -mno-cygwin -mwindows -mms-bitfields"
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
AC_SUBST(GTK_LIBS)
PKG_CHECK_MODULES(GLIB, glib-2.0)
PKG_CHECK_MODULES(LCMS, lcms)

# If the GIMP plug-in is not built, UFRaw is completely independent of
# the GIMP.
AC_MSG_CHECKING(whether to build the GIMP plug-in)
AC_ARG_ENABLE(gimp,
  [  --disable-gimp          disable build of the GIMP plug-in], ,
  enable_gimp=yes)
AM_CONDITIONAL(MAKE_GIMP, test $enable_gimp = yes)
AC_MSG_RESULT($enable_gimp)
if test $enable_gimp = yes; then
    PKG_CHECK_MODULES(GIMP, gimpui-2.0)
else
    GIMP_CFLAGS=
    GIMP_LIBS=
fi
AC_SUBST(GIMP_LIBS)

# Disable deprecated functions on tested versions of GTK
# unless using an old gimp-dev package.
if $PKG_CONFIG --atleast-version=2.2 gimpui-2.0; then
  if $PKG_CONFIG --max-version=2.8.12 gtk+-2.0; then
    AC_DEFINE(G_DISABLE_DEPRECATED, [], disable deprecated glib features)
    AC_DEFINE(GDK_DISABLE_DEPRECATED, [], disable deprecated gdk features)
    AC_DEFINE(GTK_DISABLE_DEPRECATED, [], disable deprecated gtk+ features)
  fi
fi

# Enable GTK+ 2.6 features (hidden directories in GtkFileChooser)
AC_MSG_CHECKING([if GTK+ is version 2.6 or higher])
if $PKG_CONFIG --atleast-version=2.6 gtk+-2.0; then
  have_gtk_2_6=yes
  AC_DEFINE(HAVE_GTK_2_6, [], have atleast version 2.6 of gtk+)
else
  have_gtk_2_6=no
fi
AC_MSG_RESULT($have_gtk_2_6)

# The GIMP plug-in should be installed under gimplibdir.
# XXX It is not clear if we should install in the gimplibdir-like
# place in our prefix, or in GIMP's actual lib dir.  This only matters
# if our prefix and GIMP's prefix differ.
if test $enable_gimp = yes; then
  test $prefix = NONE || pkg_prefix="--define-variable=prefix=$prefix"
  test $exec_prefix = NONE || pkg_exec_prefix="--define-variable=exec_prefix=$exec_prefix"
  GIMP_LIBDIR=`$PKG_CONFIG --variable=gimplibdir gimp-2.0 $pkg_prefix $pkg_exec_prefix`
  AC_SUBST(GIMP_LIBDIR)
fi

# Ensure that getopt_long is available.  It is included in GNU libc and
# in at least most BSD libcs.  If not found, search for it in libgnugetopt.
AC_SEARCH_LIBS(getopt_long, gnugetopt, ,
  AC_MSG_ERROR(can not build UFRaw without getopt_long))

# The ws2_32 library is required for the ntohs sybmol on WIN32. 
if test $ufraw_win32 = yes; then
  LIBS="$LIBS -lws2_32"
fi

# Make sure that pow is available, trying libm if necessary.
AC_SEARCH_LIBS(pow, m)
AC_CHECK_FUNCS(canonicalize_file_name)
AC_CHECK_FUNCS(memmem)

# Check for jpeg headers and library.
AC_CHECK_HEADER(jpeglib.h,
  AC_CHECK_LIB(jpeg, jpeg_CreateCompress))
have_jpeg=${ac_cv_lib_jpeg_jpeg_CreateCompress:-no}

# Check for png headers and library.
AC_CHECK_HEADER(png.h,
  AC_CHECK_LIB(png, png_create_info_struct))
have_png=${ac_cv_lib_png_png_create_info_struct:-no}

# Check for zlib.
AC_CHECK_LIB(z, deflate)
have_zlib=${ac_cv_lib_z_deflate:-no}

# Check for tiff headers and library.
AC_CHECK_HEADER(tiffio.h,
  AC_CHECK_LIB(tiff, TIFFSetErrorHandler))
have_tiff=${ac_cv_lib_tiff_TIFFSetErrorHandler:-no}

AC_MSG_CHECKING(whether to read raw EXIF data using libexif)
AC_ARG_WITH(libexif,
  [  --with-libexif          use libexif to read raw EXIF data],
  [ with_libexif=yes ],
  [ with_libexif=no ])
AC_MSG_RESULT($with_libexif)
have_libexif=no
COMMENT_LIBEXIF=";"
# Check for libexif via pkg-config.
if test $with_libexif = yes; then
  PKG_CHECK_MODULES(EXIF, libexif >= 0.6.13,
    [ have_libexif=yes
      AC_DEFINE(HAVE_LIBEXIF, 1, have libexif)
      COMMENT_LIBEXIF="" ],
    AC_MSG_RESULT($EXIF_PKG_ERRORS))
fi
AC_SUBST(COMMENT_LIBEXIF)

AC_MSG_CHECKING(whether to read raw EXIF data using exiv2)
AC_ARG_WITH(exiv2,
  [  --with-exiv2            use exiv2 to read raw EXIF data],
  [ with_exiv2=yes ],
  [ with_exiv2=no ])
AC_MSG_RESULT($with_exiv2)
have_exiv2=no
if test $with_exiv2 = yes; then
  AC_CHECK_PROGS(EXIV2_CONFIG, exiv2-config, , $PATH)
  if test -n "$EXIV2_CONFIG"; then
    have_exiv2=yes
    AC_DEFINE(HAVE_EXIV2, 1, have exiv2)
    EXIV2_LIBS=`$EXIV2_CONFIG --libs`
  fi
fi

UFRAW_CFLAGS="$UFRAW_CFLAGS $EXIF_CFLAGS $GIMP_CFLAGS $GTK_CFLAGS $GLIB_CFLAGS $LCMS_CFLAGS"
UFRAW_LDADD="$EXIF_LIBS $EXIV2_LIBS $GLIB_LIBS $LCMS_LIBS"

if test $ufraw_win32 = yes; then
  INSTALL_USER="cp ufraw-gimp.exe \"\$(USERPROFILE)\.gimp-2.0\plug-ins\""
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
else
  INSTALL_USER="gimptool-2.0 --install-bin ufraw-gimp"
fi
AC_SUBST(INSTALL_USER)

# XXX For binary package creation, adjusting for the build CPU is not
# appropriate.
case $host_cpu in
  686)
    UFRAW_CFLAGS="$UFRAW_CFLAGS -march=i686"
    ;;
  powerpc* | ppc*)
# there is a -floop-optimize bug in gcc 3.3.2 on powerpc (resolved in gcc 3.4)
    UFRAW_CFLAGS="$UFRAW_CFLAGS -fno-loop-optimize"
    ;;
esac

ALL_LINGUAS="fr ru"
AM_GLIB_GNU_GETTEXT
LIBS="$LIBS $INTLLIBS"
# The following is not compatible with autoconf 2.60
#AM_GLIB_DEFINE_LOCALEDIR(UFRAW_LOCALEDIR)
GETTEXT_PACKAGE=ufraw
AC_SUBST(GETTEXT_PACKAGE)

AC_SUBST(UFRAW_CFLAGS)
AC_SUBST(UFRAW_LDADD)

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

AC_CONFIG_FILES(Makefile po/Makefile.in)
AC_CONFIG_FILES(ufraw-setup.iss)
AC_OUTPUT

AC_MSG_NOTICE(====================== summary =====================)
AC_MSG_NOTICE(build GIMP plug-in: $enable_gimp)
AC_MSG_NOTICE([hidden directories support (GTK>=2.6): $have_gtk_2_6])
AC_MSG_NOTICE(EXIF support using libexif: $have_libexif)
AC_MSG_NOTICE(EXIF support using exiv2: $have_exiv2)
AC_MSG_NOTICE(JPEG support: $have_jpeg)
AC_MSG_NOTICE(PNG support: $have_png)
AC_MSG_NOTICE(TIFF support: $have_tiff)
if test $have_tiff = yes; then
  AC_MSG_NOTICE(TIFF deflate (lossless compression) support: $have_zlib)
fi
