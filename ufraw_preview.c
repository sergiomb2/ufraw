/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw_preview.c - GUI for controlling all the image manipulations
 * Copyright 2004-2009 by Udi Fuchs
 *
 * based on the GIMP plug-in by Pawel T. Jochym jochym at ifj edu pl,
 *
 * based on the GIMP plug-in by Dave Coffin
 * http://www.cybercom.net/~dcoffin/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include "uf_gtk.h"
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include "ufraw.h"
#include "ufraw_ui.h"
#include "curveeditor_widget.h"
//#undef HAVE_GTKIMAGEVIEW
#ifdef HAVE_GTKIMAGEVIEW
#include <gtkimageview/gtkimagescrollwin.h>
#include <gtkimageview/gtkimageview.h>
#endif
#if GTK_CHECK_VERSION(2,6,0)
void ufraw_chooser_toggle(GtkToggleButton *button, GtkFileChooser *filechooser);
#endif

static void adjustment_update(GtkAdjustment *adj, double *valuep);
static void button_update(GtkWidget *button, gpointer user_data);

extern GtkFileChooser *ufraw_raw_chooser(conf_data *conf,
					 const char *defPath,
					 const gchar *label,
					 GtkWindow *toplevel,
					 const gchar *cancel,
					 gboolean multiple);

/* Set to huge number so that the preview size is set by the screen size */
static const int def_preview_width = 9000;
static const int def_preview_height = 9000;
static const int his_max_height = 256;

enum { pixel_format, percent_format };
enum { without_zone, with_zone };

static char *expanderText[] = { N_("Raw histogram with conversion curves"),
    N_("Live histogram"), NULL };

static const GdkCursorType Cursors[cursor_num] = {
    GDK_HAND2, GDK_CROSSHAIR,
    GDK_LEFT_SIDE, GDK_RIGHT_SIDE, GDK_TOP_SIDE, GDK_BOTTOM_SIDE,
    GDK_TOP_LEFT_CORNER, GDK_TOP_RIGHT_CORNER,
    GDK_BOTTOM_LEFT_CORNER, GDK_BOTTOM_RIGHT_CORNER,
    GDK_FLEUR };

static const char *grayscaleModeNames[5] = {
    N_("None"),
    N_("Lightness"),
    N_("Luminance"),
    N_("Value"),
    N_("Channel Mixer")
};

preview_data *get_preview_data(void *object)
{
    GtkWidget *widget;
    if (GTK_IS_ADJUSTMENT(object)) {
	widget = g_object_get_data(object, "Parent-Widget");
    } else if (GTK_IS_MENU(object)) {
	widget = g_object_get_data(G_OBJECT(object), "Parent-Widget");
    } else if (GTK_IS_MENU_ITEM(object)) {
	GtkWidget *menu = gtk_widget_get_ancestor(object, GTK_TYPE_MENU);
	widget = g_object_get_data(G_OBJECT(menu), "Parent-Widget");
    } else {
	widget = object;
    }
    GtkWidget *parentWindow = gtk_widget_get_toplevel(widget);
    preview_data *data =
	    g_object_get_data(G_OBJECT(parentWindow), "Preview-Data");
    return data;
}

void ufraw_focus(void *window, gboolean focus)
{
    if (focus) {
	GtkWindow *parentWindow = 
		(void *)ufraw_message(UFRAW_SET_PARENT, (char *)window);
	g_object_set_data(G_OBJECT(window), "WindowParent", parentWindow);
	if (parentWindow!=NULL)
	    gtk_window_set_accept_focus(GTK_WINDOW(parentWindow), FALSE);
    } else {
	GtkWindow *parentWindow = g_object_get_data(G_OBJECT(window),
		"WindowParent");
	ufraw_message(UFRAW_SET_PARENT, (char *)parentWindow);
	if (parentWindow!=NULL)
	    gtk_window_set_accept_focus(GTK_WINDOW(parentWindow), TRUE);
    }
}

void ufraw_messenger(char *message,  void *parentWindow)
{
    GtkDialog *dialog;

    if (parentWindow==NULL) {
	ufraw_batch_messenger(message);
    } else {
	dialog = GTK_DIALOG(gtk_message_dialog_new(GTK_WINDOW(parentWindow),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "%s", message));
	gtk_dialog_run(dialog);
	gtk_widget_destroy(GTK_WIDGET(dialog));
    }
}

static void load_curve(GtkWidget *widget, long curveType)
{
    preview_data *data = get_preview_data(widget);
    GtkFileChooser *fileChooser;
    GtkFileFilter *filter;
    GSList *list, *saveList;
    CurveData c;
    char *cp;

    if (data->FreezeDialog) return;
    if ( (curveType==base_curve && CFG->BaseCurveCount>=max_curves) ||
	 (curveType==luminosity_curve && CFG->curveCount>=max_curves) ) {
	ufraw_message(UFRAW_ERROR, _("No more room for new curves."));
	return;
    }
    fileChooser = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new(_("Load curve"),
	    GTK_WINDOW(gtk_widget_get_toplevel(widget)),
	    GTK_FILE_CHOOSER_ACTION_OPEN,
	    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL));
    ufraw_focus(fileChooser, TRUE);
    gtk_file_chooser_set_select_multiple(fileChooser, TRUE);
#if GTK_CHECK_VERSION(2,6,0)
    gtk_file_chooser_set_show_hidden(fileChooser, FALSE);
    GtkWidget *button = gtk_check_button_new_with_label(_("Show hidden files"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    g_signal_connect(G_OBJECT(button), "toggled",
	    G_CALLBACK(ufraw_chooser_toggle), fileChooser);
    gtk_file_chooser_set_extra_widget(fileChooser, button);
#endif
    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, _("All curve formats"));
    gtk_file_filter_add_pattern(filter, "*.ntc");
    gtk_file_filter_add_pattern(filter, "*.NTC");
    gtk_file_filter_add_pattern(filter, "*.ncv");
    gtk_file_filter_add_pattern(filter, "*.NCV");
    gtk_file_filter_add_pattern(filter, "*.curve");
    gtk_file_filter_add_pattern(filter, "*.CURVE");
    gtk_file_chooser_add_filter(fileChooser, filter);

    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, _("UFRaw curve format"));
    gtk_file_filter_add_pattern(filter, "*.curve");
    gtk_file_filter_add_pattern(filter, "*.CURVE");
    gtk_file_chooser_add_filter(fileChooser, filter);

    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, _("Nikon curve format"));
    gtk_file_filter_add_pattern(filter, "*.ntc");
    gtk_file_filter_add_pattern(filter, "*.NTC");
    gtk_file_filter_add_pattern(filter, "*.ncv");
    gtk_file_filter_add_pattern(filter, "*.NCV");
    gtk_file_chooser_add_filter(fileChooser, filter);

    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, _("All files"));
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(fileChooser, filter);

    if (strlen(CFG->curvePath)>0)
	gtk_file_chooser_set_current_folder(fileChooser, CFG->curvePath);
    if (gtk_dialog_run(GTK_DIALOG(fileChooser))==GTK_RESPONSE_ACCEPT) {
	for ( list=saveList=gtk_file_chooser_get_filenames(fileChooser);
	      list!=NULL;
	      list=g_slist_next(list) ) {
	    c = conf_default.curve[0];
	    if (curve_load(&c, list->data)!=UFRAW_SUCCESS) continue;
	    if (curveType==base_curve) {
		if ( CFG->BaseCurveCount>= max_curves ) break;
		gtk_combo_box_append_text(data->BaseCurveCombo, c.name);
		CFG->BaseCurve[CFG->BaseCurveCount] = c;
		CFG->BaseCurveIndex = CFG->BaseCurveCount;
		CFG->BaseCurveCount++;
		/* Add curve to .ufrawrc but don't make it default */
		RC->BaseCurve[RC->BaseCurveCount++] = c;
		RC->BaseCurveCount++;
		if (CFG_cameraCurve)
		    gtk_combo_box_set_active(data->BaseCurveCombo,
			    CFG->BaseCurveIndex);
		else
		    gtk_combo_box_set_active(data->BaseCurveCombo,
			    CFG->BaseCurveIndex-2);
	    } else { /* curveType==luminosity_curve */
		if ( CFG->curveCount >= max_curves ) break;
		gtk_combo_box_append_text(data->CurveCombo, c.name);
		CFG->curve[CFG->curveCount] = c;
		CFG->curveIndex = CFG->curveCount;
		CFG->curveCount++;
		/* Add curve to .ufrawrc but don't make it default */
		RC->curve[RC->curveCount++] = c;
		RC->curveCount++;
		gtk_combo_box_set_active(data->CurveCombo, CFG->curveIndex);
	    }
	    cp = g_path_get_dirname(list->data);
	    g_strlcpy(CFG->curvePath, cp, max_path);
	    g_strlcpy(RC->curvePath, cp, max_path);
	    conf_save(RC, NULL, NULL);
	    g_free(cp);
	    g_free(list->data);
	}
	if (list!=NULL)
	ufraw_message(UFRAW_ERROR, _("No more room for new curves."));
	g_slist_free(saveList);
    }
    ufraw_focus(fileChooser, FALSE);
    gtk_widget_destroy(GTK_WIDGET(fileChooser));
}

static void save_curve(GtkWidget *widget, long curveType)
{
    preview_data *data = get_preview_data(widget);
    GtkFileChooser *fileChooser;
    GtkFileFilter *filter;
    char defFilename[max_name];
    char *filename, *cp;

    if (data->FreezeDialog) return;

    fileChooser = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new(_("Save curve"),
	    GTK_WINDOW(gtk_widget_get_toplevel(widget)),
	    GTK_FILE_CHOOSER_ACTION_SAVE,
	    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	    GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL));
    ufraw_focus(fileChooser, TRUE);
#if GTK_CHECK_VERSION(2,6,0)
    gtk_file_chooser_set_show_hidden(fileChooser, FALSE);
    GtkWidget *button = gtk_check_button_new_with_label(_("Show hidden files"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    g_signal_connect(G_OBJECT(button), "toggled",
	    G_CALLBACK(ufraw_chooser_toggle), fileChooser);
    gtk_file_chooser_set_extra_widget(fileChooser, button);
#endif
    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, _("All curve formats"));
    gtk_file_filter_add_pattern(filter, "*.ntc");
    gtk_file_filter_add_pattern(filter, "*.NTC");
    gtk_file_filter_add_pattern(filter, "*.ncv");
    gtk_file_filter_add_pattern(filter, "*.NCV");
    gtk_file_filter_add_pattern(filter, "*.curve");
    gtk_file_filter_add_pattern(filter, "*.CURVE");
    gtk_file_chooser_add_filter(fileChooser, filter);

    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, _("UFRaw curve format"));
    gtk_file_filter_add_pattern(filter, "*.curve");
    gtk_file_filter_add_pattern(filter, "*.CURVE");
    gtk_file_chooser_add_filter(fileChooser, filter);

    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, _("Nikon curve format"));
    gtk_file_filter_add_pattern(filter, "*.ntc");
    gtk_file_filter_add_pattern(filter, "*.NTC");
    gtk_file_filter_add_pattern(filter, "*.ncv");
    gtk_file_filter_add_pattern(filter, "*.NCV");
    gtk_file_chooser_add_filter(fileChooser, filter);

    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, _("All files"));
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(fileChooser, filter);

    if (strlen(CFG->curvePath)>0)
	gtk_file_chooser_set_current_folder(fileChooser, CFG->curvePath);
    if (curveType==base_curve)
	g_snprintf(defFilename, max_name, "%s.curve",
		CFG->BaseCurve[CFG->BaseCurveIndex].name);
    else
	g_snprintf(defFilename, max_name, "%s.curve",
		CFG->curve[CFG->curveIndex].name);
    gtk_file_chooser_set_current_name(fileChooser, defFilename);
    if (gtk_dialog_run(GTK_DIALOG(fileChooser))==GTK_RESPONSE_ACCEPT) {
	filename = gtk_file_chooser_get_filename(fileChooser);
	if (curveType==base_curve)
	    curve_save(&CFG->BaseCurve[CFG->BaseCurveIndex], filename);
	else
	    curve_save(&CFG->curve[CFG->curveIndex], filename);
	cp = g_path_get_dirname(filename);
	g_strlcpy(CFG->curvePath, cp, max_path);
	g_free(cp);
	g_free(filename);
    }
    ufraw_focus(fileChooser, FALSE);
    gtk_widget_destroy(GTK_WIDGET(fileChooser));
}

static void load_profile(GtkWidget *widget, long type)
{
    preview_data *data = get_preview_data(widget);
    GtkFileChooser *fileChooser;
    GSList *list, *saveList;
    GtkFileFilter *filter;
    char *filename, *cp;
    profile_data p;

    if (data->FreezeDialog) return;
    if (CFG->profileCount[type]==max_profiles) {
	ufraw_message(UFRAW_ERROR, _("No more room for new profiles."));
	return;
    }
    fileChooser = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new(
	    _("Load color profile"),
	    GTK_WINDOW(gtk_widget_get_toplevel(widget)),
	    GTK_FILE_CHOOSER_ACTION_OPEN,
	    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL));
    ufraw_focus(fileChooser, TRUE);
    gtk_file_chooser_set_select_multiple(fileChooser, TRUE);
#if GTK_CHECK_VERSION(2,6,0)
    gtk_file_chooser_set_show_hidden(fileChooser, FALSE);
    GtkWidget *button = gtk_check_button_new_with_label(_("Show hidden files"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    g_signal_connect(G_OBJECT(button), "toggled",
	    G_CALLBACK(ufraw_chooser_toggle), fileChooser);
    gtk_file_chooser_set_extra_widget(fileChooser, button);
#endif
    if (strlen(CFG->profilePath)>0)
	gtk_file_chooser_set_current_folder(fileChooser, CFG->profilePath);
    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, _("Color Profiles"));
    gtk_file_filter_add_pattern(filter, "*.icm");
    gtk_file_filter_add_pattern(filter, "*.ICM");
    gtk_file_filter_add_pattern(filter, "*.icc");
    gtk_file_filter_add_pattern(filter, "*.ICC");
    gtk_file_chooser_add_filter(fileChooser, filter);
    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, _("All files"));
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(fileChooser, filter);
    if (gtk_dialog_run(GTK_DIALOG(fileChooser))==GTK_RESPONSE_ACCEPT) {
	for (list=saveList=gtk_file_chooser_get_filenames(fileChooser);
	    list!=NULL && CFG->profileCount[type]<max_profiles;
	    list=g_slist_next(list))
	{
	    filename = list->data;
	    p = conf_default.profile[type][conf_default.profileCount[type]];
	    g_strlcpy(p.file, filename, max_path);
	    /* Make sure we update product name */
	    Developer->updateTransform = TRUE;
	    developer_profile(Developer, type, &p);
	    if (Developer->profile[type]==NULL) {
		g_free(list->data);
		continue;
	    }
	    char *base = g_path_get_basename(filename);
	    char *name = uf_file_set_type(base, "");
	    char *utf8 = g_filename_display_name(name);
	    g_strlcpy(p.name, utf8, max_name);
	    g_free(utf8);
	    g_free(name);
	    g_free(base);
	    /* Set some defaults to the profile's curve */
	    p.gamma = profile_default_gamma(&p);

	    CFG->profile[type][CFG->profileCount[type]++] = p;
	    gtk_combo_box_append_text(data->ProfileCombo[type], p.name);
	    CFG->profileIndex[type] = CFG->profileCount[type]-1;
	    cp = g_path_get_dirname(list->data);
	    g_strlcpy(CFG->profilePath, cp, max_path);
	    /* Add profile to .ufrawrc but don't make it default */
	    RC->profile[type][RC->profileCount[type]++] = p;
	    g_strlcpy(RC->profilePath, cp, max_path);
	    conf_save(RC, NULL, NULL);
	    g_free(cp);
	    g_free(list->data);
	}
	gtk_combo_box_set_active(data->ProfileCombo[type],
		CFG->profileIndex[type]);
	if (list!=NULL)
	    ufraw_message(UFRAW_ERROR, _("No more room for new profiles."));
	g_slist_free(saveList);
    }
    ufraw_focus(fileChooser, FALSE);
    gtk_widget_destroy(GTK_WIDGET(fileChooser));
}

static colorLabels *color_labels_new(GtkTable *table, int x, int y,
	char *label, int format, int zonep)
{
    colorLabels *l;
    GtkWidget *lbl;
    int c, i = 0, numlabels;

    l = g_new(colorLabels, 1);
    l->format = format;
    l->zonep = zonep;

    numlabels = (zonep == with_zone ? 5 : 3);

    if (label!=NULL){
	lbl = gtk_label_new(label);
	gtk_misc_set_alignment(GTK_MISC(lbl), 1, 0.5);
	gtk_table_attach(table, lbl, x, x+1, y, y+1,
		GTK_SHRINK|GTK_FILL, GTK_FILL, 0, 0);
	i++;
    }
    /* 0-2 for RGB, 3 for value, 4 for zone */
    for (c=0; c<numlabels; c++, i++) {
	l->labels[c] = GTK_LABEL(gtk_label_new(NULL));
	GtkWidget *event_box = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(event_box), GTK_WIDGET(l->labels[c]));
	gtk_table_attach(table, event_box, x+i, x+i+1, y, y+1,
		GTK_EXPAND|GTK_FILL, 0, 0, 0);
	switch (c)
	{
	case 0: case 1: case 2:
	    uf_label_set_width_chars(l->labels[c], 3);
	    break;
	case 3:
	    uf_label_set_width_chars(l->labels[c], 5);
	    uf_widget_set_tooltip(event_box, _("Luminosity (Y value)"));
	    break;
	case 4:
	    uf_label_set_width_chars(l->labels[c], 4);
	    uf_widget_set_tooltip(event_box, _("Adams' zone"));
	}
    }
    return l;
}

/* Return numeric zone representation from 0-1 luminance value.
 * Unlike Adams, we use arabic for now. */
static double value2zone(double v)
{
    const double zoneV = 0.18;
    double z_rel_V;
    double zone;

    /* Convert to value relative to zone V value. */
    z_rel_V = v / zoneV;

    /* Convert to log-based value, with zone V 0. */
    zone = log(z_rel_V)/log(2.0);

    /* Move zone 5 to the proper place. */
    zone += 5.0;

    return zone;
}

static void color_labels_set(colorLabels *l, double data[])
{
    int c;
    char buf1[max_name], buf2[max_name];
    /* Value black, zone purple, for no good reason. */
    const char *colorName[] = {"red", "green", "blue", "black", "purple"};

    for (c=0; c<3; c++) {
	switch (l->format) {
	case pixel_format: g_snprintf(buf1, max_name, "%3.f", data[c]); break;
	case percent_format:
		if (data[c]<10.0)
		    g_snprintf(buf1, max_name, "%2.1f%%", data[c]);
		else
		    g_snprintf(buf1, max_name, "%2.0f%%", data[c]);
		break;
	default: g_snprintf(buf1, max_name, "ERR");
	}
	g_snprintf(buf2, max_name, "<span foreground='%s'>%s</span>",
		colorName[c], buf1);
	gtk_label_set_markup(l->labels[c], buf2);
    }
    /* If value/zone not requested, just return now. */
    if (l->zonep == without_zone)
	return;

    /* Value */
    c = 3;
    g_snprintf(buf2, max_name, "<span foreground='%s'>%0.3f</span>",
	    colorName[c], data[3]);
    gtk_label_set_markup(l->labels[c], buf2);

    /* Zone */
    c = 4;
    /* 2 decimal places may be excessive */
    g_snprintf(buf2, max_name, "<span foreground='%s'>%0.2f</span>",
	    colorName[c], data[4]);
    gtk_label_set_markup(l->labels[c], buf2);
}

static void redraw_navigation_image(preview_data *data)
{
#if defined(HAVE_GTKIMAGEVIEW) && GTK_IMAGE_VIEW_DAMAGE_PIXELS==0
    GtkWidget *scroll =
	gtk_widget_get_ancestor(data->PreviewWidget, GTK_TYPE_IMAGE_SCROLL_WIN);
    GtkImageNav *nav = GTK_IMAGE_NAV(GTK_IMAGE_SCROLL_WIN(scroll)->nav);
    if ( nav->pixbuf==NULL ) return;
    int navWidth = gdk_pixbuf_get_width(nav->pixbuf);
    int navHeight = gdk_pixbuf_get_height(nav->pixbuf);
    g_object_unref(nav->pixbuf);
    // GDK_INTERP_BILINEAR is too slow
    nav->pixbuf = gdk_pixbuf_scale_simple(data->PreviewPixbuf,
	    navWidth, navHeight, GDK_INTERP_NEAREST);
#else
    (void)data;
#endif
}

static void image_draw_area(preview_data *data, int x, int y,
			    int width, int height)
{
    GtkWidget *widget = data->PreviewWidget;
#ifdef HAVE_GTKIMAGEVIEW
#if GTK_IMAGE_VIEW_DAMAGE_PIXELS==1
    GtkImageView *view = GTK_IMAGE_VIEW(widget);
    GdkRectangle rect;
    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = height;
    gtk_image_view_damage_pixels(view, &rect);
#else
    GtkImageView *view = GTK_IMAGE_VIEW(widget);
    /* Find location of area in the view. */
    GdkRectangle viewRect;
    gtk_image_view_get_viewport(view, &viewRect);
    GdkPixbuf *pixbuf = gtk_image_view_get_pixbuf(view);
    int pixbufWidth = gdk_pixbuf_get_width(pixbuf);
    int pixbufHeight = gdk_pixbuf_get_height(pixbuf);
    int viewWidth = GTK_WIDGET(view)->allocation.width;
    int viewHeight = GTK_WIDGET(view)->allocation.height;
    if ( pixbufWidth>viewWidth ) {
	if (x<viewRect.x)
	    width += x - viewRect.x;
	x = MAX(x-viewRect.x, 0);
    } else {
	x += (viewWidth - pixbufWidth) / 2;
    }
    if ( pixbufHeight>viewHeight ) {
	if (y<viewRect.y)
	    height += y - viewRect.y;
	y = MAX(y-viewRect.y, 0);
    } else {
	y += (viewHeight - pixbufHeight) / 2;
    }
//preview_notify_dirty(preview_data *data)
    /* As a workaround we use the esoteric knowledge of GtkImageView's
     * internal logic. It will reuse pieces of the cached old pixmap
     * only if nothing changes in the view, otherwise it will discard
     * the cache. So we will change the transparency background for our
     * GtkImageView since anyway it is not seen through.
     */
    GTK_IMAGE_VIEW(data->PreviewWidget)->check_color1++;

    if ( height>0 && width>0 )
	gtk_widget_queue_draw_area(widget, x, y, width, height);
#endif
#else
    if ( height>0 && width>0 )
	gtk_widget_queue_draw_area(widget, x, y, width, height);
#endif
}

/* Modify the preview image to mark crop and spot areas.
 * Note that all coordinate intervals are semi-inclusive, e.g.
 * X1 <= pixels < X2 and Y1 <= pixels < Y2
 * This approach makes computing width/height just a matter of
 * substracting X1 from X2 or Y1 from Y2.
 */
static void preview_draw_img_area(preview_data *data, ufraw_image_data *img,
                                  int x, int y, int width, int height)
{
    int pixbufHeight = gdk_pixbuf_get_height(data->PreviewPixbuf);
    if ( y<0 || y>=pixbufHeight )
	g_error("preview_draw_area(): y:%d out of range 0 <= y < %d",
		y, pixbufHeight);
    if ( y+height>pixbufHeight )
	g_error("preview_draw_area(): y+height:%d out of range y+height <= %d",
		y+height, pixbufHeight);
    if ( height==0 ) return; // Nothing to do

    int pixbufWidth = gdk_pixbuf_get_width(data->PreviewPixbuf);
    if ( x<0 || x>=pixbufWidth )
	g_error("preview_draw_area(): x:%d out of range 0 <= x < %d", x, pixbufWidth);
    if ( x+width>pixbufWidth )
	g_error("preview_draw_area(): x+width:%d out of range x+width <= %d",
		x+width, pixbufWidth);
    if ( width==0 ) return; // Nothing to do

    int rowstride = gdk_pixbuf_get_rowstride(data->PreviewPixbuf);
    guint8 *pixies = gdk_pixbuf_get_pixels(data->PreviewPixbuf);
    guint8 *p8;
    gboolean blinkOver = CFG->overExp &&
	    ( !CFG->blinkOverUnder || (data->OverUnderTicker & 3) == 1 );
    gboolean blinkUnder = CFG->underExp &&
	    ( !CFG->blinkOverUnder || (data->OverUnderTicker & 3) == 3 );
		
    /* Scale crop image coordinates to pixbuf coordinates */
    float scale_x = ((float)pixbufWidth) / data->UF->rotatedWidth;
    float scale_y = ((float)pixbufHeight) / data->UF->rotatedHeight;

    int CropY1 = floor (CFG->CropY1 * scale_y);
    int CropY2 =  ceil (CFG->CropY2 * scale_y);
    int CropX1 = floor (CFG->CropX1 * scale_x);
    int CropX2 =  ceil (CFG->CropX2 * scale_x);
    int LineDeltaX = (CropX2 - CropX1) / (CFG->drawLines + 1);
    int LineDeltaY = (CropY2 - CropY1) / (CFG->drawLines + 1);

    /* Scale spot image coordinates to pixbuf coordinates */
    int SpotY1 = floor (MIN(data->SpotY1, data->SpotY2) * scale_y);
    int SpotY2 =  ceil (MAX(data->SpotY1, data->SpotY2) * scale_y);
    int SpotX1 = floor (MIN(data->SpotX1, data->SpotX2) * scale_x);
    int SpotX2 =  ceil (MAX(data->SpotX1, data->SpotX2) * scale_x);
    int xx, yy, c;
    for (yy=y; yy<y+height; yy++) {
	p8 = pixies+yy*rowstride+x*3;
	memcpy(p8, img->buffer + yy*img->rowstride + x*img->depth, width*img->depth);
	for (xx=x; xx<x+width; xx++, p8+=3) {
	    if ( data->SpotDraw &&
		( ((yy==SpotY1-1 || yy==SpotY2) && xx>=SpotX1-1 && xx<=SpotX2) ||
		  ((xx==SpotX1-1 || xx==SpotX2) && yy>=SpotY1-1 && yy<=SpotY2) ) ) {
		if ( ((xx+yy) & 7) >= 4 )
		    p8[0] = p8[1] = p8[2] = 0;
		else
		    p8[0] = p8[1] = p8[2] = 255;
		continue;
	    }
	    /* Draw white frame around crop area */
	    if ( ((yy==CropY1-1 || yy==CropY2) && xx>=CropX1-1 && xx<=CropX2) ||
		 ((xx==CropX1-1 || xx==CropX2) && yy>=CropY1-1 && yy<=CropY2) )
	    {
		p8[0] = p8[1] = p8[2] = 255;
	    }
	    /* Shade the cropped out area */
	    else if ( yy<CropY1 || yy>=CropY2 || xx<CropX1 || xx>=CropX2 ) {
		for (c=0; c<3; c++) p8[c] = p8[c]/4;
	    }
	    else if (data->RenderMode==render_default) {
		/* Shade out the alignment lines */
		if ( CFG->drawLines &&
		     yy > CropY1 + 1 && yy < CropY2 - 2 &&
		     xx > CropX1 + 1 && xx < CropX2 - 2 ) {
		    int dx = (xx - CropX1) % LineDeltaX;
		    int dy = (yy - CropY1) % LineDeltaY;
		    if (dx == 0 || dy == 0) {
			p8[0] /= 2;
			p8[1] /= 2;
			p8[2] /= 2;
		    }
		    else if (dx == 1 || dy == 1) {
			p8[0] = 255 - (255-p8[0])/2;
			p8[1] = 255 - (255-p8[1])/2;
			p8[2] = 255 - (255-p8[2])/2;
		    }
		}
		/* Blink the overexposed/underexposed spots */
		if ( blinkOver && (p8[0]==255 || p8[1]==255 || p8[2]==255) )
		    p8[0] = p8[1] = p8[2] = 0;
		else if ( blinkUnder && (p8[0]==0 || p8[1]==0 || p8[2]==0) )
		    p8[0] = p8[1] = p8[2] = 255;
	    } else if (data->RenderMode==render_overexposed) {
		for (c=0; c<3; c++) if (p8[c]!=255) p8[c] = 0;
	    } else if (data->RenderMode==render_underexposed) {
		for (c=0; c<3; c++) if (p8[c]!=0) p8[c] = 255;
	    }
	}
    }
#ifdef _OPENMP
#pragma omp critical
#endif
    /* Redraw the changed areas */
    image_draw_area(data, x, y, width, height);
}

static void preview_draw_area(preview_data *data, int x, int y,
                              int width, int height)
{
    preview_draw_img_area (
        data, ufraw_convert_image_area (data->UF, -1, ufraw_phases_num - 1),
        x, y, width, height);
}

static gboolean switch_highlights(gpointer ptr)
{
    preview_data *data = ptr;
    /* Only redraw the highlights in the default rendering mode. */
    if (data->RenderMode!=render_default || data->FreezeDialog)
	return TRUE;
    int pixbufWidth = gdk_pixbuf_get_width(data->PreviewPixbuf);
    int pixbufHeight = gdk_pixbuf_get_height(data->PreviewPixbuf);
    if (data->RenderSubArea == -1) {
	float scale_x = ((float)pixbufWidth) / data->UF->rotatedWidth;
	float scale_y = ((float)pixbufHeight) / data->UF->rotatedHeight;
	/* Set the area to redraw based on the crop rectangle and view port. */
#ifdef HAVE_GTKIMAGEVIEW
	GdkRectangle viewRect;
	gtk_image_view_get_viewport(GTK_IMAGE_VIEW(data->PreviewWidget),
		&viewRect);
	
	int x = MAX(viewRect.x, floor(CFG->CropX1 * scale_x));
	int x2 = MIN(viewRect.x+viewRect.width, ceil(CFG->CropX2 * scale_x));
	int width =  MAX(x2 - x, 0);
	int y = MAX(viewRect.y, floor (CFG->CropY1 * scale_y));
	int y2 = MIN(viewRect.y+viewRect.height, ceil(CFG->CropY2 * scale_y));
	int height =  MAX(y2 - y, 0);
#else
	int x = floor(CFG->CropX1 * scale_x);
	int width =  MIN(ceil(CFG->CropX2 * scale_x), pixbufWidth-1) - x;
	int y = floor (CFG->CropY1 * scale_y);
	int height =  MIN(ceil(CFG->CropY2 * scale_x), pixbufHeight-1) - y;
#endif
	data->OverUnderTicker++;
	preview_draw_area(data, x, y, width, height);
    }
    /* If no blinking is needed, disable this timeout function. */
    if (!CFG->blinkOverUnder || (!CFG->overExp && !CFG->underExp)) {
	data->BlinkTimer = 0;
	return FALSE;
    }
    return TRUE;
}

