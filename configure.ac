AC_INIT(ufraw, 0.5)
AC_PREREQ(2.57)

# Create host_os, host_cpu variables
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS(config.h)
AC_PROG_CC
AC_PROG_INSTALL

CFLAGS="$CFLAGS -W -Wall -O3 -ffast-math -fomit-frame-pointer"

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
  AC_SUBST(CONSOLE)
  AC_PREFIX_DEFAULT($PREFIX)
fi

# PREFIX points to none default locations of headers and libraries.
if test "!(-z $PREFIX)"; then
  export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig$PATH_SEPARATOR$PKG_CONFIG_PATH"
  if test $ufraw_win32 = yes; then
    export PKG_CONFIG_PATH=".$PATH_SEPARATOR$PKG_CONFIG_PATH"
  fi
  CPPFLAGS="$CPPFLAGS -I$PREFIX/include"
  LDFLAGS="$LDFLAGS -L$PREFIX/lib"
fi

# If the gimp plug-in is not build, ufraw is completely independent of the gimp
AC_MSG_CHECKING(whether to build the gimp plug-in)
AC_ARG_ENABLE(gimp,
  [  --disable-gimp          disable build of the gimp plug-in], ,
  enable_gimp=yes)
AM_CONDITIONAL(MAKE_GIMP, test $enable_gimp = yes)
AC_MSG_RESULT($enable_gimp)
if test $enable_gimp = yes; then
    gimp_pkg=gimpui-2.0
fi

# DEPS packages must exist to build ufraw
PKG_CHECK_MODULES(DEPS, $gimp_pkg gtk+-2.0 >= 2.4 lcms)
if test $ufraw_win32 = yes; then
  DEPS_CFLAGS=`$PKG_CONFIG --define-variable=prefix=$PREFIX --cflags $gimp_pkg gtk+-2.0 lcms`
  AC_MSG_NOTICE(DEPS_CFLAGS=$DEPS_CFLAGS)
  DEPS_LIBS=`$PKG_CONFIG --define-variable=prefix=$PREFIX --libs $gimp_pkg gtk+-2.0 lcms`
  AC_MSG_NOTICE(DEPS_LIBS=$DEPS_LIBS)
fi

# Disable deprecated functions on tested versions of GTK
# doesn't work on win32 because we are using an old gimp-dev package
if test $ufraw_win32 = no; then
  if $PKG_CONFIG --max-version=2.6.2 gtk+-2.0; then
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

# The gimp plug-in should be installed under gimplibdir
if test $enable_gimp = yes; then
  test $prefix = NONE || pkg_prefix="--define-variable=prefix=$prefix"
  test $exec_prefix = NONE || pkg_exec_prefix="--define-variable=exec_prefix=$exec_prefix"
  GIMP_LIBDIR=`$PKG_CONFIG --variable=gimplibdir gimp-2.0 $pkg_prefix $pkg_exec_prefix`
  AC_SUBST(GIMP_LIBDIR)
fi

# Non GNU environments may require the following
AC_SEARCH_LIBS(getopt_long, gnugetopt, ,
  AC_MSG_ERROR(can not build ufraw without getopt))
#AC_SEARCH_LIBS(ntohs, ws2_32)
if test $ufraw_win32 = yes; then
  LIBS="$LIBS -lws2_32"
fi
AC_SEARCH_LIBS(nan, m)
AC_CHECK_FUNCS(nan canonicalize_file_name)

# Check for jpeg development package
have_jpeg=yes
AC_CHECK_HEADER(jpeglib.h,
  AC_CHECK_LIB(jpeg, jpeg_CreateCompress, , have_jpeg=no))
AM_CONDITIONAL(HAVE_JPEG, test $have_jpeg = yes)

# Check for zlib package
have_zlib=yes
AC_CHECK_LIB(z, deflate, , have_zlib=no)
# it is not a real check on win32 because zlib could be in a DLL,
if test $ufraw_win32 = yes -a $have_zlib = no; then
  have_zlib=yes
  AC_DEFINE(HAVE_LIBZ)
fi

# Check for tiff development package
have_tiff=yes
AC_CHECK_HEADER(tiffio.h,
  AC_CHECK_LIB(tiff, TIFFSetErrorHandler, , have_tiff=no; have_zlib=no))

AC_MSG_CHECKING(whether to try to support exif)
AC_ARG_ENABLE(exif,
  [  --disable-exif          disable exif support], ,
  enable_exif=yes)
AC_MSG_RESULT($enable_exif)
# check for exif development package, relevant only if have_tiff
if test $have_tiff = yes -a $enable_exif = yes; then
  PKG_CHECK_MODULES(EXIF, libexif >= 0.6.11,
    [ have_exif=yes
      AC_DEFINE(HAVE_LIBEXIF, [], have libexif) ],
    [ have_exif=no ])
else
  have_exif=no
fi
if test $have_exif = yes -a $ufraw_win32 = yes; then
    EXIF_CFLAGS="`$PKG_CONFIG --define-variable=prefix=$PREFIX --cflags libexif`"
    AC_DEFINE(HAVE_LIBEXIF)
    AC_MSG_NOTICE(EXIF_CFLAGS=$EXIF_CFLAGS)
    EXIF_LIBS="`$PKG_CONFIG --define-variable=prefix=$PREFIX --libs libexif`"
    AC_MSG_NOTICE(EXIF_LIBS=$EXIF_LIBS)
fi

UFRAW_CFLAGS="$EXIF_CFLAGS $DEPS_CFLAGS"
UFRAW_LDADD="$EXIF_LIBS $DEPS_LIBS"
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
# This is needed only if you plan to cross-make windows-installer from linux
      test -z $PROGRAMFILES && PROGRAMFILES="$HOME/.wine/drive_c/Program\ Files"
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

case $host_cpu in
  686)
    CFLAGS="$CFLAGS -march=i686"
    ;;
  powerpc* | ppc*)
# there is a -floop-optimize bug in gcc 3.3.2 on powerpc (resolved in gcc 3.4)
    CFLAGS="$CFLAGS -mcpu=powerpc -fno-loop-optimize"
    ;;
esac

DCRAW_CFLAGS=-Wno-sign-compare
if test "!($have_jpeg = yes)"; then
  DCRAW_FLAGS="$DCRAW_CFLAGS -DNO_JPEG"
fi
AC_SUBST(DCRAW_CFLAGS)

AC_MSG_CHECKING(whether to build extras)
AC_ARG_ENABLE(extras,
  [  --enable-extras          build extra (dcraw, nikon-curve) executables], ,
  enable_extras=no)
AC_MSG_RESULT($enable_extras)
AM_CONDITIONAL(MAKE_EXTRAS, test $enable_extras = yes)

AC_CONFIG_FILES(Makefile)
AC_CONFIG_FILES(ufraw-setup.iss)
AC_OUTPUT

AC_MSG_NOTICE(====================== summary =====================)
AC_MSG_NOTICE(build gimp plug-in: $enable_gimp)
AC_MSG_NOTICE([hidden directories support (GTK>=2.6): $have_gtk_2_6])
AC_MSG_NOTICE(EXIF support: $have_exif)
AC_MSG_NOTICE(JPEG support: $have_jpeg)
AC_MSG_NOTICE(TIFF support: $have_tiff)
AC_MSG_NOTICE(lossless compressed TIFF support: $have_zlib)

