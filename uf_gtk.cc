/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * uf_gtk.cc - gtk compatibility layer
 * Copyright 2004-2007 by Udi Fuchs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation. You should have received
 * a copy of the license along with this program.
 */

#include "uf_gtk.h"

extern "C" {

#if !GTK_CHECK_VERSION(2,12,0)
static void _uf_widget_set_tooltip_destroy_event(GObject *object,
    gpointer user_data)
{
    (void)user_data;
    GObject *Tooltips = static_cast<GObject *>(
	g_object_get_data(object, "UF-Tooltips"));
    if ( Tooltips!=NULL )
	g_object_unref(Tooltips);
}
#endif

// Interface to gtk_widget_set_tooltip_text() introduced in 2.12
// Should be called after widget has a toplevel to minimize number
// of calls to gtk_tooltips_new()
void uf_widget_set_tooltip(GtkWidget *widget, const gchar *text)
{
#if GTK_CHECK_VERSION(2,12,0)
    gtk_widget_set_tooltip_text(widget, text);
#else
    GObject *parentWindow = G_OBJECT(gtk_widget_get_toplevel(widget));
    GtkTooltips *Tooltips = static_cast<GtkTooltips *>(
	g_object_get_data(parentWindow, "UF-Tooltips"));
    if ( Tooltips==NULL ) {
	// On first call create the tooltips
	Tooltips = gtk_tooltips_new();
#if GTK_CHECK_VERSION(2,10,0)
	g_object_ref_sink(Tooltips);
#else
	g_object_ref(Tooltips);
	gtk_object_sink(GTK_OBJECT(Tooltips));
#endif
	g_object_set_data(parentWindow, "UF-Tooltips", Tooltips);
	g_signal_connect(parentWindow, "destroy",
            G_CALLBACK(_uf_widget_set_tooltip_destroy_event), NULL);
    }
    gtk_tooltips_set_tip(Tooltips, widget, text, NULL);
#endif
}

void uf_button_set_stock_image(GtkButton *button, const gchar *stock_image)
{
#if GTK_CHECK_VERSION(2,8,0)
    gtk_button_set_image(button, gtk_image_new_from_stock(
            stock_image, GTK_ICON_SIZE_BUTTON));
#else
    GtkWidget *lastImage = gtk_bin_get_child(GTK_BIN(button));
    if ( lastImage!=NULL )
	gtk_container_remove(GTK_CONTAINER(button), lastImage);

    GtkWidget *image = gtk_image_new_from_stock(stock_image,
	    GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(button), image);
    gtk_widget_show(image);
#endif
}

// Interface to gtk_window_set_icon_name() introduced in 2.6
// but was buggy until 2.8
void uf_window_set_icon_name(GtkWindow *window, const gchar *name)
{
#if GTK_CHECK_VERSION(2,8,0)
    gtk_window_set_icon_name(window, name);
#else
    gtk_window_set_icon(window,
	    gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
		    name, 48, GTK_ICON_LOOKUP_USE_BUILTIN, NULL));
#endif
}

} // extern "C"