static void start_blink(preview_data *data)
{
    if ( CFG->blinkOverUnder && (CFG->overExp || CFG->underExp) ) {
	if (!data->BlinkTimer) {
	    data->BlinkTimer = g_timeout_add(500, switch_highlights, data);
	}
    }
}

static void stop_blink(preview_data *data)
{
    if (data->BlinkTimer) {
	data->OverUnderTicker = 0;
	switch_highlights(data);
	g_source_remove(data->BlinkTimer);
	data->BlinkTimer = 0;
    }
}

static void window_map_event(GtkWidget *widget, GdkEvent *event,
			     gpointer user_data)
{
    preview_data *data = get_preview_data(widget);
    start_blink(data);
    (void)event;
    (void)user_data;
}

static void window_unmap_event(GtkWidget *widget, GdkEvent *event,
			       gpointer user_data)
{
    preview_data *data = get_preview_data(widget);
    stop_blink(data);
    (void)event;
    (void)user_data;
}

static gboolean render_prepare(preview_data *data);
static gboolean render_raw_histogram(preview_data *data);
static gboolean render_preview_image(preview_data *data);
static gboolean render_live_histogram(preview_data *data);
static gboolean render_spot(preview_data *data);
static void draw_spot(preview_data *data, gboolean draw);

static void render_special_mode(GtkWidget *widget, long mode)
{
    preview_data *data = get_preview_data(widget);
    data->RenderMode = mode;
    int width = gdk_pixbuf_get_width(data->PreviewPixbuf);
    int height = gdk_pixbuf_get_height(data->PreviewPixbuf);
    preview_draw_area(data, 0, 0, width, height);
}

static void render_init(preview_data *data)
{
    /* Check if we need a new pixbuf */
    int width = gdk_pixbuf_get_width(data->PreviewPixbuf);
    int height = gdk_pixbuf_get_height(data->PreviewPixbuf);
    ufraw_image_data *image = &data->UF->Images[ufraw_first_phase];
    if (width!=image->width || height!=image->height) {
	data->PreviewPixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
					    image->width, image->height);
	/* Clear the pixbuffer to avoid displaying garbage */
	gdk_pixbuf_fill (data->PreviewPixbuf, 0);
#ifdef HAVE_GTKIMAGEVIEW
	gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(data->PreviewWidget),
		data->PreviewPixbuf, FALSE);
	gtk_image_view_set_zoom(GTK_IMAGE_VIEW(data->PreviewWidget), 1.0);
#else
	gtk_image_set_from_pixbuf(GTK_IMAGE(data->PreviewWidget),
		data->PreviewPixbuf);
#endif
	g_object_unref(data->PreviewPixbuf);
    }
    if (data->ProgressBar!=NULL) {
	char progressText[max_name];
	if (CFG->Scale==0)
	    g_snprintf(progressText, max_name, _("size %dx%d, zoom %2.f%%"),
		    data->UF->rotatedWidth, data->UF->rotatedHeight,
		    CFG->Zoom);
	else
	    g_snprintf(progressText, max_name, _("size %dx%d, scale 1/%d"),
		    data->UF->rotatedWidth, data->UF->rotatedHeight,
		    CFG->Scale);
	gtk_progress_bar_set_text(data->ProgressBar, progressText);
	gtk_progress_bar_set_fraction(data->ProgressBar, 0);
    }
}

void preview_invalidate_layer (preview_data *data, UFRawPhase phase)
{
    for (; phase < ufraw_phases_num; phase++)
	data->UF->Images [phase].valid = 0;
}

void render_preview(preview_data *data)
{
    if (data->FreezeDialog) return;

    render_init(data);
    ufraw_convert_image_init_phase(data->UF);
    data->RenderSubArea = 0;
    g_idle_remove_by_data(data);
    g_idle_add_full(G_PRIORITY_DEFAULT_IDLE-10,
	    (GSourceFunc)(render_prepare), data, NULL);
}

static gboolean render_prepare(preview_data *data)
{
    if (data->FreezeDialog) return FALSE;

    if (CFG->autoExposure == apply_state) {
	ufraw_auto_expose(data->UF);
	gdouble lower, upper;
	g_object_get(data->ExposureAdjustment, "lower", &lower,
		"upper", &upper, NULL);
	/* Clip the exposure to prevent a "changed" signal */
	if (CFG->exposure > upper) CFG->exposure = upper;
	if (CFG->exposure < lower) CFG->exposure = lower;
	gtk_adjustment_set_value(data->ExposureAdjustment, CFG->exposure);
	gtk_widget_set_sensitive(data->ResetExposureButton,
		fabs( conf_default.exposure - CFG->exposure) > 0.001);
    }
    if (CFG->autoBlack == apply_state) {
	ufraw_auto_black(data->UF);
	curveeditor_widget_set_curve(data->CurveWidget,
		&CFG->curve[CFG->curveIndex]);
	gtk_widget_set_sensitive(data->ResetBlackButton,
		CFG->curve[CFG->curveIndex].m_anchors[0].x!=0.0 ||
		CFG->curve[CFG->curveIndex].m_anchors[0].y!=0.0 );
    }
    char text[max_name];
    g_snprintf(text, max_name, _("Black point: %0.3lf"),
	    CFG->curve[CFG->curveIndex].m_anchors[0].x);
    gtk_label_set_text(GTK_LABEL(data->BlackLabel), text);

    if ( Developer==NULL )
            Developer = developer_init();

    if ( CFG->profileIndex[display_profile]==0 ) {
	guint8 *displayProfile;
	gint profileSize;
	uf_get_display_profile(data->PreviewWidget, &displayProfile,
		&profileSize);
	developer_display_profile(Developer, displayProfile, profileSize,
		CFG->profile[display_profile]
			[CFG->profileIndex[display_profile]].productName);
    } else {
	developer_display_profile(Developer, NULL, 0,
		CFG->profile[display_profile]
			[CFG->profileIndex[display_profile]].productName);
    }
    ufraw_developer_prepare(data->UF, display_developer);

    /* The reset of the rendering can be triggered only after the call to
     * ufraw_developer_preare(). Otherwise error messages in this function
     * would cause timing problems. */
    g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
	    (GSourceFunc)(render_raw_histogram), data, NULL);
    g_idle_add_full(G_PRIORITY_DEFAULT_IDLE+10,
	    (GSourceFunc)(render_preview_image), data, NULL);
    g_idle_add_full(G_PRIORITY_DEFAULT_IDLE+20,
	    (GSourceFunc)(render_live_histogram), data, NULL);
    g_idle_add_full(G_PRIORITY_DEFAULT_IDLE+20,
	    (GSourceFunc)(render_spot), data, NULL);
    return FALSE;
}

static gboolean render_raw_histogram(preview_data *data)
{
    if (data->FreezeDialog) return FALSE;
    guint8 pix[99], *p8, pen[4][3];
    guint16 pixtmp[9999];
    image_type p16;
    int x, c, cl, y, y0, y1;
    int raw_his[raw_his_size][4], raw_his_max;

    int hisHeight = data->RawHisto->allocation.height-2;
    hisHeight = MAX(MIN(hisHeight, his_max_height), data->HisMinHeight);

    GdkPixbuf *pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(data->RawHisto));
    if ( pixbuf==NULL || gdk_pixbuf_get_height(pixbuf)!=hisHeight+2 ) {
	pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
	        raw_his_size+2, hisHeight+2);
	gtk_image_set_from_pixbuf(GTK_IMAGE(data->RawHisto), pixbuf);
	g_object_unref(pixbuf);
    }
    int colors = data->UF->colors;
    guint8 *pixies = gdk_pixbuf_get_pixels(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    memset(pixies, 0, (gdk_pixbuf_get_height(pixbuf)-1)*rowstride +
	    gdk_pixbuf_get_width(pixbuf)*gdk_pixbuf_get_n_channels(pixbuf));
    /* Normalize raw histogram data */
    for (x=0, raw_his_max=1; x<raw_his_size; x++) {
	for (c=0, y=0; c<colors; c++) {
	    if (CFG->rawHistogramScale==log_histogram)
		raw_his[x][c] = log(1+data->raw_his[x][c])*1000;
	    else
		raw_his[x][c] = data->raw_his[x][c];
	    y += raw_his[x][c];
	}
	raw_his_max = MAX(raw_his_max, y);
    }
    /* Prepare pen color, which is not effected by exposure.
     * Use a small value to avoid highlights and then normalize. */
    for (c=0; c<colors; c++) {
	for (cl=0; cl<colors; cl++) p16[cl] = 0;
	p16[c] = Developer->max * 0x08000 / Developer->rgbWB[c] *
		0x10000 / Developer->exposure;
	develope(pen[c], p16, Developer, 8, pixtmp, 1);
	guint8 max=1;
	for (cl=0; cl<3; cl++) max = MAX(pen[c][cl], max);
	for (cl=0; cl<3; cl++) pen[c][cl] = pen[c][cl] * 0xff / max;
    }
    /* Calculate the curves */
    p8 = pix;
    guint8 grayCurve[raw_his_size+1][4];
    guint8 pureCurve[raw_his_size+1][4];
    for (x=0; x<raw_his_size+1; x++) {
	for (c=0; c<colors; c++) {
	    /* Value for pixel x of color c in a gray pixel */
	    for (cl=0; cl<colors; cl++)
		p16[cl] = MIN((guint64)x*Developer->rgbMax *
			  Developer->rgbWB[c] /
			  Developer->rgbWB[cl] / raw_his_size, 0xFFFF);
	    develope(p8, p16, Developer, 8, pixtmp, 1);
	    grayCurve[x][c] = MAX(MAX(p8[0],p8[1]),p8[2]) *
		    (hisHeight-1) / MAXOUT;
	    /* Value for pixel x of pure color c */
	    p16[0] = p16[1] = p16[2] = p16[3] = 0;
	    p16[c] = MIN((guint64)x*Developer->rgbMax/raw_his_size, 0xFFFF);
	    develope(p8, p16, Developer, 8, pixtmp, 1);
	    pureCurve[x][c] = MAX(MAX(p8[0],p8[1]),p8[2]) *
		    (hisHeight-1) / MAXOUT;
	}
    }
    for (x=0; x<raw_his_size; x++) {
	/* draw the raw histogram */
	for (c=0, y0=0; c<colors; c++) {
	    for (y=0; y<raw_his[x][c]*hisHeight/raw_his_max; y++)
		for (cl=0; cl<3; cl++)
		    pixies[(hisHeight-y-y0)*rowstride
			    + 3*(x+1)+cl] = pen[c][cl];
	    y0 += y;
	}
	/* draw curves on the raw histogram */
	for (c=0; c<colors; c++) {
	    y = grayCurve[x][c];
	    y1 = grayCurve[x+1][c];
	    for (; y<=y1; y++)
		for (cl=0; cl<3; cl++)
		    pixies[(hisHeight-y)*rowstride + 3*(x+1)+cl] = pen[c][cl];
	    y1 = pureCurve[x][c];
	    for (; y<y1; y++)
		for (cl=0; cl<3; cl++)
		    pixies[(hisHeight-y)*rowstride + 3*(x+1)+cl] = pen[c][cl]/2;
	    y1 = pureCurve[x+1][c];
	    for (; y<=y1; y++)
		for (cl=0; cl<3; cl++)
		    pixies[(hisHeight-y)*rowstride + 3*(x+1)+cl] = pen[c][cl];
	}
    }
    gtk_widget_queue_draw(data->RawHisto);
    return FALSE;
}

static gboolean render_preview_image(preview_data *data)
{
    ufraw_image_data *img;
    gboolean again = TRUE;
    int i = -1;

    if (data->FreezeDialog) return FALSE;

    if (data->RenderSubArea < 0)
        return FALSE;

#ifdef HAVE_GTKIMAGEVIEW
    /* First of all, find the maximally visible yet unrendered subarea.
     * Refreshing visible subareas in the first place improves visual
     * feedback and overall user experience.
     */
    img = ufraw_convert_image_area (data->UF, -1, ufraw_phases_num - 1);

    GdkRectangle viewport;
    gtk_image_view_get_viewport (
        GTK_IMAGE_VIEW (data->PreviewWidget), &viewport);

#ifdef _OPENMP
#pragma omp parallel shared(i) reduction(||:again)
#endif
    {
    int max_area = -1;
    int subarea = -1;
    int x, y, w, h;

#ifdef _OPENMP
#pragma omp critical
#endif
    for (i++; i < 32; i++)
    {
        /* Skip valid subareas */
        if (img->valid & (1 << i))
            continue;

        ufraw_img_get_subarea_coord (img, i, &x, &y, &w, &h);

        gboolean noclip = TRUE;
        if (x < viewport.x)
        {
            w -= (viewport.x - x);
            x = viewport.x;
            noclip = FALSE;
        }
        if (x + w > viewport.x + viewport.width)
        {
            w = viewport.x + viewport.width - x;
            noclip = FALSE;
        }
        if (y < viewport.y)
        {
            h -= (viewport.y - y);
            y = viewport.y;
            noclip = FALSE;
        }
        if (y + h > viewport.y + viewport.height)
        {
            h = viewport.y + viewport.height - y;
            noclip = FALSE;
        }

        /* Compute the visible area of the subarea */
        int area = (w > 0 && h > 0) ? w * h : 0;

        if (area > max_area)
        {
            max_area = area;
            subarea = i;
            /* If this area is fully visible, stop searching */
            if (noclip)
                break;
        }
    }

#else
#ifdef _OPENMP
#pragma omp critical
#endif
    subarea = data->RenderSubArea++;
    if (subarea > 31)
	subarea = -1;
#endif

    if (subarea < 0)
    {
        data->RenderSubArea = -1;
        redraw_navigation_image(data);
        again = FALSE;
    }
    else {
	ufraw_image_data *img1 = ufraw_convert_image_area (
	    data->UF, subarea, ufraw_phases_num - 1);
	if (!img1)
	    again = FALSE;

	else {
	    ufraw_img_get_subarea_coord (img1, subarea, &x, &y, &w, &h);
	    preview_draw_img_area (data, img1, x, y, w, h);
	}
    }

    }

    return again;
}

static gboolean render_live_histogram(preview_data *data)
{
    if (data->FreezeDialog) return FALSE;

    int width = gdk_pixbuf_get_width(data->PreviewPixbuf);
    int height = gdk_pixbuf_get_height(data->PreviewPixbuf);
    int i, x, y, c, min, max;
    guint8 *p8;
    ufraw_image_data img = data->UF->Images[ufraw_develop_phase];
    double rgb[3];
    guint64 sum[3], sqr[3];
    int live_his[live_his_size][4];
    memset(live_his, 0, sizeof(live_his));
    for (i=0; i < img.height*img.width; i++) {
	p8 = img.buffer + i*img.depth;
	for (c=0, max=0, min=0x100; c<3; c++) {
	    max = MAX(max, p8[c]);
	    min = MIN(min, p8[c]);
	    live_his[p8[c]][c]++;
	}
	if (CFG->histogram==luminosity_histogram)
	    live_his[(int)(0.3*p8[0]+0.59*p8[1]+0.11*p8[2])][3]++;
	if (CFG->histogram==value_histogram)
	    live_his[max][3]++;
	if (CFG->histogram==saturation_histogram) {
	    if (max==0) live_his[0][3]++;
	    else live_his[255*(max-min)/max][3]++;
	}
    }
    int hisHeight = MIN(data->LiveHisto->allocation.height-2, his_max_height);
    hisHeight = MAX(hisHeight, data->HisMinHeight);

    GdkPixbuf *pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(data->LiveHisto));
    if ( pixbuf==NULL || gdk_pixbuf_get_height(pixbuf)!=hisHeight+2 ) {
	pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
	        live_his_size+2, hisHeight+2);
	gtk_image_set_from_pixbuf(GTK_IMAGE(data->LiveHisto), pixbuf);
	g_object_unref(pixbuf);
    }
    guint8 *pixies = gdk_pixbuf_get_pixels(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    memset(pixies, 0, (gdk_pixbuf_get_height(pixbuf)-1)*rowstride +
	    gdk_pixbuf_get_width(pixbuf)*gdk_pixbuf_get_n_channels(pixbuf));
    for (c=0; c<3; c++) {
	sum[c] = 0;
	sqr[c] = 0;
	for (x=1; x<live_his_size; x++) {
	    sum[c] += x*live_his[x][c];
	    sqr[c] += (guint64)x*x*live_his[x][c];
	}
    }
    int live_his_max;
    for (x=1, live_his_max=1; x<live_his_size-1; x++) {
	if (CFG->liveHistogramScale==log_histogram)
	    for (c=0; c<4; c++) live_his[x][c] = log(1+live_his[x][c])*1000;
	if (CFG->histogram==rgb_histogram)
	    for (c=0; c<3; c++)
		live_his_max = MAX(live_his_max, live_his[x][c]);
	else if (CFG->histogram==r_g_b_histogram)
	    live_his_max = MAX(live_his_max,
		    live_his[x][0]+live_his[x][1]+live_his[x][2]);
	else live_his_max = MAX(live_his_max, live_his[x][3]);
    }
    for (x=0; x<live_his_size; x++) for (y=0; y<hisHeight; y++)
	if (CFG->histogram==r_g_b_histogram) {
	    if (y*live_his_max < live_his[x][0]*hisHeight)
		pixies[(hisHeight-y)*rowstride+3*(x+1)+0] = 255;
	    else if (y*live_his_max <
		    (live_his[x][0]+live_his[x][1])*hisHeight)
		pixies[(hisHeight-y)*rowstride+3*(x+1)+1] = 255;
	    else if (y*live_his_max <
		    (live_his[x][0]+live_his[x][1]+live_his[x][2])
		    *hisHeight)
		pixies[(hisHeight-y)*rowstride+3*(x+1)+2] = 255;
	} else {
	    for (c=0; c<3; c++)
		if (CFG->histogram==rgb_histogram) {
		    if (y*live_his_max<live_his[x][c]*hisHeight)
			pixies[(hisHeight-y)*rowstride+3*(x+1)+c] = 255;
		} else {
		    if (y*live_his_max<live_his[x][3]*hisHeight)
			pixies[(hisHeight-y)*rowstride+3*(x+1)+c] = 255;
		}
       }

    /* draw vertical line at quarters "behind" the live histogram */
    for (y=-1; y<hisHeight+1; y++)
	for (x=64; x<255; x+=64) {
	    guint8 *pix = pixies+(hisHeight-y)*rowstride+3*(x+1);
	    if (pix[0]==0 && pix[1]==0 && pix[2]==0 )
		for (c=0; c<3; c++)
		    pix[c]=96;	/* gray */
	}
    gtk_widget_queue_draw(data->LiveHisto);
    for (c=0;c<3;c++) rgb[c] = sum[c]/height/width;
    color_labels_set(data->AvrLabels, rgb);
    for (c=0;c<3;c++) rgb[c] = sqrt(sqr[c]/height/width-rgb[c]*rgb[c]);
    color_labels_set(data->DevLabels, rgb);
    for (c=0;c<3;c++) rgb[c] = 100.0*live_his[live_his_size-1][c]/height/width;
    color_labels_set(data->OverLabels, rgb);
    for (c=0;c<3;c++) rgb[c] = 100.0*live_his[0][c]/height/width;
    color_labels_set(data->UnderLabels, rgb);

    return FALSE;
}

struct spot
{
    int StartY;
    int EndY;
    int StartX;
    int EndX;
    int Size;
};

static void calculate_spot(preview_data *data, struct spot *spot,
			   int width, int height)
{
    int spotHeight = abs(data->SpotY1 - data->SpotY2)
	    * height / data->UF->rotatedHeight + 1;
    spot->StartY = MIN(data->SpotY1, data->SpotY2)
	    * height / data->UF->rotatedHeight;
    if (spotHeight + spot->StartY > height)
	spot->StartY = height - spotHeight;
    spot->EndY = spot->StartY + spotHeight;

    int spotWidth = abs(data->SpotX1 - data->SpotX2)
	    * width / data->UF->rotatedWidth + 1;
    spot->StartX = MIN(data->SpotX1, data->SpotX2)
	    * width / data->UF->rotatedWidth;
    if (spotWidth + spot->StartX > width)
	spot->StartX = width - spotWidth;
    spot->EndX = spot->StartX + spotWidth;

    spot->Size = spotWidth * spotHeight;
}

static gboolean render_spot(preview_data *data)
{
    if (data->FreezeDialog) return FALSE;

    if (data->SpotX1<0) return FALSE;
    if ( data->SpotX1>=data->UF->rotatedWidth ||
	 data->SpotY1>=data->UF->rotatedHeight ) return FALSE;
    int width = data->UF->Images[ufraw_develop_phase].width;
    int height = data->UF->Images[ufraw_develop_phase].height;
    int outDepth = data->UF->Images[ufraw_develop_phase].depth;
    void *outBuffer = data->UF->Images[ufraw_develop_phase].buffer;
    int rawDepth = data->UF->Images[ufraw_first_phase].depth;
    void *rawBuffer = data->UF->Images[ufraw_first_phase].buffer;
    /* We assume that first_phase and final_phase buffer sizes are the same. */
    /* Scale image coordinates to Images[ufraw_develop_phase] coordinates */
    struct spot spot;
    calculate_spot(data, &spot, width, height);
    guint64 rawSum[4], outSum[3];
    int c, y, x;
    for (c=0; c<3; c++) rawSum[c] = outSum[c] = 0;
    for (y=spot.StartY; y<spot.EndY; y++) {
	guint16 *rawPixie = rawBuffer + (y*width + spot.StartX)*rawDepth;
	guint8 *outPixie = outBuffer + (y*width + spot.StartX)*outDepth;
	for (x=spot.StartX;
	     x<spot.EndX;
	     x++, rawPixie += rawDepth, outPixie += outDepth) {
	    for (c=0; c<data->UF->colors; c++)
		rawSum[c] += rawPixie[c];
	    for (c=0; c<3; c++)
		outSum[c] += outPixie[c];
	}
    }
    double rgb[5];
    for (c=0; c<3; c++) rgb[c] = outSum[c] / spot.Size;
    /*
     * Convert RGB pixel value to 0-1 space, intending to reprsent,
     * absent contrast manipulation and color issues, luminance relative
     * to an 18% grey card at 0.18.
     * The RGB color space is approximately linearized sRGB as it is not
     * affected from the ICC profile.
     */
    guint16 rawChannels[4], linearChannels[3];
    for (c=0; c<data->UF->colors; c++)
	rawChannels[c] = rawSum[c] / spot.Size;
    develop_linear(rawChannels, linearChannels, Developer);
    double yValue = 0.5;
    extern const double xyz_rgb[3][3];
    for (c=0; c<3; c++)
	yValue += xyz_rgb[1][c] * linearChannels[c];
    yValue /= 0xFFFF;
    if ( Developer->clipHighlights==film_highlights )
	yValue *= (double)Developer->exposure/0x10000;
    rgb[3] = yValue;
    rgb[4] = value2zone(yValue);
    color_labels_set(data->SpotLabels, rgb);
    char tmp[max_name];
    g_snprintf(tmp, max_name, "<span background='#%02X%02X%02X'>"
	    "                    </span>",
	    (int)rgb[0], (int)rgb[1], (int)rgb[2]);
    gtk_label_set_markup(data->SpotPatch, tmp);
    gtk_widget_show(GTK_WIDGET(data->SpotTable));
    if ( data->PageNum!=data->PageNumCrop )
	draw_spot(data, TRUE);

    return FALSE;
}

static void close_spot(GtkWidget *widget, gpointer user_data)
{
    (void)user_data;
    preview_data *data = get_preview_data(widget);
    draw_spot(data, FALSE);
    data->SpotX1 = -1;
    gtk_widget_hide(GTK_WIDGET(data->SpotTable));
}

static void draw_spot(preview_data *data, gboolean draw)
{
    if (data->SpotX1<0) return;
    int width = gdk_pixbuf_get_width(data->PreviewPixbuf);
    int height = gdk_pixbuf_get_height(data->PreviewPixbuf);
    data->SpotDraw = draw;
    /* Scale spot image coordinates to pixbuf coordinates */
    int SpotY1 = MAX(MIN(data->SpotY1, data->SpotY2)
	    * height / data->UF->rotatedHeight - 1, 0);
    int SpotY2 = MIN(MAX(data->SpotY1, data->SpotY2)
	    * height / data->UF->rotatedHeight + 1, height-1);
    int SpotX1 = MAX(MIN(data->SpotX1, data->SpotX2)
	    * width / data->UF->rotatedWidth - 1, 0);
    int SpotX2 = MIN(MAX(data->SpotX1, data->SpotX2)
	    * width / data->UF->rotatedWidth + 1, width-1);
    preview_draw_area(data, SpotX1, SpotY1, SpotX2-SpotX1+1, 1);
    preview_draw_area(data, SpotX1, SpotY2, SpotX2-SpotX1+1, 1);
    preview_draw_area(data, SpotX1, SpotY1, 1, SpotY2-SpotY1+1);
    preview_draw_area(data, SpotX2, SpotY1, 1, SpotY2-SpotY1+1);
}

static void lch_to_color(float lch[3], GdkColor *color)
{
    gint64 rgb[3];
    uf_cielch_to_rgb(lch, rgb);
    color->red = pow((double)MIN(rgb[0], 0xFFFF)/0xFFFF, 0.45) * 0xFFFF;
    color->green = pow((double)MIN(rgb[1], 0xFFFF)/0xFFFF, 0.45) * 0xFFFF;
    color->blue = pow((double)MIN(rgb[2], 0xFFFF)/0xFFFF, 0.45) * 0xFFFF;
}

static void widget_set_hue(GtkWidget *widget, double hue)
{
    float lch[3];
    GdkColor hueColor;
    lch[1] = 181.019336; // max_colorfulness = sqrt(128*128+128*128)
    lch[2] = hue * M_PI / 180;

    lch[0] = 0.75 * 100.0; // max_luminance = 100
    lch_to_color(lch, &hueColor);
    gtk_widget_modify_bg(widget, GTK_STATE_NORMAL, &hueColor);

    lch[0] = 1.0 * 100.0; // max_luminance = 100
    lch_to_color(lch, &hueColor);
    gtk_widget_modify_bg(widget, GTK_STATE_PRELIGHT, &hueColor);

    lch[0] = 0.5 * 100.0; // max_luminance = 100
    lch_to_color(lch, &hueColor);
    gtk_widget_modify_bg(widget, GTK_STATE_ACTIVE, &hueColor);
}

static void update_shrink_ranges(preview_data *data);

/* update the UI entries that could have changed automatically */
static void update_scales(preview_data *data)
{
    if (data->FreezeDialog) return;
    data->FreezeDialog = TRUE;

    if (CFG->BaseCurveIndex>camera_curve && !CFG_cameraCurve)
	gtk_combo_box_set_active(data->BaseCurveCombo, CFG->BaseCurveIndex-2);
    else
	gtk_combo_box_set_active(data->BaseCurveCombo, CFG->BaseCurveIndex);
    gtk_combo_box_set_active(data->CurveCombo, CFG->curveIndex);

    int i;
    GList *l;
    double max;
    char tmp[max_name];

    gtk_combo_box_set_active(data->WBCombo, 0);
    for (i=0, l=data->WBPresets; l!=NULL; i++, l=g_list_next(l))
	if (!strcmp(CFG->wb, l->data))
	    gtk_combo_box_set_active(data->WBCombo, i);

    if (data->WBTuningAdjustment!=NULL)
	gtk_adjustment_set_value(data->WBTuningAdjustment, CFG->WBTuning);
    gtk_adjustment_set_value(data->TemperatureAdjustment, CFG->temperature);
    gtk_adjustment_set_value(data->GreenAdjustment, CFG->green);
    gtk_adjustment_set_value(data->ExposureAdjustment, CFG->exposure);
    gtk_adjustment_set_value(data->ThresholdAdjustment, CFG->threshold);
#ifdef UFRAW_CONTRAST
    gtk_adjustment_set_value(data->ContrastAdjustment, CFG->contrast);
#endif
    gtk_adjustment_set_value(data->SaturationAdjustment, CFG->saturation);
    gtk_adjustment_set_value(data->GammaAdjustment,
	    CFG->profile[0][CFG->profileIndex[0]].gamma);
    gtk_adjustment_set_value(data->LinearAdjustment,
	    CFG->profile[0][CFG->profileIndex[0]].linear);
    for (i=0; i<data->UF->colors; i++)
	gtk_adjustment_set_value(data->ChannelAdjustment[i],
		CFG->chanMul[i]);
    gtk_adjustment_set_value(data->ZoomAdjustment, CFG->Zoom);

    uf_combo_box_set_data(data->BitDepthCombo,
	&CFG->profile[out_profile][CFG->profileIndex[out_profile]].BitDepth);
    gtk_toggle_button_set_active(data->AutoExposureButton, CFG->autoExposure);
    gtk_toggle_button_set_active(data->AutoBlackButton, CFG->autoBlack);
    curveeditor_widget_set_curve(data->CurveWidget,
	    &CFG->curve[CFG->curveIndex]);

    gtk_widget_set_sensitive(data->ResetWBButton,
	    ( strcmp(CFG->wb, data->initialWB)
	    ||fabs(CFG->temperature-data->initialTemperature)>1
	    ||fabs(CFG->green-data->initialGreen)>0.001
	    ||CFG->chanMul[0]!=data->initialChanMul[0]
	    ||CFG->chanMul[1]!=data->initialChanMul[1]
	    ||CFG->chanMul[2]!=data->initialChanMul[2]
	    ||CFG->chanMul[3]!=data->initialChanMul[3] ) );
    gtk_widget_set_sensitive(data->ResetGammaButton,
	    fabs( profile_default_gamma(&CFG->profile[0][CFG->profileIndex[0]])
		- CFG->profile[0][CFG->profileIndex[0]].gamma) > 0.001);
    gtk_widget_set_sensitive(data->ResetLinearButton,
	    fabs( profile_default_linear(&CFG->profile[0][CFG->profileIndex[0]])
		- CFG->profile[0][CFG->profileIndex[0]].linear) > 0.001);
    gtk_widget_set_sensitive(data->ResetExposureButton,
	    fabs( conf_default.exposure - CFG->exposure) > 0.001);
    gtk_widget_set_sensitive(data->ResetThresholdButton,
	    fabs( conf_default.threshold - CFG->threshold) > 1);
#ifdef UFRAW_CONTRAST
    gtk_widget_set_sensitive(data->ResetContrastButton,
	    fabs( conf_default.contrast - CFG->contrast) > 0.001);
#endif
    gtk_widget_set_sensitive(data->ResetSaturationButton,
	    fabs( conf_default.saturation - CFG->saturation) > 0.001);
    gtk_widget_set_sensitive(data->ResetBaseCurveButton,
	    CFG->BaseCurve[CFG->BaseCurveIndex].m_numAnchors>2 ||
	    CFG->BaseCurve[CFG->BaseCurveIndex].m_anchors[0].x!=0.0 ||
	    CFG->BaseCurve[CFG->BaseCurveIndex].m_anchors[0].y!=0.0 ||
	    CFG->BaseCurve[CFG->BaseCurveIndex].m_anchors[1].x!=1.0 ||
	    CFG->BaseCurve[CFG->BaseCurveIndex].m_anchors[1].y!=1.0 );
    gtk_widget_set_sensitive(data->ResetBlackButton,
	    CFG->curve[CFG->curveIndex].m_anchors[0].x!=0.0 ||
	    CFG->curve[CFG->curveIndex].m_anchors[0].y!=0.0 );
    gtk_widget_set_sensitive(data->ResetCurveButton,
	    CFG->curve[CFG->curveIndex].m_numAnchors>2 ||
	    CFG->curve[CFG->curveIndex].m_anchors[1].x!=1.0 ||
	    CFG->curve[CFG->curveIndex].m_anchors[1].y!=1.0 );
    for (i = 0; i < 3; ++i)
        gtk_adjustment_set_value(data->GrayscaleMixers[i],
				 CFG->grayscaleMixer[i]);
    for (i = 0; i < CFG->lightnessAdjustmentCount; ++i) {
	gtk_adjustment_set_value(data->LightnessAdjustment[i],
	    CFG->lightnessAdjustment[i].adjustment);
	gtk_widget_set_sensitive(data->ResetLightnessAdjustmentButton[i],
	    fabs(CFG->lightnessAdjustment[i].adjustment - 1.0) >= 0.01);
    }
    gtk_widget_set_sensitive(GTK_WIDGET(data->ResetGrayscaleChannelMixerButton),
	(CFG->grayscaleMixer[0] != conf_default.grayscaleMixer[0])
	|| (CFG->grayscaleMixer[1] != conf_default.grayscaleMixer[1])
	|| (CFG->grayscaleMixer[2] != conf_default.grayscaleMixer[2]));
    gtk_widget_set_sensitive(GTK_WIDGET(data->GrayscaleMixerTable),
			     CFG->grayscaleMode == grayscale_mixer);

    for (max = 1, i = 0; i < 3; ++i)
        max = MAX(max, CFG->grayscaleMixer[i]);
    g_snprintf(tmp, max_name, "<span background='#%02X%02X%02X'>"
	    "                    </span>",
	       (int)(MAX(CFG->grayscaleMixer[0], 0) / max * 255),
	       (int)(MAX(CFG->grayscaleMixer[1], 0) / max * 255),
	       (int)(MAX(CFG->grayscaleMixer[2], 0) / max * 255));
    gtk_label_set_markup(data->GrayscaleMixerColor, tmp);

    if ( fabs(data->shrink-floor(data->shrink+0.0005))<0.0005 ) {
	CFG->shrink = floor(data->shrink+0.0005);
	CFG->size = 0;
    } else {
	CFG->shrink = 1;
	CFG->size = floor(MAX(data->height, data->width)+0.5);
    }
    data->FreezeDialog = FALSE;
    update_shrink_ranges(data);
    render_preview(data);
}

