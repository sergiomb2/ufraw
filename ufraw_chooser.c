/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 * by Udi Fuchs
 *
 * ufraw_chooser.c - A very simple file chooser for UFRaw.
 *
 * UFRaw is licensed under the GNU General Public License.
 * It uses DCRaw code to do the actual raw decoding.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>     /* for printf */
#include <string.h>
#include <gtk/gtk.h>
#include "ufraw.h"
#include "ufraw_icon.h"

void ufraw_chooser(cfg_data *cfg, char *defPath)
{
    image_data *image;
    GtkFileChooser *fileChooser;
    GSList *list, *saveList;
    GtkFileFilter *filter;
    char *filename, *cp;
    char **extList, **l, ext[max_name];

    fileChooser = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new("UFRaw", NULL,
            GTK_FILE_CHOOSER_ACTION_OPEN,
            GTK_STOCK_QUIT, GTK_RESPONSE_CANCEL,
            GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL));
    gtk_window_set_type_hint(GTK_WINDOW(fileChooser),
	    GDK_WINDOW_TYPE_HINT_NORMAL);
    gtk_window_set_icon(GTK_WINDOW(fileChooser),
            gdk_pixbuf_new_from_inline(-1, ufraw_icon, FALSE, NULL));
    ufraw_message(UFRAW_SET_PARENT, (char *)fileChooser);

    if (defPath!=NULL)
	gtk_file_chooser_set_current_folder(fileChooser, defPath);
    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, "Raw images");
    extList = g_strsplit(raw_ext, ",", 100);
    for (l=extList; *l!=NULL; l++)
        if (strcmp(*l, "jpg") && strcmp(*l, "tif")) {
            snprintf(ext, max_name, "*.%s", *l);
            gtk_file_filter_add_pattern(filter, ext);
            gtk_file_filter_add_pattern(filter, cp=g_ascii_strup(ext,-1));
            g_free(cp);
        }
    g_strfreev(extList);
    gtk_file_chooser_add_filter(fileChooser, filter);

    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, "Raw jpg's");
    gtk_file_filter_add_pattern(filter, "*.jpg");
    gtk_file_filter_add_pattern(filter, "*.JPG");
    gtk_file_chooser_add_filter(fileChooser, filter);

    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, "Raw tif's");
    gtk_file_filter_add_pattern(filter, "*.tif");
    gtk_file_filter_add_pattern(filter, "*.TIF");
    gtk_file_chooser_add_filter(fileChooser, filter);

    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, "All files");
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(fileChooser, filter);

#ifdef HAVE_GTK_2_6
    gtk_file_chooser_set_show_hidden(fileChooser, TRUE);
#endif
    gtk_file_chooser_set_select_multiple(fileChooser, TRUE);
    /* Add shortcut to folder of last opened file */
    if (strlen(cfg->inputFilename)>0) {
        char *cp = g_path_get_dirname(cfg->inputFilename);
        gtk_file_chooser_add_shortcut_folder( fileChooser, cp, NULL);
        g_free(cp);
    }
    gtk_widget_show(GTK_WIDGET(fileChooser));
    while (gtk_dialog_run(GTK_DIALOG(fileChooser))==GTK_RESPONSE_ACCEPT) {
        for(list=saveList=gtk_file_chooser_get_filenames(fileChooser);
        list!=NULL; list=g_slist_next(list)) {
            filename = list->data;
            image = ufraw_open(filename);
            if (image==NULL) {
                ufraw_message(UFRAW_REPORT, NULL);
                continue;
            }
            ufraw_config(image, cfg);
            ufraw_preview(image, FALSE, ufraw_saver);
            g_free(filename);
        }
        g_slist_free(saveList);
    }
    gtk_widget_destroy(GTK_WIDGET(fileChooser));
    ufraw_message(UFRAW_SET_PARENT, NULL);
}
