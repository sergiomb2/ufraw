/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * uf_gtk.h - gtk compatibility header
 * Copyright 2004-2012 by Udi Fuchs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _UF_GTK_H
#define _UF_GTK_H

#include <gtk/gtk.h>
#include <ufobject.h>

#ifdef __cplusplus
extern "C" {
#endif

// Create a GtkCheckButton with a label and a value that gets updated
    GtkWidget *uf_check_button_new(const char *label, gboolean *valuep);

// Create a new ComboBox text with small width.
// The widget must be added with GTK_EXPAND|GTK_FILL.
    GtkWidget *uf_combo_box_new_text();

// Append text with data to combo box
    void uf_combo_box_append_text(GtkComboBox *combo, const char *text, void *data);

// activate combo box according to data or index, if there is no data
    void uf_combo_box_set_active(GtkComboBox *combo, int value);

// remove combo box entry according to data or index, if there is no data
    void uf_combo_box_remove_text(GtkComboBox *combo, int value);

// Set combo box data and keep it up to date
    void uf_combo_box_set_data(GtkComboBox *combo, int *valuep);

// Get the display ICC profile of the monitor associated with the widget.
    void uf_get_display_profile(GtkWidget *widget,
                                guint8 **buffer, gint *buffer_size);

    /*
     * The following functions create GtkWidgets for UFObjects.
     * These widgets are already created with callbacks, so that changes
     * in the widget value are applied to the UFObjects and vice-versa.
     */
    GtkWidget *ufnumber_hscale_new(UFObject *object);
    GtkWidget *ufnumber_spin_button_new(UFObject *object);
    GtkWidget *ufnumber_array_hscale_new(UFObject *object, int index);
    GtkWidget *ufnumber_array_spin_button_new(UFObject *object, int index);
    GtkWidget *ufobject_reset_button_new(const char *tip);
    void ufobject_reset_button_add(GtkWidget *button, UFObject *object);
    GtkWidget *ufstring_entry_new(UFObject *object);
    GtkWidget *ufarray_combo_box_new(UFObject *object);
    GtkWidget *ufarray_combo_box_entry_new(UFObject *object);

#ifdef __cplusplus
}
#endif

#endif /*_UF_GTK_H*/