static void curve_update(GtkWidget *widget, long curveType)
{
    preview_data *data = get_preview_data(widget);
    if (curveType==base_curve) {
	CFG->BaseCurveIndex = manual_curve;
	CFG->BaseCurve[CFG->BaseCurveIndex] =
	    *curveeditor_widget_get_curve(data->BaseCurveWidget);
	if (CFG->autoExposure==enabled_state) CFG->autoExposure = apply_state;
	if (CFG->autoBlack==enabled_state) CFG->autoBlack = apply_state;
    } else {
	CFG->curveIndex = manual_curve;
	CFG->curve[CFG->curveIndex] =
	    *curveeditor_widget_get_curve(data->CurveWidget);
	CFG->autoBlack = FALSE;
    }
    preview_invalidate_layer (data, ufraw_develop_phase);
    update_scales(data);
}

static void spot_wb_event(GtkWidget *widget, gpointer user_data)
{
    preview_data *data = get_preview_data(widget);
    struct spot spot;
    int width, height, x, y, c;
    guint64 rgb[4];

    user_data = user_data;

    if (data->FreezeDialog) return;
    if (data->SpotX1<=0) return;
    width = gdk_pixbuf_get_width(data->PreviewPixbuf);
    height = gdk_pixbuf_get_height(data->PreviewPixbuf);
    /* Scale image coordinates to pixbuf coordinates */
    calculate_spot(data, &spot, width, height);
    ufraw_image_data *image = &data->UF->Images[ufraw_first_phase];

    for (c=0; c<4; c++) rgb[c] = 0;
    for (y=spot.StartY; y<spot.EndY; y++)
	for (x=spot.StartX; x<spot.EndX; x++) {
	    guint16 *pixie = (guint16*)(image->buffer +
		    (y*image->width + x)*image->depth);
	    for (c=0; c<data->UF->colors; c++)
		rgb[c] += pixie[c];
	}
    for (c=0; c<4; c++) rgb[c] = MAX(rgb[c], 1);
    for (c=0; c<data->UF->colors; c++)
	CFG->chanMul[c] = (double)spot.Size * data->UF->rgbMax / rgb[c];
    if (data->UF->colors<4) CFG->chanMul[3] = 0.0;
    ufraw_message(UFRAW_SET_LOG,
	    "spot_wb: channel multipliers = { %.0f, %.0f, %.0f, %.0f }\n",
	    CFG->chanMul[0], CFG->chanMul[1], CFG->chanMul[2], CFG->chanMul[3]);
    g_strlcpy(CFG->wb, spot_wb, max_name);
    ufraw_set_wb(data->UF);
    if (CFG->autoExposure==enabled_state) CFG->autoExposure = apply_state;
    if (CFG->autoBlack==enabled_state) CFG->autoBlack = apply_state;
    preview_invalidate_layer (data, ufraw_develop_phase);
    update_scales(data);
}

static void remove_hue_event(GtkWidget *widget, gpointer user_data)
{
    preview_data *data = get_preview_data(widget);
    long i = (long)user_data;

    for (;i<CFG->lightnessAdjustmentCount-1; i++) {
	CFG->lightnessAdjustment[i] = CFG->lightnessAdjustment[i+1];
    	widget_set_hue(data->LightnessHueSelectButton[i],
		CFG->lightnessAdjustment[i].hue);
    }
    CFG->lightnessAdjustment[i] = conf_default.lightnessAdjustment[i];
    gtk_widget_hide(GTK_WIDGET(data->LightnessAdjustmentTable[i]));
    CFG->lightnessAdjustmentCount--;

    preview_invalidate_layer (data, ufraw_develop_phase);
    update_scales(data);
}

static void calculate_hue(preview_data *data, int i)
{
    float lch[3];
    int width = data->UF->Images[ufraw_develop_phase].width;
    int height = data->UF->Images[ufraw_develop_phase].height;
    int rawDepth = data->UF->Images[ufraw_first_phase].depth;
    void *rawBuffer = data->UF->Images[ufraw_first_phase].buffer;
    /* We assume that first_phase and final_phase buffer sizes are the same. */
    /* Scale image coordinates to Images[ufraw_develop_phase] coordinates */
    struct spot spot;
    calculate_spot(data, &spot, width, height);

    guint64 rawSum[4];
    int c, y, x;
    for (c=0; c<3; c++) rawSum[c] = 0;
    for (y=spot.StartY; y<spot.EndY; y++) {
	guint16 *rawPixie = rawBuffer + (y*width + spot.StartX)*rawDepth;
	for (x=spot.StartX; x<spot.EndX; x++, rawPixie += rawDepth) {
	    for (c=0; c<data->UF->colors; c++)
		rawSum[c] += rawPixie[c];
	}
    }
    guint16 rawChannels[4];
    for (c=0; c<data->UF->colors; c++)
	rawChannels[c] = rawSum[c] / spot.Size;

    uf_raw_to_cielch(Developer, rawChannels, lch);
    CFG->lightnessAdjustment[i].hue = lch[2];

    double hue = lch[2];
    double sum = 0;
    for (y=spot.StartY; y<spot.EndY; y++) {
	guint16 *rawPixie = rawBuffer + (y*width + spot.StartX)*rawDepth;
	for (x=spot.StartX; x<spot.EndX; x++, rawPixie += rawDepth) {
	    uf_raw_to_cielch(Developer, rawPixie, lch);
	    double diff = fabs(hue - lch[2]);
	    if (diff > 180.0)
		diff = 360.0 - diff;
	    sum += diff * diff;
	}
    }
    double stddev = sqrt(sum / spot.Size);

    CFG->lightnessAdjustment[i].hueWidth = MIN(stddev * 2, 60);
}

static void select_hue_event(GtkWidget *widget, gpointer user_data)
{
    preview_data *data = get_preview_data(widget);
    long i = (long)user_data;

    if (data->FreezeDialog) return;
    if (data->SpotX1 == -1) return;

    if (i < 0) {
	if (CFG->lightnessAdjustmentCount >= max_adjustments) {
	    ufraw_message(UFRAW_ERROR, _("No more room for new lightness adjustments."));
	    return;
	}
	i = CFG->lightnessAdjustmentCount++;
    }

    calculate_hue(data, i);
    widget_set_hue(data->LightnessHueSelectButton[i],
		   CFG->lightnessAdjustment[i].hue);
    gtk_widget_show_all(GTK_WIDGET(data->LightnessAdjustmentTable[i]));

    preview_invalidate_layer (data, ufraw_develop_phase);
    update_scales(data);
}

static void event_coordinate_rescale(gdouble *x, gdouble *y, preview_data *data)
{
    int width = gdk_pixbuf_get_width(data->PreviewPixbuf);
    int height = gdk_pixbuf_get_height(data->PreviewPixbuf);
#ifdef HAVE_GTKIMAGEVIEW
    /* Find location of event in the view. */
    GdkRectangle viewRect;
    gtk_image_view_get_viewport(GTK_IMAGE_VIEW(data->PreviewWidget),
	    &viewRect);
    int viewWidth = data->PreviewWidget->allocation.width;
    int viewHeight = data->PreviewWidget->allocation.height;
    if ( viewWidth<width ) {
	*x += viewRect.x;
    } else {
	*x -= (viewWidth - width) / 2;
    }
    if ( viewHeight<height ) {
	*y += viewRect.y;
    } else {
	*y -= (viewHeight - height) / 2;
    }
#endif
    if ( *x<0) *x = 0;
    if ( *x>width ) *x = width;
    if ( *y<0 ) *y = 0;
    if ( *y>height ) *y = height;
    /* Scale pixbuf coordinates to image coordinates */
    *x = *x * data->UF->rotatedWidth / width;
    *y = *y * data->UF->rotatedHeight / height;
}

static gboolean preview_button_press_event(GtkWidget *event_box,
	GdkEventButton *event, gpointer user_data)
{
    preview_data *data = get_preview_data(event_box);
    (void)user_data;
    if (data->FreezeDialog) return FALSE;
    if (event->button!=1) return FALSE;
    event_coordinate_rescale(&event->x, &event->y, data);
    if ( data->PageNum==data->PageNumSpot ||
	 data->PageNum==data->PageNumLightness ||
	 data->PageNum==data->PageNumGray ) {
	data->PreviewButtonPressed = TRUE;
	draw_spot(data, FALSE);
	data->SpotX1 = data->SpotX2 = event->x;
	data->SpotY1 = data->SpotY2 = event->y;
	g_idle_add_full(G_PRIORITY_DEFAULT_IDLE+20,
		(GSourceFunc)(render_spot), data, NULL);
	return TRUE;
    }
    if ( data->PageNum==data->PageNumCrop ) {
	data->PreviewButtonPressed = TRUE;
	return TRUE;
    }
    return FALSE;
}

static gboolean preview_button_release_event(GtkWidget *event_box,
	GdkEventButton *event, gpointer user_data)
{
    preview_data *data = get_preview_data(event_box);
    (void)user_data;
    if (event->button!=1) return FALSE;
    data->PreviewButtonPressed = FALSE;
    return TRUE;
}

static void update_crop_ranges(preview_data *data);
static void fix_crop_aspect(preview_data *data, CursorType cursor);
static void set_new_aspect(preview_data *data);
static void refresh_aspect(preview_data *data);

static gboolean crop_motion_notify(preview_data *data, GdkEventMotion *event)
{
    event_coordinate_rescale(&event->x, &event->y, data);
    int x = event->x;
    int y = event->y;
    int pixbufHeight = gdk_pixbuf_get_height(data->PreviewPixbuf);
    int pixbufWidth = gdk_pixbuf_get_width(data->PreviewPixbuf);
    int sideSizeX = MIN(16 * data->UF->rotatedWidth / pixbufWidth,
	    (CFG->CropX2-CFG->CropX1)/3);
    int sideSizeY = MIN(16 * data->UF->rotatedHeight / pixbufHeight,
	    (CFG->CropY2-CFG->CropY1)/3);

    if ( (event->state&GDK_BUTTON1_MASK)==0 ) {
	const CursorType tr_cursor [16] =
	{
	    crop_cursor, crop_cursor, crop_cursor, crop_cursor,
	    crop_cursor, top_left_cursor, left_cursor, bottom_left_cursor,
	    crop_cursor, top_cursor, move_cursor, bottom_cursor,
	    crop_cursor, top_right_cursor, right_cursor, bottom_right_cursor
	};

	int sel_cursor = 0;
	if (y >= CFG->CropY1 - 1)
	{
	    if (y < CFG->CropY1 + sideSizeY)
		sel_cursor |= 1;
	    else if (y <= CFG->CropY2 - sideSizeY)
		sel_cursor |= 2;
	    else if (y <= CFG->CropY2)
		sel_cursor |= 3;
	}
	if (x >= CFG->CropX1 - 1)
	{
	    if (x < CFG->CropX1 + sideSizeX)
		sel_cursor |= 4;
	    else if (x <= CFG->CropX2 - sideSizeX)
		sel_cursor |= 8;
	    else if (x <= CFG->CropX2)
		sel_cursor |= 12;
	}

	data->CropMotionType = tr_cursor [sel_cursor];

	GtkWidget *event_box =
	    gtk_widget_get_ancestor(data->PreviewWidget, GTK_TYPE_EVENT_BOX);
	gdk_window_set_cursor(event_box->window,
		data->Cursor[data->CropMotionType]);
    } else {
	if ( data->CropMotionType==top_cursor ||
	     data->CropMotionType==top_left_cursor ||
	     data->CropMotionType==top_right_cursor )
	    if (y<CFG->CropY2) CFG->CropY1 = y;
	if ( data->CropMotionType==bottom_cursor ||
	     data->CropMotionType==bottom_left_cursor ||
	     data->CropMotionType==bottom_right_cursor )
	    if (y>CFG->CropY1) CFG->CropY2 = y;
	if ( data->CropMotionType==left_cursor ||
	     data->CropMotionType==top_left_cursor ||
	     data->CropMotionType==bottom_left_cursor )
	    if (x<CFG->CropX2) CFG->CropX1 = x;
	if ( data->CropMotionType==right_cursor ||
	     data->CropMotionType==top_right_cursor ||
	     data->CropMotionType==bottom_right_cursor )
	    if (x>CFG->CropX1) CFG->CropX2 = x;
	if ( data->CropMotionType==move_cursor )
	{
	    int d = x - data->OldMouseX;
	    if (CFG->CropX1 + d < 0)
		d = -CFG->CropX1;
	    if (CFG->CropX2 + d >= data->UF->rotatedWidth)
		d = data->UF->rotatedWidth - CFG->CropX2;
	    CFG->CropX1 += d;
	    CFG->CropX2 += d;

	    d = y - data->OldMouseY;
	    if (CFG->CropY1 + d < 0)
		d = -CFG->CropY1;
	    if (CFG->CropY2 + d >= data->UF->rotatedHeight)
		d = data->UF->rotatedHeight - CFG->CropY2;
	    CFG->CropY1 += d;
	    CFG->CropY2 += d;
	}
	if ( data->CropMotionType!=crop_cursor )
	{
	    fix_crop_aspect(data, data->CropMotionType);
	    update_crop_ranges(data);
	    if (!CFG->LockAspect) refresh_aspect(data);
	}
    }

    data->OldMouseX = x;
    data->OldMouseY = y;

    return TRUE;
}

static gboolean preview_motion_notify_event(GtkWidget *event_box,
	GdkEventMotion *event, gpointer user_data)
{
    preview_data *data = get_preview_data(event_box);

    (void)user_data;
#ifdef HAVE_GTKIMAGEVIEW
    if (!gtk_event_box_get_above_child(GTK_EVENT_BOX(event_box))) return FALSE;
#endif
    if ( data->PageNum==data->PageNumCrop )
	return crop_motion_notify(data, event);
    if ((event->state&GDK_BUTTON1_MASK)==0) return FALSE;
    if ( !data->PreviewButtonPressed ) return FALSE;
    draw_spot(data, FALSE);
    event_coordinate_rescale(&event->x, &event->y, data);
    data->SpotX2 = event->x;
    data->SpotY2 = event->y;
    g_idle_add_full(G_PRIORITY_DEFAULT_IDLE+20,
	    (GSourceFunc)(render_spot), data, NULL);
    return TRUE;
}

#ifdef HAVE_GTKIMAGEVIEW
static gboolean (*gtk_image_view_scroll_event)(GtkWidget *widget,
	GdkEventScroll *event);

static gboolean preview_scroll_event(GtkWidget *widget, GdkEventScroll *event)
{
    // GtkImageView only knows how to handle scroll up or down
    // We also disable Ctrl+scroll which activates the zoom
    if ( ( event->direction==GDK_SCROLL_UP ||
	   event->direction==GDK_SCROLL_DOWN ) &&
	 event->state & GDK_CONTROL_MASK )
	(*gtk_image_view_scroll_event)(widget, event);
    return TRUE;
}
#endif

static void create_base_image(preview_data *data)
{
    int shrinkSave = CFG->shrink;
    int sizeSave = CFG->size;
    if (CFG->Scale==0) {
	int cropHeight = data->UF->conf->CropY2 - data->UF->conf->CropY1;
	int cropWidth = data->UF->conf->CropX2 - data->UF->conf->CropX1;
	int cropSize = MAX(cropHeight, cropWidth);
	CFG->size = CFG->Zoom / 100.0 * cropSize;
	CFG->shrink = 0;
    } else {
	CFG->size = 0;
	CFG->shrink = CFG->Scale;
    }
    preview_invalidate_layer (data, ufraw_denoise_phase);
    ufraw_convert_image_init(data->UF);
    ufraw_convert_image_first_phase(data->UF, FALSE);
    ufraw_rotate_image_buffer(&data->UF->Images[ufraw_first_phase], data->UF->conf->rotationAngle);
    CFG->shrink = shrinkSave;
    CFG->size = sizeSave;
}

static void update_shrink_ranges(preview_data *data)
{
    if (data->FreezeDialog) return;
    data->FreezeDialog++;

    int croppedWidth = CFG->CropX2 - CFG->CropX1;
    int croppedHeight = CFG->CropY2 - CFG->CropY1;
    if (CFG->size > 0) {
	if (croppedHeight > croppedWidth) {
	    data->height = CFG->size;
	    data->width = CFG->size * croppedWidth / croppedHeight;
	    data->shrink = (double)croppedHeight / CFG->size;
	} else {
	    data->width = CFG->size;
	    data->height = CFG->size * croppedHeight / croppedWidth;
	    data->shrink = (double)croppedWidth / CFG->size;
	}
    } else {
	if (CFG->shrink<1) {
	    ufraw_message(UFRAW_ERROR, _("Fatal Error: uf->conf->shrink<1"));
	    CFG->shrink = 1;
	}
	data->height = croppedHeight / CFG->shrink;
	data->width = croppedWidth / CFG->shrink;
	data->shrink = CFG->shrink;
    }
    gtk_spin_button_set_range(data->HeightSpin, 0, croppedHeight);
    gtk_spin_button_set_range(data->WidthSpin, 0, croppedWidth);
    gtk_adjustment_set_value(data->ShrinkAdjustment, data->shrink);
    gtk_adjustment_set_value(data->HeightAdjustment, data->height);
    gtk_adjustment_set_value(data->WidthAdjustment, data->width);

    data->FreezeDialog--;
}

static void update_crop_ranges(preview_data *data)
{
    if (data->FreezeDialog) return;

    /* Avoid recursive handling of the same event */
    data->FreezeDialog++;

    int CropX1 = CFG->CropX1;
    int CropX2 = CFG->CropX2;
    int CropY1 = CFG->CropY1;
    int CropY2 = CFG->CropY2;

    gtk_spin_button_set_range(data->CropX1Spin, 0, CropX2-1);
    gtk_spin_button_set_range(data->CropY1Spin, 0, CropY2-1);
    gtk_spin_button_set_range(data->CropX2Spin,
	CropX1+1, data->UF->rotatedWidth);
    gtk_spin_button_set_range(data->CropY2Spin,
	CropY1+1, data->UF->rotatedHeight);

    gtk_adjustment_set_value(data->CropX1Adjustment, CropX1);
    gtk_adjustment_set_value(data->CropY1Adjustment, CropY1);
    gtk_adjustment_set_value(data->CropX2Adjustment, CropX2);
    gtk_adjustment_set_value(data->CropY2Adjustment, CropY2);

    data->FreezeDialog--;

    int pixbufHeight = gdk_pixbuf_get_height(data->PreviewPixbuf);
    int pixbufWidth = gdk_pixbuf_get_width(data->PreviewPixbuf);
    float scale_x = ((float)pixbufWidth) / data->UF->rotatedWidth;
    float scale_y = ((float)pixbufHeight) / data->UF->rotatedHeight;

    CropX1 = floor (CropX1 * scale_x);
    CropX2 = MIN(ceil(CropX2 * scale_x), pixbufWidth-1);
    CropY1 = floor (CropY1 * scale_y);
    CropY2 = MIN(ceil(CropY2 * scale_y), pixbufHeight-1);

    int x1[4], x2[4], y1[4], y2[4], i=0;
    if (CropX1!=data->DrawnCropX1) {
	x1[i] = MIN(CropX1, data->DrawnCropX1);
	x2[i] = MAX(CropX1, data->DrawnCropX1);
	y1[i] = MIN(CropY1, data->DrawnCropY1);
	y2[i] = MAX(CropY2, data->DrawnCropY2);
	data->DrawnCropX1 = CropX1;
	i++;
    }
    if (CropX2!=data->DrawnCropX2) {
	x1[i] = MIN(CropX2, data->DrawnCropX2);
	x2[i] = MAX(CropX2, data->DrawnCropX2);
	y1[i] = MIN(CropY1, data->DrawnCropY1);
	y2[i] = MAX(CropY2, data->DrawnCropY2);
	data->DrawnCropX2 = CropX2;
	i++;
    }
    if (CropY1!=data->DrawnCropY1) {
	y1[i] = MIN(CropY1, data->DrawnCropY1);
	y2[i] = MAX(CropY1, data->DrawnCropY1);
	x1[i] = MIN(CropX1, data->DrawnCropX1);
	x2[i] = MAX(CropX2, data->DrawnCropX2);
	data->DrawnCropY1 = CropY1;
	i++;
    }
    if (CropY2!=data->DrawnCropY2) {
	y1[i] = MIN(CropY2, data->DrawnCropY2);
	y2[i] = MAX(CropY2, data->DrawnCropY2);
	x1[i] = MIN(CropX1, data->DrawnCropX1);
	x2[i] = MAX(CropX2, data->DrawnCropX2);
	data->DrawnCropY2 = CropY2;
	i++;
    }
    for (i--; i>=0; i--) {
	x1[i] = MAX (x1[i] - 1, 0);
	x2[i] = MIN (x2[i] + 1, pixbufWidth);
	y1[i] = MAX (y1[i] - 1, 0);
	y2[i] = MIN (y2[i] + 1, pixbufHeight);
	preview_draw_area (data, x1[i], y1[i], x2[i]-x1[i], y2[i]-y1[i]);
    }
    update_shrink_ranges(data);
    redraw_navigation_image(data);
}

static void crop_reset(GtkWidget *widget, gpointer user_data)
{
    preview_data *data = get_preview_data(widget);
    user_data = user_data;

    CFG->CropX1 = 0;
    CFG->CropY1 = 0;
    CFG->CropX2 = data->UF->rotatedWidth;
    CFG->CropY2 = data->UF->rotatedHeight;
    data->AspectRatio = ((float)data->UF->rotatedWidth) / data->UF->rotatedHeight;

    refresh_aspect(data);
    set_new_aspect(data);
}

static void zoom_in_event(GtkWidget *widget, gpointer user_data)
{
    preview_data *data = get_preview_data(widget);
    const double prev_zoom = CFG->Zoom;
    (void)user_data;
    if (data->FreezeDialog) return;
    if (CFG->Scale==0) {
	if (CFG->Zoom<100.0/max_scale) CFG->Zoom = 100.0/max_scale;
	CFG->Scale = floor(100.0/CFG->Zoom);
	if (CFG->Scale==100.0/CFG->Zoom) CFG->Scale--;
    } else {
	CFG->Scale--;
    }
    CFG->Scale = LIM(CFG->Scale, min_scale, max_scale);
    CFG->Zoom = 100.0/CFG->Scale;
    if (prev_zoom != CFG->Zoom) {
	create_base_image(data);
	gtk_adjustment_set_value(data->ZoomAdjustment, CFG->Zoom);
	render_preview(data);
    }
}

static void zoom_out_event(GtkWidget *widget, gpointer user_data)
{
    preview_data *data = get_preview_data(widget);
    const double prev_zoom = CFG->Zoom;
    (void)user_data;
    if (data->FreezeDialog) return;
    if (CFG->Scale==0) {
	if (CFG->Zoom<100.0/max_scale) CFG->Zoom = 100.0/max_scale;
	CFG->Scale = ceil(100.0/CFG->Zoom);
	if (CFG->Scale==100.0/CFG->Zoom) CFG->Scale++;
    } else {
	CFG->Scale++;
    }
    CFG->Scale = LIM(CFG->Scale, min_scale, max_scale);
    CFG->Zoom = 100.0/CFG->Scale;
    if (prev_zoom != CFG->Zoom) {
	create_base_image(data);
	gtk_adjustment_set_value(data->ZoomAdjustment, CFG->Zoom);
	render_preview(data);
    }
}

#ifdef HAVE_GTKIMAGEVIEW
static void zoom_fit_event(GtkWidget *widget, gpointer user_data)
{
    preview_data *data = get_preview_data(widget);
    const double prev_zoom = CFG->Zoom;
    (void)user_data;
    if (data->FreezeDialog) return;
    GtkWidget *preview =
	gtk_widget_get_ancestor(data->PreviewWidget, GTK_TYPE_IMAGE_SCROLL_WIN);
    int previewWidth = preview->allocation.width;
    int previewHeight = preview->allocation.height;
    double wScale = (double)data->UF->rotatedWidth / previewWidth;
    double hScale = (double)data->UF->rotatedHeight / previewHeight;
    CFG->Zoom = 100.0/LIM(MAX(wScale, hScale), min_scale, max_scale);
    CFG->Scale = 0;
    if (prev_zoom != CFG->Zoom) {
	create_base_image(data);
	gtk_adjustment_set_value(data->ZoomAdjustment, CFG->Zoom);
	render_preview(data);
    }
}

static void zoom_max_event(GtkWidget *widget, gpointer user_data)
{
    preview_data *data = get_preview_data(widget);
    const double prev_zoom = CFG->Zoom;
    (void)user_data;
    if (data->FreezeDialog) return;
    CFG->Scale = min_scale;
    CFG->Zoom = 100.0/CFG->Scale;
    if (prev_zoom != CFG->Zoom) {
	create_base_image(data);
	gtk_adjustment_set_value(data->ZoomAdjustment, CFG->Zoom);
	render_preview(data);
    }
}
#endif

static void flip_image(GtkWidget *widget, int flip)
{
    int temp;
    preview_data *data = get_preview_data(widget);

    if (data->FreezeDialog) return;
    ufraw_flip_image(data->UF, flip);

    if (flip & 1) {
	temp = CFG->CropX1;
	CFG->CropX1 = data->UF->rotatedWidth - CFG->CropX2;
	CFG->CropX2 = data->UF->rotatedWidth - temp;
	if ( data->SpotX1>=0 ) {
	    temp = data->SpotX1;
	    data->SpotX1 = data->UF->rotatedWidth - data->SpotX2;
	    data->SpotX2 = data->UF->rotatedWidth - temp;
	}
    }
    if (flip & 2) {
	temp = CFG->CropY1;
	CFG->CropY1 = data->UF->rotatedHeight - CFG->CropY2;
	CFG->CropY2 = data->UF->rotatedHeight - temp;
	if ( data->SpotX1>=0 ) {
	    temp = data->SpotY1;
	    data->SpotY1 = data->UF->rotatedHeight - data->SpotY2;
	    data->SpotY2 = data->UF->rotatedHeight - temp;
	}
    }
    if (flip & 4) {
	temp = data->UF->rotatedWidth;
	data->UF->rotatedWidth = data->UF->rotatedHeight;
	data->UF->rotatedHeight = temp;
	temp = data->UF->initialWidth;
	data->UF->initialWidth = data->UF->initialHeight;
	data->UF->initialHeight = temp;
	temp = CFG->CropX1;
	CFG->CropX1 = CFG->CropY1;
	CFG->CropY1 = temp;
	temp = CFG->CropX2;
	CFG->CropX2 = CFG->CropY2;
	CFG->CropY2 = temp;
	if ( data->SpotX1>=0 ) {
	    temp = data->SpotX1;
	    data->SpotX1 = data->SpotY1;
	    data->SpotY1 = temp;
	    temp = data->SpotX2;
	    data->SpotX2 = data->SpotY2;
	    data->SpotY2 = temp;
	}
    }
    ++data->FreezeDialog;
    switch (flip) {
	case 6:
	    gtk_adjustment_set_value(data->RotationAdjustment,
		    data->unnormalized_angle + 90);
	    break;
	case 5:
	    gtk_adjustment_set_value(data->RotationAdjustment,
		    data->unnormalized_angle - 90);
	    break;
	case 1:
	case 2:
	    data->reference_orientation ^= flip;
	    gtk_adjustment_set_value(data->RotationAdjustment,
		    -data->unnormalized_angle);
	    break;
    }
    --data->FreezeDialog;
    render_init(data);
    if ( data->RenderSubArea >= 0 ) {
	/* We are in the middle or a rendering scan,
	 * so just start from the beginning. */
	data->RenderSubArea = 0;
    } else {
	/* Full image was already rendered.
	 * We only need to draw the flipped image. */
	render_special_mode(widget, render_default);
    }
    update_crop_ranges(data);
    refresh_aspect(data);
}

