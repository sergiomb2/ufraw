/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 * by Udi Fuchs
 *
 * ufraw_saver.c - The GUI file saver.
 *
 * UFRaw is licensed under the GNU General Public License.
 * It uses DCRaw code to do the actual raw decoding.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>     /* for printf */
#include <string.h>
#include <math.h>      /* for fabs, floor */
#include <gtk/gtk.h>
#include "ufraw.h"

void ufraw_radio_button_update(GtkWidget *button, int *valuep)
{
    GtkWidget *dialog = gtk_widget_get_ancestor(button, GTK_TYPE_DIALOG);
    if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
        return;
    *valuep = (long)g_object_get_data(G_OBJECT(button), "ButtonValue");
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    char *base = g_path_get_basename(filename);
    g_free(filename);
    char *newname = uf_file_set_type(base, file_type[*valuep]);
    g_free(base);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), newname);
    g_free(newname);
}

void ufraw_saver_adjustment_update(GObject *adj, gboolean *valuep)
{
    GtkDialog *dialog = GTK_DIALOG(g_object_get_data(adj, "ufraw-dialog"));
    if (!g_object_get_data(G_OBJECT(dialog), "ufraw-dialog-run"))
	return;
    *valuep = TRUE;
    gtk_dialog_response(dialog, GTK_RESPONSE_APPLY);
}

void ufraw_saver_set_type(GtkWidget *widget, cfg_data *cfg)
{
    char *filename, *type;
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
    type = strrchr(filename, '.');
    if (type==NULL) {
        g_free(filename);
        return;
    }
    if (!strcmp(type,".ppm") && cfg->type!=ppm16_type)
        cfg->type = ppm8_type;
    if (!strcmp(type,".tif") && cfg->type!=tiff16_type)
        cfg->type = tiff8_type;
    if (!strcmp(type,".jpg"))
        cfg->type = jpeg_type;
    g_free(filename);
    gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_APPLY);
}

long ufraw_saver(void *widget, gpointer user_data)
{
    GtkWindow *parentWindow;
    GtkFileChooser *fileChooser;
    GtkWidget *expander, *box, *table, *widg, *button, *overwriteButton;
    GtkAdjustment *shrinkAdj, *heightAdj, *widthAdj;
    GtkComboBox *intCombo, *idCombo;
    GtkToggleButton *ppmButton, *tiffButton, *jpegButton;
#ifdef HAVE_LIBZ
    GtkWidget *losslessButton;
#endif
#ifdef HAVE_LIBEXIF
    GtkWidget *exifButton;
#endif
#ifdef HAVE_LIBJPEG
    GtkAdjustment *compressAdj;
#endif
    image_data *image = user_data;
    tiffButton = tiffButton;
    jpegButton = jpegButton;
    float shrink;
    float height, width;
    gboolean shrinkUpdate=FALSE, heightUpdate=FALSE, widthUpdate=FALSE;
    char *filename, *absFilename;
    int status;

    if (image->cfg->size > 0) {
	if (image->height > image->width) {
	    height = image->cfg->size;
	    width = image->cfg->size * image->width / image->height;
	    shrink = (float)image->height / image->cfg->size;
	} else {
            width = image->cfg->size;
	    height = image->cfg->size * image->height / image->width;
	    shrink = (float)image->width / image->cfg->size;
	}
    } else {
	if (image->cfg->shrink<1) {
            ufraw_message(UFRAW_ERROR, "Fatal Error: image->cfg->shrink<1");
	    image->cfg->shrink = 1;
	}
	height = image->height / image->cfg->shrink;
	width = image->width / image->cfg->shrink;
	shrink = image->cfg->shrink;
    }
    filename = uf_file_set_type(image->filename, file_type[image->cfg->type]);
    if (strlen(image->cfg->outputPath)>0) {
	char *cp = g_path_get_basename(filename);
	g_free(filename);
	filename = g_build_filename(image->cfg->outputPath, cp, NULL);
	g_free(cp);
    }
    absFilename = uf_file_set_absolute(filename);
    if (widget==NULL) {
	char *text = g_strdup_printf("Filename: %s\nSize: %d x %d%s",
		absFilename, (int)height, (int)width,
		image->cfg->createID==also_id ? "\nCreate also ID file" :
		image->cfg->createID==only_id ? "\nCreate only ID file" : "");
	return (long)text;
    }
    parentWindow = GTK_WINDOW(gtk_widget_get_toplevel(widget));
    if (!strcmp(gtk_button_get_label(GTK_BUTTON(widget)), GTK_STOCK_SAVE)) {
        if ( !image->cfg->overwrite && image->cfg->createID!=only_id
	   && g_file_test(absFilename, G_FILE_TEST_EXISTS) ) {
            GtkWidget *dialog;
            char message[max_path];
            int response;
            dialog = gtk_dialog_new_with_buttons("File exists", parentWindow,
                GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_NO, GTK_RESPONSE_NO,
                GTK_STOCK_YES, GTK_RESPONSE_YES, NULL);
            snprintf(message, max_path,
                "File '%s' already exists.\nOverwrite?", filename);
            widg = gtk_label_new(message);
            gtk_label_set_line_wrap(GTK_LABEL(widg), TRUE);
            gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), widg);
            gtk_widget_show_all(dialog);
            response = gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            if (response!=GTK_RESPONSE_YES)
                return UFRAW_SUCCESS;
        }
	g_strlcpy(image->cfg->outputFilename, absFilename, max_path);
        g_free(filename);
	g_free(absFilename);
        status = ufraw_write_image(image);
	if (status==UFRAW_SUCCESS) {
            g_object_set_data(G_OBJECT(parentWindow), "WindowResponse",
                    (gpointer)GTK_RESPONSE_OK);
            gtk_main_quit();
        } else if (status==UFRAW_ABORT_SAVE) {
            g_object_set_data(G_OBJECT(parentWindow), "WindowResponse",
                    (gpointer)GTK_RESPONSE_CANCEL);
            gtk_main_quit();
        } else
	    preview_progress("", 0);
        return UFRAW_SUCCESS;
    }
    fileChooser = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new(
            "Save image", parentWindow, GTK_FILE_CHOOSER_ACTION_SAVE,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
            GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL));

    gtk_dialog_set_default_response(GTK_DIALOG(fileChooser), GTK_RESPONSE_OK);
    ufraw_focus(fileChooser, TRUE);
