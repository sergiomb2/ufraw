/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * uf_gtk.cc - gtk compatibility layer
 * Copyright 2004-2012 by Udi Fuchs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "uf_gtk.h"
#include <glib/gi18n.h>
#include <assert.h>
#include <list>

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

    static void _uf_toggle_button_toggled(GtkToggleButton *button, gboolean *valuep)
    {
        *valuep = gtk_toggle_button_get_active(button);
    }

// Create a GtkCheckButton with a label an a value that gets updated
    GtkWidget *uf_check_button_new(const char *label, gboolean *valuep)
    {
        GtkWidget *button = gtk_check_button_new_with_label(label);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), *valuep);
        g_signal_connect(G_OBJECT(button), "toggled",
                         G_CALLBACK(_uf_toggle_button_toggled), valuep);
        return button;
    }

// Create a new ComboBox text with small width.
// The widget must be added with GTK_EXPAND|GTK_FILL.
    GtkWidget *uf_combo_box_new_text()
    {
        GtkWidget *combo = gtk_combo_box_new_text();
        gtk_widget_set_size_request(combo, 50, -1);
        return combo;
    }

// Append text with data to combo box
    void uf_combo_box_append_text(GtkComboBox *combo, const char *text, void *data)
    {
        gtk_combo_box_append_text(combo, text);
        GList *list = static_cast<GList *>(
                          g_object_get_data(G_OBJECT(combo), "uf-combo-list"));
        list = g_list_append(list, data);
        g_object_set_data(G_OBJECT(combo), "uf-combo-list", list);
    }

// activate combo box according to data or index, if there is no data
    void uf_combo_box_set_active(GtkComboBox *combo, int value)
    {
        GList *list = static_cast<GList *>(
                          g_object_get_data(G_OBJECT(combo), "uf-combo-list"));
        if (list != NULL) {
            int i;
            for (i = 0; list != NULL; i++, list = g_list_next(list)) {
                if (value == GPOINTER_TO_INT(list->data)) {
                    gtk_combo_box_set_active(combo, i);
                    return;
                }
            }
            // If value not found set activate first entry
            gtk_combo_box_set_active(combo, 0);
        } else {
            gtk_combo_box_set_active(combo, value);
        }
    }

    static void _uf_combo_changed(GtkComboBox *combo, int *valuep)
    {
        GList *list = static_cast<GList *>(
                          g_object_get_data(G_OBJECT(combo), "uf-combo-list"));
        if (list != NULL) {
            int i = gtk_combo_box_get_active(combo);
            *valuep = GPOINTER_TO_INT(g_list_nth_data(list, i));
        } else {
            *valuep = gtk_combo_box_get_active(combo);
        }
    }

// Set combo box data and keep it up to date
    void uf_combo_box_set_data(GtkComboBox *combo, int *valuep)
    {
        gulong handler_id = reinterpret_cast<gulong>(
                                g_object_get_data(G_OBJECT(combo), "uf-combo-handler-id"));
        if (handler_id != 0)
            g_signal_handler_disconnect(G_OBJECT(combo), handler_id);

        uf_combo_box_set_active(combo, *valuep);
        if (gtk_combo_box_get_active(combo) == 0) {
            // If value was not found in uf-combo-list, set it to first entry
            GList *list = static_cast<GList *>(
                              g_object_get_data(G_OBJECT(combo), "uf-combo-list"));
            if (list != NULL)
                *valuep = GPOINTER_TO_INT(list->data);
        }
        handler_id = g_signal_connect(G_OBJECT(combo), "changed",
                                      G_CALLBACK(_uf_combo_changed), valuep);
        g_object_set_data(G_OBJECT(combo), "uf-combo-handler-id",
                          reinterpret_cast<gpointer>(handler_id));
    }

