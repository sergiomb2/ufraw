/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw_icons.c - load icons for UFRaw's GUI.
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

ufraw_private_function GdkPixbuf *load_icon(GtkIconFactory *factory,
    const guint8 *icon,const char *name)
{
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_inline(-1, icon, FALSE, NULL);
    GtkIconSet *iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    gtk_icon_factory_add(factory, name, iconset);
    return pixbuf;
}

void ufraw_icons_init()
{
    GtkIconFactory *factory = gtk_icon_factory_new();
    gtk_icon_factory_add_default(factory);

    GdkPixbuf *pixbuf = load_icon(factory, ufraw_icon, "ufraw");
    gtk_icon_theme_add_builtin_icon("ufraw", 48, pixbuf);

    load_icon(factory, exposure_24, "exposure");
    load_icon(factory, film_24, "clip-highlights-film");
    load_icon(factory, digital_24, "clip-highlights-digital");
    load_icon(factory, restore_lch_24, "restore-highlights-lch");
    load_icon(factory, restore_hsv_24, "restore-highlights-hsv");
    load_icon(factory, interpolation_24, "interpolation");
    load_icon(factory, white_balance_24, "white-balance");
    load_icon(factory, color_management_24, "color-management");
    load_icon(factory, color_corrections_24, "color-corrections");
    load_icon(factory, icc_profile_camera_24, "icc-profile-camera");
    load_icon(factory, icc_profile_output_24, "icc-profile-output");
    load_icon(factory, icc_profile_display_24, "icc-profile-display");
    load_icon(factory, curve_24, "base-curve");
    load_icon(factory, flip_horiz_24, "object-flip-horizontal");
    load_icon(factory, flip_vert_24, "object-flip-vertical");
    load_icon(factory, rotate_90_24, "object-rotate-right");
    load_icon(factory, rotate_270_24, "object-rotate-left");
    load_icon(factory, lock_24, "object-lock");
    load_icon(factory, exif_24, "exif");
    load_icon(factory, crop_24, "crop");
    load_icon(factory, rectify_24, "rectify");
    load_icon(factory, gimp_24, "gimp");
}
