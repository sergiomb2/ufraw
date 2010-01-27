/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * uf_gtk.cc - gtk compatibility layer
 * Copyright 2004-2010 by Udi Fuchs
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
    if ( list!=NULL ) {
	int i;
	for (i=0; list!=NULL; i++, list=g_list_next(list)) {
	    if ( value==GPOINTER_TO_INT(list->data) ) {
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
    if ( list!=NULL ) {
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
    if ( handler_id!=0 )
	g_signal_handler_disconnect(G_OBJECT(combo), handler_id);

    uf_combo_box_set_active(combo, *valuep);
    if ( gtk_combo_box_get_active(combo)==0 ) {
	// If value was not found in uf-combo-list, set it to first entry
	GList *list = static_cast<GList *>(
		g_object_get_data(G_OBJECT(combo), "uf-combo-list"));
	if ( list!=NULL )
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
    if ( list!=NULL ) {
	int i;
	for (i=0; list!=NULL; i++, list=g_list_next(list)) {
	    if ( value==GPOINTER_TO_INT(list->data) ) {
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

    switch (command)
    {
    case openWriteSpool:
	g_return_val_if_fail(transfer->data==NULL && transfer->len==0, -1);
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

/*
 * The following functions create GtkWidgets for UFObjects.
 * These widgets are already created with callbacks, so that changes
 * in the widget value are applied to the UFObjects and vice-versa.
 */

class _UFWidgetData {
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

void _ufobject_reset_button_state(UFObject *object) {
    _UFWidgetData *data = static_cast<_UFWidgetData *>(object->UserData);
    if (data->button == NULL)
	return;
    _UFObjectList *list = static_cast<_UFObjectList *>(
	    g_object_get_data(G_OBJECT(data->button), "UFObjectList"));
    bool isDefault = true;
    for (_UFObjectList::iterator iter = list->begin();
	    iter != list->end(); iter++) {
	isDefault &= (*iter)->IsDefault();
    }
    gtk_widget_set_sensitive(GTK_WIDGET(data->button), !isDefault);
}

static void _ufnumber_adjustment_changed(GtkAdjustment *adj, UFObject *object) {
    UFNumber &num = *object;
    num.Set(gtk_adjustment_get_value(adj));
}

static void _ufnumber_object_event(UFObject *object, UFEventType type) {
    _UFWidgetData *data = static_cast<_UFWidgetData *>(object->UserData);
    if (type == uf_destroyed) {
	delete data;
	return;
    }
    UFNumber &num = *object;
    gtk_adjustment_set_value(data->adjustment(0), num.DoubleValue());
    _ufobject_reset_button_state(object);
}

static void _ufnumber_array_adjustment_changed(GtkAdjustment *adj,
	UFObject *object) {
    _UFWidgetData *data = static_cast<_UFWidgetData *>(object->UserData);
    UFNumberArray &array = *object;
    for (int i = 0; i < array.Size(); i++)
	if (data->adjustment(i) == adj)
	    array.Set(i, gtk_adjustment_get_value(data->adjustment(i)));
}

static void _ufnumber_array_object_event(UFObject *object, UFEventType type) {
    _UFWidgetData *data = static_cast<_UFWidgetData *>(object->UserData);
    if (type == uf_destroyed) {
	delete data;
	return;
    }
    UFNumberArray &array = *object;
    for (int i = 0; i < array.Size(); i++)
	gtk_adjustment_set_value(data->adjustment(i), array.DoubleValue(i));
    _ufobject_reset_button_state(object);
}

static void _ufstring_object_event(UFObject *object, UFEventType type) {
    _UFWidgetData *data = static_cast<_UFWidgetData *>(object->UserData);
    if (type == uf_destroyed) {
	delete data;
	return;
    }
    UFString &string = *object;
    GtkComboBox *combo = GTK_COMBO_BOX(data->gobject[0]);
    UFTokenList &list = string.GetTokens();
    int i = 0;
    for (UFTokenList::iterator iter = list.begin();
	    iter != list.end(); iter++, i++) {
	if (string.IsEqual(*iter)) {
	    gtk_combo_box_set_active(combo, i);
	    return;
	}
    }
    // If value not found activate first entry
    g_warning("_ufstring_object_event() value not found");
    gtk_combo_box_set_active(combo, 0);
}

/* Return the widget-data for the object.
 * Create the widget-data, if it was not set already.
 */
static _UFWidgetData &_ufnumber_widget_data(UFNumber &number) {
    if (number.UserData != NULL)
	return *static_cast<_UFWidgetData *>(number.UserData);
    _UFWidgetData &data = *(new _UFWidgetData);
    number.UserData = &data;
    data.gobject[0] = G_OBJECT(gtk_adjustment_new(number.DoubleValue(),
	    number.Minimum(), number.Maximum(),
	    number.Step(), number.Jump(), 0.0));
    g_signal_connect(G_OBJECT(data.adjustment(0)), "value-changed",
	    G_CALLBACK(_ufnumber_adjustment_changed), &number);
    number.SetEventHandle(_ufnumber_object_event);
    return data;
}

static _UFWidgetData &_ufnumber_array_widget_data(UFNumberArray &array) {
    if (array.UserData != NULL)
	return *static_cast<_UFWidgetData *>(array.UserData);
    _UFWidgetData &data = *(new _UFWidgetData(array.Size()));
    array.UserData = &data;
    for (int i = 0; i < array.Size(); i++) {
	data.gobject[i] = G_OBJECT(gtk_adjustment_new(array.DoubleValue(i),
		array.Minimum(), array.Maximum(),
		array.Step(), array.Jump(), 0.0));
	g_signal_connect(G_OBJECT(data.adjustment(i)), "value-changed",
	    G_CALLBACK(_ufnumber_array_adjustment_changed), &array);
    }
    array.SetEventHandle(_ufnumber_array_object_event);
    return data;
}

static _UFWidgetData &_ufstring_widget_data(UFString &string) {
    if (string.UserData != NULL)
	return *static_cast<_UFWidgetData *>(string.UserData);
    _UFWidgetData &data = *(new _UFWidgetData);
    string.UserData = &data;
    data.gobject[0] = NULL;
    string.SetEventHandle(_ufstring_object_event);
    return data;
}

GtkWidget *ufnumber_hscale_new(UFObject *object) {
    UFNumber &number = *object;
    _UFWidgetData &data = _ufnumber_widget_data(number);
    GtkWidget *scale = gtk_hscale_new(data.adjustment(0));
    gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
    return scale;
}

GtkWidget *ufnumber_spin_button_new(UFObject *object) {
    UFNumber &number = *object;
    _UFWidgetData &data = _ufnumber_widget_data(number);
    GtkWidget *spin = gtk_spin_button_new(data.adjustment(0), number.Step(),
	    number.AccuracyDigits());
    gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(spin), FALSE);
    gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin),
	    GTK_UPDATE_IF_VALID);
    return spin;
}

GtkWidget *ufnumber_array_hscale_new(UFObject *object, int index) {
    UFNumberArray &array = *object;
    _UFWidgetData &data = _ufnumber_array_widget_data(array);
    GtkWidget *scale = gtk_hscale_new(data.adjustment(index));
    gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
    return scale;
}

GtkWidget *ufnumber_array_spin_button_new(UFObject *object, int index) {
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
    delete list;
}

GtkWidget *ufobject_reset_button_new(const char *tip) {
    GtkWidget *button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(button), 
	    gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_BUTTON));
    if (tip != NULL)
	uf_widget_set_tooltip(button, tip);
    _UFObjectList *objectList = new _UFObjectList;
    g_object_set_data(G_OBJECT(button), "UFObjectList", objectList);
    
    g_signal_connect(G_OBJECT(button), "clicked",
	    G_CALLBACK(_ufobject_reset_clicked), objectList);
    g_signal_connect(G_OBJECT(button), "destroy",
	    G_CALLBACK(_ufobject_reset_destroy), objectList);
    return button;
}

void ufobject_reset_button_add(GtkWidget *button, UFObject *object) {
    assert(object->UserData != NULL);
    _UFWidgetData *data = static_cast<_UFWidgetData *>(object->UserData);
    data->button = GTK_BUTTON(button);
    _UFObjectList *objectList = static_cast<_UFObjectList *>(
	    g_object_get_data(G_OBJECT(button), "UFObjectList"));
    assert(objectList != NULL);
    objectList->push_back(object);
    _ufobject_reset_button_state(object);
}

static void _ufstring_combo_changed(GtkWidget *combo, UFObject *object) {
    UFString &string = *object;
    UFTokenList &list = string.GetTokens();
    int i = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    UFTokenList::iterator iter = list.begin();
    std::advance(iter, i);
    string.Set(*iter);
    _ufobject_reset_button_state(object);
}

// Create a new ComboBox text with small width.
// The widget must be added with GTK_EXPAND|GTK_FILL.
GtkWidget *ufstring_combo_box_new(UFObject *object) {
    UFString &string = *object;
    _UFWidgetData &data = _ufstring_widget_data(string);
    GtkWidget *combo = gtk_combo_box_new_text();
    gtk_widget_set_size_request(combo, 50, -1);
    data.gobject[0] = G_OBJECT(combo);
    g_signal_connect_after(G_OBJECT(combo), "changed",
	G_CALLBACK(_ufstring_combo_changed), object);
    UFTokenList &list = string.GetTokens();
    for (UFTokenList::iterator iter = list.begin();
	    iter != list.end(); iter++) {
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), _(*iter));
    }
    _ufstring_object_event(object, uf_value_changed);
    return combo;
}

} // extern "C"