/*
 * Used to offer choices of aspect ratios, and to show numbers
 * as ratios.
 */
static const struct
{
    float val;
    const char text[5];
} predef_aspects [] =
{
    { 21.0/ 9.0, "21:9"  },
    { 16.0/10.0, "16:10" },
    { 16.0/ 9.0, "16:9"  },
    {  3.0/ 2.0,  "3:2"  },
    {  7.0/ 5.0,  "7:5"  },
    {  4.0/ 3.0,  "4:3"  },
    {  5.0/ 4.0,  "5:4"  },
    {  1.0/ 1.0,  "1:1"  },
    {  4.0/ 5.0,  "4:5"  },
    {  3.0/ 4.0,  "3:4"  },
    {  5.0/ 7.0,  "5:7"  },
    {  2.0/ 3.0,  "2:3"  },
};

static void refresh_aspect(preview_data *data)
{
    int dy = CFG->CropY2 - CFG->CropY1;
    data->AspectRatio = dy ? ((CFG->CropX2 - CFG->CropX1) / (float)dy) : 1.0;

    // Look through a predefined list of aspect ratios
    size_t i;
    for (i = 0; i < sizeof (predef_aspects) / sizeof (predef_aspects [0]); i++)
	if (data->AspectRatio >= predef_aspects [i].val * 0.999 &&
	    data->AspectRatio <= predef_aspects [i].val * 1.001)
	{
	    data->FreezeDialog++;
	    gtk_entry_set_text (data->AspectEntry, predef_aspects [i].text);
	    data->FreezeDialog--;
	    return;
	}
    char *text = g_strdup_printf ("%.4g", data->AspectRatio);
    data->FreezeDialog++;
    gtk_entry_set_text (data->AspectEntry, text);
    data->FreezeDialog--;
    g_free(text);
}

static void fix_crop_aspect(preview_data *data, CursorType cursor)
{
    float aspect;

    /* Sanity checks first */
    if (CFG->CropX2 < CFG->CropX1)
	CFG->CropX2 = CFG->CropX1;
    if (CFG->CropY2 < CFG->CropY1)
	CFG->CropY2 = CFG->CropY1;
    if (CFG->CropX1 < 0)
	CFG->CropX1 = 0;
    if (CFG->CropX2 > data->UF->rotatedWidth)
	CFG->CropX2 = data->UF->rotatedWidth;
    if (CFG->CropY1 < 0)
	CFG->CropY1 = 0;
    if (CFG->CropY2 > data->UF->rotatedHeight)
	CFG->CropY2 = data->UF->rotatedHeight;

    if (!CFG->LockAspect)
	return;

    if (data->AspectRatio == 0)
	aspect = ((float)data->UF->rotatedWidth) / data->UF->rotatedHeight;
    else
	aspect = data->AspectRatio;

    switch (cursor)
    {
	case left_cursor:
	case right_cursor:
	    {
		/* Try to expand the rectangle evenly vertically,
		 * if space permits */
		float cy = (CFG->CropY1 + CFG->CropY2) / 2.0;
		float dy = (CFG->CropX2 - CFG->CropX1) * 0.5 / aspect;
		int fix_dx = 0;
		if (dy > cy)
		    dy = cy, fix_dx++;
		if (cy + dy > data->UF->rotatedHeight)
		    dy = data->UF->rotatedHeight - cy, fix_dx++;
		if (fix_dx)
		{
		    float dx = rint (dy * 2.0 * aspect);
		    if (cursor == left_cursor)
			CFG->CropX1 = CFG->CropX2 - dx;
		    else
			CFG->CropX2 = CFG->CropX1 + dx;
		}
		CFG->CropY1 = rint (cy - dy);
		CFG->CropY2 = rint (cy + dy);
	    }
	    break;
	case top_cursor:
	case bottom_cursor:
	    {
		/* Try to expand the rectangle evenly horizontally,
		 * if space permits */
		float cx = (CFG->CropX1 + CFG->CropX2) / 2.0;
		float dx = (CFG->CropY2 - CFG->CropY1) * 0.5 * aspect;
		int fix_dy = 0;
		if (dx > cx)
		    dx = cx, fix_dy++;
		if (cx + dx > data->UF->rotatedWidth)
		    dx = data->UF->rotatedWidth - cx, fix_dy++;
		if (fix_dy)
		{
		    float dy = rint (dx * 2.0 / aspect);
		    if (cursor == top_cursor)
			CFG->CropY1 = CFG->CropY2 - dy;
		    else
			CFG->CropY2 = CFG->CropY1 + dy;
		}
		CFG->CropX1 = rint(cx - dx);
		CFG->CropX2 = rint(cx + dx);
	    }
	    break;

	case top_left_cursor:
	case top_right_cursor:
	case bottom_left_cursor:
	case bottom_right_cursor:
	    {
		/* Adjust rectangle width/height according to aspect ratio
		 * trying to preserve the area of the crop rectangle.
		 * See the comment in set_new_aspect() */
		float dy, dx = sqrt(aspect * (CFG->CropX2 - CFG->CropX1) *
				    (CFG->CropY2 - CFG->CropY1));

		for (;;)
		{
		    if (cursor == top_left_cursor ||
			cursor == bottom_left_cursor)
		    {
			if (CFG->CropX2 < dx)
			    dx = CFG->CropX2;
			CFG->CropX1 = CFG->CropX2 - dx;
		    }
		    else
		    {
			if (CFG->CropX1 + dx > data->UF->rotatedWidth)
			    dx = data->UF->rotatedWidth - CFG->CropX1;
			CFG->CropX2 = CFG->CropX1 + dx;
		    }

		    dy = dx / aspect;

		    if (cursor == top_left_cursor ||
			cursor == top_right_cursor)
		    {
			if (CFG->CropY2 < dy)
			{
			    dx = CFG->CropY2 * aspect;
			    continue;
			}
			CFG->CropY1 = CFG->CropY2 - dy;
		    }
		    else
		    {
			if (CFG->CropY1 + dy > data->UF->rotatedHeight)
			{
			    dx = (data->UF->rotatedHeight - CFG->CropY1) * aspect;
			    continue;
			}
			CFG->CropY2 = CFG->CropY1 + dy;
		    }
		    break;
		}
	    }
	    break;

	default:
	    return;
    }

    update_crop_ranges(data);
}

/* Modify current crop area so that it fits current aspect ratio */
static void set_new_aspect(preview_data *data)
{
    float cx, cy, dx, dy;

    /* Crop area center never changes */
    cx = (CFG->CropX1 + CFG->CropX2) / 2.0;
    cy = (CFG->CropY1 + CFG->CropY2) / 2.0;

    /* Adjust the current crop area width/height taking into account
     * the new aspect ratio. The rule is to keep one of the dimensions
     * and modify other dimension in such a way that the area of the
     * new crop area will be maximal.
     */
    dx = CFG->CropX2 - cx;
    dy = CFG->CropY2 - cy;
    if (dx / dy > data->AspectRatio)
	dy = dx / data->AspectRatio;
    else
	dx = dy * data->AspectRatio;

    if (dx > cx)
    {
	dx = cx;
	dy = dx / data->AspectRatio;
    }
    if (cx + dx > data->UF->rotatedWidth)
    {
	dx = data->UF->rotatedWidth - cx;
	dy = dx / data->AspectRatio;
    }

    if (dy > cy)
    {
	dy = cy;
	dx = dy * data->AspectRatio;
    }
    if (cy + dy > data->UF->rotatedHeight)
    {
	dy = data->UF->rotatedHeight - cy;
	dx = dy * data->AspectRatio;
    }

    CFG->CropX1 = rint (cx - dx);
    CFG->CropX2 = rint (cx + dx);
    CFG->CropY1 = rint (cy - dy);
    CFG->CropY2 = rint (cy + dy);

    update_crop_ranges(data);
}

static void lock_aspect(GtkWidget *widget)
{
    preview_data *data = get_preview_data(widget);
    CFG->LockAspect = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void aspect_changed(GtkWidget *widget, gpointer user_data)
{
    preview_data *data = get_preview_data(widget);
    if (data->FreezeDialog) return;
    (void)user_data;
    const gchar *text = gtk_entry_get_text(data->AspectEntry);
    if (text)
    {
	float aspect = 0.0, aspect_div, aspect_quot;
	if (sscanf(text, "%f : %f", &aspect_div, &aspect_quot) == 2)
	{
	    if (aspect_quot)
		aspect = aspect_div / aspect_quot;
	}
	else
	    sscanf(text, "%g", &aspect);

	if (aspect >= 0.1 && aspect <= 10.0)
	    data->AspectRatio = aspect;
    }
    set_new_aspect (data);
    CFG->LockAspect = TRUE;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->LockAspectButton), TRUE);
}

static void set_darkframe_label(preview_data *data)
{
    if (CFG->darkframeFile[0] != '\0') {
	char *basename = g_path_get_basename(CFG->darkframeFile);
	gtk_label_set_text(GTK_LABEL(data->DarkFrameLabel), basename);
	g_free(basename);
    } else {
	// No darkframe file
	gtk_label_set_text(GTK_LABEL(data->DarkFrameLabel), _("None"));
    }
}

static void set_darkframe(preview_data *data)
{
    set_darkframe_label(data);
    create_base_image(data);
    render_preview(data);
}

static void load_darkframe(GtkWidget *widget, void *unused)
{
    preview_data *data = get_preview_data(widget);
    GtkFileChooser *fileChooser;
    char *basedir;

    if (data->FreezeDialog) return;
    basedir = g_path_get_dirname(CFG->darkframeFile);
    fileChooser = ufraw_raw_chooser(CFG,
				    basedir,
				    _("Load dark frame"),
				    GTK_WINDOW(gtk_widget_get_toplevel(widget)),
				    GTK_STOCK_CANCEL,
				    FALSE);
    free(basedir);

    if (gtk_dialog_run(GTK_DIALOG(fileChooser))==GTK_RESPONSE_ACCEPT) {
	char *filename = gtk_file_chooser_get_filename(fileChooser);
	g_strlcpy(CFG->darkframeFile, filename, max_path);
	g_free(filename);
	ufraw_load_darkframe(data->UF);
	set_darkframe(data);
    }
    ufraw_focus(fileChooser, FALSE);
    gtk_widget_destroy(GTK_WIDGET(fileChooser));
    (void)unused;
}

static void reset_darkframe(GtkWidget *widget, void *unused)
{
    preview_data *data = get_preview_data(widget);

    if (data->FreezeDialog) return;
    if (CFG->darkframe == NULL) return;

    ufraw_close(CFG->darkframe);
    g_free(CFG->darkframe);
    CFG->darkframe = NULL;
    CFG->darkframeFile[0] = '\0';

    set_darkframe(data);
    (void)unused;
}

GtkWidget *notebook_page_new(GtkNotebook *notebook, char *text, char *icon)
{
    GtkWidget *page = gtk_vbox_new(FALSE, 0);
    if ( icon==NULL ) {
	GtkWidget *label = gtk_label_new(text);
	gtk_notebook_append_page(notebook, GTK_WIDGET(page), label);
    } else {
	GtkWidget *event_box = gtk_event_box_new();
	GtkWidget *image = gtk_image_new_from_stock(icon,
		GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_container_add(GTK_CONTAINER(event_box), image);
	gtk_widget_show_all(event_box);
	gtk_notebook_append_page(notebook, GTK_WIDGET(page), event_box);
	uf_widget_set_tooltip(event_box, text);
    }
    return page;
}

static void expander_expanded(GtkExpander *expander,
    GParamSpec *param_spec, gpointer user_data)
{
    (void)param_spec;
    (void)user_data;
    preview_data *data = get_preview_data(expander);

    GtkWidget *panel = gtk_widget_get_parent(GTK_WIDGET(expander));
    if ( gtk_expander_get_expanded(expander) ) {
	GtkWidget *histogram =
		g_object_get_data(G_OBJECT(expander), "expander-histogram");
	if ( histogram!=NULL ) {
	    g_object_set_data(G_OBJECT(expander),
		    "expander-maximized", (gpointer)FALSE);
	    gtk_widget_set_size_request(histogram, -1, data->HisMinHeight);
	}
	gboolean expanderMaximized = GPOINTER_TO_INT(
		g_object_get_data(G_OBJECT(expander), "expander-maximized"));
	if ( !expanderMaximized )
	    gtk_box_set_child_packing(GTK_BOX(panel), GTK_WIDGET(expander),
		    TRUE, TRUE, 0, GTK_PACK_START);
    } else {
	gtk_box_set_child_packing(GTK_BOX(panel), GTK_WIDGET(expander),
	    FALSE, FALSE, 0, GTK_PACK_START);
    }
}

GtkWidget *table_with_frame(GtkWidget *box, char *label, gboolean expand)
{
    GtkWidget *frame = gtk_frame_new(NULL);
    if (label!=NULL) {
	GtkWidget *expander = gtk_expander_new(label);
	gtk_expander_set_expanded(GTK_EXPANDER(expander), expand);
	gtk_box_pack_start(GTK_BOX(box), expander, TRUE, TRUE, 0);
	g_signal_connect (expander, "notify::expanded",
		G_CALLBACK (expander_expanded), NULL);
	gtk_container_add(GTK_CONTAINER(expander), frame);
    } else {
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);
    }
    GtkWidget *table = gtk_table_new(10, 10, FALSE);
    gtk_container_add(GTK_CONTAINER(frame), table);
    return table;
}

static void button_update(GtkWidget *button, gpointer user_data)
{
    preview_data *data = get_preview_data(button);
    user_data = user_data;
    int i;

    if (button==data->ResetWBButton) {
	int c;
	g_strlcpy(CFG->wb, data->initialWB, max_name);
	CFG->temperature = data->initialTemperature;
	CFG->green = data->initialGreen;
	for (c=0; c<4; c++) CFG->chanMul[c] = data->initialChanMul[c];
    }
    if (button==data->ResetGammaButton) {
	CFG->profile[0][CFG->profileIndex[0]].gamma =
		profile_default_gamma(&CFG->profile[0][CFG->profileIndex[0]]);
    }
    if (button==data->ResetLinearButton) {
	CFG->profile[0][CFG->profileIndex[0]].linear =
		profile_default_linear(&CFG->profile[0][CFG->profileIndex[0]]);
    }
    if (button==GTK_WIDGET(data->AutoExposureButton)) {
	CFG->autoExposure =
	    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
    }
    if (button==data->ResetExposureButton) {
	CFG->exposure = conf_default.exposure;
	CFG->autoExposure = FALSE;
    }
    if (button==data->ResetThresholdButton) {
	CFG->threshold = conf_default.threshold;
    }
#ifdef UFRAW_CONTRAST
    if (button==data->ResetContrastButton) {
        CFG->contrast = conf_default.contrast;
    }
#endif
    if (button==data->ResetSaturationButton) {
	CFG->saturation = conf_default.saturation;
    }
    if (button==GTK_WIDGET(data->AutoBlackButton)) {
	CFG->autoBlack =
	    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
    }
    if (button==data->ResetBlackButton) {
	CurveDataSetPoint(&CFG->curve[CFG->curveIndex], 0,
		conf_default.black, 0);
	CFG->autoBlack = FALSE;
    }
    if (button==data->AutoCurveButton) {
	CFG->curveIndex = manual_curve;
	ufraw_auto_curve(data->UF);
    }
    if (button==data->ResetBaseCurveButton) {
	if (CFG->BaseCurveIndex==manual_curve) {
	    CFG->BaseCurve[CFG->BaseCurveIndex].m_numAnchors = 2;
	    CFG->BaseCurve[CFG->BaseCurveIndex].m_anchors[0].x = 0.0;
	    CFG->BaseCurve[CFG->BaseCurveIndex].m_anchors[0].y = 0.0;
	    CFG->BaseCurve[CFG->BaseCurveIndex].m_anchors[1].x = 1.0;
	    CFG->BaseCurve[CFG->BaseCurveIndex].m_anchors[1].y = 1.0;
	} else {
	    CFG->BaseCurveIndex = linear_curve;
	}
	curveeditor_widget_set_curve(data->BaseCurveWidget,
		&CFG->BaseCurve[CFG->BaseCurveIndex]);
    }
    if (button==data->ResetCurveButton) {
	if (CFG->curveIndex==manual_curve) {
	    CFG->curve[CFG->curveIndex].m_numAnchors = 2;
	    CFG->curve[CFG->curveIndex].m_anchors[1].x = 1.0;
	    CFG->curve[CFG->curveIndex].m_anchors[1].y = 1.0;
	} else {
	    CFG->curveIndex = linear_curve;
	}
    }
    if (button==data->ResetGrayscaleChannelMixerButton) {
        CFG->grayscaleMixer[0] = conf_default.grayscaleMixer[0];
        CFG->grayscaleMixer[1] = conf_default.grayscaleMixer[1];
        CFG->grayscaleMixer[2] = conf_default.grayscaleMixer[2];
    }
    for (i = 0; i < max_adjustments; ++i) {
	if (button == data->ResetLightnessAdjustmentButton[i]) {
	    CFG->lightnessAdjustment[i].adjustment = 1.0;
	    break;
	}
    }
    if (CFG->autoExposure==enabled_state) CFG->autoExposure = apply_state;
    if (CFG->autoBlack==enabled_state) CFG->autoBlack = apply_state;
    preview_invalidate_layer (data, ufraw_develop_phase);
    update_scales(data);
    return;
}

static void grayscale_update(GtkWidget *button, gpointer user_data)
{
  int i;
  preview_data *data = get_preview_data(button);

  for (i = 0; i < 6; ++i)
      if (button == data->GrayscaleButtons[i]
	  && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
	  CFG->grayscaleMode = i;

  update_scales(data);
  preview_invalidate_layer (data, ufraw_develop_phase);
  (void)user_data;
}

static void restore_details_button_set(GtkButton *button, preview_data *data)
{
    const char *state;
    switch (CFG->restoreDetails) {
    case clip_details:
	uf_button_set_stock_image(button, GTK_STOCK_CUT);
	state = _("clip");
	break;
    case restore_lch_details:
	uf_button_set_stock_image(button, "restore-highlights-lch");
	state = _("restore in LCH space for soft details");
	break;
    case restore_hsv_details:
	uf_button_set_stock_image(button, "restore-highlights-hsv");
	state = _("restore in HSV space for sharp details");
	break;
    default:
	state = "Error";
    }
    char *text = g_strdup_printf(_("Restore details for negative EV\n"
	    "Current state: %s"), state);
    uf_widget_set_tooltip(GTK_WIDGET(button), text);
    g_free(text);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
}

static void clip_highlights_button_set(GtkButton *button, preview_data *data)
{
    const char *state;
    switch (CFG->clipHighlights) {
    case digital_highlights:
	uf_button_set_stock_image(button, "clip-highlights-digital");
	state = _("digital linear");
	break;
    case film_highlights:
	uf_button_set_stock_image(button, "clip-highlights-film");
	state = _("soft film like");
	break;
    default:
	state = "Error";
    }
    char *text = g_strdup_printf(_("Clip highlights for positive EV\n"
	    "Current state: %s"), state);
    uf_widget_set_tooltip(GTK_WIDGET(button), text);
    g_free(text);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
}

static void toggle_button_update(GtkToggleButton *button, gboolean *valuep)
{
    preview_data *data = get_preview_data(button);
    if (valuep==&CFG->restoreDetails) {
	/* Our untoggling of the button creates a redundant event. */
	if ( gtk_toggle_button_get_active(button) ) {
	    /* Scroll through the settings. */
	    CFG->restoreDetails =
		    (CFG->restoreDetails+1) % restore_types;
	    restore_details_button_set(GTK_BUTTON(button), data);
            preview_invalidate_layer (data, ufraw_develop_phase);
	    update_scales(data);
	}
    } else if (valuep==&CFG->clipHighlights) {
	/* Our untoggling of the button creates a redundant event. */
	if ( gtk_toggle_button_get_active(button) ) {
	    /* Scroll through the settings. */
	    CFG->clipHighlights =
		    (CFG->clipHighlights+1) % highlights_types;
	    clip_highlights_button_set(GTK_BUTTON(button), data);
            preview_invalidate_layer (data, ufraw_develop_phase);
	    update_scales(data);
	}
    } else {
	*valuep = gtk_toggle_button_get_active(button);
	if ( valuep==&CFG->overExp || valuep==&CFG->underExp ) {
	    start_blink(data);
	    switch_highlights(data);
	} else {
            preview_invalidate_layer (data, ufraw_develop_phase);
	    render_preview(data);
	}
    }
}

static void toggle_button(GtkTable *table, int x, int y, char *label,
	gboolean *valuep)
{
    GtkWidget *widget, *align;

    widget = gtk_check_button_new_with_label(label);
    if (label==NULL) {
	align = gtk_alignment_new(1, 0, 0, 1);
	gtk_container_add(GTK_CONTAINER(align), widget);
    } else align = widget;
    gtk_table_attach(table, align, x, x+1, y, y+1, 0, 0, 0, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), *valuep);
    g_signal_connect(G_OBJECT(widget), "toggled",
	    G_CALLBACK(toggle_button_update), valuep);
}

static void adjustment_update_int(GtkAdjustment *adj, int *valuep)
{
    *valuep = (int)floor(gtk_adjustment_get_value(adj) + 0.5);
} 

static void adjustment_update(GtkAdjustment *adj, double *valuep)
{
    preview_data *data = get_preview_data(adj);
    if (data->FreezeDialog) return;

    if (valuep==&CFG->profile[0][0].gamma)
	valuep = (void *)&CFG->profile[0][CFG->profileIndex[0]].gamma;
    if (valuep==&CFG->profile[0][0].linear)
	valuep = (void *)&CFG->profile[0][CFG->profileIndex[0]].linear;

    if ( (int *)valuep==&CFG->CropX1 || (int *)valuep==&CFG->CropX2 ||
	 (int *)valuep==&CFG->CropY1 || (int *)valuep==&CFG->CropY2 ) {
	*((int *)valuep) = (int) gtk_adjustment_get_value(adj);
	CursorType cursor = left_cursor;
	if ( (int *)valuep==&CFG->CropX1) cursor = left_cursor;
	if ( (int *)valuep==&CFG->CropX2) cursor = right_cursor;
	if ( (int *)valuep==&CFG->CropY1) cursor = top_cursor;
	if ( (int *)valuep==&CFG->CropY2) cursor = bottom_cursor;
	fix_crop_aspect(data, cursor);
	update_crop_ranges(data);
	if (!CFG->LockAspect) refresh_aspect(data);
	return;
    }
    /* Do nothing if value didn't really change */
    long accuracy =
	(long)g_object_get_data(G_OBJECT(adj), "Adjustment-Accuracy");
    float change = fabs(*valuep-gtk_adjustment_get_value(adj));
    float min_change = pow(10,-accuracy)/2;
    if ( change < min_change )
	return;
    else
	*valuep = gtk_adjustment_get_value(adj);

    if (valuep==&CFG->temperature || valuep==&CFG->green)
    {
	g_strlcpy(CFG->wb, manual_wb, max_name);
	ufraw_set_wb(data->UF);
    }
    else if (valuep>=&CFG->chanMul[0] && valuep<=&CFG->chanMul[3])
    {
	g_strlcpy(CFG->wb, spot_wb, max_name);
	ufraw_set_wb(data->UF);
    }
    else if (valuep==&CFG->WBTuning)
    {
        if (strcmp(data->UF->conf->wb, auto_wb)==0 ||
            strcmp(data->UF->conf->wb, camera_wb)==0)
            /* Prevent recalculation of Camera/Auto WB on WBTuning events */
            data->UF->conf->WBTuning = 0;
        else
	    ufraw_set_wb(data->UF);
    }
    else if (valuep==&CFG->exposure)
    {
        CFG->autoExposure = FALSE;
        if (CFG->autoBlack==enabled_state) CFG->autoBlack = apply_state;
    }
    else if (valuep==&CFG->Zoom)
    {
	CFG->Scale = 0;
	create_base_image(data);
    }
    else if (valuep==&CFG->threshold)
        preview_invalidate_layer (data, ufraw_denoise_phase);
    else
    {
        if (CFG->autoExposure==enabled_state) CFG->autoExposure = apply_state;
        if (CFG->autoBlack==enabled_state) CFG->autoBlack = apply_state;
    }

    int croppedWidth = CFG->CropX2 - CFG->CropX1;
    int croppedHeight = CFG->CropY2 - CFG->CropY1;
    if (valuep==&data->shrink)
    {
        data->height = croppedHeight / data->shrink;
        data->width = croppedWidth / data->shrink;
    }
    if (valuep==&data->height)
    {
        data->width = data->height * croppedWidth / croppedHeight;
        data->shrink = (double)croppedHeight / data->height;
    }
    if (valuep==&data->width)
    {
        data->height = data->width * croppedHeight / croppedWidth;
        data->shrink = (double)croppedWidth / data->width;
    }

    preview_invalidate_layer (data, ufraw_develop_phase);
    update_scales (data);
}

GtkWidget *stock_image_button(const gchar *stock_id, GtkIconSize size,
			      const char *tip, GCallback callback, void *data)
{
    GtkWidget *button;
    button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(button),
	gtk_image_new_from_stock(stock_id, size));
    if (tip != NULL)
	uf_widget_set_tooltip(button, tip);
    g_signal_connect(G_OBJECT(button), "clicked", callback, data);
    return button;
}

GtkWidget *stock_icon_button(const gchar *stock_id,
			     const char *tip, GCallback callback, void *data)
{
    return stock_image_button(stock_id, GTK_ICON_SIZE_BUTTON,
			      tip, callback, data);
}

/*
 * Crop and spot area cannot be rotated but we make sure their coordinates
 * are valid. Try to preserve them over a rotate forth and back and try to
 * preserve their geometry.
 */
static void set_rotation_angle(preview_data *data, double angle)
{
    int d;

    if (data->FreezeDialog)
	return;

    /* Normalize the "unnormalized" value displayed to the user to
     * -180 < a <= 180, though we later normalize to an orientation
     * and flip plus 0 <= a < 90 rotation for processing.  */
    data->unnormalized_angle = remainder(angle, 360);
    gtk_adjustment_set_value(data->RotationAdjustment,
			     data->unnormalized_angle);
    CFG->rotationAngle = data->unnormalized_angle;
    CFG->orientation = data->reference_orientation;
    ufraw_normalize_rotation(data->UF);
    ufraw_get_image_dimensions(data->UF->raw, data->UF);
    d = MIN(CFG->CropX1, CFG->CropX2 - data->UF->rotatedWidth);
    if (d > 0) {
	CFG->CropX1 -= d;
	CFG->CropX2 -= d;
    }
    d = MIN(CFG->CropY1, CFG->CropY2 - data->UF->rotatedHeight);
    if (d > 0) {
	CFG->CropY1 -= d;
	CFG->CropY2 -= d;
    }
    d = MIN(data->SpotX1, data->SpotX2 - data->UF->rotatedWidth);
    if (d > 0) {
	data->SpotX1 -= d;
	data->SpotX2 -= d;
    }
    d = MIN(data->SpotY1, data->SpotY2 - data->UF->rotatedHeight);
    if (d > 0) {
	data->SpotY1 -= d;
	data->SpotY2 -= d;
    }
    if (data->SpotX2 > data->UF->rotatedWidth ||
	data->SpotY2 > data->UF->rotatedHeight) {
	data->SpotDraw = FALSE;
	data->SpotX1 = -1;
	data->SpotX2 = -1;
	data->SpotY1 = -1;
	data->SpotY2 = -1;
    }
    create_base_image(data);
    if (CFG->CropX2 > data->UF->rotatedWidth)
	fix_crop_aspect(data, top_right_cursor);
    else if (CFG->CropY2 > data->UF->rotatedHeight)
	fix_crop_aspect(data, bottom_left_cursor);
    update_crop_ranges(data);
    if (!CFG->LockAspect)
	refresh_aspect(data);
    update_scales(data);
}

static void adjustment_update_rotation(GtkAdjustment *adj, gpointer user_data)
{
    preview_data *data = get_preview_data(adj);
    set_rotation_angle(data, gtk_adjustment_get_value(adj));
    (void)user_data;
}

static void adjustment_reset_rotation(GtkAdjustment *adj, gpointer user_data)
{
    preview_data *data = get_preview_data(adj);
    set_rotation_angle(data, 0);
    (void)user_data;
}

GtkWidget *reset_button(const char *tip, GCallback callback, void *data)
{
    return stock_icon_button(GTK_STOCK_REFRESH, tip, callback, data);
}

GtkAdjustment *adjustment_scale(GtkTable *table, int x, int y, const char *label,
    double value, void *valuep, double min, double max, double step, double jump,
    long accuracy, const char *tip, GCallback callback,
    GtkWidget **resetButton, const char *resetTip, GCallback resetCallback)
{
    GtkAdjustment *adj;
    GtkWidget *w, *l, *icon;

    if ( label!=NULL ) {
	w = gtk_event_box_new();
	if (label [0] == '@') {
	    icon = gtk_image_new_from_stock(label + 1, GTK_ICON_SIZE_LARGE_TOOLBAR);
	    gtk_container_add(GTK_CONTAINER(w), icon);
	} else {
	    l = gtk_label_new(label);
	    gtk_misc_set_alignment(GTK_MISC(l), 1, 0.5);
	    gtk_container_add(GTK_CONTAINER(w), l);
	}
	gtk_table_attach(table, w, x, x+1, y, y+1, GTK_SHRINK|GTK_FILL, 0, 0, 0);
	uf_widget_set_tooltip(w, tip);
    }
    adj = GTK_ADJUSTMENT(gtk_adjustment_new(value, min, max, step, jump, 0));
    g_object_set_data(G_OBJECT(adj), "Adjustment-Accuracy",(gpointer)accuracy);

    w = gtk_hscale_new(adj);
    g_object_set_data(G_OBJECT(adj), "Parent-Widget", w);
    gtk_scale_set_draw_value(GTK_SCALE(w), FALSE);
    gtk_table_attach(table, w, x+1, x+5, y, y+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    uf_widget_set_tooltip(w, tip);
    g_signal_connect(G_OBJECT(adj), "value-changed", callback, valuep);

    w = gtk_spin_button_new(adj, step, accuracy);
    gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(w), FALSE);
    gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(w), GTK_UPDATE_IF_VALID);
    gtk_table_attach(table, w, x+5, x+7, y, y+1, GTK_SHRINK|GTK_FILL, 0, 0, 0);
    uf_widget_set_tooltip(w, tip);

    if (resetButton != NULL) {
	*resetButton = reset_button(resetTip, resetCallback, NULL);
	gtk_table_attach(table, *resetButton, x+7, x+8, y, y+1, 0, 0, 0, 0);
    }

    return adj;
}