// remove combo box entry according to data or index, if there is no data
    void uf_combo_box_remove_text(GtkComboBox *combo, int value)
    {
        GList *list = static_cast<GList *>(
                          g_object_get_data(G_OBJECT(combo), "uf-combo-list"));
        if (list != NULL) {
            int i;
            for (i = 0; list != NULL; i++, list = g_list_next(list)) {
                if (value == GPOINTER_TO_INT(list->data)) {
                    gtk_combo_box_remove_text(combo, i);
                    list = g_list_remove(list, list->data);
                    g_object_set_data(G_OBJECT(combo), "uf-combo-list", list);
                    return;
                }
            }
        } else {
            gtk_combo_box_remove_text(combo, value);
        }
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
        ProfileTransfer *transfer = static_cast<ProfileTransfer*>(refCon);

        switch (command) {
            case openWriteSpool:
                g_return_val_if_fail(transfer->data == NULL && transfer->len == 0, -1);
                break;

            case writeSpool:
                transfer->data = static_cast<guchar*>(
                                     g_realloc(transfer->data, transfer->len + *size));
                memcpy(transfer->data + transfer->len, data, *size);
                transfer->len += *size;
                break;

            default:
                break;
        }
        return 0;
    }
#endif /* GDK_WINDOWING_QUARTZ */

// On X11 the display profile can be embedded using the 'xicc' command.
    void uf_get_display_profile(GtkWidget *widget,
                                guint8 **buffer, gint *buffer_size)
    {
        g_free(*buffer);
        *buffer = NULL;
        *buffer_size = 0;
#if defined GDK_WINDOWING_X11
        GdkScreen *screen = gtk_widget_get_screen(widget);
        if (screen == NULL)
            screen = gdk_screen_get_default();
        int monitor = gdk_screen_get_monitor_at_window(screen, widget->window);
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
        if (screen == NULL)
            screen = gdk_screen_get_default();
        int monitor = gdk_screen_get_monitor_at_window(screen, widget->window);

        CMProfileRef prof = NULL;
        CMGetProfileByAVID(monitor, &prof);
        if (prof == NULL)
            return;

        ProfileTransfer transfer = { NULL, 0 };
        Boolean foo;
        CMFlattenProfile(prof, 0, _uf_lcms_flatten_profile, &transfer, &foo);
        CMCloseProfile(prof);

        *buffer = transfer.data;
        *buffer_size = transfer.len;

#elif defined G_OS_WIN32
        (void)widget;
        HDC hdc = GetDC(NULL);
        if (hdc == NULL)
            return;

        DWORD len = 0;
        GetICMProfile(hdc, &len, NULL);
        gchar *path = g_new(gchar, len);

        if (GetICMProfile(hdc, &len, path)) {
            gsize size;
            g_file_get_contents(path, (gchar**)buffer, &size, NULL);
            *buffer_size = size;
        }
        g_free(path);
        ReleaseDC(NULL, hdc);
#endif
    }