#ifdef HAVE_GTK_2_6
    gtk_file_chooser_set_show_hidden(fileChooser, TRUE);
#endif
    char *path = g_path_get_dirname(absFilename);
    gtk_file_chooser_set_current_folder(fileChooser, path);
    g_free(path);
    char *base = g_path_get_basename(filename);
    gtk_file_chooser_set_current_name(fileChooser, base);
    g_free(base);

    expander = gtk_expander_new("Output options");
    gtk_expander_set_expanded(GTK_EXPANDER(expander), TRUE);
    gtk_file_chooser_set_extra_widget(fileChooser, expander);
    box = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(expander), box);

    if (image->cfg->interpolation==half_interpolation) {
        image->cfg->interpolation = full_interpolation;
        if (image->cfg->shrink<2) image->cfg->shrink = 2;
    }
    intCombo = GTK_COMBO_BOX(gtk_combo_box_new_text());
    gtk_combo_box_append_text(intCombo, "Full interpolation");
    gtk_combo_box_append_text(intCombo, "Four color interpolation");
    gtk_combo_box_append_text(intCombo, "Quick interpolation");
    gtk_combo_box_set_active(intCombo, image->cfg->interpolation);
    gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(intCombo), TRUE, TRUE, 0);

    table = gtk_table_new(5, 1, FALSE);
    gtk_box_pack_start(GTK_BOX(box), table, TRUE, TRUE, 0);
    widg = gtk_label_new("Shrink factor");
    gtk_table_attach(GTK_TABLE(table), widg, 0, 1, 1, 2, 0, 0, 0, 0);
    shrinkAdj = GTK_ADJUSTMENT(gtk_adjustment_new(shrink,
            1, 100, 1, 2, 3));
    g_object_set_data(G_OBJECT(shrinkAdj), "ufraw-dialog", fileChooser);
    g_signal_connect(G_OBJECT(shrinkAdj), "value-changed",
            G_CALLBACK(ufraw_saver_adjustment_update), &shrinkUpdate);
    widg = gtk_spin_button_new(shrinkAdj, 1, 3);
    gtk_table_attach(GTK_TABLE(table), widg, 1, 2, 1, 2, 0, 0, 0, 0);

    widg = gtk_label_new(" Height");
    gtk_table_attach(GTK_TABLE(table), widg, 2, 3, 1, 2, 0, 0, 0, 0);
    heightAdj = GTK_ADJUSTMENT(gtk_adjustment_new(height,
            image->height/100, image->height, 10, 100, 0));
    g_object_set_data(G_OBJECT(heightAdj), "ufraw-dialog", fileChooser);
    g_signal_connect(G_OBJECT(heightAdj), "value-changed",
            G_CALLBACK(ufraw_saver_adjustment_update), &heightUpdate);
    widg = gtk_spin_button_new(heightAdj, 10, 0);
    gtk_table_attach(GTK_TABLE(table), widg, 3, 4, 1, 2, 0, 0, 0, 0);

    widg = gtk_label_new(" Width");
    gtk_table_attach(GTK_TABLE(table), widg, 4, 5, 1, 2, 0, 0, 0, 0);
    widthAdj = GTK_ADJUSTMENT(gtk_adjustment_new(width,
            image->width/100, image->width, 10, 100, 0));
    g_object_set_data(G_OBJECT(widthAdj), "ufraw-dialog", fileChooser);
    g_signal_connect(G_OBJECT(widthAdj), "value-changed",
            G_CALLBACK(ufraw_saver_adjustment_update), &widthUpdate);
    widg = gtk_spin_button_new(widthAdj, 10, 0);
    gtk_table_attach(GTK_TABLE(table), widg, 5, 6, 1, 2, 0, 0, 0, 0);

    gtk_box_pack_start(GTK_BOX(box), gtk_hseparator_new(), TRUE, TRUE, 0);
    button = gtk_radio_button_new_with_label(NULL, "8-bit ppm");
    ppmButton = GTK_TOGGLE_BUTTON(button);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
            image->cfg->type==ppm8_type);
    g_object_set_data(G_OBJECT(button), "ButtonValue", (gpointer)ppm8_type);
    g_signal_connect(G_OBJECT(button), "toggled",
            G_CALLBACK(ufraw_radio_button_update), &image->cfg->type);
    gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);

    button = gtk_radio_button_new_with_label_from_widget(
            GTK_RADIO_BUTTON(button), "16-bit ppm");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
            image->cfg->type==ppm16_type);
    g_object_set_data(G_OBJECT(button), "ButtonValue", (gpointer)ppm16_type);
    g_signal_connect(G_OBJECT(button), "toggled",
            G_CALLBACK(ufraw_radio_button_update), &image->cfg->type);
    gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);
