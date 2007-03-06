# $Id: Makefile.am,v 1.3 2007/03/06 21:59:14 udifuchs Exp $

noinst_HEADERS = ufraw_icons.h
icon_DATA = ufraw.png
icondir = $(datadir)/pixmaps

CLEANFILES = ufraw_icons.h

UFRAW_ICONS = ufraw.png exposure-24.png film-24.png digital-24.png \
    restore-lch-24.png restore-hsv-24.png interpolation-24.png
EXTRA_DIST = $(UFRAW_ICONS) exposure.svg film.svg digital.svg \
    restore-lch.svg restore-hsv.svg interpolation.svg

ufraw_icons.h: $(UFRAW_ICONS)
	gdk-pixbuf-csource --build-list  ufraw_icon $(srcdir)/ufraw.png \
		ufraw_exposure_24 $(srcdir)/exposure-24.png \
		ufraw_film_24 $(srcdir)/film-24.png \
		ufraw_digital_24 $(srcdir)/digital-24.png \
		ufraw_restore_lch_24 $(srcdir)/restore-lch-24.png \
		ufraw_restore_hsv_24 $(srcdir)/restore-hsv-24.png \
		ufraw_interpolation_24 $(srcdir)/interpolation-24.png \
		> $@ || rm -f $@
