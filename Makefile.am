# $Id: Makefile.am,v 1.20 2006/08/26 13:29:16 udifuchs Exp $

if MAKE_EXTRAS
  bin_PROGRAMS = ufraw ufraw-batch dcraw nikon-curve
else
  bin_PROGRAMS = ufraw ufraw-batch
endif

if MAKE_GIMP
  gimpbin_PROGRAMS = ufraw-gimp
  gimpbindir = $(GIMP_LIBDIR)/plug-ins
endif

POD2MAN= @POD2MAN@
man_MANS = ufraw.1

CLEANFILES = ufraw.1

if INSTALL_MIME
  app_DATA = ufraw.desktop
  appdir = $(datadir)/applications

  mime_DATA = ufraw-mime.xml
  mimedir = $(datadir)/mime/packages

  schemas_DATA = ufraw.schemas
  schemasdir = $(datadir)/gconf/schemas
endif

icon_DATA = ufraw.png
icondir = $(datadir)/pixmaps

#html_DATA = user_guide.html
#htmldir = $(datadir)/doc/ufraw

UFRAW_COMMON = ufraw_ufraw.c ufraw_routines.c ufraw_exif.c ufraw_developer.c \
    ufraw_conf.c ufraw_writer.c ufraw_embedded.c ufraw.h ufraw_icon.h \
    wb_presets.c dcraw_api.cc dcraw_api.h dcraw_indi.c dcraw.h \
    nikon_curve.c nikon_curve.h \
    ufraw_exiv2.cc iccjpeg.c iccjpeg.h
UFRAW_GTK_COMMON = $(UFRAW_COMMON) ufraw_preview.c ufraw_saver.c \
    ufraw_chooser.c curveeditor_widget.c curveeditor_widget.h

if UFRAW_WIN32
  UFRAW_ICON = ufraw_icon.opc
else
  UFRAW_ICON =
endif

EXTRA_DIST = ufraw.desktop ufraw.png ufraw_icon.ico ufraw_icon.rc \
    ufraw-mime.xml generate_schemas.sh \
    ufraw-setup.jpg autogen.sh MANIFEST ufraw.pod

AM_CFLAGS = $(UFRAW_CFLAGS)
AM_CXXFLAGS = $(UFRAW_CFLAGS)
LDADD = dcraw_nomain.o $(UFRAW_ICON) $(UFRAW_LDADD)

ufraw_SOURCES = ufraw.c $(UFRAW_GTK_COMMON)
ufraw_LDADD = dcraw_nomain.o $(UFRAW_ICON) $(UFRAW_LDADD) $(GTK_LIBS)

ufraw_batch_SOURCES = ufraw-batch.c $(UFRAW_COMMON)
ufraw_batch_LDFLAGS = @CONSOLE@

if MAKE_GIMP
  ufraw_gimp_SOURCES = ufraw-gimp.c $(UFRAW_GTK_COMMON)
  ufraw_gimp_LDADD = dcraw_nomain.o $(UFRAW_ICON) $(UFRAW_LDADD) $(GIMP_LIBS)
endif

dcraw_nomain.o: dcraw.cc
	$(COMPILE) -DDCRAW_NOMAIN -DNO_LCMS -o $@ -c $^

if MAKE_EXTRAS
  dcraw_SOURCES = dcraw.cc
  dcraw_CXXFLAGS = $(UFRAW_CFLAGS)
  dcraw_LDFLAGS = @CONSOLE@
  dcraw_LDADD = $(UFRAW_LDADD)

  nikon_curve_SOURCES = nikon_curve.c
  nikon_curve_CFLAGS = $(UFRAW_CFLAGS) -D_STAND_ALONE_
  nikon_curve_LDFLAGS = @CONSOLE@
  nikon_curve_LDADD = $(UFRAW_LDADD)
endif

#ufraw_icon.h: ufraw.png
#	gdk-pixbuf-csource --raw $^ --name=ufraw_icon > $@

#ufraw_icon.ico: ufraw.png
#	{ pngtopnm $^ | pnmquant 256; pngtopnm -alpha $^; } | ppmtowinicon -andpgms -output $@ - -

ufraw_icon.opc: ufraw_icon.rc ufraw_icon.ico
	$(WINDRES) -i $< -o $@

.pod.1:
	$(POD2MAN) --section 1 --center "" --release UFRAW $< $@

ufraw.schemas: generate_schemas.sh
	$(srcdir)/$< $(prefix) $@

install-user: all
	$(INSTALL_USER)

install-exec-local:
if UFRAW_WIN32
	@echo "################################################################"
	@echo "'make install' is not really supported on cygwin/mingw32"
	@echo "'make install-windows' will make a standard windows installation"
	@echo "################################################################"
endif

install-windows: windows-installer
	$(WINE) ./ufraw-$(VERSION)-setup.exe

windows-installer: ufraw-$(VERSION)-setup.exe

ufraw-$(VERSION)-setup.exe: ufraw-setup.iss ufraw.exe ufraw-batch.exe \
    ufraw-gimp.exe ufraw-setup.bmp ufraw_icon.bmp ufraw_icon.ico \
    $(PREFIX)/bin/liblcms-1.dll $(PREFIX)/bin/libexif-12.dll
	strip ufraw.exe ufraw-batch.exe ufraw-gimp.exe
	$(WINE) $(ISCC) ufraw-setup.iss

ufraw-setup.bmp: ufraw-setup.jpg
	montage -geometry +0+0 $< $@

ufraw_icon.bmp: ufraw.png
	montage -geometry +0+0 $< $@