#ifdef HAVE_LIBTIFF
    gtk_box_pack_start(GTK_BOX(box), gtk_hseparator_new(), TRUE, TRUE, 0);
    table = gtk_table_new(5, 1, FALSE);
    gtk_box_pack_start(GTK_BOX(box), table, TRUE, TRUE, 0);
    button = gtk_radio_button_new_with_label_from_widget(
            GTK_RADIO_BUTTON(button), "8-bit TIFF");
    tiffButton = GTK_TOGGLE_BUTTON(button);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
            image->cfg->type==tiff8_type);
    g_object_set_data(G_OBJECT(button), "ButtonValue", (gpointer)tiff8_type);
    g_signal_connect(G_OBJECT(button), "toggled",
            G_CALLBACK(ufraw_radio_button_update), &image->cfg->type);
    gtk_table_attach(GTK_TABLE(table), button, 0, 1, 0, 1, 0, 0, 0, 0);
    button = gtk_radio_button_new_with_label_from_widget(
            GTK_RADIO_BUTTON(button), "16-bit TIFF");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
            image->cfg->type==tiff16_type);
    g_object_set_data(G_OBJECT(button), "ButtonValue", (gpointer)tiff16_type);
    g_signal_connect(G_OBJECT(button), "toggled",
            G_CALLBACK(ufraw_radio_button_update), &image->cfg->type);
    gtk_table_attach(GTK_TABLE(table), button, 0, 1, 1, 2, 0, 0, 0, 0);
