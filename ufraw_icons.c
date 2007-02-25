/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw_saver.c - The GUI file saver.
 * Copyright 2004-2006 by Udi Fuchs
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
    gtk_icon_theme_add_builtin_icon("ufraw-ufraw", 48, pixbuf);
    iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    gtk_icon_factory_add(factory, "ufraw-ufraw", iconset);

    pixbuf = gdk_pixbuf_new_from_inline(-1, ufraw_exposure_24, FALSE, NULL);
    iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    gtk_icon_factory_add(factory, "ufraw-exposure", iconset);

    pixbuf = gdk_pixbuf_new_from_inline(-1, ufraw_film_24, FALSE, NULL);
    iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    gtk_icon_factory_add(factory, "ufraw-film", iconset);

    pixbuf = gdk_pixbuf_new_from_inline(-1, ufraw_digital_24, FALSE, NULL);
    iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    gtk_icon_factory_add(factory, "ufraw-digital", iconset);

    pixbuf = gdk_pixbuf_new_from_inline(-1, ufraw_restore_lch_24, FALSE, NULL);
    iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    gtk_icon_factory_add(factory, "ufraw-restore-lch", iconset);

    pixbuf = gdk_pixbuf_new_from_inline(-1, ufraw_restore_hsv_24, FALSE, NULL);
    iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    gtk_icon_factory_add(factory, "ufraw-restore-hsv", iconset);

    pixbuf = gdk_pixbuf_new_from_inline(-1, ufraw_interpolation_24, FALSE, NULL);
    iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    gtk_icon_factory_add(factory, "ufraw-interpolation", iconset);
}