static void set_save_tooltip(preview_data *data)
{
    char *absFilename = uf_file_set_absolute(CFG->outputFilename);
    char *utf8 = g_filename_display_name(absFilename);
    char *text = g_strdup_printf(_("Filename: %s%s"), utf8,
	    CFG->createID==also_id ? _("\nCreate also ID file") :
	    CFG->createID==only_id ? _("\nCreate only ID file") : "");
    g_free(utf8);
    g_free(absFilename);
    uf_widget_set_tooltip(data->SaveButton, text);
    g_free(text);
}

static void outpath_chooser_changed(GtkFileChooser *chooser, gpointer user_data)
{
    (void)user_data;
    preview_data *data = get_preview_data(chooser);
    if (data->FreezeDialog) return;

    char *path = gtk_file_chooser_get_filename(chooser);
    if ( path==NULL ) {
	// We should never get here
	g_warning("No output path in chooser");
	return;
    }
    // Set the chosen path in the output filename
    char *basename = g_path_get_basename(CFG->outputFilename);
    char *filename = g_build_filename(path, basename , NULL);
    g_free(basename);
    g_strlcpy(CFG->outputFilename, filename, max_path);
    g_free(filename);

    // Set the chosen path as the output path,
    // only if it is different from the input path
    char *dirname = g_path_get_dirname(CFG->inputFilename);
    if ( strcmp(path, dirname)==0 ) {
	g_strlcpy(CFG->outputPath, "", max_path);
    } else {
	g_strlcpy(CFG->outputPath, path, max_path);
    }
    g_free(dirname);
    g_free(path);

    set_save_tooltip(data);
}

static void outfile_entry_changed(GtkEntry *entry, gpointer user_data)
{
    (void)user_data;
    preview_data *data = get_preview_data(entry);
    if (data->FreezeDialog) return;

    char *dirname = g_path_get_dirname(CFG->outputFilename);
    char *fne = g_filename_from_utf8 (gtk_entry_get_text(entry),
                                      -1, NULL, NULL, NULL);
    char *filename = g_build_filename(dirname, fne, NULL);
    g_strlcpy(CFG->outputFilename, filename, max_path);
    g_free(filename);
    g_free(dirname);
    g_free (fne);

    // Update the output type combo
    char *type = strrchr(CFG->outputFilename, '.');
    if (type==NULL)
        return;
    int i;
    for (i=0; data->TypeComboMap[i]>=0; i++) {
	if ( strcasecmp(type, file_type[data->TypeComboMap[i]])==0 )
	    gtk_combo_box_set_active(data->TypeCombo, i);
    }
    set_save_tooltip(data);
}

static void type_combo_changed(GtkComboBox *combo, gpointer *user_data)
{
    (void)user_data;
    preview_data *data = get_preview_data(combo);
    if (data->FreezeDialog) return;

    int i = gtk_combo_box_get_active(combo);
    CFG->type = data->TypeComboMap[i];
    // If type has not changed, do nothing
    char *type = strrchr(CFG->outputFilename, '.');
    if ( type!=NULL && strcasecmp(type, file_type[CFG->type])==0 )
	return;
    char *outfile =
	    uf_file_set_type(CFG->outputFilename, file_type[CFG->type]);
    g_strlcpy(CFG->outputFilename, outfile, max_path);
    g_free(outfile);
    char *basename = g_path_get_basename(CFG->outputFilename);
    gtk_entry_set_text(data->OutFileEntry, basename);
    g_free(basename);

    set_save_tooltip(data);
}

static void combo_update(GtkWidget *combo, gint *valuep)
{
    preview_data *data = get_preview_data(combo);
    if (data->FreezeDialog) return;

    if ((char *)valuep==CFG->wb) {
	int i = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
	g_strlcpy(CFG->wb, g_list_nth_data(data->WBPresets, i), max_name);
    } else {
	*valuep = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    }
    if (valuep==&CFG->BaseCurveIndex) {
	if (!CFG_cameraCurve && CFG->BaseCurveIndex>camera_curve-2)
	    CFG->BaseCurveIndex += 2;
	curveeditor_widget_set_curve(data->BaseCurveWidget,
		&CFG->BaseCurve[CFG->BaseCurveIndex]);
    } else if (valuep==&CFG->curveIndex) {
	curveeditor_widget_set_curve(data->CurveWidget,
		&CFG->curve[CFG->curveIndex]);
    } else if ((char *)valuep==CFG->wb) {
	ufraw_set_wb(data->UF);
    }
    if (CFG->autoExposure==enabled_state) CFG->autoExposure = apply_state;
    if (CFG->autoBlack==enabled_state) CFG->autoBlack = apply_state;
    preview_invalidate_layer (data, ufraw_develop_phase);
    update_scales(data);
}

static void combo_update_simple(GtkWidget *combo, gpointer user_data)
{
    (void)user_data;
    preview_data *data = get_preview_data(combo);
    if (data->FreezeDialog) return;

    if (CFG->autoExposure==enabled_state) CFG->autoExposure = apply_state;
    if (CFG->autoBlack==enabled_state) CFG->autoBlack = apply_state;
    preview_invalidate_layer (data, ufraw_develop_phase);
    update_scales(data);
}

static void radio_menu_update(GtkWidget *item, gint *valuep)
{
    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item))) {
	preview_data *data = get_preview_data(item);
	*valuep = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
		    "Radio-Value"));
	render_preview(data);
    }
}

static void container_remove(GtkWidget *widget, gpointer user_data)
{
    GtkContainer *container = GTK_CONTAINER(user_data);
    gtk_container_remove(container, widget);
}

static void delete_from_list(GtkWidget *widget, gpointer user_data)
{
    preview_data *data = get_preview_data(widget);
    GtkDialog *dialog;
    long type, index;

    dialog = GTK_DIALOG(gtk_widget_get_ancestor(widget, GTK_TYPE_DIALOG));
    type = (long)g_object_get_data(G_OBJECT(widget), "Type");
    index = (long)user_data;
    if (type<profile_types) {
	gtk_combo_box_remove_text(data->ProfileCombo[type], index);
	CFG->profileCount[type]--;
	if ( CFG->profileIndex[type]==index )
	    CFG->profileIndex[type] = conf_default.profileIndex[type];
	else if ( CFG->profileIndex[type]>index )
	    CFG->profileIndex[type]--;
	for (; index<CFG->profileCount[type]; index++)
	    CFG->profile[type][index] = CFG->profile[type][index+1];
	gtk_combo_box_set_active(data->ProfileCombo[type],
		CFG->profileIndex[type]);
    } else if (type==profile_types+base_curve) {
	if (CFG_cameraCurve)
	    gtk_combo_box_remove_text(data->BaseCurveCombo, index);
	else
	    gtk_combo_box_remove_text(data->BaseCurveCombo, index-2);
	CFG->BaseCurveCount--;
	if ( CFG->BaseCurveIndex==index )
	    CFG->BaseCurveIndex = conf_default.BaseCurveIndex;
	else if ( CFG->BaseCurveIndex>index )
	    CFG->BaseCurveIndex--;
	if ( CFG->BaseCurveIndex==camera_curve && !CFG_cameraCurve )
	    CFG->BaseCurveIndex = linear_curve;
	for (; index<CFG->BaseCurveCount; index++)
	    CFG->BaseCurve[index] = CFG->BaseCurve[index+1];
	if ( CFG->BaseCurveIndex>camera_curve && !CFG_cameraCurve )
	    gtk_combo_box_set_active(data->BaseCurveCombo,
		    CFG->BaseCurveIndex-2);
	else
	    gtk_combo_box_set_active(data->BaseCurveCombo,
		    CFG->BaseCurveIndex);
	curveeditor_widget_set_curve(data->BaseCurveWidget,
		&CFG->BaseCurve[CFG->BaseCurveIndex]);
    } else if (type==profile_types+luminosity_curve) {
	gtk_combo_box_remove_text(data->CurveCombo, index);
	CFG->curveCount--;
	if ( CFG->curveIndex==index )
	    CFG->curveIndex = conf_default.curveIndex;
	else if ( CFG->curveIndex>index )
	    CFG->curveIndex--;
	for (; index<CFG->curveCount; index++)
	    CFG->curve[index] = CFG->curve[index+1];
	gtk_combo_box_set_active(data->CurveCombo, CFG->curveIndex);
	curveeditor_widget_set_curve(data->CurveWidget,
	    &CFG->curve[CFG->curveIndex]);
    }
    data->OptionsChanged = TRUE;
    gtk_dialog_response(dialog, GTK_RESPONSE_APPLY);
}

static void configuration_save(GtkWidget *widget, gpointer user_data)
{
    preview_data *data = get_preview_data(widget);
    user_data = user_data;
    conf_save(CFG, NULL, NULL);
    *RC = *data->UF->conf;
}

static void gimp_reset_clicked(GtkWidget *widget, GtkEntry *entry)
{
    (void)widget;
    gtk_entry_set_text(entry, conf_default.remoteGimpCommand);
}

static void options_dialog(GtkWidget *widget, gpointer user_data)
{
    preview_data *data = get_preview_data(widget);
    GtkWidget *optionsDialog, *profileTable[profile_types];
    GtkWidget *notebook, *label, *page, *button, *text, *box, *image;
    GtkTable *baseCurveTable, *curveTable;
    GtkTextBuffer *confBuffer, *buffer;
    char txt[max_name], *buf;
    long i, j, response;

    user_data = user_data;
    if (data->FreezeDialog) return;
    data->OptionsChanged = FALSE;
    int saveBaseCurveIndex = CFG->BaseCurveIndex;
    int saveCurveIndex = CFG->curveIndex;
    int saveProfileIndex[profile_types];
    for (i=0; i<profile_types; i++)
	saveProfileIndex[i] = CFG->profileIndex[i];
    optionsDialog = gtk_dialog_new_with_buttons(_("UFRaw options"),
	    GTK_WINDOW(gtk_widget_get_toplevel(widget)),
	    GTK_DIALOG_DESTROY_WITH_PARENT,
	    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	    GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
    gtk_window_resize(GTK_WINDOW(optionsDialog), 600, 400);
    g_object_set_data(G_OBJECT(optionsDialog), "Preview-Data", data);
    ufraw_focus(optionsDialog, TRUE);
    gtk_dialog_set_default_response(GTK_DIALOG(optionsDialog),
	    GTK_RESPONSE_OK);
    notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(optionsDialog)->vbox),
	    notebook);

    label = gtk_label_new(_("Settings"));
    page = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(page),
            GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, label);

    box = gtk_vbox_new(FALSE, 0);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(page), box);

    profileTable[in_profile] =
	    table_with_frame(box, _("Input color profiles"), TRUE);
    profileTable[out_profile] =
	    table_with_frame(box, _("Output color profiles"), TRUE);
    profileTable[display_profile] =
	    table_with_frame(box, _("Display color profiles"), TRUE);
    baseCurveTable = GTK_TABLE(table_with_frame(box, _("Base Curves"), TRUE));
    curveTable = GTK_TABLE(table_with_frame(box, _("Luminosity Curves"), TRUE));
    GtkTable *settingsTable =
	    GTK_TABLE(table_with_frame(box, _("Settings"), TRUE));
    // Remote Gimp command entry
    label = gtk_label_new(_("Remote Gimp command"));
    gtk_table_attach(settingsTable, label, 0, 1, 0, 1, 0, 0, 0, 0);
    GtkEntry *gimpEntry = GTK_ENTRY(gtk_entry_new());
    gtk_entry_set_max_length(gimpEntry, max_path);
    gtk_entry_set_text(gimpEntry, data->UF->conf->remoteGimpCommand);
    gtk_table_attach(settingsTable, GTK_WIDGET(gimpEntry), 1, 2, 0, 1,
	    GTK_EXPAND|GTK_FILL, 0, 0, 0);
    // Remote Gimp command reset button
    button = reset_button(
	_("Reset command to default"),
	G_CALLBACK(gimp_reset_clicked), gimpEntry);
    gtk_table_attach(settingsTable, button, 2, 3, 0, 1, 0, 0, 0, 0);
    // blinkOverUnder toggle button
    GtkWidget *blinkButton = gtk_check_button_new_with_label(
	    _("Blink Over/Underexposure Indicators"));
    gtk_table_attach(settingsTable, blinkButton, 0, 2, 1, 2, GTK_FILL, 0, 0, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(blinkButton),
	    CFG->blinkOverUnder);
    // drawLines toggle button
    double drawLines = CFG->drawLines;
    GtkAdjustment *drawLinesAdjustment = adjustment_scale(
	settingsTable, 0, 2, _("Alignment Line Count"),
	drawLines, &drawLines, 0, 21, 1, 1, 0,
	_("Number of alignment lines to overlay over the crop area"),
	G_CALLBACK(adjustment_update), NULL, NULL, NULL);

    label = gtk_label_new(_("Configuration"));
    page = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(page),
	    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, label);

    text = gtk_text_view_new();
    gtk_container_add(GTK_CONTAINER(page), text);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
    confBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));

    GtkWidget *hBox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), hBox, FALSE, FALSE, 0);
    button = gtk_button_new_with_label(_("Save configuration"));
    gtk_box_pack_start(GTK_BOX(hBox), button, FALSE, FALSE, 0);
    uf_widget_set_tooltip(button,
	    _("Save configuration to resource file ($HOME/.ufrawrc)"));
    g_signal_connect(G_OBJECT(button), "clicked",
	    G_CALLBACK(configuration_save), NULL);

    label = gtk_label_new(_("Log"));
    page = gtk_scrolled_window_new(NULL, NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, label);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(page),
	    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    text = gtk_text_view_new();
    gtk_container_add(GTK_CONTAINER(page), text);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
    char *log = ufraw_message(UFRAW_GET_LOG, NULL);
    if (log!=NULL) {
	char *utf8_log = g_filename_display_name(log);
	gtk_text_buffer_set_text(buffer, utf8_log, -1);
	g_free(utf8_log);
    }
    label = gtk_label_new(_("About"));
    box = gtk_vbox_new(FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box, label);
    image = gtk_image_new_from_stock("ufraw", GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 0);
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label),
	    "<span size='xx-large'><b>UFRaw " VERSION "</b></span>\n");
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), _(
	    "The <b>U</b>nidentified <b>F</b>lying <b>Raw</b> "
	    "(<b>UFRaw</b>) is a utility to\n"
	    "read and manipulate raw images from digital cameras.\n"
	    "UFRaw relies on <b>D</b>igital <b>C</b>amera "
	    "<b>Raw</b> (<b>DCRaw</b>)\n"
	    "for the actual encoding of the raw images.\n\n"
	    "Author: Udi Fuchs\n"
	    "Homepage: http://ufraw.sourceforge.net/\n\n"));
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

    while (1) {
	for (j=0; j<profile_types; j++) {
	    gtk_container_foreach(GTK_CONTAINER(profileTable[j]),
		    (GtkCallback)(container_remove), profileTable[j]);
	    GtkTable *table = GTK_TABLE(profileTable[j]);
	    for (i=conf_default.profileCount[j]; i<CFG->profileCount[j]; i++) {
		g_snprintf(txt, max_name, "%s (%s)",
			CFG->profile[j][i].name,
			CFG->profile[j][i].productName);
		label = gtk_label_new(txt);
		gtk_table_attach_defaults(table, label, 0, 1, i, i+1);
		button = gtk_button_new_from_stock(GTK_STOCK_DELETE);
		g_object_set_data(G_OBJECT(button), "Type", (void*)j);
		g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(delete_from_list), (gpointer)i);
		gtk_table_attach(table, button, 1, 2, i, i+1, 0, 0, 0, 0);
	    }
	}
	gtk_container_foreach(GTK_CONTAINER(baseCurveTable),
		(GtkCallback)(container_remove), baseCurveTable);
	for (i=camera_curve+1; i<CFG->BaseCurveCount; i++) {
	    label = gtk_label_new(CFG->BaseCurve[i].name);
	    gtk_table_attach_defaults(baseCurveTable, label, 0, 1, i, i+1);
	    button = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	    g_object_set_data(G_OBJECT(button), "Type",
		    (gpointer)profile_types+base_curve);
	    g_signal_connect(G_OBJECT(button), "clicked",
		    G_CALLBACK(delete_from_list), (gpointer)i);
	    gtk_table_attach(baseCurveTable, button, 1, 2, i, i+1, 0, 0, 0, 0);
	}
	gtk_container_foreach(GTK_CONTAINER(curveTable),
		(GtkCallback)(container_remove), curveTable);
	for (i=linear_curve+1; i<CFG->curveCount; i++) {
	    label = gtk_label_new(CFG->curve[i].name);
	    gtk_table_attach_defaults(curveTable, label, 0, 1, i, i+1);
	    button = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	    g_object_set_data(G_OBJECT(button), "Type",
		    (gpointer)profile_types+luminosity_curve);
	    g_signal_connect(G_OBJECT(button), "clicked",
		    G_CALLBACK(delete_from_list), (gpointer)i);
	    gtk_table_attach(curveTable, button, 1, 2, i, i+1, 0, 0, 0, 0);
	}
	conf_save(CFG, NULL, &buf);
	gtk_text_buffer_set_text(confBuffer, buf, -1);
	g_free(buf);
	gtk_widget_show_all(optionsDialog);
	response = gtk_dialog_run(GTK_DIALOG(optionsDialog));
	/* APPLY only marks that something changed and we need to refresh */
	if (response==GTK_RESPONSE_APPLY)
	    continue;

	if ( strcmp(CFG->remoteGimpCommand, gtk_entry_get_text(gimpEntry))!=0 )
	    data->OptionsChanged = TRUE;
	if ( CFG->blinkOverUnder!=
	     gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(blinkButton)) )
	    data->OptionsChanged = TRUE;
	if ( CFG->drawLines!= gtk_adjustment_get_value(drawLinesAdjustment) )
	    data->OptionsChanged = TRUE;

	if ( !data->OptionsChanged ) {
	    /* If nothing changed there is nothing to do */
    	} else if ( response==GTK_RESPONSE_OK ) {
	    g_strlcpy(CFG->remoteGimpCommand,
		    gtk_entry_get_text(gimpEntry), max_path);
	    g_strlcpy(RC->remoteGimpCommand, CFG->remoteGimpCommand, max_path);
	    RC->blinkOverUnder = CFG->blinkOverUnder =
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(blinkButton));
	    RC->drawLines = CFG->drawLines =
		gtk_adjustment_get_value(drawLinesAdjustment);

	    /* Copy profiles and curves from CFG to RC and save .ufrawrc */
	    if ( memcmp(&RC->BaseCurve[RC->BaseCurveIndex],
		    &CFG->BaseCurve[RC->BaseCurveIndex], sizeof(CurveData))!=0 )
		RC->BaseCurveIndex = CFG->BaseCurveIndex;
	    for (i=0; i<CFG->BaseCurveCount; i++)
		RC->BaseCurve[i] = CFG->BaseCurve[i];
	    RC->BaseCurveCount = CFG->BaseCurveCount;

	    if ( memcmp(&RC->curve[RC->curveIndex],
		    &CFG->curve[RC->curveIndex], sizeof(CurveData))!=0 )
		RC->curveIndex = CFG->curveIndex;
	    for (i=0; i<CFG->curveCount; i++)
		RC->curve[i] = CFG->curve[i];
	    RC->curveCount = CFG->curveCount;

	    for (i=0; i<profile_types; i++) {
		if ( memcmp(&RC->profile[i][RC->profileIndex[i]],
			&CFG->profile[i][RC->profileIndex[i]],
			sizeof(profile_data))!=0 )
		    RC->profileIndex[i] = CFG->profileIndex[i];
		for (j=0; j<CFG->profileCount[i]; j++)
		    RC->profile[i][j] = CFG->profile[i][j];
		RC->profileCount[i] = CFG->profileCount[i];
	    }
	    conf_save(RC, NULL, NULL);
	} else { /* response==GTK_RESPONSE_CANCEL or window closed */
	    /* Copy profiles and curves from RC to CFG */

	    /* We might remove 'too much' here, but it causes no harm */
	    for (i=0; i<CFG->BaseCurveCount; i++)
		gtk_combo_box_remove_text(data->BaseCurveCombo, 0);
	    for (i=0; i<RC->BaseCurveCount; i++) {
		CFG->BaseCurve[i] = RC->BaseCurve[i];
		if ( (i==custom_curve || i==camera_curve) && !CFG_cameraCurve )
		    continue;
		if ( i<=camera_curve )
		    gtk_combo_box_append_text(data->BaseCurveCombo,
			    _(CFG->BaseCurve[i].name));
		else
		    gtk_combo_box_append_text(data->BaseCurveCombo,
			    CFG->BaseCurve[i].name);
	    }
	    CFG->BaseCurveCount = RC->BaseCurveCount;
	    CFG->BaseCurveIndex = saveBaseCurveIndex;
	    if (CFG->BaseCurveIndex>camera_curve && !CFG_cameraCurve)
		gtk_combo_box_set_active(data->BaseCurveCombo,
			CFG->BaseCurveIndex-2);
	    else
		gtk_combo_box_set_active(data->BaseCurveCombo,
			CFG->BaseCurveIndex);

	    for (i=0; i<CFG->curveCount; i++)
		gtk_combo_box_remove_text(data->CurveCombo, 0);
	    for (i=0; i<RC->curveCount; i++) {
		CFG->curve[i] = RC->curve[i];
		if ( i<=linear_curve )
		    gtk_combo_box_append_text(data->CurveCombo,
			    _(CFG->curve[i].name));
		else
		    gtk_combo_box_append_text(data->CurveCombo,
			    CFG->curve[i].name);
	    }
	    CFG->curveCount = RC->curveCount;
	    CFG->curveIndex = saveCurveIndex;
	    gtk_combo_box_set_active(data->CurveCombo, CFG->curveIndex);

	    for (j=0; j<profile_types; j++) {
		for (i=0; i<CFG->profileCount[j]; i++)
		    gtk_combo_box_remove_text(data->ProfileCombo[j], 0);
		for (i=0; i<RC->profileCount[j]; i++) {
		    CFG->profile[j][i] = RC->profile[j][i];
		    if ( i<conf_default.profileCount[j] )
			gtk_combo_box_append_text(data->ProfileCombo[j],
				_(CFG->profile[j][i].name));
		    else
			gtk_combo_box_append_text(data->ProfileCombo[j],
				CFG->profile[j][i].name);
		}
		CFG->profileCount[j] = RC->profileCount[j];
		CFG->profileIndex[j] = saveProfileIndex[j];
		gtk_combo_box_set_active(data->ProfileCombo[j],
			CFG->profileIndex[j]);
	    }
	}
	ufraw_focus(optionsDialog, FALSE);
	gtk_widget_destroy(optionsDialog);
	start_blink(data);
        preview_invalidate_layer (data, ufraw_develop_phase);
	render_preview(data);
	return;
    }
}

static gboolean window_delete_event(GtkWidget *widget, GdkEvent *event,
	gpointer user_data)
{
    preview_data *data = get_preview_data(widget);
    user_data = user_data;
    event = event;
    if (data->FreezeDialog) return TRUE;
    g_object_set_data(G_OBJECT(widget), "WindowResponse",
	    (gpointer)GTK_RESPONSE_CANCEL);
    gtk_main_quit();
    return TRUE;
}

static void expander_state(GtkWidget *widget, gpointer user_data)
{
    preview_data *data = get_preview_data(widget);
    const char *text;
    GtkWidget *label;
    int i;

    user_data = user_data;
    if (!GTK_IS_EXPANDER(widget)) return;
    label = gtk_expander_get_label_widget(GTK_EXPANDER(widget));
    text = gtk_label_get_text(GTK_LABEL(label));
    for (i=0; expanderText[i]!=NULL; i++)
	if (!strcmp(text, _(expanderText[i])))
	    CFG->expander[i] = gtk_expander_get_expanded(GTK_EXPANDER(widget));
}

static void panel_size_allocate(GtkWidget *panel,
    GtkAllocation *allocation, gpointer user_data)
{
    (void)user_data;
    preview_data *data = get_preview_data(panel);

    // Raw expander status
    GtkWidget *rawExpander =
	    gtk_widget_get_ancestor(data->RawHisto, GTK_TYPE_EXPANDER);
    gboolean rawMaximized = GPOINTER_TO_INT(
	    g_object_get_data(G_OBJECT(rawExpander), "expander-maximized"));
    int rawHisHeight = data->RawHisto->allocation.height-2;
    gboolean rawExpanded =
	    gtk_expander_get_expanded(GTK_EXPANDER(rawExpander));
    // Live expander status
    GtkWidget *liveExpander =
	    gtk_widget_get_ancestor(data->LiveHisto, GTK_TYPE_EXPANDER);
    gboolean liveMaximized = GPOINTER_TO_INT(
	    g_object_get_data(G_OBJECT(liveExpander), "expander-maximized"));
    int liveHisHeight = data->LiveHisto->allocation.height-2;
    gboolean liveExpanded =
	    gtk_expander_get_expanded(GTK_EXPANDER(liveExpander));

    // Stop allocating space to histogram after maximum height was reached
    gboolean maximizeAll = TRUE;
    if ( rawExpanded && rawHisHeight<=his_max_height )
	maximizeAll = FALSE;
    if ( liveExpanded && liveHisHeight<=his_max_height )
	maximizeAll = FALSE;

    if ( maximizeAll && !rawMaximized ) {
	gtk_box_set_child_packing(GTK_BOX(panel), rawExpander,
		FALSE, FALSE, 0, GTK_PACK_START);
	gtk_widget_set_size_request(data->RawHisto,
		data->RawHisto->allocation.width, his_max_height+2);
	g_object_set_data(G_OBJECT(rawExpander),
	    "expander-histogram", data->RawHisto);
	g_object_set_data(G_OBJECT(rawExpander),
	    "expander-maximized", (gpointer)TRUE);
    }
    if ( maximizeAll && !liveMaximized ) {
	GtkWidget *expander =
		gtk_widget_get_ancestor(data->LiveHisto, GTK_TYPE_EXPANDER);
	gtk_box_set_child_packing(GTK_BOX(panel), expander,
		FALSE, FALSE, 0, GTK_PACK_START);
	gtk_widget_set_size_request(data->LiveHisto,
		data->LiveHisto->allocation.width, his_max_height+2);
	g_object_set_data(G_OBJECT(liveExpander),
	    "expander-histogram", data->LiveHisto);
	g_object_set_data(G_OBJECT(liveExpander),
	    "expander-maximized", (gpointer)TRUE);
    }

    GList *children = gtk_container_get_children(GTK_CONTAINER(panel));
    int childAlloc = 0;
    for (;children!=NULL; children=g_list_next(children))
	childAlloc += GTK_WIDGET(children->data)->allocation.height;

    // If there is no extra space in the box expandable widgets must
    // be shrinkable
    if ( allocation->height==childAlloc ) {
	if ( rawExpanded && rawMaximized ) {
	    if ( data->RawHisto->requisition.height!=data->HisMinHeight )
		gtk_widget_set_size_request(data->RawHisto,
			data->RawHisto->allocation.width, data->HisMinHeight);
	    gboolean rawExpandable;
	    gtk_box_query_child_packing(GTK_BOX(panel), rawExpander,
		    &rawExpandable, NULL, NULL, NULL);
	    if ( !rawExpandable )
		gtk_box_set_child_packing(GTK_BOX(panel), rawExpander,
			TRUE, TRUE, 0, GTK_PACK_START);
	    g_object_set_data(G_OBJECT(rawExpander),
		    "expander-maximized", (gpointer)FALSE);
	}
	if ( liveExpanded && liveMaximized ) {
	    if ( data->LiveHisto->requisition.height!=data->HisMinHeight )
		gtk_widget_set_size_request(data->LiveHisto,
			data->LiveHisto->allocation.width, data->HisMinHeight);
	    gboolean liveExpandable;
	    gtk_box_query_child_packing(GTK_BOX(panel), liveExpander,
		    &liveExpandable, NULL, NULL, NULL);
	    if ( !liveExpandable )
		gtk_box_set_child_packing(GTK_BOX(panel), liveExpander,
			TRUE, TRUE, 0, GTK_PACK_START);
	    g_object_set_data(G_OBJECT(liveExpander),
		    "expander-maximized", (gpointer)FALSE);
	}
    }
    // Redraw histograms if size allocation has changed
    GdkPixbuf *pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(data->RawHisto));
    rawHisHeight = data->RawHisto->allocation.height;
    if ( pixbuf==NULL || gdk_pixbuf_get_height(pixbuf)!=rawHisHeight )
	g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
		(GSourceFunc)(render_raw_histogram), data, NULL);

    pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(data->LiveHisto));
    liveHisHeight = data->LiveHisto->allocation.height;
    if ( pixbuf==NULL || gdk_pixbuf_get_height(pixbuf)!=liveHisHeight )
	g_idle_add_full(G_PRIORITY_DEFAULT_IDLE+20,
		(GSourceFunc)(render_live_histogram), data, NULL);
}

static gboolean histogram_menu(GtkMenu *menu, GdkEventButton *event)
{
    preview_data *data = get_preview_data(menu);
    if (data->FreezeDialog) return FALSE;
    if (event->button!=3) return FALSE;
    gtk_menu_popup(menu, NULL, NULL, NULL, NULL, event->button, event->time);
    return TRUE;
}

void preview_progress(void *widget, char *text, double progress)
{
    if (widget==NULL) return;
    preview_data *data = get_preview_data(widget);
    if (data->ProgressBar==NULL) return;
    gtk_progress_bar_set_text(data->ProgressBar, text);
    gtk_progress_bar_set_fraction(data->ProgressBar, progress);
    while (gtk_events_pending()) gtk_main_iteration();
}

