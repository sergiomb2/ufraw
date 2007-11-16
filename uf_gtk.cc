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
#include <glib/gi18n.h>

#ifdef GDK_WINDOWING_QUARTZ
#include <Carbon/Carbon.h>
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>
#endif

#ifdef G_OS_WIN32
#define STRICT
#include <windows.h>
#endif

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

void uf_label_set_width_chars(GtkLabel *label, gint n_chars)
{
#if GTK_CHECK_VERSION(2,6,0)
    gtk_label_set_width_chars(label, n_chars);
#else
    (void)label;
    (void)n_chars;
#endif
}

// Get the display ICC profile of the monitor associated with the widget.
// For X display, uses the ICC profile specifications version 0.2 from
// http://burtonini.com/blog/computers/xicc
// Based on code from Gimp's modules/cdisplay_lcms.c
#ifdef GDK_WINDOWING_QUARTZ
typedef struct {
    guchar *data;
    gsize   len;
} ProfileTransfer;

enum {
    openReadSpool  = 1, /* start read data process         */
    openWriteSpool = 2, /* start write data process        */
    readSpool      = 3, /* read specified number of bytes  */
    writeSpool     = 4, /* write specified number of bytes */
    closeSpool     = 5  /* complete data transfer process  */
};

static OSErr _uf_lcms_flatten_profile(SInt32  command,
    SInt32 *size, void *data, void *refCon)
{
    ProfileTransfer *transfer = refCon;

    switch (command)
    {
    case openWriteSpool:
	g_return_val_if_fail(transfer->data==NULL && transfer->len==0, -1);
	break;

    case writeSpool:
	transfer->data = g_realloc(transfer->data, transfer->len + *size);
	memcpy(transfer->data + transfer->len, data, *size);
	transfer->len += *size;
	break;

    default:
	break;
    }
    return 0;
}
#endif /* GDK_WINDOWING_QUARTZ */

void uf_get_display_profile(GtkWidget *widget,
    guint8 **buffer, gint *buffer_size)
{
    *buffer = NULL;
    *buffer_size = 0;
#if defined GDK_WINDOWING_X11
    GdkScreen *screen = gtk_widget_get_screen(widget);
    if ( screen==NULL )
	screen = gdk_screen_get_default();
    int monitor = gdk_screen_get_monitor_at_window (screen, widget->window);
    char *atom_name;
    if (monitor > 0)
	atom_name = g_strdup_printf("_ICC_PROFILE_%d", monitor);
    else
	atom_name = g_strdup("_ICC_PROFILE");

    GdkAtom type = GDK_NONE;
    gint format = 0;
    gdk_property_get(gdk_screen_get_root_window(screen),
	    gdk_atom_intern(atom_name, FALSE), GDK_NONE,
	    0, 64 * 1024 * 1024, FALSE,
	    &type, &format, buffer_size, buffer);
    g_free(atom_name);

#elif defined GDK_WINDOWING_QUARTZ
    GdkScreen *screen = gtk_widget_get_screen(widget);
    if ( screen==NULL )
	screen = gdk_screen_get_default();
    int monitor = gdk_screen_get_monitor_at_window(screen, widget->window);

    CMProfileRef prof = NULL;
    CMGetProfileByAVID(monitor, &prof);
    if ( prof==NULL )
	return;

    ProfileTransfer transfer = { NULL, 0 };
    Boolean foo;
    CMFlattenProfile(prof, 0, _uf_lcms_flatten_profile, &transfer, &foo);
    CMCloseProfile(prof);

    *buffer = transfer.data;
    *buffer_size = transfer.len;

#elif defined G_OS_WIN32
    (void)widget;
    HDC hdc = GetDC (NULL);
    if ( hdc==NULL )
	return;

    DWORD len = 0;
    GetICMProfile (hdc, &len, NULL);
    gchar *path = g_new (gchar, len);

    if (GetICMProfile (hdc, &len, path)) {
	gsize size;
	g_file_get_contents(path, (gchar**)buffer, &size, NULL);
	*buffer_size = size;
    }
    g_free (path);
    ReleaseDC (NULL, hdc);
#endif
}

// Translate text message from GtkImageView:
const char *uf_gtkimageview_text = N_("Open the navigator window");

} // extern "C"
