dnl $Id: configure.ac,v 1.24 2006/01/25 06:20:00 nkbj Exp $
AC_INIT(UFRaw, 0.7)
AC_PREREQ(2.57)

# Create host_os, host_cpu variables
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)
AC_PROG_CC
AC_PROG_INSTALL

AC_CHECK_PROGS(POD2MAN, pod2man, , $PATH)

# nikon_curve makes some sizeof assumptions, requiring the following tests
# XXX Fix nikon_curve to use C99 standard types so it will work on 64-bit
# machines.
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

# XXX  gcc-specific
CFLAGS="$CFLAGS -W -Wall -O3 -ffast-math -fomit-frame-pointer"
# Uncomment next line for release to strip the binaries of debug information
#LDFLAGS="$LDFLAGS -s"

# Set default PREFIX, if not already defined according to os.
# windows will require some special attention
# in cygwin we always cross-compile to mingw32
case $host_os in
  *cygwin* )
    ufraw_win32=yes
    test -z $PREFIX && PREFIX=/cygdrive/c/mingw32
    ;;
  *mingw* )
    ufraw_win32=yes
    test -z $PREFIX && PREFIX=/c/mingw32
    ;;
  *darwin* )
    ufraw_win32=no
    test -z $PREFIX && PREFIX=/sw
    ;;
  * )
    ufraw_win32=no
    ;;
esac
AM_CONDITIONAL(UFRAW_WIN32, test $ufraw_win32 = yes)

# In windows --prefix=$PREFIX is set as default
if test $ufraw_win32 = yes; then
  CFLAGS="$CFLAGS -mno-cygwin -mwindows -mms-bitfields"
  CONSOLE="-mconsole"
  #AC_PREFIX_DEFAULT($PREFIX) # this ac macro ignores the test condition
  prefix=$PREFIX
else
  CONSOLE=
fi
AC_SUBST(CONSOLE)

# If prefix is non-empty, add prefix's pkgconfig dir to the pkgconfig search
# path, so we'll find programs/libraries also installed to this prefix.
# XXX Really, where we are building to and where we obtain things from are
# logically separate, and this should be solved by --with-gimp=/foo/bar
# arguments.
if test "!(-z $PREFIX)"; then
  echo "Adding pkgconfig/cppflags/ldflags for $PREFIX."
  export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig$PATH_SEPARATOR$PKG_CONFIG_PATH"
  CPPFLAGS="$CPPFLAGS -I$PREFIX/include"
  LDFLAGS="$LDFLAGS -L$PREFIX/lib"
fi

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
  if $PKG_CONFIG --max-version=2.8.9 gtk+-2.0; then
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

# XXX A comment is required to explain this. 
#AC_SEARCH_LIBS(ntohs, ws2_32)
if test $ufraw_win32 = yes; then
  LIBS="$LIBS -lws2_32"
fi

# Make sure that pow is available, trying libm if necessary.
AC_SEARCH_LIBS(pow, m)
AC_CHECK_FUNCS(canonicalize_file_name)

# Check for jpeg headers and library.
AC_CHECK_HEADER(jpeglib.h,
  AC_CHECK_LIB(jpeg, jpeg_CreateCompress))
have_jpeg=${ac_cv_lib_jpeg_jpeg_CreateCompress:-no}

# Check for zlib.
AC_CHECK_LIB(z, deflate)
have_zlib=${ac_cv_lib_z_deflate:-no}

# Check for tiff headers and library.
AC_CHECK_HEADER(tiffio.h,
  AC_CHECK_LIB(tiff, TIFFSetErrorHandler))
have_tiff=${ac_cv_lib_tiff_TIFFSetErrorHandler:-no}

AC_MSG_CHECKING(whether to try to support exif)
AC_ARG_ENABLE(exif,
  [  --disable-exif          disable exif support], ,
  enable_exif=yes)
AC_MSG_RESULT($enable_exif)
# If we have tiff, check for exif via pkg-config.
if test $have_tiff = yes -a $enable_exif = yes; then
  PKG_CHECK_MODULES(EXIF, libexif >= 0.6.11,
    [ have_exif=yes
      AC_DEFINE(HAVE_LIBEXIF, [], have libexif) ],
    [ have_exif=no ])
else
  have_exif=no
fi

UFRAW_CFLAGS="$EXIF_CFLAGS $GIMP_CFLAGS $GTK_CFLAGS $GLIB_CFLAGS $LCMS_CFLAGS"
if test $have_jpeg = no; then
  # DCRaw assumes jpeg unless NO_JPEG, rather than using HAVE_LIBJPEG
  UFRAW_CFLAGS="$UFRAW_CFLAGS -DNO_JPEG"
fi
UFRAW_LDADD="$EXIF_LIBS $GLIB_LIBS $LCMS_LIBS"
AC_SUBST(UFRAW_CFLAGS)
AC_SUBST(UFRAW_LDADD)

if test $ufraw_win32 = yes; then
  INSTALL_USER="cp ufraw-gimp.exe \"\$(USERPROFILE)\.gimp-2.0\plug-ins\""
  if test -z $target_alias; then
    WINDRES="windres"
  else
    WINDRES="$target_alias-windres"
  fi
  AC_SUBST(WINDRES)
  # For the windows-intstaller make needs the location of the DLLs
  AC_SUBST(PREFIX)
  # and ISCC (ufraw-setup.iss.in) needs them in dos format...
  test -z $DOSPREFIX && DOSPREFIX="C:\\mingw32"
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
    CFLAGS="$CFLAGS -march=i686"
    ;;
  powerpc* | ppc*)
# there is a -floop-optimize bug in gcc 3.3.2 on powerpc (resolved in gcc 3.4)
    CFLAGS="$CFLAGS -mcpu=powerpc -fno-loop-optimize"
    ;;
esac

# XXX gcc-specific
DCRAW_CFLAGS=-Wno-sign-compare

AC_SUBST(DCRAW_CFLAGS)

AC_MSG_CHECKING(whether to build extras)
AC_ARG_ENABLE(extras,
  [  --enable-extras          build extra (dcraw, nikon-curve, ufraw-exif) executables], ,
  enable_extras=no)
AC_MSG_RESULT($enable_extras)
AM_CONDITIONAL(MAKE_EXTRAS, test $enable_extras = yes)

AC_CONFIG_FILES(Makefile)
AC_CONFIG_FILES(ufraw-setup.iss)
AC_OUTPUT

AC_MSG_NOTICE(====================== summary =====================)
AC_MSG_NOTICE(build GIMP plug-in: $enable_gimp)
AC_MSG_NOTICE([hidden directories support (GTK>=2.6): $have_gtk_2_6])
AC_MSG_NOTICE(EXIF support: $have_exif)
AC_MSG_NOTICE(JPEG support: $have_jpeg)
AC_MSG_NOTICE(TIFF support: $have_tiff)
if test $have_tiff = yes; then
  AC_MSG_NOTICE(TIFF deflate (lossless compression) support: $have_zlib)
fi