static void control_button_event(GtkWidget *widget, long type)
{
    preview_data *data = get_preview_data(widget);
    if (data->FreezeDialog==TRUE) return;
    data->FreezeDialog = TRUE;
    GtkWindow *window = GTK_WINDOW(gtk_widget_get_toplevel(widget));
    gtk_widget_set_sensitive(data->Controls, FALSE);
    long response = UFRAW_NO_RESPONSE;
    int status = UFRAW_SUCCESS;

    switch (type) {
    case cancel_button:
	response = GTK_RESPONSE_CANCEL;
	break;
    case delete_button:
	if ( ufraw_delete(widget, data->UF)==UFRAW_SUCCESS )
	    response = UFRAW_RESPONSE_DELETE;
	break;
    case save_button:
	status = ufraw_save_now(data->UF, widget);
	response = GTK_RESPONSE_OK;
	break;
    case gimp_button:
	if ( ufraw_send_to_gimp(data->UF)==UFRAW_SUCCESS )
	    response = GTK_RESPONSE_OK;
	break;
    case ok_button:
	status = (*data->SaveFunc)(data->UF, widget);
	response = GTK_RESPONSE_OK;
    }
    // cases that set error status require redrawing of the preview image
    if ( status!=UFRAW_SUCCESS ) {
	preview_progress(widget, "", 0);
	create_base_image(data);
    } else if ( response!=UFRAW_NO_RESPONSE ) {
	g_object_set_data(G_OBJECT(window), "WindowResponse",
		(gpointer)response);
	gtk_main_quit();
    }
    data->FreezeDialog = FALSE;
    gtk_widget_set_sensitive(data->Controls, TRUE);
}

static gboolean control_button_key_press_event(
    GtkWidget *widget, GdkEventKey *event, preview_data *data)
{
    if (data->FreezeDialog==TRUE) return FALSE;
    (void)widget;
    // Check that the Alt key is pressed
    if ( !(event->state&GDK_MOD1_MASK) ) return FALSE;
    int i;
    for (i=0; i<num_buttons; i++) {
	if ( data->ButtonMnemonic[i]==0 ) continue;
	if ( gdk_keyval_to_lower(event->keyval)==data->ButtonMnemonic[i] ) {
	    control_button_event(data->ControlButton[i], i);
	    return TRUE;
	}
    }
    return FALSE;
}

static GtkWidget *control_button(const char *stockImage, const char *tip,
    ControlButtons buttonEnum, preview_data *data)
{
    GtkWidget *button = gtk_button_new();
    uf_button_set_stock_image(GTK_BUTTON(button), stockImage);
    g_signal_connect(G_OBJECT(button), "clicked",
	    G_CALLBACK(control_button_event), (gpointer)buttonEnum);
    char **tipParts = g_strsplit(tip, "_", 2);
    if ( tipParts[0]==NULL || tipParts[1]==NULL ) {
	// No mnemonic
	uf_widget_set_tooltip(button, tip);
	return button;
    }

    char hot_char [7];
    int hot_char_len = g_utf8_next_char (tipParts[1]) - tipParts[1];
    memcpy (hot_char, tipParts[1], hot_char_len);
    hot_char [hot_char_len] = 0;

    char *tooltip = g_strdup_printf(_("%s%s (Alt-%s)"),
	    tipParts[0], tipParts[1], hot_char);
    uf_widget_set_tooltip(button, tooltip);
    g_free(tooltip);
    data->ButtonMnemonic[buttonEnum] = gdk_keyval_to_lower(
	    gdk_unicode_to_keyval(g_utf8_get_char (tipParts[1])));
    data->ControlButton[buttonEnum] = button;
    g_strfreev(tipParts);
    return button;
}

static void notebook_switch_page(GtkNotebook *notebook, GtkNotebookPage *page,
	int page_num, gpointer user_data)
{
    (void)page;
    (void)user_data;
    preview_data *data = get_preview_data(notebook);
    if (data->FreezeDialog==TRUE) return;

    GtkWidget *event_box =
	    gtk_widget_get_ancestor(data->PreviewWidget, GTK_TYPE_EVENT_BOX);
#ifdef HAVE_GTKIMAGEVIEW
    if ( page_num==data->PageNumSpot ||
	 page_num==data->PageNumLightness ||
	 page_num==data->PageNumGray ) {
	gtk_event_box_set_above_child(GTK_EVENT_BOX(event_box), TRUE);
	gdk_window_set_cursor(event_box->window, data->Cursor[spot_cursor]);
	draw_spot(data, TRUE);
    } else if ( page_num==data->PageNumCrop ) {
	gtk_event_box_set_above_child(GTK_EVENT_BOX(event_box), TRUE);
	gdk_window_set_cursor(event_box->window, data->Cursor[crop_cursor]);
	draw_spot(data, FALSE);
    } else {
	gtk_event_box_set_above_child(GTK_EVENT_BOX(event_box), FALSE);
	draw_spot(data, TRUE);
    }
#else
    if ( page_num==data->PageNumCrop ) {
	gdk_window_set_cursor(event_box->window, data->Cursor[crop_cursor]);
	draw_spot(data, FALSE);
    } else {
	gdk_window_set_cursor(event_box->window, data->Cursor[spot_cursor]);
	draw_spot(data, TRUE);
    }
#endif
    data->PageNum = page_num;
}

static void list_store_add(GtkListStore *store, char *name, char *var)
{
    if ( var!=NULL && strlen(var)>0 )
    {
	GtkTreeIter iter;
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 0, name, 1, var, -1);
    }
}

static void rawhistogram_fill_interface(preview_data *data,
					GtkTable* table)
{
    GtkWidget *event_box;
    GdkPixbuf *pixbuf;
    guint8 *pixies;
    GtkWidget *menu;
    GSList *group;
    GtkWidget *menu_item;
    int rowstride;

    event_box = gtk_event_box_new();
    gtk_table_attach(table, event_box, 0, 1, 1, 2,
	    GTK_EXPAND, GTK_EXPAND|GTK_FILL, 0, 0);
    pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
	    raw_his_size+2, his_max_height+2);
    data->RawHisto = gtk_image_new_from_pixbuf(pixbuf);
    gtk_widget_set_size_request(data->RawHisto,
	    raw_his_size+2, data->HisMinHeight+2);
    g_object_unref(pixbuf);
    gtk_container_add(GTK_CONTAINER(event_box), data->RawHisto);
    pixies = gdk_pixbuf_get_pixels(pixbuf);
    rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    memset(pixies, 0, (gdk_pixbuf_get_height(pixbuf)-1)* rowstride +
	    gdk_pixbuf_get_width(pixbuf)*gdk_pixbuf_get_n_channels(pixbuf));

    menu = gtk_menu_new();
    g_object_set_data(G_OBJECT(menu), "Parent-Widget", event_box);
    g_signal_connect_swapped(G_OBJECT(event_box), "button_press_event",
	    G_CALLBACK(histogram_menu), menu);
    group = NULL;
    menu_item = gtk_radio_menu_item_new_with_label(group, _("Linear"));
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 0, 1);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    CFG->rawHistogramScale==linear_histogram);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)linear_histogram);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
	    G_CALLBACK(radio_menu_update), &CFG->rawHistogramScale);
    menu_item = gtk_radio_menu_item_new_with_label(group, _("Logarithmic"));
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 1, 2);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    CFG->rawHistogramScale==log_histogram);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)log_histogram);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
	    G_CALLBACK(radio_menu_update), &CFG->rawHistogramScale);
    gtk_widget_show_all(menu);
}

static void livehistogram_fill_interface(preview_data *data,
					 GtkTable *table)
{
    GtkWidget *event_box;
    GdkPixbuf *pixbuf;
    guint8 *pixies;
    GtkWidget *menu;
    GSList *group;
    GtkWidget *menu_item;
    GtkWidget *button;
    int rowstride;
    int i;

    event_box = gtk_event_box_new();
    gtk_table_attach(table, event_box, 0, 7, 1, 2,
	    0, GTK_EXPAND|GTK_FILL, 0, 0);
    pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
	    live_his_size+2, his_max_height+2);
    data->LiveHisto = gtk_image_new_from_pixbuf(pixbuf);
    gtk_container_add(GTK_CONTAINER(event_box), data->LiveHisto);
    gtk_widget_set_size_request(data->LiveHisto,
	    raw_his_size+2, data->HisMinHeight+2);
    g_object_unref(pixbuf);
    pixies = gdk_pixbuf_get_pixels(pixbuf);
    rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    memset(pixies, 0, (gdk_pixbuf_get_height(pixbuf)-1)* rowstride +
	    gdk_pixbuf_get_width(pixbuf)*gdk_pixbuf_get_n_channels(pixbuf));

    menu = gtk_menu_new();
    g_object_set_data(G_OBJECT(menu), "Parent-Widget", event_box);
    g_signal_connect_swapped(G_OBJECT(event_box), "button_press_event",
	    G_CALLBACK(histogram_menu), menu);
    group = NULL;
    menu_item = gtk_radio_menu_item_new_with_label(group, _("RGB histogram"));
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 0, 1);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    CFG->histogram==rgb_histogram);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)rgb_histogram);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
	    G_CALLBACK(radio_menu_update), &CFG->histogram);
    menu_item = gtk_radio_menu_item_new_with_label(group, _("R+G+B histogram"));
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 1, 2);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    CFG->histogram==r_g_b_histogram);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)r_g_b_histogram);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
	    G_CALLBACK(radio_menu_update), &CFG->histogram);
    menu_item = gtk_radio_menu_item_new_with_label(group,
	    _("Luminosity histogram"));
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 2, 3);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    CFG->histogram==luminosity_histogram);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)luminosity_histogram);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
	    G_CALLBACK(radio_menu_update), &CFG->histogram);
    menu_item = gtk_radio_menu_item_new_with_label(group,
	    _("Value (maximum) histogram"));
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 3, 4);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    CFG->histogram==value_histogram);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)value_histogram);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
	    G_CALLBACK(radio_menu_update), &CFG->histogram);
    menu_item = gtk_radio_menu_item_new_with_label(group,
	    _("Saturation histogram"));
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 4, 5);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    CFG->histogram==saturation_histogram);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)saturation_histogram);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
	    G_CALLBACK(radio_menu_update), &CFG->histogram);

    menu_item = gtk_separator_menu_item_new();
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 5, 6);

    group = NULL;
    menu_item = gtk_radio_menu_item_new_with_label(group, _("Linear"));
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 6, 7);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    CFG->liveHistogramScale==linear_histogram);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)linear_histogram);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
	    G_CALLBACK(radio_menu_update), &CFG->liveHistogramScale);
    menu_item = gtk_radio_menu_item_new_with_label(group, _("Logarithmic"));
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 7, 8);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    CFG->liveHistogramScale==log_histogram);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)log_histogram);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
	    G_CALLBACK(radio_menu_update), &CFG->liveHistogramScale);
    gtk_widget_show_all(menu);

    i = 2;
    data->AvrLabels = color_labels_new(table, 0, i++, _("Average:"),
	    pixel_format, without_zone);
    data->DevLabels = color_labels_new(table, 0, i++, _("Std. deviation:"),
	    pixel_format, without_zone);
    data->OverLabels = color_labels_new(table, 0, i,
	    _("Overexposed:"), percent_format, without_zone);
    toggle_button(table, 4, i, NULL, &CFG->overExp);
    button = gtk_button_new_with_label(_("Indicate"));
    gtk_table_attach(table, button, 6, 7, i, i+1,
	    GTK_SHRINK|GTK_FILL, GTK_FILL, 0, 0);
    g_signal_connect(G_OBJECT(button), "pressed",
	    G_CALLBACK(render_special_mode), (void *)render_overexposed);
    g_signal_connect(G_OBJECT(button), "released",
	    G_CALLBACK(render_special_mode), (void *)render_default);
    i++;
    data->UnderLabels = color_labels_new(table, 0, i, _("Underexposed:"),
	    percent_format, without_zone);
    toggle_button(table, 4, i, NULL, &CFG->underExp);
    button = gtk_button_new_with_label(_("Indicate"));
    gtk_table_attach(table, button, 6, 7, i, i+1,
	    GTK_SHRINK|GTK_FILL, GTK_FILL, 0, 0);
    g_signal_connect(G_OBJECT(button), "pressed",
	    G_CALLBACK(render_special_mode), (void *)render_underexposed);
    g_signal_connect(G_OBJECT(button), "released",
	    G_CALLBACK(render_special_mode), (void *)render_default);
}

static void whitebalance_fill_interface(preview_data *data,
					GtkWidget *page)
{
    GtkTable *table;
    GtkTable *subTable;
    int i;
    GtkWidget *event_box;
    GtkWidget *button;
    GtkWidget *label;
    GtkComboBox *combo;
    GtkBox *box;
    GtkWidget *frame;
    ufraw_data *uf = data->UF;

    /* Start of White Balance setting page */

    table = GTK_TABLE(table_with_frame(page, NULL, TRUE));
    subTable = GTK_TABLE(gtk_table_new(10, 1, FALSE));
    gtk_table_attach(table, GTK_WIDGET(subTable), 0, 1, 0 , 1,
	    GTK_EXPAND|GTK_FILL, 0, 0, 0);
//    label = gtk_label_new("White balance");
//    gtk_table_attach(subTable, label, 0, 1, 0 , 1, 0, 0, 0, 0);
    data->WBCombo = GTK_COMBO_BOX(uf_combo_box_new_text());
    data->WBPresets = NULL;
    const wb_data *lastPreset = NULL;
    gboolean make_model_match = FALSE, make_model_fine_tuning = FALSE;
    char model[max_name];
    if ( strcmp(data->UF->conf->make,"MINOLTA")==0 &&
	    ( strncmp(uf->conf->model, "ALPHA", 5)==0 ||
	      strncmp(uf->conf->model, "MAXXUM", 6)==0 ) ) {
	/* Canonize Minolta model names (copied from dcraw) */
	g_snprintf(model, max_name, "DYNAX %s",
	uf->conf->model+6+(uf->conf->model[0]=='M'));
    } else {
	g_strlcpy(model, uf->conf->model, max_name);
    }
    for (i=0; i<wb_preset_count; i++) {
	if (strcmp(wb_preset[i].make, "")==0) {
	    /* Common presets */
	    data->WBPresets = g_list_append(data->WBPresets,
		    (void*)wb_preset[i].name);
	    gtk_combo_box_append_text(data->WBCombo, _(wb_preset[i].name));
	} else if ( (strcmp(wb_preset[i].make, uf->conf->make)==0 ) &&
		    (strcmp(wb_preset[i].model, model)==0)) {
	    /* Camera specific presets */
	    make_model_match = TRUE;
	    if ( lastPreset==NULL ||
		strcmp(wb_preset[i].name,lastPreset->name)!=0 ) {
		data->WBPresets = g_list_append(data->WBPresets,
			(void*)wb_preset[i].name);
		gtk_combo_box_append_text(data->WBCombo, _(wb_preset[i].name));
	    } else {
		/* Fine tuning presets */
		make_model_fine_tuning = TRUE;
	    }
	    lastPreset = &wb_preset[i];
	}
    }
    GList *l;
    gtk_combo_box_set_active(data->WBCombo, 0);
    for (i=0, l=data->WBPresets; g_list_next(l)!=NULL; i++, l=g_list_next(l))
	if (!strcmp(CFG->wb, l->data))
	    gtk_combo_box_set_active(data->WBCombo, i);
    g_signal_connect(G_OBJECT(data->WBCombo), "changed",
	    G_CALLBACK(combo_update), CFG->wb);
    event_box = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(event_box), GTK_WIDGET(data->WBCombo));
    if ( make_model_fine_tuning || !make_model_match)
	gtk_table_attach(subTable, event_box, 0, 6, 0, 1, GTK_FILL, 0, 0, 0);
    else
	gtk_table_attach(subTable, event_box, 0, 7, 0, 1, GTK_FILL, 0, 0, 0);
    uf_widget_set_tooltip(event_box, _("White Balance"));

    data->WBTuningAdjustment = NULL;
    if (make_model_fine_tuning) {
	data->WBTuningAdjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		CFG->WBTuning, -9, 9, 1, 1, 0));
	g_object_set_data(G_OBJECT(data->WBTuningAdjustment),
		"Adjustment-Accuracy",(gpointer)0);
	button = gtk_spin_button_new(data->WBTuningAdjustment, 1, 0);
    	g_object_set_data(G_OBJECT(data->WBTuningAdjustment),
		"Parent-Widget", button);
	gtk_table_attach(subTable, button, 6, 7, 0, 1, GTK_SHRINK, 0, 0, 0);
	g_signal_connect(G_OBJECT(data->WBTuningAdjustment), "value-changed",
		G_CALLBACK(adjustment_update), &CFG->WBTuning);
    } else if (!make_model_match) {
	event_box = gtk_event_box_new();
	label = gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING,
		GTK_ICON_SIZE_BUTTON);
	gtk_container_add(GTK_CONTAINER(event_box), label);
	gtk_table_attach(subTable, event_box, 6, 7, 0, 1, GTK_FILL, 0, 0, 0);
	uf_widget_set_tooltip(event_box,
	    _("There are no white balance presets for your camera model.\n"
	      "Check UFRaw's webpage for information on how to get your\n"
	      "camera supported."));
    }
    data->ResetWBButton = reset_button(
	_("Reset white balance to initial value"),
	G_CALLBACK(button_update), NULL);
    gtk_table_attach(subTable, data->ResetWBButton, 7, 8, 0, 1, 0, 0, 0, 0);

    data->TemperatureAdjustment = adjustment_scale(subTable, 0, 1,
	    _("Temperature"), CFG->temperature, &CFG->temperature,
	    2000, 15000, 50, 200, 0,
	    _("White balance color temperature (K)"),
	    G_CALLBACK(adjustment_update), NULL, NULL, NULL);
    data->GreenAdjustment = adjustment_scale(subTable, 0, 2, _("Green"),
	    CFG->green, &CFG->green, 0.2, 2.5, 0.010, 0.050, 3,
	    _("Green component"),
	    G_CALLBACK(adjustment_update), NULL, NULL, NULL);
    // Spot WB button:
    button = stock_icon_button(GTK_STOCK_COLOR_PICKER,
	_("Select a spot on the preview image to apply spot white balance"),
	G_CALLBACK(spot_wb_event), NULL);
    gtk_table_attach(subTable, button, 7, 8, 1, 3, 0, 0, 0, 0);

    GtkBox *subbox = GTK_BOX(gtk_hbox_new(0,0));
    gtk_table_attach(table, GTK_WIDGET(subbox), 0, 1, 1, 2, 0, 0, 0, 0);
    label = gtk_label_new(_("Chan. multipliers:"));
    gtk_box_pack_start(subbox, label, 0, 0, 0);
    for (i=0; i<data->UF->colors; i++) {
	data->ChannelAdjustment[i] = GTK_ADJUSTMENT(gtk_adjustment_new(
		CFG->chanMul[i], 0.5, 9.0, 0.001, 0.001, 0));
	g_object_set_data(G_OBJECT(data->ChannelAdjustment[i]),
		"Adjustment-Accuracy",(gpointer)3);
	button = gtk_spin_button_new(data->ChannelAdjustment[i], 0.001, 3);
    	g_object_set_data(G_OBJECT(data->ChannelAdjustment[i]),
		"Parent-Widget", button);
	gtk_box_pack_start(subbox, button, 0, 0, 0);
	g_signal_connect(G_OBJECT(data->ChannelAdjustment[i]), "value-changed",
		G_CALLBACK(adjustment_update), &CFG->chanMul[i]);
    }
    /* Interpolation is temporeraly in the WB page */
    table = GTK_TABLE(table_with_frame(page, NULL, TRUE));
//    box = GTK_BOX(gtk_hbox_new(FALSE, 6));
//    gtk_table_attach(table, GTK_WIDGET(box), 0, 1, 0, 1,
//	    GTK_EXPAND|GTK_FILL, 0, 0, 0);
    event_box = gtk_event_box_new();
    GtkWidget *icon = gtk_image_new_from_stock("interpolation",
	    GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(event_box), icon);
    gtk_table_attach(table, event_box, 0, 1, 0, 1, 0, 0, 0, 0);
    uf_widget_set_tooltip(event_box, _("Bayer pattern interpolation"));
    combo = GTK_COMBO_BOX(uf_combo_box_new_text());
    if ( data->UF->HaveFilters ) {
	if ( data->UF->colors==4 ) {
	    uf_combo_box_append_text(combo, _("VNG four color interpolation"),
		    (void*)four_color_interpolation);
	    uf_combo_box_append_text(combo, _("Bilinear interpolation"),
		    (void*)bilinear_interpolation);
	} else {
	    uf_combo_box_append_text(combo, _("AHD interpolation"),
		    (void*)ahd_interpolation);
	    uf_combo_box_append_text(combo, _("VNG interpolation"),
		    (void*)vng_interpolation);
	    uf_combo_box_append_text(combo, _("VNG four color interpolation"),
		    (void*)four_color_interpolation);
	    uf_combo_box_append_text(combo, _("PPG interpolation"),
		    (void*)ppg_interpolation);
	    uf_combo_box_append_text(combo, _("Bilinear interpolation"),
		    (void*)bilinear_interpolation);
	}
#ifdef ENABLE_INTERP_NONE
	uf_combo_box_append_text(combo, _("No interpolation"),
	    (void*)none_interpolation);
#endif
	uf_combo_box_set_data(combo, &CFG->interpolation);
    } else {
	gtk_combo_box_append_text(combo, _("No Bayer pattern"));
	gtk_combo_box_set_active(combo, 0);
	gtk_widget_set_sensitive(GTK_WIDGET(combo), FALSE);
    }
    gtk_table_attach(table, GTK_WIDGET(combo), 1, 2, 0, 1,
	    GTK_EXPAND|GTK_FILL, 0, 0, 0);
    // Color smoothing button
    button = gtk_toggle_button_new();
    gtk_container_add(GTK_CONTAINER(button),
	    gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_BUTTON));
    gtk_table_attach(table, button, 2, 3, 0, 1, 0, 0, 0, 0);
    uf_widget_set_tooltip(button, _("Apply color smoothing"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
	    CFG->smoothing);
    g_signal_connect(G_OBJECT(button), "toggled",
	    G_CALLBACK(toggle_button_update), &CFG->smoothing);
    if ( !data->UF->HaveFilters )
	gtk_widget_set_sensitive(button, FALSE);

    /* Denoising is temporeraly in the WB page */
    table = GTK_TABLE(table_with_frame(page, NULL, TRUE));
    data->ThresholdAdjustment = adjustment_scale(table, 0, 0, _("Denoise"),
	    CFG->threshold, &CFG->threshold, 0.0, 1000.0, 10, 50, 0,
	    _("Threshold for wavelet denoising"),
	    G_CALLBACK(adjustment_update),
	    &data->ResetThresholdButton,
	    _("Reset denoise threshold to default"), G_CALLBACK(button_update));

    // Dark frame controls:
    box = GTK_BOX(gtk_hbox_new(FALSE, 0));
    frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(page), frame, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(box));

    label = gtk_label_new(_("Dark Frame:"));
    gtk_box_pack_start(box, label, FALSE, FALSE, 0);

    label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_box_pack_start(box, label, TRUE, TRUE, 6);
    data->DarkFrameLabel = GTK_LABEL(label);
    set_darkframe_label(data);

    button = stock_icon_button(GTK_STOCK_OPEN, _("Load dark frame"),
	    G_CALLBACK(load_darkframe), NULL);
    gtk_box_pack_start(box, button, FALSE, FALSE, 0);

    button = reset_button(
	_("Reset dark frame"), G_CALLBACK(reset_darkframe), NULL);
    gtk_box_pack_start(box, button, FALSE, FALSE, 0);

    /* Without GtkImageView, zoom controls cannot be bellow the image because
     * if the image is zoomed in too much the controls will be out of
     * the screen and it would be impossible to zoom out again. */
#ifndef HAVE_GTKIMAGEVIEW
    // Zoom controls:
    table = GTK_TABLE(table_with_frame(page, NULL, TRUE));
    label = gtk_label_new(_("Zoom:"));
    gtk_table_attach(table, label, 0, 1, 0, 1, 0, 0, 0, 0);

    // Zoom out button:
    button = stock_icon_button(GTK_STOCK_ZOOM_OUT, NULL,
	    G_CALLBACK(zoom_out_event), NULL);
    gtk_table_attach(table, button, 1, 2, 0, 1, 0, 0, 0, 0);

    // Zoom percentage spin button:
    data->ZoomAdjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		CFG->Zoom, 5, 50, 1, 1, 0));
    g_object_set_data(G_OBJECT(data->ZoomAdjustment),
		"Adjustment-Accuracy", (gpointer)0);
    button = gtk_spin_button_new(data->ZoomAdjustment, 1, 0);
    g_object_set_data(G_OBJECT(data->ZoomAdjustment), "Parent-Widget", button);
    g_signal_connect(G_OBJECT(data->ZoomAdjustment), "value-changed",
		G_CALLBACK(adjustment_update), &CFG->Zoom);
    gtk_table_attach(table, button, 2, 3, 0, 1, 0, 0, 0, 0);
    uf_widget_set_tooltip(button, _("Zoom percentage"));

    // Zoom in button:
    button = stock_icon_button(GTK_STOCK_ZOOM_IN, NULL,
	    G_CALLBACK(zoom_in_event), NULL);
    gtk_table_attach(table, button, 3, 4, 0, 1, 0, 0, 0, 0);
#endif // !HAVE_GTKIMAGEVIEW

    /* End of White Balance setting page */
}

static void lightness_fill_interface(preview_data *data, GtkWidget *page)
{
    long i;

    /* Start of Lightness Adjustments page */
    GtkTable *table = GTK_TABLE(table_with_frame(page, NULL, TRUE));

    for (i = 0; i < max_adjustments; ++i) {
    	GtkTable *subTable = GTK_TABLE(gtk_table_new(10, 1, FALSE));
	gtk_table_attach(table, GTK_WIDGET(subTable), 0, 10, i, i+1,
		GTK_EXPAND|GTK_FILL, 0, 0, 0);
    	data->LightnessAdjustmentTable[i] = subTable;
	data->LightnessAdjustment[i] = adjustment_scale(subTable, 0, 0, NULL,
		CFG->lightnessAdjustment[i].adjustment,
		&CFG->lightnessAdjustment[i].adjustment,
		0, 2, 0.01, 0.10, 2, NULL,
		G_CALLBACK(adjustment_update),
		&data->ResetLightnessAdjustmentButton[i],
		_("Reset adjustment"), G_CALLBACK(button_update));

	data->LightnessHueSelectButton[i] = stock_icon_button(
		GTK_STOCK_COLOR_PICKER,
		_("Select a spot on the preview image to choose hue"),
		G_CALLBACK(select_hue_event), (gpointer)i);
    	widget_set_hue(data->LightnessHueSelectButton[i],
		CFG->lightnessAdjustment[i].hue);
	gtk_table_attach(subTable, data->LightnessHueSelectButton[i],
		8, 9, 0, 1, 0, 0, 0, 0);

	data->LightnessHueRemoveButton[i] = stock_icon_button(
		GTK_STOCK_CANCEL,
		_("Remove adjustment"),
		G_CALLBACK(remove_hue_event), (gpointer)i);
	gtk_table_attach(subTable, data->LightnessHueRemoveButton[i],
		9, 10, 0, 1, 0, 0, 0, 0);
    }

    // Hue select button:
    data->LightnessHueSelectNewButton = stock_icon_button(
	GTK_STOCK_COLOR_PICKER,
	_("Select a spot on the preview image to choose hue"),
	G_CALLBACK(select_hue_event), (gpointer)-1);
    gtk_table_attach(table, data->LightnessHueSelectNewButton,
		0, 1, max_adjustments, max_adjustments+1, 0, 0, 0, 0);

    /* End of Lightness Adjustments page */
}

static void grayscale_fill_interface(preview_data *data,
				     GtkWidget *page)
{
    GtkTable *table;
    GtkWidget *label;
    int i;

    /* Start of Grayscale page */
    table = GTK_TABLE(table_with_frame(page, NULL, TRUE));

    label = gtk_label_new(_("Grayscale Mode:"));
    gtk_table_attach(table, label, 0, 1, 0, 1, 0, 0, 0, 0);

    for (i = 0; i < 5; ++i) {
        data->GrayscaleButtons[i] = gtk_radio_button_new_with_label_from_widget(
	    (i > 0) ? GTK_RADIO_BUTTON(data->GrayscaleButtons[0]) : NULL,
	    _(grayscaleModeNames[i]));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
	    data->GrayscaleButtons[i]), CFG->grayscaleMode == i);
        gtk_table_attach(table, data->GrayscaleButtons[i],
			 1, 2, i, i+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
	g_signal_connect(G_OBJECT(data->GrayscaleButtons[i]), "toggled",
			 G_CALLBACK(grayscale_update), NULL);
    }

    data->GrayscaleMixerTable = GTK_TABLE(gtk_table_new(3, 3, FALSE));
    gtk_table_attach(table, GTK_WIDGET(data->GrayscaleMixerTable), 0, 2, 6, 7,
		     GTK_EXPAND|GTK_FILL, 0, 0, 0);
    for (i = 0; i < 3; ++i) {
        data->GrayscaleMixers[i] = adjustment_scale(
	    data->GrayscaleMixerTable, 0, i,
	    i==0 ? "@channel-red" : i==1 ? "@channel-green" : "@channel-blue",
	    CFG->grayscaleMixer[i], &CFG->grayscaleMixer[i],
	    -2.0, 2.0, .01, 0.10, 2, NULL, G_CALLBACK(adjustment_update),
	    NULL, NULL, NULL);
    }

    data->GrayscaleMixerColor = GTK_LABEL(gtk_label_new(NULL));
    gtk_widget_set_size_request(GTK_WIDGET(data->GrayscaleMixerColor), 99, -1);
    gtk_table_attach(data->GrayscaleMixerTable,
	GTK_WIDGET(data->GrayscaleMixerColor),
	0, 6, 3, 4, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    data->ResetGrayscaleChannelMixerButton = reset_button(
	_("Reset channel mixer"), G_CALLBACK(button_update), NULL);
    gtk_table_attach(data->GrayscaleMixerTable,
	data->ResetGrayscaleChannelMixerButton, 6, 7, 3, 4, 0, 0, 0, 0);
    /* End of Grayscale page */
}

