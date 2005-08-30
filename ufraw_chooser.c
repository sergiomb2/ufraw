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

#ifdef HAVE_GTK_2_6
void ufraw_chooser_toggle(GtkToggleButton *button, GtkFileChooser *fileChooser)
{
    gtk_file_chooser_set_show_hidden(fileChooser,
	    gtk_toggle_button_get_active(button));
}
#endif

void ufraw_chooser(conf_data *conf, char *defPath)
{
    ufraw_data *uf;
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

    if (defPath!=NULL) {
	char *fullPath = uf_file_set_absolute(defPath);
	gtk_file_chooser_set_current_folder(fileChooser, fullPath);
	g_free(fullPath);
    }
    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, "Raw images");
    extList = g_strsplit(raw_ext, ",", 100);
    for (l=extList; *l!=NULL; l++)
        if (strcmp(*l, "jpg") && strcmp(*l, "tif") && strcmp(*l, "ufraw")) {
            snprintf(ext, max_name, "*.%s", *l);
            gtk_file_filter_add_pattern(filter, ext);
            gtk_file_filter_add_pattern(filter, cp=g_ascii_strup(ext,-1));
            g_free(cp);
        }
    g_strfreev(extList);
    gtk_file_chooser_add_filter(fileChooser, filter);

    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, "UFRaw ID files");
    gtk_file_filter_add_pattern(filter, "*.ufraw");
    gtk_file_filter_add_pattern(filter, "*.UFRAW");
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
    gtk_file_chooser_set_show_hidden(fileChooser, FALSE);
    GtkWidget *button = gtk_check_button_new_with_label( "Show hidden files");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    g_signal_connect(G_OBJECT(button), "toggled",
	    G_CALLBACK(ufraw_chooser_toggle), fileChooser);
    gtk_file_chooser_set_extra_widget(fileChooser, button);
#endif
    gtk_file_chooser_set_select_multiple(fileChooser, TRUE);
    /* Add shortcut to folder of last opened file */
    if (strlen(conf->inputFilename)>0) {
        char *cp = g_path_get_dirname(conf->inputFilename);
        gtk_file_chooser_add_shortcut_folder( fileChooser, cp, NULL);
        g_free(cp);
    }
    gtk_widget_show(GTK_WIDGET(fileChooser));
    while (gtk_dialog_run(GTK_DIALOG(fileChooser))==GTK_RESPONSE_ACCEPT) {
	/* Load $HOME/.ufrawrc */
	conf_load(conf, NULL);	    
        for(list=saveList=gtk_file_chooser_get_filenames(fileChooser);
        list!=NULL; list=g_slist_next(list)) {
            filename = list->data;
            uf = ufraw_open(filename);
            if (uf==NULL) {
                ufraw_message(UFRAW_REPORT, NULL);
                continue;
            }
            ufraw_config(uf, conf, NULL, NULL);
            ufraw_preview(uf, FALSE, ufraw_saver);
            g_free(filename);
	    *conf = *uf->conf;
        }
        g_slist_free(saveList);
    }
    gtk_widget_destroy(GTK_WIDGET(fileChooser));
    ufraw_message(UFRAW_SET_PARENT, NULL);
}
