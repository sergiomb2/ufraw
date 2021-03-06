/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw_chooser.c - A very simple file chooser for UFRaw.
 * Copyright 2004-2016 by Udi Fuchs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "ufraw.h"
#include "uf_gtk.h"
#include <string.h>
#include <glib/gi18n.h>

void ufraw_chooser_toggle(GtkToggleButton *button, GtkFileChooser *fileChooser)
{
    gtk_file_chooser_set_show_hidden(fileChooser,
                                     gtk_toggle_button_get_active(button));
}

/* Create a GtkFileChooser dialog for selecting raw files */
GtkFileChooser *ufraw_raw_chooser(conf_data *conf,
                                  const char *defPath,
                                  const gchar *label,
                                  GtkWindow *toplevel,
                                  const gchar *cancel,
                                  gboolean multiple)
{
    GtkFileChooser *fileChooser;
    GtkFileFilter *filter;
    char *cp;
    char **extList, **l, ext[max_name];

    fileChooser = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new(label, toplevel,
                                   GTK_FILE_CHOOSER_ACTION_OPEN,
                                   cancel, GTK_RESPONSE_CANCEL,
                                   GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL));
    if (toplevel == NULL)
        gtk_window_set_type_hint(GTK_WINDOW(fileChooser),
                                 GDK_WINDOW_TYPE_HINT_NORMAL);
    else
        ufraw_focus(fileChooser, TRUE);
    gtk_window_set_icon_name(GTK_WINDOW(fileChooser), "ufraw");
    ufraw_message(UFRAW_SET_PARENT, (char *)fileChooser);

    if (defPath != NULL) {
        char *fullPath = uf_file_set_absolute(defPath);
        gtk_file_chooser_set_current_folder(fileChooser, fullPath);
        g_free(fullPath);
    }
    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, _("Raw images"));
    extList = g_strsplit(raw_ext, ",", 100);
    for (l = extList; *l != NULL; l++)
        if (strcmp(*l, "jpeg") && strcmp(*l, "jpg") && strcmp(*l, "tif") &&
                strcmp(*l, "tiff") && strcmp(*l, "ufraw")) {
            snprintf(ext, max_name, "*.%s", *l);
            gtk_file_filter_add_pattern(filter, ext);
            gtk_file_filter_add_pattern(filter, cp = g_ascii_strup(ext, -1));
            g_free(cp);
#ifdef HAVE_LIBZ
            snprintf(ext, max_name, "*.%s.gz", *l);
            gtk_file_filter_add_pattern(filter, ext);
            snprintf(ext, max_name, "*.%s.GZ", *l);
            gtk_file_filter_add_pattern(filter, ext);
            snprintf(ext, max_name, "*.%s.gz", cp = g_ascii_strup(*l, -1));
            g_free(cp);
            gtk_file_filter_add_pattern(filter, ext);
            snprintf(ext, max_name, "*.%s.GZ", cp = g_ascii_strup(*l, -1));
            g_free(cp);
            gtk_file_filter_add_pattern(filter, ext);
#endif
#ifdef HAVE_LIBBZ2
            snprintf(ext, max_name, "*.%s.bz2", *l);
            gtk_file_filter_add_pattern(filter, ext);
            snprintf(ext, max_name, "*.%s.BZ2", *l);
            gtk_file_filter_add_pattern(filter, ext);
            snprintf(ext, max_name, "*.%s.bz2", cp = g_ascii_strup(*l, -1));
            g_free(cp);
            gtk_file_filter_add_pattern(filter, ext);
            snprintf(ext, max_name, "*.%s.BZ2", cp = g_ascii_strup(*l, -1));
            g_free(cp);
            gtk_file_filter_add_pattern(filter, ext);
#endif
        }
    g_strfreev(extList);
    gtk_file_chooser_add_filter(fileChooser, filter);

    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, _("UFRaw ID files"));
    gtk_file_filter_add_pattern(filter, "*.ufraw");
    gtk_file_filter_add_pattern(filter, "*.UFRAW");
    gtk_file_chooser_add_filter(fileChooser, filter);

    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, _("Raw jpeg's"));
    gtk_file_filter_add_pattern(filter, "*.jpeg");
    gtk_file_filter_add_pattern(filter, "*.jpg");
    gtk_file_filter_add_pattern(filter, "*.JPEG");
    gtk_file_filter_add_pattern(filter, "*.JPG");
    gtk_file_chooser_add_filter(fileChooser, filter);

    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, _("Raw tiff's"));
    gtk_file_filter_add_pattern(filter, "*.tif");
    gtk_file_filter_add_pattern(filter, "*.tiff");
    gtk_file_filter_add_pattern(filter, "*.TIF");
    gtk_file_filter_add_pattern(filter, "*.TIFF");
    gtk_file_chooser_add_filter(fileChooser, filter);

    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, _("All files"));
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(fileChooser, filter);

    gtk_file_chooser_set_show_hidden(fileChooser, FALSE);
    GtkWidget *button = gtk_check_button_new_with_label(_("Show hidden files"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    g_signal_connect(G_OBJECT(button), "toggled",
                     G_CALLBACK(ufraw_chooser_toggle), fileChooser);
    gtk_file_chooser_set_extra_widget(fileChooser, button);
    if (multiple)
        gtk_file_chooser_set_select_multiple(fileChooser, TRUE);
    /* Add shortcut to folder of last opened file */
    if (strlen(conf->inputFilename) > 0) {
        char *cp = g_path_get_dirname(conf->inputFilename);
        gtk_file_chooser_add_shortcut_folder(fileChooser, cp, NULL);
        g_free(cp);
    }
    gtk_widget_show(GTK_WIDGET(fileChooser));
    return fileChooser;
}

void ufraw_chooser(conf_data *rc, conf_data *conf, conf_data *cmd,
                   const char *defPath)
{
    ufraw_data *uf;
    GtkFileChooser *fileChooser;
    GSList *list, *saveList;
    char *filename;

    fileChooser = ufraw_raw_chooser(rc, defPath, "UFRaw", NULL,
                                    GTK_STOCK_QUIT, TRUE);

    while (gtk_dialog_run(GTK_DIALOG(fileChooser)) == GTK_RESPONSE_ACCEPT) {
        for (list = saveList = gtk_file_chooser_get_filenames(fileChooser);
                list != NULL; list = g_slist_next(list)) {
            filename = list->data;
            uf = ufraw_open(filename);
            if (uf == NULL) {
                ufraw_message(UFRAW_REPORT, NULL);
                continue;
            }
            int status = ufraw_config(uf, rc, conf, cmd);
            if (status == UFRAW_ERROR) {
                ufraw_close_darkframe(uf->conf);
                ufraw_close(uf);
            } else {
                ufraw_preview(uf, rc, FALSE, NULL);
            }
            g_free(uf);
            g_free(filename);
        }
        g_slist_free(saveList);
    }
    if (rc->darkframe != NULL)
        ufraw_close_darkframe(rc);

    gtk_widget_destroy(GTK_WIDGET(fileChooser));
    ufraw_message(UFRAW_SET_PARENT, NULL);
}