static void basecurve_fill_interface(preview_data *data,
				     GtkWidget *page,
				     int curveeditorHeight)
{
    GtkTable *table;
    GtkBox *box;
    GtkWidget *align;
    GtkWidget *button;
    int i;

    /* Start of Base Curve page */
    table = GTK_TABLE(table_with_frame(page, NULL, TRUE));
    box = GTK_BOX(gtk_hbox_new(FALSE, 0));
    gtk_table_attach(table, GTK_WIDGET(box), 0, 9, 0, 1,
	    GTK_EXPAND|GTK_FILL, 0, 0, 0);

    data->BaseCurveCombo = GTK_COMBO_BOX(uf_combo_box_new_text());
    /* Fill in the curve names, skipping custom and camera curves if there is
     * no cameraCurve. This will make some mess later with the counting */
    for (i=0; i<CFG->BaseCurveCount; i++) {
	if ( (i==custom_curve || i==camera_curve) && !CFG_cameraCurve )
	    continue;
	if ( i<=camera_curve )
	    gtk_combo_box_append_text(data->BaseCurveCombo,
		    _(CFG->BaseCurve[i].name));
	else
	    gtk_combo_box_append_text(data->BaseCurveCombo,
		    CFG->BaseCurve[i].name);
    }
    /* This is part of the mess with the combo_box counting */
    if (CFG->BaseCurveIndex>camera_curve && !CFG_cameraCurve)
	gtk_combo_box_set_active(data->BaseCurveCombo, CFG->BaseCurveIndex-2);
    else
	gtk_combo_box_set_active(data->BaseCurveCombo, CFG->BaseCurveIndex);
    g_signal_connect(G_OBJECT(data->BaseCurveCombo), "changed",
	    G_CALLBACK(combo_update), &CFG->BaseCurveIndex);
    gtk_box_pack_start(box, GTK_WIDGET(data->BaseCurveCombo), TRUE, TRUE, 0);
    button = stock_icon_button(GTK_STOCK_OPEN, _("Load base curve"),
	    G_CALLBACK(load_curve), (gpointer)base_curve);
    gtk_box_pack_start(box, button, FALSE, FALSE, 0);
    button = stock_icon_button(GTK_STOCK_SAVE_AS, _("Save base curve"),
	    G_CALLBACK(save_curve), (gpointer)base_curve);
    gtk_box_pack_start(box, button, FALSE, FALSE, 0);

    box = GTK_BOX(gtk_hbox_new(FALSE, 0));
    gtk_table_attach(table, GTK_WIDGET(box), 0, 9, 1, 2,
	    GTK_EXPAND|GTK_FILL, 0, 0, 0);
    data->BaseCurveWidget = curveeditor_widget_new(curveeditorHeight, 256,
	    curve_update, (gpointer)base_curve);
    curveeditor_widget_set_curve(data->BaseCurveWidget,
	    &CFG->BaseCurve[CFG->BaseCurveIndex]);
    align = gtk_alignment_new(1, 0, 0, 1);
    gtk_container_add(GTK_CONTAINER(align), GTK_WIDGET(data->BaseCurveWidget));
    gtk_box_pack_start(box, align, TRUE, TRUE, 0);

    data->ResetBaseCurveButton = reset_button(
	_("Reset base curve to default"), G_CALLBACK(button_update), NULL);
    align = gtk_alignment_new(0, 1, 1, 0);
    gtk_container_add(GTK_CONTAINER(align),
	    GTK_WIDGET(data->ResetBaseCurveButton));
    gtk_box_pack_start(box, align, FALSE, FALSE, 0);
    /* End of Base Curve page */
}

static void colormgmt_fill_interface(preview_data *data,
				     GtkWidget *page,
				     int plugin)
{
    GtkTable *table;
    GtkWidget *button;
    GtkWidget *event_box;
    GtkWidget *icon;
    GtkWidget *label;
    GtkComboBox *combo;
    int i;
    long j;

    /* Start of Color management page */
    table = GTK_TABLE(table_with_frame(page, NULL, TRUE));
    for (j=0; j<profile_types; j++) {
	event_box = gtk_event_box_new();
	icon = gtk_image_new_from_stock(
		j==in_profile ? "icc-profile-camera" :
		j==out_profile ? "icc-profile-output" :
		j==display_profile ? "icc-profile-display" : "error",
		GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_container_add(GTK_CONTAINER(event_box), icon);
	gtk_table_attach(table, event_box, 0, 1, 4*j+1, 4*j+2, 0, 0, 0, 0);
	uf_widget_set_tooltip(event_box,
		j==in_profile ? _("Input ICC profile") :
		j==out_profile ? _("Output ICC profile") :
		j==display_profile ? _("Display ICC profile") : "Error");
	data->ProfileCombo[j] = GTK_COMBO_BOX(uf_combo_box_new_text());
	for (i=0; i<CFG->profileCount[j]; i++)
	    if ( i<conf_default.profileCount[j] ) {
		gtk_combo_box_append_text(data->ProfileCombo[j],
			_(CFG->profile[j][i].name));
	    } else {
		gtk_combo_box_append_text(data->ProfileCombo[j],
			CFG->profile[j][i].name);
	    }
	uf_combo_box_set_data(data->ProfileCombo[j], &CFG->profileIndex[j]);
	g_signal_connect_after(G_OBJECT(data->ProfileCombo[j]), "changed",
		G_CALLBACK(combo_update_simple), NULL);
	gtk_table_attach(table, GTK_WIDGET(data->ProfileCombo[j]),
		1, 8, 4*j+1, 4*j+2, GTK_FILL, GTK_FILL, 0, 0);
	button = stock_icon_button(GTK_STOCK_OPEN, NULL,
		G_CALLBACK(load_profile), (void *)j);
	gtk_table_attach(table, button, 8, 9, 4*j+1, 4*j+2,
		GTK_SHRINK, GTK_FILL, 0, 0);
    }
    data->GammaAdjustment = adjustment_scale(table, 1, 3, _("Gamma"),
	    CFG->profile[0][CFG->profileIndex[0]].gamma,
	    &CFG->profile[0][0].gamma, 0.1, 1.0, 0.01, 0.05, 2,
	    _("Gamma correction for the input profile"),
	    G_CALLBACK(adjustment_update),
	    &data->ResetGammaButton, _("Reset gamma to default"),
	    G_CALLBACK(button_update));

    data->LinearAdjustment = adjustment_scale(table, 1, 4, _("Linearity"),
	    CFG->profile[0][CFG->profileIndex[0]].linear,
	    &CFG->profile[0][0].linear, 0.0, 1.0, 0.01, 0.05, 3,
	    _("Linear part of the gamma correction"),
	    G_CALLBACK(adjustment_update),
	    &data->ResetLinearButton, _("Reset linearity to default"),
	    G_CALLBACK(button_update));

    label = gtk_label_new(_("Output intent"));
    gtk_table_attach(table, label, 0, 3, 6, 7, 0, 0, 0, 0);
    combo = GTK_COMBO_BOX(uf_combo_box_new_text());
    gtk_combo_box_append_text(combo, _("Perceptual"));
    gtk_combo_box_append_text(combo, _("Relative colorimetric"));
    gtk_combo_box_append_text(combo, _("Saturation"));
    gtk_combo_box_append_text(combo, _("Absolute colorimetric"));
    uf_combo_box_set_data(GTK_COMBO_BOX(combo),
	    (int*)&CFG->intent[out_profile]);
    g_signal_connect_after(G_OBJECT(combo), "changed",
	    G_CALLBACK(combo_update_simple), NULL);
    gtk_table_attach(table, GTK_WIDGET(combo), 3, 8, 6, 7, GTK_FILL, 0, 0, 0);

    label = gtk_label_new(_("Output bit depth"));
    gtk_table_attach(table, label, 0, 4, 7, 8, 0, 0, 0, 0);
    data->BitDepthCombo = GTK_COMBO_BOX(uf_combo_box_new_text());
    uf_combo_box_append_text(data->BitDepthCombo, "8", (void*)8);
    if ( plugin!=1 )
	uf_combo_box_append_text(data->BitDepthCombo, "16", (void*)16);
    uf_combo_box_set_data(GTK_COMBO_BOX(data->BitDepthCombo),
	&CFG->profile[out_profile][CFG->profileIndex[out_profile]].BitDepth);
    g_signal_connect_after(G_OBJECT(data->BitDepthCombo), "changed",
	    G_CALLBACK(combo_update_simple), NULL);
    gtk_table_attach(table, GTK_WIDGET(data->BitDepthCombo), 4, 5, 7, 8,
	    0, 0, 0, 0);

    label = gtk_label_new(_("Display intent"));
    gtk_table_attach(table, label, 0, 3, 10, 11, 0, 0, 0, 0);
    combo = GTK_COMBO_BOX(uf_combo_box_new_text());
    gtk_combo_box_append_text(combo, _("Perceptual"));
    gtk_combo_box_append_text(combo, _("Relative colorimetric"));
    gtk_combo_box_append_text(combo, _("Saturation"));
    gtk_combo_box_append_text(combo, _("Absolute colorimetric"));
    gtk_combo_box_append_text(combo, _("Disable soft proofing"));
    uf_combo_box_set_data(GTK_COMBO_BOX(combo),
	    (int*)&CFG->intent[display_profile]);
    g_signal_connect_after(G_OBJECT(combo), "changed",
	    G_CALLBACK(combo_update_simple), NULL);
    gtk_table_attach(table, GTK_WIDGET(combo), 3, 8, 10, 11, GTK_FILL, 0, 0, 0);
    /* End of Color management page */
}

static void corrections_fill_interface(preview_data *data,
				       GtkWidget *page,
				       int curveeditorHeight)
{
    GtkTable *table;
    GtkWidget *button;
    GtkBox *box;
    GtkTable *subTable;
    int i;

    /* Start of Corrections page */

    /* Contrast and Saturation adjustments */
    table = GTK_TABLE(table_with_frame(page, NULL, TRUE));

#ifdef UFRAW_CONTRAST
    data->ContrastAdjustment = adjustment_scale(table, 0, 0, _("Contrast"),
	    CFG->contrast, &CFG->contrast, 0, 8.0, 0.01, 0.1, 2,
	    _("Global contrast adjustment"), G_CALLBACK(adjustment_update),
	    &data->ResetContrastButton, _("Reset global contrast to default"),
	    G_CALLBACK(button_update));
#endif
    data->SaturationAdjustment = adjustment_scale(table, 0, 1, _("Saturation"),
	    CFG->saturation, &CFG->saturation, 0.0, 8.0, 0.01, 0.1, 2,
	    _("Saturation"), G_CALLBACK(adjustment_update),
	    &data->ResetSaturationButton, _("Reset saturation to default"),
	    G_CALLBACK(button_update));

    table = GTK_TABLE(table_with_frame(page, NULL, TRUE));
    box = GTK_BOX(gtk_hbox_new(FALSE, 0));
    gtk_table_attach(table, GTK_WIDGET(box), 0, 9, 0, 1,
	    GTK_EXPAND|GTK_FILL, 0, 0, 0);
    data->CurveCombo = GTK_COMBO_BOX(uf_combo_box_new_text());
    /* Fill in the curve names */
    for (i=0; i<CFG->curveCount; i++)
	if ( i<=linear_curve )
	    gtk_combo_box_append_text(data->CurveCombo, _(CFG->curve[i].name));
	else
	    gtk_combo_box_append_text(data->CurveCombo, CFG->curve[i].name);
    gtk_combo_box_set_active(data->CurveCombo, CFG->curveIndex);
    g_signal_connect(G_OBJECT(data->CurveCombo), "changed",
	    G_CALLBACK(combo_update), &CFG->curveIndex);
    gtk_box_pack_start(box, GTK_WIDGET(data->CurveCombo), TRUE, TRUE, 0);
    button = stock_icon_button(GTK_STOCK_OPEN, _("Load curve"),
	    G_CALLBACK(load_curve), (gpointer)luminosity_curve);
    gtk_box_pack_start(box, button, FALSE, FALSE, 0);
    button = stock_icon_button(GTK_STOCK_SAVE_AS, _("Save curve"),
	    G_CALLBACK(save_curve), (gpointer)luminosity_curve);
    gtk_box_pack_start(box, button, FALSE, FALSE, 0);

    subTable = GTK_TABLE(gtk_table_new(9, 9, FALSE));
    gtk_table_attach(table, GTK_WIDGET(subTable), 0, 1, 1 , 2,
	    GTK_EXPAND|GTK_FILL, 0, 0, 0);
    data->CurveWidget = curveeditor_widget_new(curveeditorHeight, 256,
	    curve_update, (gpointer)luminosity_curve);
    curveeditor_widget_set_curve(data->CurveWidget,
	    &CFG->curve[CFG->curveIndex]);
    gtk_table_attach(subTable, data->CurveWidget, 1, 8, 1, 8,
	    GTK_EXPAND|GTK_FILL, 0, 0, 0);

    data->AutoCurveButton = stock_icon_button(GTK_STOCK_EXECUTE,
	    _("Auto adjust curve\n(Flatten histogram)"),
	    G_CALLBACK(button_update), NULL);
    gtk_table_attach(subTable, data->AutoCurveButton, 8, 9, 6, 7, 0, 0, 0, 0);

    data->ResetCurveButton = reset_button(
	_("Reset curve to default"), G_CALLBACK(button_update), NULL);
    gtk_table_attach(subTable, data->ResetCurveButton, 8, 9, 7, 8, 0, 0, 0, 0);

    data->BlackLabel = gtk_label_new(_("Black point: 0.000"));
#if GTK_CHECK_VERSION(2,6,0)
    gtk_misc_set_alignment(GTK_MISC(data->BlackLabel), 0.5, 1.0);
    gtk_label_set_angle(GTK_LABEL(data->BlackLabel), 90);
    gtk_table_attach(subTable, data->BlackLabel, 0, 1, 5, 6,
	    0, GTK_FILL|GTK_EXPAND, 0, 0);
#else
    button = gtk_alignment_new(0, 0, 0, 0);
    gtk_table_attach(subTable, button, 0, 1, 5, 6,
	    0, GTK_FILL|GTK_EXPAND, 0, 0);
    gtk_misc_set_alignment(GTK_MISC(data->BlackLabel), 0.0, 0.5);
    gtk_table_attach(subTable, data->BlackLabel, 1, 8, 8, 9, GTK_FILL, 0, 0, 0);
#endif
    data->ResetBlackButton = reset_button(
	_("Reset black-point to default"), G_CALLBACK(button_update), NULL);
    gtk_table_attach(subTable, GTK_WIDGET(data->ResetBlackButton), 0, 1, 7, 8,
	    0, 0, 0, 0);

    data->AutoBlackButton = GTK_TOGGLE_BUTTON(gtk_toggle_button_new());
    gtk_container_add(GTK_CONTAINER(data->AutoBlackButton),
	    gtk_image_new_from_stock(GTK_STOCK_EXECUTE, GTK_ICON_SIZE_BUTTON));
    gtk_table_attach(subTable, GTK_WIDGET(data->AutoBlackButton), 0, 1, 6, 7,
	    0, GTK_SHRINK, 0, 0);
    uf_widget_set_tooltip(GTK_WIDGET(data->AutoBlackButton),
	    _("Auto adjust black-point"));
    gtk_toggle_button_set_active(data->AutoBlackButton, CFG->autoBlack);
    g_signal_connect(G_OBJECT(data->AutoBlackButton), "clicked",
	    G_CALLBACK(button_update), NULL);
    /* End of Corrections page */
}

static void transformations_fill_interface(preview_data *data,
					   GtkWidget *page)
{
    GtkTable *table;
    GtkWidget *button;
    GtkWidget *entry;
    GtkWidget *label;

    /* Start of transformations page */
    table = GTK_TABLE(table_with_frame(page, NULL, TRUE));

    /* Start of Crop controls */
    label = gtk_label_new(_("Left:"));
    gtk_table_attach(table, label, 0, 1, 1, 2, 0, 0, 0, 0);

    data->CropX1Adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
        CFG->CropX1, 0, data->UF->rotatedWidth, 1, 1, 0));
    data->CropX1Spin = GTK_SPIN_BUTTON(gtk_spin_button_new(
	data->CropX1Adjustment, 1, 0));
    g_object_set_data(G_OBJECT(data->CropX1Adjustment),
        "Parent-Widget", data->CropX1Spin);
    gtk_table_attach(
        table, GTK_WIDGET(data->CropX1Spin), 1, 2, 1, 2, 0, 0, 0, 0);
    g_signal_connect(G_OBJECT(data->CropX1Adjustment), "value-changed",
        G_CALLBACK(adjustment_update), &CFG->CropX1);

    label = gtk_label_new(_("Top:"));
    gtk_table_attach(table, label, 1, 2, 0, 1, 0, 0, 0, 0);

    data->CropY1Adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
	CFG->CropY1, 0, data->UF->rotatedHeight, 1, 1, 0));
    data->CropY1Spin = GTK_SPIN_BUTTON(gtk_spin_button_new(
	data->CropY1Adjustment, 1, 0));
    g_object_set_data(G_OBJECT(data->CropY1Adjustment),
	"Parent-Widget", data->CropY1Spin);
    gtk_table_attach(table,
	GTK_WIDGET(data->CropY1Spin), 2, 3, 0, 1, 0, 0, 0, 0);
    g_signal_connect(G_OBJECT(data->CropY1Adjustment), "value-changed",
	G_CALLBACK(adjustment_update), &CFG->CropY1);

    label = gtk_label_new(_("Right:"));
    gtk_table_attach(table, label, 2, 3, 1, 2, 0, 0, 0, 0);

    data->CropX2Adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
	CFG->CropX2, 0, data->UF->rotatedWidth, 1, 1, 0));
    data->CropX2Spin = GTK_SPIN_BUTTON(gtk_spin_button_new(
	data->CropX2Adjustment, 1, 0));
    g_object_set_data(G_OBJECT(data->CropX2Adjustment),
	"Parent-Widget", data->CropX2Spin);
    gtk_table_attach(table,
	GTK_WIDGET(data->CropX2Spin), 3, 4, 1, 2, 0, 0, 0, 0);
    g_signal_connect(G_OBJECT(data->CropX2Adjustment), "value-changed",
	G_CALLBACK(adjustment_update), &CFG->CropX2);

    label = gtk_label_new(_("Bottom:"));
    gtk_table_attach(table, label, 1, 2, 2, 3, 0, 0, 0, 0);

    data->CropY2Adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
	CFG->CropY2, 0, data->UF->rotatedHeight, 1, 1, 0));
    data->CropY2Spin = GTK_SPIN_BUTTON(gtk_spin_button_new(
	data->CropY2Adjustment, 1, 0));
    g_object_set_data(G_OBJECT(data->CropY2Adjustment),
	"Parent-Widget", data->CropY2Spin);
    gtk_table_attach(table,
	GTK_WIDGET(data->CropY2Spin), 2, 3, 2, 3, 0, 0, 0, 0);
    g_signal_connect(G_OBJECT(data->CropY2Adjustment), "value-changed",
	G_CALLBACK(adjustment_update), &CFG->CropY2);

    // Crop reset button:
    button = reset_button(
	_("Reset the crop region"), G_CALLBACK(crop_reset), NULL);
    gtk_table_attach(table, button, 4, 5, 1, 2, 0, 0, 0, 0);

    /* Aspect ratio controls */
    table = GTK_TABLE(table_with_frame(page, NULL, FALSE));

    label = gtk_label_new(_("Aspect ratio:"));
    gtk_table_attach(table, label, 0, 1, 0, 1, 0, 0, 0, 0);

    entry = gtk_combo_box_entry_new_text();
    data->AspectEntry = GTK_ENTRY (GTK_BIN (entry)->child);
    gtk_entry_set_width_chars (data->AspectEntry, 6);
    gtk_entry_set_alignment (data->AspectEntry, 0.5);
    gtk_table_attach(table, GTK_WIDGET (entry), 1, 2, 0, 1, 0, 0, 5, 0);
    uf_widget_set_tooltip(GTK_WIDGET(data->AspectEntry),
	    _("Crop area aspect ratio.\n"
	      "Can be entered in decimal notation (1.273)\n"
	      "or as a ratio of two numbers (14:11)"));

    size_t s;
    for (s = 0; s < sizeof (predef_aspects) / sizeof (predef_aspects [0]); s++)
    {
	gtk_combo_box_append_text (GTK_COMBO_BOX(entry),
				   predef_aspects [s].text);
    }

    g_signal_connect(G_OBJECT(entry), "changed",
	    G_CALLBACK(aspect_changed), NULL);

    button = gtk_toggle_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), CFG->LockAspect);
    gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_stock(
	    "object-lock", GTK_ICON_SIZE_BUTTON));
    //gtk_box_pack_start (hbox, button, FALSE, FALSE, 0);
    gtk_table_attach(table, button, 2, 3, 0, 1, 0, 0, 0, 0);
    uf_widget_set_tooltip(button, _("Lock aspect ratio"));
    g_signal_connect(G_OBJECT(button), "clicked",
		     G_CALLBACK(lock_aspect), 0);
    data->LockAspectButton = GTK_TOGGLE_BUTTON(button);

    /* Get initial aspect ratio */
    data->AspectRatio = ((float)data->UF->rotatedWidth) / data->UF->rotatedHeight;
    refresh_aspect (data);

    /* Size/shrink controls */
    table = GTK_TABLE(table_with_frame(page, NULL, TRUE));
    label = gtk_label_new(_("Shrink factor"));
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, 0, 0, 0, 0);
    data->ShrinkAdjustment = GTK_ADJUSTMENT(gtk_adjustment_new(CFG->shrink,
	    1, 100, 1, 2, 0));
    g_object_set_data(G_OBJECT(data->ShrinkAdjustment),
		"Adjustment-Accuracy", (gpointer)0);
    data->ShrinkSpin = GTK_SPIN_BUTTON(gtk_spin_button_new(
	    data->ShrinkAdjustment, 1, 3));
    g_object_set_data(G_OBJECT(data->ShrinkAdjustment), "Parent-Widget",
	    data->ShrinkSpin);
    g_signal_connect(G_OBJECT(data->ShrinkAdjustment), "value-changed",
	    G_CALLBACK(adjustment_update), &data->shrink);
    gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(data->ShrinkSpin),
	    1, 2, 0, 1, 0, 0, 0, 0);

    label = gtk_label_new(_("Height"));
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, 1, 2, 0, 0, 0, 0);
    data->height = 0;
    data->HeightAdjustment = GTK_ADJUSTMENT(gtk_adjustment_new(data->height,
	    10, 0, 10, 100, 0));
    g_object_set_data(G_OBJECT(data->HeightAdjustment),
		"Adjustment-Accuracy", (gpointer)0);
    data->HeightSpin = GTK_SPIN_BUTTON(gtk_spin_button_new(
	    data->HeightAdjustment, 10, 0));
    g_object_set_data(G_OBJECT(data->HeightAdjustment), "Parent-Widget",
	    data->HeightSpin);
    g_signal_connect(G_OBJECT(data->HeightAdjustment), "value-changed",
	    G_CALLBACK(adjustment_update), &data->height);
    gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(data->HeightSpin),
		2, 3, 1, 2, 0, 0, 0, 0);

    label = gtk_label_new(_("Width"));
    gtk_table_attach(GTK_TABLE(table), label, 3, 4, 1, 2, 0, 0, 0, 0);
    data->width = 0;
    data->WidthAdjustment = GTK_ADJUSTMENT(gtk_adjustment_new(data->width,
	    10, 0, 10, 100, 0));
    g_object_set_data(G_OBJECT(data->WidthAdjustment),
		"Adjustment-Accuracy", (gpointer)0);
    data->WidthSpin = GTK_SPIN_BUTTON(gtk_spin_button_new(
	    data->WidthAdjustment, 10, 0));
    g_object_set_data(G_OBJECT(data->WidthAdjustment), "Parent-Widget",
	    data->WidthSpin);
    g_signal_connect(G_OBJECT(data->WidthAdjustment), "value-changed",
	    G_CALLBACK(adjustment_update), &data->width);
    gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(data->WidthSpin),
	    4, 5, 1, 2, 0, 0, 0, 0);

    /* Orientation controls */
    table = GTK_TABLE(table_with_frame(page, NULL, TRUE));

    label = gtk_label_new(_("Orientation:"));
    gtk_table_attach(table, label, 0, 1, 0, 1, 0, 0, 0, 0);

    button = stock_image_button("object-rotate-right", GTK_ICON_SIZE_LARGE_TOOLBAR,
				NULL, G_CALLBACK(flip_image), (gpointer)6);
    gtk_table_attach(table, button, 1, 2, 0, 1, 0, 0, 0, 0);

    button = stock_image_button("object-rotate-left", GTK_ICON_SIZE_LARGE_TOOLBAR,
				NULL, G_CALLBACK(flip_image), (gpointer)5);
    gtk_table_attach(table, button, 2, 3, 0, 1, 0, 0, 0, 0);

    button = stock_image_button("object-flip-horizontal", GTK_ICON_SIZE_LARGE_TOOLBAR,
				NULL, G_CALLBACK(flip_image), (gpointer)1);
    gtk_table_attach(table, button, 3, 4, 0, 1, 0, 0, 0, 0);

    button = stock_image_button("object-flip-vertical", GTK_ICON_SIZE_LARGE_TOOLBAR,
				NULL, G_CALLBACK(flip_image), (gpointer)2);
    gtk_table_attach(table, button, 4, 5, 0, 1, 0, 0, 0, 0);

    /* Rotation controls */
    data->unnormalized_angle = CFG->rotationAngle;
    data->reference_orientation = CFG->orientation;
    table = GTK_TABLE(table_with_frame(page, NULL, TRUE));
    data->RotationAdjustment = adjustment_scale(
	table, 0, 0, _("Rotation"),
	data->unnormalized_angle, &data->unnormalized_angle,
	-360, 360, 0.1, 1, 2, _("Rotation Angle"),
	G_CALLBACK(adjustment_update_rotation),
	&data->ResetRotationAdjustment, _("Reset Rotation Angle"),
	G_CALLBACK(adjustment_reset_rotation));

    /* End of transformation page */
}

static void save_fill_interface(preview_data *data,
				GtkWidget *page,
				int plugin)
{
    GtkWidget *button;
    GtkWidget *frame;
    GtkWidget *vBox;
    GtkWidget *hBox;
    GtkWidget *label;
    GtkWidget *event_box;
    int i;

	/* Start of Save page */
	frame = gtk_frame_new(NULL);
	gtk_box_pack_start(GTK_BOX(page), frame, FALSE, FALSE, 0);
	vBox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(vBox));

	hBox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vBox), hBox, FALSE, FALSE, 0);
	label = gtk_label_new(_("Path"));
	gtk_box_pack_start(GTK_BOX(hBox), label, FALSE, FALSE, 0);
	GtkWidget *chooser = gtk_file_chooser_button_new(
		_("Select output path"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	/* Add shortcut to path of input file */
	if ( strlen(CFG->inputFilename)>0 ) {
	    char *cp = g_path_get_dirname(CFG->inputFilename);
	    gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(chooser),
		    cp, NULL);
	    g_free(cp);
	}
	// Set a small width to make sure the combo-box is not too big.
	// The final size is set by the EXPAND|FILL options.
	gtk_widget_set_size_request(chooser, 50, -1);
	char *absFilename = uf_file_set_absolute(CFG->outputFilename);
	gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(chooser),
		absFilename);
	g_free(absFilename);
	gtk_box_pack_start(GTK_BOX(hBox), chooser, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(chooser), "selection-changed",
		G_CALLBACK(outpath_chooser_changed), NULL);
	if ( plugin==3 )
	    gtk_widget_set_sensitive(chooser, FALSE);

	hBox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vBox), hBox, FALSE, FALSE, 0);
	label = gtk_label_new(_("Filename"));
	gtk_box_pack_start(GTK_BOX(hBox), label, FALSE, FALSE, 0);
	data->OutFileEntry = GTK_ENTRY(gtk_entry_new());
        char *basename = g_path_get_basename(CFG->outputFilename);
        char *basename_dn = g_filename_display_name (basename);
        gtk_entry_set_text(data->OutFileEntry, basename_dn);
        g_free(basename_dn);
	g_free(basename);
	gtk_box_pack_start(GTK_BOX(hBox), GTK_WIDGET(data->OutFileEntry),
		TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(data->OutFileEntry), "changed",
		G_CALLBACK(outfile_entry_changed), NULL);
	if ( plugin==3 )
	    gtk_widget_set_sensitive(GTK_WIDGET(data->OutFileEntry), FALSE);
	data->TypeCombo = GTK_COMBO_BOX(gtk_combo_box_new_text());
	i = 0;
	gtk_combo_box_append_text(data->TypeCombo, "PPM");
	data->TypeComboMap[i++] = ppm_type;
#ifdef HAVE_LIBPNG
	gtk_combo_box_append_text(data->TypeCombo, "PNG");
	data->TypeComboMap[i++] = png_type;
#endif
#ifdef HAVE_LIBTIFF
	gtk_combo_box_append_text(data->TypeCombo, "TIFF");
	data->TypeComboMap[i++] = tiff_type;
#endif
#ifdef HAVE_LIBJPEG
	gtk_combo_box_append_text(data->TypeCombo, "JPEG");
	data->TypeComboMap[i++] = jpeg_type;
#endif
#ifdef HAVE_LIBCFITSIO
	gtk_combo_box_append_text(data->TypeCombo, "FITS");
	data->TypeComboMap[i++] = fits_type;
#endif
	data->TypeComboMap[i] = -1;
	for (i=0; data->TypeComboMap[i]>=0; i++)
	    if ( data->TypeComboMap[i]==CFG->type )
		gtk_combo_box_set_active(data->TypeCombo, i);
	gtk_box_pack_start(GTK_BOX(hBox), GTK_WIDGET(data->TypeCombo),
		FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(data->TypeCombo), "changed",
		G_CALLBACK(type_combo_changed), &CFG->type);
	if ( plugin==3 )
	    gtk_widget_set_sensitive(GTK_WIDGET(data->TypeCombo), FALSE);

	gtk_box_pack_start(GTK_BOX(vBox), gtk_hseparator_new(),
		FALSE, FALSE, 0);