#ifdef HAVE_LIBZ
    losslessButton = gtk_check_button_new_with_label("ZIP Compress (lossless)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(losslessButton),
                    image->cfg->losslessCompress);
    gtk_table_attach(GTK_TABLE(table), losslessButton, 1, 2, 2, 3, 0, 0, 0, 0);
#endif
#endif /*HAVE_LIBTIFF*/
#ifdef HAVE_LIBJPEG
    gtk_box_pack_start(GTK_BOX(box), gtk_hseparator_new(), TRUE, TRUE, 0);
    table = gtk_table_new(5, 1, FALSE);
    gtk_box_pack_start(GTK_BOX(box), table, TRUE, TRUE, 0);
    button = gtk_radio_button_new_with_label_from_widget(
            GTK_RADIO_BUTTON(button), "JPEG");
    jpegButton = GTK_TOGGLE_BUTTON(button);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
            image->cfg->type==jpeg_type);
    g_object_set_data(G_OBJECT(button), "ButtonValue", (gpointer)jpeg_type);
    g_signal_connect(G_OBJECT(button), "toggled",
            G_CALLBACK(ufraw_radio_button_update), &image->cfg->type);
    gtk_table_attach(GTK_TABLE(table), button, 0, 1, 0, 1, 0, 0, 0, 0);
    widg = gtk_label_new("Compression level");
    gtk_table_attach(GTK_TABLE(table), widg, 1, 2, 1, 2, 0, 0, 0, 0);
    compressAdj = GTK_ADJUSTMENT(gtk_adjustment_new(image->cfg->compression,
            0, 100, 5, 10, 0));
    widg = gtk_hscale_new(compressAdj);
    gtk_scale_set_draw_value(GTK_SCALE(widg), FALSE);
    gtk_table_attach(GTK_TABLE(table), widg, 2, 3, 1, 2,
            GTK_EXPAND|GTK_FILL, 0, 0, 0);
    widg = gtk_spin_button_new(compressAdj, 5, 0);
    gtk_table_attach(GTK_TABLE(table), widg, 3, 4, 1, 2, 0, 0, 0, 0);
#ifdef HAVE_LIBEXIF
    exifButton = gtk_check_button_new_with_label("Embed EXIF data");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(exifButton),
                    image->cfg->embedExif && image->exifBuf!=NULL);
    gtk_widget_set_sensitive(exifButton, image->exifBuf!=NULL);
    gtk_table_attach(GTK_TABLE(table), exifButton, 1, 2, 2, 3, 0, 0, 0, 0);