// Translate text message from GtkImageView:
    const char *uf_gtkimageview_text = N_("Open the navigator window");

    /*
     * The following functions create GtkWidgets for UFObjects.
     * These widgets are already created with callbacks, so that changes
     * in the widget value are applied to the UFObjects and vice-versa.
     */

    class _UFWidgetData
    {
    public:
        GObject **gobject;
        GtkButton *button;
        explicit _UFWidgetData(int size = 1) {
            if (size != 0)
                gobject = g_new0(GObject *, size);
            else
                gobject = NULL;
            button = NULL;
        }
        ~_UFWidgetData() {
            g_free(gobject);
        }
        GtkAdjustment *adjustment(int index) {
            return GTK_ADJUSTMENT(gobject[index]);
        }
    };

    typedef std::list<UFObject *> _UFObjectList;

    void _ufobject_reset_button_state(UFObject *object)
    {
        _UFWidgetData *data = static_cast<_UFWidgetData *>(object->UserData());
        if (data->button == NULL)
            return;
        _UFObjectList *list = static_cast<_UFObjectList *>(
                                  g_object_get_data(G_OBJECT(data->button), "UFObjectList"));
        if (list == NULL)
            return;
        bool isDefault = true;
        for (_UFObjectList::iterator iter = list->begin();
                iter != list->end(); iter++) {
            isDefault &= (*iter)->IsDefault();
        }
        gtk_widget_set_sensitive(GTK_WIDGET(data->button), !isDefault);
    }

    static void _ufnumber_adjustment_changed(GtkAdjustment *adj, UFObject *object)
    {
        UFNumber &num = *object;
        num.Set(gtk_adjustment_get_value(adj));
    }

    static void _ufnumber_object_event(UFObject *object, UFEventType type)
    {
        _UFWidgetData *data = static_cast<_UFWidgetData *>(object->UserData());
        if (type == uf_destroyed) {
            delete data;
            return;
        }
        UFNumber &num = *object;
        if (data->adjustment(0) != NULL)
            gtk_adjustment_set_value(data->adjustment(0), num.DoubleValue());
        _ufobject_reset_button_state(object);
    }

    static void _ufnumber_array_adjustment_changed(GtkAdjustment *adj,
            UFObject *object)
    {
        _UFWidgetData *data = static_cast<_UFWidgetData *>(object->UserData());
        UFNumberArray &array = *object;
        for (int i = 0; i < array.Size(); i++)
            if (data->adjustment(i) == adj)
                array.Set(i, gtk_adjustment_get_value(data->adjustment(i)));
    }

    static void _ufnumber_array_object_event(UFObject *object, UFEventType type)
    {
        _UFWidgetData *data = static_cast<_UFWidgetData *>(object->UserData());
        if (type == uf_destroyed) {
            delete data;
            return;
        }
        UFNumberArray &array = *object;
        for (int i = 0; i < array.Size(); i++)
            gtk_adjustment_set_value(data->adjustment(i), array.DoubleValue(i));
        _ufobject_reset_button_state(object);
    }

    static void _ufstring_object_event(UFObject *object, UFEventType type)
    {
        _UFWidgetData *data = static_cast<_UFWidgetData *>(object->UserData());
        if (type == uf_destroyed) {
            delete data;
            return;
        }
        UFString &string = *object;
        gtk_entry_set_text(GTK_ENTRY(data->gobject[0]), string.StringValue());
    }

    static void _ufarray_object_event(UFObject *object, UFEventType type)
    {
        _UFWidgetData *data = static_cast<_UFWidgetData *>(object->UserData());
        if (type == uf_destroyed) {
            delete data;
            return;
        }
        if (type != uf_value_changed)
            return;
        GtkComboBox *combo = GTK_COMBO_BOX(data->gobject[0]);
        UFArray &array = *object;
        if (array.Index() >= 0) {
            gtk_combo_box_set_active(combo, array.Index());
            return;
        }
        if (GTK_IS_COMBO_BOX_ENTRY(combo)) {
            GtkWidget *entry = gtk_bin_get_child(GTK_BIN(combo));
            gtk_entry_set_text(GTK_ENTRY(entry), array.StringValue());
        } else { // GTK_IS_COMBO_BOX()
            // If value not found activate first entry
            g_warning("_ufarray_object_event() value not found");
            gtk_combo_box_set_active(combo, 0);
        }
    }

    /* Return the widget-data for the object.
     * Create the widget-data, if it was not set already.
     */
    static void _ufobject_adjustment_destroyed(GtkAdjustment *,
            void **adjustmentPointer)
    {
        *adjustmentPointer = NULL;
    }

    static _UFWidgetData &_ufnumber_widget_data(UFNumber &number)
    {
        if (number.UserData() == NULL) {
            _UFWidgetData *datap = new _UFWidgetData;
            number.SetUserData(datap);
        }
        _UFWidgetData &data = *static_cast<_UFWidgetData *>(number.UserData());
        if (data.gobject[0] != NULL)
            return data;
        data.gobject[0] = G_OBJECT(gtk_adjustment_new(number.DoubleValue(),
                                   number.Minimum(), number.Maximum(),
                                   number.Step(), number.Jump(), 0.0));
        g_signal_connect(G_OBJECT(data.adjustment(0)), "value-changed",
                         G_CALLBACK(_ufnumber_adjustment_changed), &number);
        g_signal_connect(G_OBJECT(data.adjustment(0)), "destroy",
                         G_CALLBACK(_ufobject_adjustment_destroyed), &data.gobject[0]);
        number.SetEventHandle(_ufnumber_object_event);
        return data;
    }

    static _UFWidgetData &_ufnumber_array_widget_data(UFNumberArray &array)
    {
        if (array.UserData() == NULL) {
            _UFWidgetData *datap = new _UFWidgetData(array.Size());
            array.SetUserData(datap);
        }
        _UFWidgetData &data = *static_cast<_UFWidgetData *>(array.UserData());
        for (int i = 0; i < array.Size(); i++) {
            if (data.gobject[i] != NULL)
                continue;
            data.gobject[i] = G_OBJECT(gtk_adjustment_new(array.DoubleValue(i),
                                       array.Minimum(), array.Maximum(),
                                       array.Step(), array.Jump(), 0.0));
            g_signal_connect(G_OBJECT(data.adjustment(i)), "value-changed",
                             G_CALLBACK(_ufnumber_array_adjustment_changed), &array);
            g_signal_connect(G_OBJECT(data.adjustment(0)), "destroy",
                             G_CALLBACK(_ufobject_adjustment_destroyed), &data.gobject[i]);
        }
        array.SetEventHandle(_ufnumber_array_object_event);
        return data;
    }

    static _UFWidgetData &_ufstring_widget_data(UFString &string)
    {
        if (string.UserData() != NULL)
            return *static_cast<_UFWidgetData *>(string.UserData());
        _UFWidgetData &data = *(new _UFWidgetData);
        string.SetUserData(&data);
        data.gobject[0] = NULL;
        string.SetEventHandle(_ufstring_object_event);
        return data;
    }

    static _UFWidgetData &_ufarray_widget_data(UFArray &array)
    {
        if (array.UserData() != NULL)
            return *static_cast<_UFWidgetData *>(array.UserData());
        _UFWidgetData &data = *(new _UFWidgetData);
        array.SetUserData(&data);
        data.gobject[0] = NULL;
        array.SetEventHandle(_ufarray_object_event);
        return data;
    }

    GtkWidget *ufnumber_hscale_new(UFObject *object)
    {
        UFNumber &number = *object;
        _UFWidgetData &data = _ufnumber_widget_data(number);
        GtkWidget *scale = gtk_hscale_new(data.adjustment(0));
        gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
        return scale;
    }

    GtkWidget *ufnumber_spin_button_new(UFObject *object)
    {
        UFNumber &number = *object;
        _UFWidgetData &data = _ufnumber_widget_data(number);
        GtkWidget *spin = gtk_spin_button_new(data.adjustment(0), number.Step(),
                                              number.AccuracyDigits());
        gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(spin), FALSE);
        gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin),
                                          GTK_UPDATE_IF_VALID);
        return spin;
    }

    GtkWidget *ufnumber_array_hscale_new(UFObject *object, int index)
    {
        UFNumberArray &array = *object;
        _UFWidgetData &data = _ufnumber_array_widget_data(array);
        GtkWidget *scale = gtk_hscale_new(data.adjustment(index));
        gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
        return scale;
    }

    GtkWidget *ufnumber_array_spin_button_new(UFObject *object, int index)
    {
        UFNumberArray &array = *object;
        _UFWidgetData &data = _ufnumber_array_widget_data(array);
        GtkWidget *spin = gtk_spin_button_new(data.adjustment(index),
                                              array.Step(), array.AccuracyDigits());
        gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(spin), FALSE);
        gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin),
                                          GTK_UPDATE_IF_VALID);
        return spin;
    }

    static void _ufobject_reset_clicked(GtkWidget * /*widget*/, _UFObjectList *list)
    {
        for (_UFObjectList::iterator iter = list->begin();
                iter != list->end(); iter++) {
            (*iter)->Reset();
        }
    }

    static void _ufobject_reset_destroy(GtkWidget * /*widget*/, _UFObjectList *list)
    {
        for (_UFObjectList::iterator iter = list->begin();
                iter != list->end(); iter++) {
            _UFWidgetData *data = static_cast<_UFWidgetData *>((*iter)->UserData());
            data->button = NULL;
        }
        delete list;
    }

    GtkWidget *ufobject_reset_button_new(const char *tip)
    {
        GtkWidget *button = gtk_button_new();
        gtk_container_add(GTK_CONTAINER(button),
                          gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_BUTTON));
        if (tip != NULL)
            gtk_widget_set_tooltip_text(button, tip);
        _UFObjectList *objectList = new _UFObjectList;
        g_object_set_data(G_OBJECT(button), "UFObjectList", objectList);

        g_signal_connect(G_OBJECT(button), "clicked",
                         G_CALLBACK(_ufobject_reset_clicked), objectList);
        g_signal_connect(G_OBJECT(button), "destroy",
                         G_CALLBACK(_ufobject_reset_destroy), objectList);
        return button;
    }

    void ufobject_reset_button_add(GtkWidget *button, UFObject *object)
    {
        assert(object->UserData() != NULL);
        _UFWidgetData *data = static_cast<_UFWidgetData *>(object->UserData());
        data->button = GTK_BUTTON(button);
        _UFObjectList *objectList = static_cast<_UFObjectList *>(
                                        g_object_get_data(G_OBJECT(button), "UFObjectList"));
        assert(objectList != NULL);
        objectList->push_back(object);
        _ufobject_reset_button_state(object);
    }

    static void _ufstring_entry_changed(GtkWidget *entry, UFObject *object)
    {
        UFString &string = *object;
        string.Set(gtk_entry_get_text(GTK_ENTRY(entry)));
        _ufobject_reset_button_state(object);
    }