#ifdef HAVE_LIBJPEG
	hBox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vBox), hBox, FALSE, FALSE, 0);
	label = gtk_label_new(_("JPEG compression level"));
	gtk_box_pack_start(GTK_BOX(hBox), label, FALSE, FALSE, 0);
	GtkAdjustment *compressAdj =
		GTK_ADJUSTMENT(gtk_adjustment_new(CFG->compression,
		0, 100, 5, 10, 0));
	GtkWidget *scale = gtk_hscale_new(compressAdj);
	gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
	gtk_box_pack_start(GTK_BOX(hBox), scale, TRUE, TRUE, 0);
	button = gtk_spin_button_new(compressAdj, 5, 0);
	gtk_box_pack_start(GTK_BOX(hBox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(compressAdj), "value-changed",
		G_CALLBACK(adjustment_update_int), &CFG->compression);

	button = uf_check_button_new( _("JPEG progressive encoding"),
		&CFG->progressiveJPEG);
	gtk_box_pack_start(GTK_BOX(vBox), button, FALSE, FALSE, 0);
#endif // HAVE_LIBJPEG

#if defined(HAVE_LIBTIFF) && defined(HAVE_LIBZ)
	button = uf_check_button_new( _("TIFF lossless Compress"),
		&CFG->losslessCompress);
	gtk_box_pack_start(GTK_BOX(vBox), button, FALSE, FALSE, 0);
#endif // HAVE_LIBTIFF && HAVE_LIBZ

#if defined(HAVE_EXIV2) && (defined(HAVE_LIBJPEG) || defined(HAVE_LIBPNG))
	button = uf_check_button_new( _("Embed EXIF data in output"),
		&CFG->embedExif);
	gtk_widget_set_sensitive(button, data->UF->inputExifBuf!=NULL);
	gtk_box_pack_start(GTK_BOX(vBox), button, FALSE, FALSE, 0);
#endif // HAVE_EXIV2 && (HAVE_LIBJPEG || HAVE_LIBPNG)

	hBox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vBox), hBox, FALSE, FALSE, 0);
	label = gtk_label_new(_("Create ID file "));
	gtk_box_pack_start(GTK_BOX(hBox), label, FALSE, FALSE, 0);
	GtkComboBox *idCombo = GTK_COMBO_BOX(gtk_combo_box_new_text());
	gtk_combo_box_append_text(idCombo, _("No"));
	gtk_combo_box_append_text(idCombo, _("Also"));
	gtk_combo_box_append_text(idCombo, _("Only"));
	uf_combo_box_set_data(idCombo, &CFG->createID);
	gtk_box_pack_start(GTK_BOX(hBox), GTK_WIDGET(idCombo), FALSE, FALSE, 0);

	hBox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vBox), hBox, FALSE, FALSE, 0);
	event_box = gtk_event_box_new();
	gtk_box_pack_start(GTK_BOX(hBox), event_box, FALSE, FALSE, 0);
	label = gtk_label_new(_("Save image defaults "));
	gtk_container_add(GTK_CONTAINER(event_box), label);
	uf_widget_set_tooltip(event_box,
		_("Save current image manipulation parameters as defaults.\n"
		  "The output parameters in this window are always saved."));
	GtkComboBox *confCombo = GTK_COMBO_BOX(gtk_combo_box_new_text());
	gtk_combo_box_append_text(confCombo, _("Never again"));
	gtk_combo_box_append_text(confCombo, _("Always"));
	gtk_combo_box_append_text(confCombo, _("Just this once"));
	uf_combo_box_set_data(confCombo, &CFG->saveConfiguration);
	gtk_box_pack_start(GTK_BOX(hBox), GTK_WIDGET(confCombo),
		FALSE, FALSE, 0);

	button = uf_check_button_new( _("Remember output path"),
		&CFG->RememberOutputPath);
	gtk_box_pack_start(GTK_BOX(vBox), button, FALSE, FALSE, 0);

	button = uf_check_button_new(
		_("Overwrite existing files without asking"), &CFG->overwrite);
	gtk_box_pack_start(GTK_BOX(vBox), button, FALSE, FALSE, 0);
	/* End of Save page */
}

static void exif_fill_interface(preview_data *data,
				GtkWidget *page)
{
    GtkWidget *align;
    GtkWidget *frame;
    GtkWidget *vBox;
    GtkWidget *label;
    ufraw_data *uf = data->UF;

    /* Start of EXIF page */
    frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(page), frame, TRUE, TRUE, 0);
    vBox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(vBox));

    GtkListStore *store;
    store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

    align = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX (vBox), align, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(align),
	    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget *treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
    gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview), 0);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
    // does not work???
    gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (treeview), FALSE);
    g_object_unref (store);

    gtk_container_add(GTK_CONTAINER (align), treeview);

    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes (
	    _("Tag"), gtk_cell_renderer_text_new (), "text", 0, NULL);
    gtk_tree_view_column_set_sort_column_id (column, 0);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    column = gtk_tree_view_column_new_with_attributes (
	    _("Value"), gtk_cell_renderer_text_new (), "text", 1, NULL);
    gtk_tree_view_column_set_sort_column_id (column, 1);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    // Fill table with EXIF tags
    list_store_add(store, _("Camera maker"), CFG->make);
    list_store_add(store, _("Camera model"), CFG->model);
    list_store_add(store, _("Timestamp"), CFG->timestampText);
    list_store_add(store, _("Shutter time"), CFG->shutterText);
    list_store_add(store, _("Aperture"), CFG->apertureText);
    list_store_add(store, _("ISO speed"), CFG->isoText);
    list_store_add(store, _("Focal length"), CFG->focalLenText);
    list_store_add(store, _("35mm focal length"), CFG->focalLen35Text);
    list_store_add(store, _("Lens"), CFG->lensText);
    list_store_add(store, _("Flash"), CFG->flashText);
    list_store_add(store, _("White balance"), CFG->whiteBalanceText);

    label = gtk_label_new(NULL);
    gchar *message = g_strdup_printf (_("EXIF data read by %s"), CFG->exifSource);
    gtk_label_set_markup(GTK_LABEL(label), message);
    gtk_box_pack_start(GTK_BOX (vBox), label, FALSE, FALSE, 0);
    g_free (message);

    if (uf->inputExifBuf==NULL)
    {
	label = gtk_label_new(NULL);
	char *text = g_strdup_printf("<span foreground='red'>%s</span>",
			_("Warning: EXIF data will not be sent to output"));
	gtk_label_set_markup(GTK_LABEL(label), text);
	g_free(text);
	gtk_box_pack_start(GTK_BOX (vBox), label, FALSE, FALSE, 0);
    }
    /* End of EXIF page */
}

int ufraw_preview(ufraw_data *uf, conf_data *rc, int plugin,
	long (*save_func)())
{
    GtkWidget *previewWindow, *previewVBox;
    GtkTable *table;
    GtkBox *previewHBox, *box, *hbox;
    GtkWidget *button, *vBox, *page;
    GdkRectangle screen;
    int max_preview_width, max_preview_height;
    int preview_width, preview_height, i, c;
    int status, curveeditorHeight;
    preview_data PreviewData;
    preview_data *data = &PreviewData;

    /* Fill the whole structure with zeros, to avoid surprises */
    memset (&PreviewData, 0, sizeof (PreviewData));

    data->UF = uf;
    data->SaveFunc = save_func;

    data->rc = rc;
    data->SpotX1 = -1;
    data->SpotX2 = -1;
    data->SpotY1 = -1;
    data->SpotY2 = -1;
    data->SpotDraw = FALSE;
    data->FreezeDialog = TRUE;
    data->DrawnCropX1 = 0;
    data->DrawnCropX2 = 99999;
    data->DrawnCropY1 = 0;
    data->DrawnCropY2 = 99999;

    data->AspectRatio = 0.0;
    data->BlinkTimer = 0;
    for (i=0; i<num_buttons; i++) {
	data->ControlButton[i] = NULL;
	data->ButtonMnemonic[i] = 0;
    }
    previewWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    char *utf8_filename = g_filename_display_name(uf->filename);
    char *previewHeader = g_strdup_printf(_("%s - UFRaw"), utf8_filename);
    gtk_window_set_title(GTK_WINDOW(previewWindow), previewHeader);
    g_free(previewHeader);
    g_free(utf8_filename);

    ufraw_icons_init();
    uf_window_set_icon_name(GTK_WINDOW(previewWindow), "ufraw");
    g_signal_connect(G_OBJECT(previewWindow), "delete-event",
	    G_CALLBACK(window_delete_event), NULL);
    g_signal_connect(G_OBJECT(previewWindow), "map-event",
		     G_CALLBACK(window_map_event), NULL);
    g_signal_connect(G_OBJECT(previewWindow), "unmap-event",
		     G_CALLBACK(window_unmap_event), NULL);
    g_signal_connect(G_OBJECT(previewWindow), "key-press-event",
		     G_CALLBACK(control_button_key_press_event), data);
    g_object_set_data(G_OBJECT(previewWindow), "Preview-Data", data);
    ufraw_focus(previewWindow, TRUE);
    uf->widget = previewWindow;

    /* With the following guesses the window usually fits into the screen.
     * There should be more intelligent settings to window size. */
    gdk_screen_get_monitor_geometry(gdk_screen_get_default(), 0, &screen);
    max_preview_width = MIN(def_preview_width, screen.width-416);
    max_preview_height = MIN(def_preview_height, screen.height-152);
    CFG->Scale = MAX((uf->rotatedWidth-1)/max_preview_width,
	    (uf->rotatedHeight-1)/max_preview_height)+1;
    CFG->Scale = MAX(2, CFG->Scale);
    CFG->Zoom = 100.0 / CFG->Scale;
    // Make preview size a tiny bit larger to prevent rounding errors
    // that will cause the scrollbars to appear.
    preview_width = (uf->rotatedWidth+1) / CFG->Scale;
    preview_height = (uf->rotatedHeight+1) / CFG->Scale;
    if (screen.height<=600) {
	curveeditorHeight = 192;
	data->HisMinHeight = 0;
    } else if (screen.height<700) {
	curveeditorHeight = 192;
	data->HisMinHeight = 48;
    } else if (screen.height<800) {
	curveeditorHeight = 192;
	data->HisMinHeight = 64;
    } else if (screen.height<900) {
	curveeditorHeight = 192;
	data->HisMinHeight = 80;
    } else {
	curveeditorHeight = 256;
	data->HisMinHeight = 96;
    }
    previewHBox = GTK_BOX(gtk_hbox_new(FALSE, 0));
    gtk_container_add(GTK_CONTAINER(previewWindow), GTK_WIDGET(previewHBox));
    previewVBox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(previewHBox, previewVBox, FALSE, FALSE, 2);
    g_signal_connect(G_OBJECT(previewVBox), "size-allocate",
	    G_CALLBACK(panel_size_allocate), NULL);

    table = GTK_TABLE(table_with_frame(previewVBox,
	    _(expanderText[raw_expander]), CFG->expander[raw_expander]));
    rawhistogram_fill_interface(data, table);

    // Spot values:
    data->SpotTable = GTK_TABLE(table_with_frame(previewVBox, NULL, FALSE));
    data->SpotLabels = color_labels_new(data->SpotTable, 0, 0,
	    _("Spot values:"), pixel_format, with_zone);
    data->SpotPatch = GTK_LABEL(gtk_label_new(NULL));
    // Set a small width to make sure the combo-box is not too big.
    // The final size is set by the EXPAND|FILL options.
    gtk_widget_set_size_request(GTK_WIDGET(data->SpotPatch), 50, -1);
    gtk_table_attach(data->SpotTable, GTK_WIDGET(data->SpotPatch),
	    6, 7, 0, 1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    button = stock_image_button(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU,
				NULL, G_CALLBACK(close_spot), NULL);
    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
    gtk_table_attach(data->SpotTable, button, 7, 8, 0, 1, 0, 0, 0, 0);

    // Exposure:
    table = GTK_TABLE(table_with_frame(previewVBox, NULL, FALSE));
    data->ExposureAdjustment = adjustment_scale(table, 0, 0, "@exposure",
	    CFG->exposure, &CFG->exposure,
	    -6, 6, 0.01, 1.0/6, 2, _("Exposure compensation in EV"),
	    G_CALLBACK(adjustment_update), NULL, NULL, NULL);

    button = gtk_toggle_button_new();
    gtk_table_attach(table, button, 7, 8, 0, 1, 0, 0, 0, 0);
    restore_details_button_set(GTK_BUTTON(button), data);
    g_signal_connect(G_OBJECT(button), "toggled",
	    G_CALLBACK(toggle_button_update), &CFG->restoreDetails);

    button = gtk_toggle_button_new();
    gtk_table_attach(table, button, 8, 9, 0, 1, 0, 0, 0, 0);
    clip_highlights_button_set(GTK_BUTTON(button), data);
    g_signal_connect(G_OBJECT(button), "toggled",
	    G_CALLBACK(toggle_button_update), &CFG->clipHighlights);

    data->AutoExposureButton = GTK_TOGGLE_BUTTON(gtk_toggle_button_new());
    gtk_container_add(GTK_CONTAINER(data->AutoExposureButton),
	    gtk_image_new_from_stock(GTK_STOCK_EXECUTE, GTK_ICON_SIZE_BUTTON));
    gtk_table_attach(table, GTK_WIDGET(data->AutoExposureButton), 9, 10, 0, 1,
	    0, 0, 0, 0);
    uf_widget_set_tooltip(GTK_WIDGET(data->AutoExposureButton),
	    _("Auto adjust exposure"));
    gtk_toggle_button_set_active(data->AutoExposureButton, CFG->autoExposure);
    g_signal_connect(G_OBJECT(data->AutoExposureButton), "clicked",
	    G_CALLBACK(button_update), NULL);

    data->ResetExposureButton = reset_button(
	_("Reset exposure to default"), G_CALLBACK(button_update), NULL);
    gtk_table_attach(table, data->ResetExposureButton, 10, 11, 0, 1, 0,0,0,0);

    GtkNotebook *notebook = GTK_NOTEBOOK(gtk_notebook_new());
    g_signal_connect(G_OBJECT(notebook), "switch-page",
	    G_CALLBACK(notebook_switch_page), NULL);
    data->Controls = GTK_WIDGET(notebook);
    gtk_box_pack_start(GTK_BOX(previewVBox), GTK_WIDGET(notebook),
	    FALSE, FALSE, 0);

    page = notebook_page_new(notebook, _("White balance"), "white-balance");
    /* Set this page to be the opening page. */
    data->PageNumSpot = gtk_notebook_page_num(notebook, page);
    data->PageNum = data->PageNumSpot;
    whitebalance_fill_interface(data, page);

    page = notebook_page_new(notebook, _("Grayscale"), "grayscale");
    data->PageNumGray = gtk_notebook_page_num(notebook, page);
    grayscale_fill_interface(data, page);

#ifdef HAVE_LENSFUN
    /* Lens correction page */
    page = notebook_page_new(notebook, _("Lens correction"), "lens");
    lens_fill_interface (data, page);
#endif /* HAVE_LENSFUN */

    page = notebook_page_new(notebook, _("Base curve"), "base-curve");
    basecurve_fill_interface(data, page, curveeditorHeight);

    page = notebook_page_new(notebook, _("Color management"),
	    "color-management");
    colormgmt_fill_interface(data, page, plugin);

    page = notebook_page_new(notebook, _("Correct luminosity, saturation"),
	    "color-corrections");
    corrections_fill_interface(data, page, curveeditorHeight);

    page = notebook_page_new(notebook, _("Lightness Adjustments"), "hueadjust");
    data->PageNumLightness = gtk_notebook_page_num(notebook, page);
    lightness_fill_interface(data, page);

    page = notebook_page_new(notebook, _("Crop and rotate"), "crop");
    data->PageNumCrop = gtk_notebook_page_num(notebook, page);
    transformations_fill_interface(data, page);

    if ( plugin==0  || plugin==3 ) {
	page = notebook_page_new(notebook, _("Save"), GTK_STOCK_SAVE_AS);
	save_fill_interface(data, page, plugin);
    }

    page = notebook_page_new(notebook, _("EXIF"), "exif");
    exif_fill_interface(data, page);

    table = GTK_TABLE(table_with_frame(previewVBox,
	    _(expanderText[live_expander]), CFG->expander[live_expander]));
    livehistogram_fill_interface(data, table);

    /* Right side of the preview window */
    vBox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(previewHBox, vBox, TRUE, TRUE, 2);
    GtkWidget *PreviewEventBox = gtk_event_box_new();
#ifdef HAVE_GTKIMAGEVIEW
    data->PreviewWidget = gtk_image_view_new();
    gtk_image_view_set_zoom(GTK_IMAGE_VIEW(data->PreviewWidget), 1.0);
    gtk_event_box_set_above_child(GTK_EVENT_BOX(PreviewEventBox), TRUE);

    GtkWidget *scroll = gtk_image_scroll_win_new(
	    GTK_IMAGE_VIEW(data->PreviewWidget));
    gtk_widget_set_size_request(scroll, preview_width, preview_height);
    GtkWidget *container =
	    gtk_widget_get_ancestor(data->PreviewWidget, GTK_TYPE_TABLE);
    g_object_ref(G_OBJECT(data->PreviewWidget));
    gtk_container_remove(GTK_CONTAINER(container), data->PreviewWidget);
    gtk_container_add(GTK_CONTAINER(PreviewEventBox), data->PreviewWidget);
    g_object_unref(G_OBJECT(data->PreviewWidget));
    gtk_table_attach(GTK_TABLE(container), PreviewEventBox, 0, 1, 0, 1,
	    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    gtk_box_pack_start(GTK_BOX(vBox), scroll, TRUE, TRUE, 0);
#else
    GtkWidget *align = gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_box_pack_start(GTK_BOX(vBox), align, TRUE, TRUE, 0);
    box = GTK_BOX(gtk_vbox_new(FALSE, 0));
    gtk_container_add(GTK_CONTAINER(align), GTK_WIDGET(box));
    gtk_box_pack_start(box, PreviewEventBox, FALSE, FALSE, 0);
    data->PreviewPixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
	    preview_width, preview_height);
    data->PreviewWidget = gtk_image_new_from_pixbuf(data->PreviewPixbuf);
    g_object_unref(data->PreviewPixbuf);
    gtk_misc_set_alignment(GTK_MISC(data->PreviewWidget), 0, 0);
    gtk_container_add(GTK_CONTAINER(PreviewEventBox), data->PreviewWidget);
#endif
    data->PreviewButtonPressed = FALSE;
    g_signal_connect(G_OBJECT(PreviewEventBox), "button-press-event",
	    G_CALLBACK(preview_button_press_event), NULL);
    g_signal_connect(G_OBJECT(PreviewEventBox), "button-release-event",
	    G_CALLBACK(preview_button_release_event), NULL);
    g_signal_connect(G_OBJECT(PreviewEventBox), "motion-notify-event",
	    G_CALLBACK(preview_motion_notify_event), NULL);
    gtk_widget_add_events(PreviewEventBox, GDK_POINTER_MOTION_MASK);

#ifdef HAVE_GTKIMAGEVIEW
    // Hide zoom key bindings from GtkImageView
    GtkImageViewClass *klass =
	    GTK_IMAGE_VIEW_GET_CLASS(GTK_IMAGE_VIEW(data->PreviewWidget));
    GtkBindingSet *binding_set = gtk_binding_set_by_class(klass);
    gtk_binding_entry_remove(binding_set, GDK_1, 0);
    gtk_binding_entry_remove(binding_set, GDK_2, 0);
    gtk_binding_entry_remove(binding_set, GDK_3, 0);
    gtk_binding_entry_remove(binding_set, GDK_plus, 0);
    gtk_binding_entry_remove(binding_set, GDK_equal, 0);
    gtk_binding_entry_remove(binding_set, GDK_KP_Add, 0);
    gtk_binding_entry_remove(binding_set, GDK_minus, 0);
    gtk_binding_entry_remove(binding_set, GDK_KP_Subtract, 0);
    gtk_binding_entry_remove(binding_set, GDK_x, 0);
    // GtkImageView should only get the scoll up/down events
    GtkWidgetClass *widget_class = (GtkWidgetClass *)klass;
    gtk_image_view_scroll_event = widget_class->scroll_event;
    widget_class->scroll_event = preview_scroll_event;
#endif

    data->ProgressBar = GTK_PROGRESS_BAR(gtk_progress_bar_new());
    gtk_box_pack_start(GTK_BOX(vBox), GTK_WIDGET(data->ProgressBar),
	    FALSE, FALSE, 0);

    /* Control buttons at the bottom */
    GtkBox *ControlsBox = GTK_BOX(gtk_hbox_new(FALSE, 6));
    gtk_box_pack_start(GTK_BOX(vBox), GTK_WIDGET(ControlsBox), FALSE, FALSE, 6);
    // Zoom buttons are centered:
    GtkBox *ZoomBox = GTK_BOX(gtk_hbox_new(FALSE, 0));
    gtk_box_pack_start(ControlsBox, GTK_WIDGET(ZoomBox), TRUE, FALSE, 0);
#ifdef HAVE_GTKIMAGEVIEW
    // Zoom out button:
    button = stock_icon_button(GTK_STOCK_ZOOM_OUT, NULL,
	    G_CALLBACK(zoom_out_event), NULL);
    gtk_box_pack_start(ZoomBox, button, FALSE, FALSE, 0);

    // Zoom percentage spin button:
    data->ZoomAdjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
		CFG->Zoom, 5, 50, 1, 1, 0));
    g_object_set_data(G_OBJECT(data->ZoomAdjustment),
		"Adjustment-Accuracy", (gpointer)0);
    button = gtk_spin_button_new(data->ZoomAdjustment, 1, 0);
    g_object_set_data(G_OBJECT(data->ZoomAdjustment), "Parent-Widget", button);
    g_signal_connect(G_OBJECT(data->ZoomAdjustment), "value-changed",
		G_CALLBACK(adjustment_update), &CFG->Zoom);
    gtk_box_pack_start(ZoomBox, button, FALSE, FALSE, 0);
    uf_widget_set_tooltip(button, _("Zoom percentage"));

    // Zoom in button:
    button = stock_icon_button(GTK_STOCK_ZOOM_IN, NULL,
	    G_CALLBACK(zoom_in_event), NULL);
    gtk_box_pack_start(ZoomBox, button, FALSE, FALSE, 0);

    // Zoom fit button:
    button = stock_icon_button(GTK_STOCK_ZOOM_FIT, NULL,
	    G_CALLBACK(zoom_fit_event), NULL);
    gtk_box_pack_start(ZoomBox, button, FALSE, FALSE, 0);

    // Zoom max button:
    button = stock_icon_button(GTK_STOCK_ZOOM_100, NULL,
	    G_CALLBACK(zoom_max_event), NULL);
    gtk_box_pack_start(ZoomBox, button, FALSE, FALSE, 0);
#endif // HAVE_GTKIMAGEVIEW

    // The rest of the control button are aligned to the right
    box = GTK_BOX(gtk_hbox_new(FALSE, 6));
    gtk_box_pack_start(GTK_BOX(ControlsBox), GTK_WIDGET(box), FALSE, FALSE, 0);
    /* Options button */
    button = gtk_button_new();
    hbox = GTK_BOX(gtk_hbox_new(FALSE, 6));
    gtk_container_add(GTK_CONTAINER(button), GTK_WIDGET(hbox));
    gtk_box_pack_start(hbox, gtk_image_new_from_stock(
	    GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_BUTTON), FALSE, FALSE, 0);
    gtk_box_pack_start(hbox, gtk_label_new(_("Options")),
	    FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(button), "clicked",
	    G_CALLBACK(options_dialog), previewWindow);
    gtk_box_pack_start(box, button, FALSE, FALSE, 0);

    if (!plugin) {
	// Comment to translator: All control buttons
	// "_Delete", "_Cancel", "_Save", "Save _As", "Send to _Gimp"
	// should have unique mnemonics.
	// Delete button:
	button = control_button(GTK_STOCK_DELETE, _("_Delete"),
		delete_button, data);
	gtk_box_pack_start(box, button, FALSE, FALSE, 0);
    }
    // Cancel button:
    button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    g_signal_connect(G_OBJECT(button), "clicked",
	    G_CALLBACK(control_button_event), cancel_button);
    gtk_box_pack_start(box, button, FALSE, FALSE, 0);

    /* plugin=0 : Normal stand-alone
     * plugin=1 : Gimp plug-in
     * plugin=2 : Cinepaint plug-in
     * plugin=3 : Stand-alone with --output option */
    if ( plugin==1 || plugin==2 ) {
	// OK button for the plug-in
	data->SaveButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_box_pack_start(box, data->SaveButton, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(data->SaveButton), "clicked",
		G_CALLBACK(control_button_event), (gpointer)ok_button);
	gtk_widget_grab_focus(data->SaveButton);
    } else {
	// Save button for the stand-alone tool
	data->SaveButton = gtk_button_new_from_stock(GTK_STOCK_SAVE);
	gtk_box_pack_start(box, data->SaveButton, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(data->SaveButton), "clicked",
		G_CALLBACK(control_button_event), (gpointer)save_button);
	set_save_tooltip(data);
    }
    if ( plugin==0 ) {
	// Send to Gimp button
	GtkWidget *gimpButton = control_button("gimp",
		_("Send image to _Gimp"), gimp_button, data);
	gtk_box_pack_start(box, gimpButton, FALSE, FALSE, 0);
    }
#ifdef HAVE_GTKIMAGEVIEW
    // Apply WindowMaximized state from previous session
    if ( CFG->WindowMaximized )
	gtk_window_maximize(GTK_WINDOW(previewWindow));
#endif
    gtk_widget_show_all(previewWindow);
    gtk_widget_hide(GTK_WIDGET(data->SpotTable));
    for (i=CFG->lightnessAdjustmentCount; i<max_adjustments; i++)
	gtk_widget_hide(GTK_WIDGET(data->LightnessAdjustmentTable[i]));
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), data->PageNumSpot);

    // preview_progress() changes the size of the progress bar
    // and processes the event queue.
    preview_progress(previewWindow, _("Loading preview"), 0.2);
#ifdef HAVE_GTKIMAGEVIEW
#if !GTK_CHECK_VERSION(2,8,0)
    /* There is a bug that causes the mouse location to be misplaced
     * in the event-box. The following voodoo seems to fix the mapping. */
    gtk_widget_set_size_request(scroll, preview_width+1, preview_height+1);
    while (gtk_events_pending()) gtk_main_iteration();
#endif
    /* After window size was set, the user may want to shrink it */
    gtk_widget_set_size_request(scroll, -1, -1);

    if ( CFG->WindowMaximized ) {
	// scroll widget is allocated size only after gtk_widget_show_all()
	int scrollWidth = scroll->allocation.width;
	int scrollHeight = scroll->allocation.height;
	double wScale = (double)data->UF->rotatedWidth / scrollWidth;
	double hScale = (double)data->UF->rotatedHeight / scrollHeight;
	CFG->Zoom = 100/MAX(wScale, hScale);
	CFG->Scale = 0;
	preview_width = uf->rotatedWidth * CFG->Zoom/100;
	preview_height = uf->rotatedHeight * CFG->Zoom/100;
    }
    // Allocate the preview pixbuf
    data->PreviewPixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
	    preview_width, preview_height);
    gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(data->PreviewWidget),
	    data->PreviewPixbuf, FALSE);
    g_object_unref(data->PreviewPixbuf);
    // Get the empty preview displayed
    while (gtk_events_pending()) gtk_main_iteration();
#endif
    for (i=0; i<cursor_num; i++)
	data->Cursor[i] = gdk_cursor_new(Cursors[i]);
    gdk_window_set_cursor(PreviewEventBox->window, data->Cursor[spot_cursor]);
    gtk_widget_set_sensitive(data->Controls, FALSE);
    ufraw_load_raw(uf);
    gtk_widget_set_sensitive(data->Controls, TRUE);

    create_base_image(data);

    /* Collect raw histogram data */
    memset(data->raw_his, 0, sizeof(data->raw_his));
    ufraw_image_data *image = &data->UF->Images[ufraw_first_phase];
    for (i=0; i<image->height*image->width; i++) {
	    guint16 *buf = (guint16*)(image->buffer+i*image->depth);
	    for (c=0; c<data->UF->colors; c++)
		data->raw_his[MIN( buf[c] *
			(raw_his_size-1) / data->UF->rgbMax,
			raw_his_size-1) ][c]++;
    }
    /* Save initial WB data for the sake of "Reset WB" */
    g_strlcpy(data->initialWB, CFG->wb, max_name);
    data->initialTemperature = CFG->temperature;
    data->initialGreen = CFG->green;
    for (i=0; i<4; i++) data->initialChanMul[i] = CFG->chanMul[i];

    /* Update the curve editor in case ufraw_convert_image() modified it. */
    curveeditor_widget_set_curve(data->CurveWidget,
	    &CFG->curve[CFG->curveIndex]);

    data->FreezeDialog = FALSE;
    data->RenderMode = render_default;
    update_crop_ranges(data);
    update_scales(data);

    data->OverUnderTicker = 0;

    gtk_main();
    status = (long)g_object_get_data(G_OBJECT(previewWindow),
	    "WindowResponse");
    gtk_container_foreach(GTK_CONTAINER(previewVBox),
	    (GtkCallback)(expander_state), NULL);

    CFG->WindowMaximized =
	    ( gdk_window_get_state(GTK_WIDGET(previewWindow)->window)
	    & GDK_WINDOW_STATE_MAXIMIZED ) == GDK_WINDOW_STATE_MAXIMIZED;

    ufraw_focus(previewWindow, FALSE);
    gtk_widget_destroy(previewWindow);
    for (i=0; i<cursor_num; i++)
	gdk_cursor_unref(data->Cursor[i]);
    /* Make sure that there are no preview idle task remaining */
    g_idle_remove_by_data(data);
    if (data->BlinkTimer) {
	g_source_remove(data->BlinkTimer);
	data->BlinkTimer = 0;
    }

    if ( status==GTK_RESPONSE_OK ) {
	gboolean SaveRC = FALSE;
	if ( RC->RememberOutputPath!=CFG->RememberOutputPath ) {
	    RC->RememberOutputPath = CFG->RememberOutputPath;
	    SaveRC = TRUE;
	}
	if ( !CFG->RememberOutputPath )
	    g_strlcpy(CFG->outputPath, "", max_path);
	if ( strncmp(RC->outputPath, CFG->outputPath, max_path)!=0 ) {
	    g_strlcpy(RC->outputPath, CFG->outputPath, max_path);
	    SaveRC = TRUE;
	}
	if ( CFG->saveConfiguration==enabled_state ) {
	    /* Save configuration from CFG, but not the output filename. */
	    strcpy(CFG->outputFilename, "");
	    conf_save(CFG, NULL, NULL);
	/* If save 'only this once' was chosen, then so be it */
	} else if ( CFG->saveConfiguration==apply_state ) {
	    CFG->saveConfiguration = disabled_state;
	    /* Save configuration from CFG, but not the output filename. */
	    strcpy(CFG->outputFilename, "");
	    conf_save(CFG, NULL, NULL);
	} else if ( CFG->saveConfiguration==disabled_state ) {
	    /* If save 'never again' was set in this session, we still
	     * need to save this setting */
	    if ( RC->saveConfiguration!=disabled_state ) {
		RC->saveConfiguration = disabled_state;
		conf_save(RC, NULL, NULL);
	    } else if ( SaveRC ) {
		conf_save(RC, NULL, NULL);
	    }
	    strcpy(RC->inputFilename, "");
	    strcpy(RC->outputFilename, "");
	}
    } else {
	strcpy(RC->inputFilename, "");
	strcpy(RC->outputFilename, "");
    }
    // UFRAW_RESPONSE_DELETE requires no special action
    ufraw_close(data->UF);
    g_list_free(data->WBPresets);
    g_free(data->SpotLabels);
    g_free(data->AvrLabels);
    g_free(data->DevLabels);
    g_free(data->OverLabels);
    g_free(data->UnderLabels);

    if (status!=GTK_RESPONSE_OK) return UFRAW_CANCEL;
    return UFRAW_SUCCESS;
}
