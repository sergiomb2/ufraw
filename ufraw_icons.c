/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw_saver.c - The GUI file saver.
 * Copyright 2004-2007 by Udi Fuchs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation. You should have received
 * a copy of the license along with this program.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include "ufraw.h"
#include "icons/ufraw_icons.h"

void ufraw_icons_init()
{
    GdkPixbuf *pixbuf;
    GtkIconSet *iconset;
    GtkIconFactory *factory = gtk_icon_factory_new();
    gtk_icon_factory_add_default(factory);

    pixbuf = gdk_pixbuf_new_from_inline(-1, ufraw_icon, FALSE, NULL);
    gtk_icon_theme_add_builtin_icon("ufraw", 48, pixbuf);
    iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    gtk_icon_factory_add(factory, "ufraw", iconset);

    pixbuf = gdk_pixbuf_new_from_inline(-1, exposure_24, FALSE, NULL);
    iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    gtk_icon_factory_add(factory, "exposure", iconset);

    pixbuf = gdk_pixbuf_new_from_inline(-1, film_24, FALSE, NULL);
    iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    gtk_icon_factory_add(factory, "clip-highlights-film", iconset);

    pixbuf = gdk_pixbuf_new_from_inline(-1, digital_24, FALSE, NULL);
    iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    gtk_icon_factory_add(factory, "clip-highlights-digital", iconset);

    pixbuf = gdk_pixbuf_new_from_inline(-1, restore_lch_24, FALSE, NULL);
    iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    gtk_icon_factory_add(factory, "restore-highlights-lch", iconset);

    pixbuf = gdk_pixbuf_new_from_inline(-1, restore_hsv_24, FALSE, NULL);
    iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    gtk_icon_factory_add(factory, "restore-highlights-hsv", iconset);

    pixbuf = gdk_pixbuf_new_from_inline(-1, interpolation_24, FALSE, NULL);
    iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    gtk_icon_factory_add(factory, "interpolation", iconset);

    pixbuf = gdk_pixbuf_new_from_inline(-1, white_balance_24, FALSE, NULL);
    iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    gtk_icon_factory_add(factory, "white-balance", iconset);

    pixbuf = gdk_pixbuf_new_from_inline(-1, color_management_24, FALSE, NULL);
    iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    gtk_icon_factory_add(factory, "color-management", iconset);

    pixbuf = gdk_pixbuf_new_from_inline(-1, color_corrections_24, FALSE, NULL);
    iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    gtk_icon_factory_add(factory, "color-corrections", iconset);

    pixbuf = gdk_pixbuf_new_from_inline(-1, icc_profile_camera_24, FALSE, NULL);
    iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    gtk_icon_factory_add(factory, "icc-profile-camera", iconset);

    pixbuf = gdk_pixbuf_new_from_inline(-1, icc_profile_output_24, FALSE, NULL);
    iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    gtk_icon_factory_add(factory, "icc-profile-output", iconset);

    pixbuf = gdk_pixbuf_new_from_inline(-1, icc_profile_display_24, FALSE, NULL);
    iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    gtk_icon_factory_add(factory, "icc-profile-display", iconset);

    pixbuf = gdk_pixbuf_new_from_inline(-1, curve_24, FALSE, NULL);
    iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    gtk_icon_factory_add(factory, "base-curve", iconset);
}
