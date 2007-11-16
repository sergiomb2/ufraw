/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * uf_gtk.h - gtk compatibility header
 * Copyright 2004-2007 by Udi Fuchs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation. You should have received
 * a copy of the license along with this program.
 */

#ifndef _UF_GTK_H
#define _UF_GTK_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <gtk/gtk.h>

// Interface to gtk_widget_set_tooltip_text() introduced in 2.12
// Should be called after widget has a toplevel to minimize number
// of calls to gtk_tooltips_new()
void uf_widget_set_tooltip(GtkWidget *widget, const gchar *text);

void uf_button_set_stock_image(GtkButton *button, const gchar *stock_image);

void uf_label_set_width_chars(GtkLabel *label, gint n_chars);

// Interface to gtk_window_set_icon_name() introduced in 2.6
void uf_window_set_icon_name(GtkWindow *window, const gchar *name);

// Get the display ICC profile of the monitor associated with the widget.
void uf_get_display_profile(GtkWidget *widget,
     guint8 **buffer, gint *buffer_size);

#ifdef  __cplusplus
}
#endif

#endif /*_UF_GTK_H*/
