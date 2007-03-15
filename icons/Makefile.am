# $Id: Makefile.am,v 1.4 2007/03/15 23:50:56 udifuchs Exp $

noinst_HEADERS = ufraw_icons.h
icon_DATA = ufraw.png
icondir = $(datadir)/pixmaps

CLEANFILES = ufraw_icons.h

UFRAW_ICONS = ufraw.png exposure-24.png film-24.png digital-24.png \
    restore-lch-24.png restore-hsv-24.png interpolation-24.png \
    white-balance-24.png color-management-24.png color-corrections-24.png \
    icc-profile-camera-24.png icc-profile-output-24.png \
    icc-profile-display-24.png curve-24.png

EXTRA_DIST = $(UFRAW_ICONS) exposure.svg film.svg digital.svg \
    restore-lch.svg restore-hsv.svg interpolation.svg \
    white-balance.svg color-corrections.svg \
    icc-profile-output.svg icc-profile-display.svg curve.svg

ufraw_icons.h: $(UFRAW_ICONS)
	gdk-pixbuf-csource --build-list  ufraw_icon $(srcdir)/ufraw.png \
		exposure_24 $(srcdir)/exposure-24.png \
		film_24 $(srcdir)/film-24.png \
		digital_24 $(srcdir)/digital-24.png \
		restore_lch_24 $(srcdir)/restore-lch-24.png \
		restore_hsv_24 $(srcdir)/restore-hsv-24.png \
		interpolation_24 $(srcdir)/interpolation-24.png \
		white_balance_24 $(srcdir)/white-balance-24.png \
		color_management_24 $(srcdir)/color-management-24.png \
		color_corrections_24 $(srcdir)/color-corrections-24.png \
		icc_profile_camera_24 $(srcdir)/icc-profile-camera-24.png \
		icc_profile_output_24 $(srcdir)/icc-profile-output-24.png \
		icc_profile_display_24 $(srcdir)/icc-profile-display-24.png \
		curve_24 $(srcdir)/curve-24.png \
		> $@ || rm -f $@