// Create a new GtkEntry with small width.
// The widget must be added with GTK_EXPAND|GTK_FILL.
    GtkWidget *ufstring_entry_new(UFObject *object)
    {
        GtkWidget *entry = gtk_entry_new();
        gtk_widget_set_size_request(entry, 50, -1);
        g_signal_connect_after(G_OBJECT(entry), "changed",
                               G_CALLBACK(_ufstring_entry_changed), object);
        UFString &string = *object;
        _UFWidgetData &data = _ufstring_widget_data(string);
        data.gobject[0] = G_OBJECT(entry);
        _ufstring_object_event(object, uf_value_changed);
        return entry;
    }

    static void _ufarray_combo_changed(GtkWidget *combo, UFObject *object)
    {
        UFArray &array = *object;
        int i = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
        array.SetIndex(i);
        _ufobject_reset_button_state(object);
    }

    static bool _ufarray_entry_changed(GtkWidget *entry, GdkEventFocus *event,
                                       UFObject *object)
    {
        if (event->in)
            return false;
        UFArray &array = *object;
        array.Set(gtk_entry_get_text(GTK_ENTRY(entry)));
        _ufobject_reset_button_state(object);
        return false;
    }

    GtkWidget *_ufarray_combo_box_new(UFObject *object, GtkWidget *combo)
    {
        UFArray &array = *object;
        _UFWidgetData &data = _ufarray_widget_data(array);
        gtk_widget_set_size_request(combo, 50, -1);
        data.gobject[0] = G_OBJECT(combo);
        UFGroupList list = array.List();
        for (UFGroupList::iterator iter = list.begin();
                iter != list.end(); iter++) {
            gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
                                      _((*iter)->StringValue()));
        }
        _ufarray_object_event(object, uf_value_changed);
        return combo;
    }

// Create a new ComboBox with small width.
// The widget must be added with GTK_EXPAND|GTK_FILL.
    GtkWidget *ufarray_combo_box_new(UFObject *object)
    {
        GtkWidget *combo = gtk_combo_box_new_text();
        g_signal_connect_after(G_OBJECT(combo), "changed",
                               G_CALLBACK(_ufarray_combo_changed), object);
        return _ufarray_combo_box_new(object, combo);
    }

// Create a new ComboBoxEntry with small width.
// The widget must be added with GTK_EXPAND|GTK_FILL.
    GtkWidget *ufarray_combo_box_entry_new(UFObject *object)
    {
        GtkWidget *combo = gtk_combo_box_entry_new_text();
        g_signal_connect_after(G_OBJECT(combo), "changed",
                               G_CALLBACK(_ufarray_combo_changed), object);
        GtkWidget *entry = gtk_bin_get_child(GTK_BIN(combo));
        g_signal_connect_after(G_OBJECT(entry), "focus-out-event",
                               G_CALLBACK(_ufarray_entry_changed), object);
        return _ufarray_combo_box_new(object, combo);
    }

} // extern "C"