#endif
#endif /*HAVE_LIBJPEG*/

    gtk_box_pack_start(GTK_BOX(box), gtk_hseparator_new(), TRUE, TRUE, 0);
    idCombo = GTK_COMBO_BOX(gtk_combo_box_new_text());
    gtk_combo_box_append_text(idCombo, "Do not create ID file");
    gtk_combo_box_append_text(idCombo, "Create also ID file");
    gtk_combo_box_append_text(idCombo, "Create only ID file");
    gtk_combo_box_set_active(idCombo, image->cfg->createID);
    gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(idCombo), TRUE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box), gtk_hseparator_new(), TRUE, TRUE, 0);
    overwriteButton = gtk_check_button_new_with_label(
            "Overwrite existing files without asking");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(overwriteButton),
            image->cfg->overwrite);
    gtk_box_pack_start(GTK_BOX(box), overwriteButton, TRUE, TRUE, 0);
    gtk_widget_show_all(expander);

    if (strlen(image->cfg->inputFilename)>0) {
        char *cp = g_path_get_dirname(image->cfg->inputFilename);
        if (strcmp(cp,"."))
            gtk_file_chooser_add_shortcut_folder(fileChooser, cp, NULL);
        g_free(cp);
    }
    g_signal_connect(G_OBJECT(fileChooser), "selection-changed",
            G_CALLBACK(ufraw_saver_set_type), image->cfg);
    while(1) {
        g_object_set_data(G_OBJECT(fileChooser), "ufraw-dialog-run",
		(gpointer)TRUE);
        status = gtk_dialog_run(GTK_DIALOG(fileChooser));
        g_object_set_data(G_OBJECT(fileChooser), "ufraw-dialog-run",
		(gpointer)FALSE);
        if (status==GTK_RESPONSE_CANCEL) {
	    ufraw_focus(fileChooser, FALSE);
            gtk_widget_destroy(GTK_WIDGET(fileChooser));
            return UFRAW_CANCEL;
        }
        if (status==GTK_RESPONSE_APPLY) {
            gtk_toggle_button_set_active(ppmButton,
                    image->cfg->type==ppm8_type);
#ifdef HAVE_LIBTIFF
            gtk_toggle_button_set_active(tiffButton,
                    image->cfg->type==tiff8_type);
#endif
#ifdef HAVE_LIBJPEG
            gtk_toggle_button_set_active(jpegButton,
                    image->cfg->type==jpeg_type);
#endif
	    if (shrinkUpdate) {
	        shrink = gtk_adjustment_get_value(shrinkAdj);
	        height = image->height / shrink;
	        width = image->width / shrink;
		shrinkUpdate = FALSE;
	    }
	    if (heightUpdate) {
	        height = gtk_adjustment_get_value(heightAdj);
	        width = height * image->width / image->height;
	        shrink = (float)image->height / height;
		heightUpdate = FALSE;
	    }
	    if (widthUpdate) {
	        width = gtk_adjustment_get_value(widthAdj);
	        height = width * image->height / image->width;
	        shrink = (float)image->width / width;
		widthUpdate = FALSE;
	    }
	    gtk_adjustment_set_value(shrinkAdj, shrink);
	    gtk_adjustment_set_value(heightAdj, height);
	    gtk_adjustment_set_value(widthAdj, width);
            continue;
        }
        filename = gtk_file_chooser_get_filename(fileChooser);
        image->cfg->interpolation = gtk_combo_box_get_active(intCombo);
        image->cfg->createID = gtk_combo_box_get_active(idCombo);
	if (fabs(shrink-floor(shrink+0.0005))<0.0005) {
	    image->cfg->shrink = floor(shrink+0.0005);
	    image->cfg->size = 0;
	} else {
	    image->cfg->shrink = 1;
	    image->cfg->size = floor(MAX(height, width)+0.5);
	}
        image->cfg->overwrite = gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(overwriteButton));
#ifdef HAVE_LIBZ
        image->cfg->losslessCompress = gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(losslessButton));
#endif
#ifdef HAVE_LIBJPEG
        image->cfg->compression = gtk_adjustment_get_value(compressAdj);
#endif
#ifdef HAVE_LIBEXIF
        image->cfg->embedExif = gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(exifButton));
#endif
        if ( !image->cfg->overwrite && image->cfg->createID!=only_id
	   && g_file_test(filename, G_FILE_TEST_EXISTS) ) {
            GtkWidget *dialog;
            char message[max_path];
            int response;
            dialog = gtk_dialog_new_with_buttons("File exists",
                GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(fileChooser))),
                GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_NO, GTK_RESPONSE_NO,
                GTK_STOCK_YES, GTK_RESPONSE_YES, NULL);
            snprintf(message, max_path,
                "File '%s' already exists.\nOverwrite?", filename);
            widg = gtk_label_new(message);
            gtk_label_set_line_wrap(GTK_LABEL(widg), TRUE);
            gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), widg);
            gtk_widget_show_all(dialog);
            response = gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            if (response!=GTK_RESPONSE_YES)
                continue;
        }
	g_strlcpy(image->cfg->outputFilename, filename, max_path);
        g_free(filename);
        status = ufraw_write_image(image);
        if (status==UFRAW_SUCCESS || status==UFRAW_ABORT_SAVE) {
	    ufraw_focus(fileChooser, FALSE);
            gtk_widget_destroy(GTK_WIDGET(fileChooser));
            if (status==UFRAW_SUCCESS)
                g_object_set_data(G_OBJECT(parentWindow), "WindowResponse",
                        (gpointer)GTK_RESPONSE_OK);
            else
                g_object_set_data(G_OBJECT(parentWindow), "WindowResponse",
                        (gpointer)GTK_RESPONSE_CANCEL);
            gtk_main_quit();
            return UFRAW_SUCCESS;
        }
    }
}
