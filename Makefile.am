# $Id: Makefile.am,v 1.37 2007/05/12 05:16:14 nkbj Exp $

SUBDIRS=po icons

if MAKE_EXTRAS
  bin_PROGRAMS = ufraw ufraw-batch dcraw nikon-curve
else
  bin_PROGRAMS = ufraw ufraw-batch
endif

if MAKE_GIMP
  gimpbin_PROGRAMS = ufraw-gimp
  gimpbindir = $(GIMP_LIBDIR)/plug-ins
endif

if MAKE_CINEPAINT
  cinepaintbin_PROGRAMS = ufraw-cinepaint
  cinepaintbindir = $(CINEPAINT_PROGRAMPLUGINDIR)/plug-ins
endif

POD2MAN= @POD2MAN@
man_MANS = ufraw.1

noinst_LIBRARIES = libufraw.a

CLEANFILES = ufraw.1 ufraw.schemas ufraw_icon.opc ufraw-setup.bmp

if INSTALL_MIME
  app_DATA = ufraw.desktop
  appdir = $(datadir)/applications

  mime_DATA = ufraw-mime.xml
  mimedir = $(datadir)/mime/packages

  schemas_DATA = ufraw.schemas
  schemasdir = $(datadir)/gconf/schemas
endif

if UFRAW_WIN32
  UFRAW_ICON = ufraw_icon.opc
else
  UFRAW_ICON =
endif

EXTRA_DIST = ufraw.desktop ufraw_icon.ico ufraw_icon.rc \
    ufraw-mime.xml generate_schemas.sh \
    ufraw-setup.jpg autogen.sh MANIFEST ufraw.pod

AM_CPPFLAGS = $(UFRAW_CFLAGS) -DDCRAW_NOMAIN \
		-DUFRAW_LOCALEDIR=\"$(datadir)/locale\"
LDADD = libufraw.a $(UFRAW_ICON) $(UFRAW_LDADD)
LINK = $(CXXLINK)

libufraw_a_SOURCES = \
    dcraw.cc ufraw_ufraw.c ufraw_routines.c ufraw_developer.c \
    ufraw_conf.c ufraw_writer.c ufraw_embedded.c ufraw.h \
    wb_presets.c dcraw_api.cc dcraw_api.h dcraw_indi.c dcraw.h \
    nikon_curve.c nikon_curve.h \
    ufraw_exiv2.cc iccjpeg.c iccjpeg.h \
    ufraw_preview.c ufraw_saver.c \
    ufraw_chooser.c ufraw_icons.c icons/ufraw_icons.h \
    curveeditor_widget.c curveeditor_widget.h

ufraw_SOURCES = ufraw.c
ufraw_LDADD = $(LDADD) $(GTK_LIBS)

ufraw_batch_SOURCES = ufraw-batch.c
ufraw_batch_LDFLAGS = @CONSOLE@

if MAKE_GIMP
  ufraw_gimp_SOURCES = ufraw-gimp.c
  ufraw_gimp_CPPFLAGS = $(AM_CPPFLAGS) $(GIMP_CFLAGS) 
  ufraw_gimp_LDADD = $(LDADD) $(GIMP_LIBS)
endif

if MAKE_CINEPAINT
  ufraw_cinepaint_SOURCES = ufraw-gimp.c
  ufraw_cinepaint_CPPFLAGS = $(AM_CPPFLAGS) $(CINEPAINT_CFLAGS) \
				-DUFRAW_CINEPAINT
  ufraw_cinepaint_LDADD = $(LDADD) $(CINEPAINT_LIBS) $(GTK_LIBS)
endif

if MAKE_EXTRAS
  dcraw_SOURCES = dcraw.cc
  dcraw_CPPFLAGS = $(UFRAW_CFLAGS)
  dcraw_LDFLAGS = @CONSOLE@
  dcraw_LDADD = $(UFRAW_LDADD)

  nikon_curve_SOURCES = nikon_curve.c
  nikon_curve_CFLAGS = $(UFRAW_CFLAGS) -D_STAND_ALONE_
  nikon_curve_LDFLAGS = @CONSOLE@
  nikon_curve_LDADD = $(UFRAW_LDADD)
endif

#ufraw_icon.ico: icons/ufraw.png
#	{ pngtopnm $^ | pnmquant 256; pngtopnm -alpha $^; } | ppmtowinicon -andpgms -output $@ - -

ufraw_icon.opc: ufraw_icon.rc ufraw_icon.ico
	$(WINDRES) --include-dir=$(srcdir) --input $< --output $@

.pod.1:
	$(POD2MAN) --section 1 --center "" --release UFRAW $< $@

ufraw.schemas: generate_schemas.sh
	$(srcdir)/generate_schemas.sh $(prefix) $@

install-windows: windows-installer
	$(WINE) ./ufraw-$(VERSION)-setup.exe

windows-installer: ufraw-$(VERSION)-setup.exe

ufraw-$(VERSION)-setup.exe: ufraw-setup.iss ufraw.exe ufraw-batch.exe \
    ufraw-gimp.exe ufraw-setup.bmp ufraw_icon.bmp ufraw_icon.ico \
    $(PREFIX)/bin/liblcms-1-16-ufraw.dll
	strip ufraw.exe ufraw-batch.exe ufraw-gimp.exe
	$(WINE) $(ISCC) ufraw-setup.iss

ufraw-setup.bmp: ufraw-setup.jpg
	montage -geometry +0+0 $< $@

ufraw_icon.bmp: icons/ufraw.png
	montage -geometry +0+0 $< $@
