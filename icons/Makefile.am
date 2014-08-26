# $Id: Makefile.am,v 1.17 2014/08/26 03:00:39 nkbj Exp $

noinst_HEADERS = ufraw_icons.h
icon_DATA = ufraw.png
icondir = $(datadir)/pixmaps

MAINTAINERCLEANFILES = ufraw_icons.h

UFRAW_ICONS = ufraw.png exposure-24.png film-24.png digital-24.png \
    restore-lch-24.png restore-hsv-24.png interpolation-24.png \
    white-balance-24.png color-management-24.png color-corrections-24.png \
    icc-profile-camera-24.png icc-profile-output-24.png \
    icc-profile-display-24.png curve-24.png grayscale-24.png \
    flip-horiz-24.png flip-vert-24.png rotate-90-24.png rotate-270-24.png \
    lock-24.png unlock-24.png exif-24.png crop-24.png rectify-24.png \
    gimp-24.png channel-blue-24.png channel-green-24.png channel-red-24.png \
    channel-grey-24.png lens-24.png tca-24.png vignetting-24.png distortion-24.png \
    automatic-24.png manual-24.png \
    geometry-24.png hueadjust-24.png

EXTRA_DIST = $(UFRAW_ICONS) ufraw_icons.h exposure.svg film.svg digital.svg \
    restore-lch.svg restore-hsv.svg interpolation.svg \
    white-balance.svg color-corrections.svg \
    icc-profile-output.svg icc-profile-display.svg curve.svg \
    lock.svg unlock.svg exif.svg crop.svg rectify.svg grayscale.svg \
    automatic.svg manual.svg \
    lens.svg tca.svg vignetting.svg distortion.svg geometry.svg hueadjust.svg

ufraw_icons.h: $(UFRAW_ICONS) Makefile.am
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
		lock_24 $(srcdir)/lock-24.png \
		unlock_24 $(srcdir)/unlock-24.png \
		automatic_24 $(srcdir)/automatic-24.png \
		manual_24 $(srcdir)/manual-24.png \
		exif_24 $(srcdir)/exif-24.png \
		crop_24 $(srcdir)/crop-24.png \
		rectify_24 $(srcdir)/rectify-24.png \
		lens_24 $(srcdir)/lens-24.png \
		flip_horiz_24 $(srcdir)/flip-horiz-24.png \
		flip_vert_24 $(srcdir)/flip-vert-24.png \
		rotate_90_24 $(srcdir)/rotate-90-24.png \
		rotate_270_24 $(srcdir)/rotate-270-24.png \
		gimp_24 $(srcdir)/gimp-24.png \
		grayscale_24 $(srcdir)/grayscale-24.png \
		channel_blue_24 $(srcdir)/channel-blue-24.png \
		channel_green_24 $(srcdir)/channel-green-24.png \
		channel_red_24 $(srcdir)/channel-red-24.png \
		channel_grey_24 $(srcdir)/channel-grey-24.png \
		tca_24 $(srcdir)/tca-24.png \
		distortion_24 $(srcdir)/distortion-24.png \
		vignetting_24 $(srcdir)/vignetting-24.png \
		geometry_24 $(srcdir)/geometry-24.png \
		hueadjust_24 $(srcdir)/hueadjust-24.png \
		> $@ || { rm -f $@; exit 1; }
