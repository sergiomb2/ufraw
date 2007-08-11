/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw_saver.c - The GUI file saver.
 * Copyright 2004-2007 by Udi Fuchs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation. You should have received
 * a copy of the license along with this program.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>	/* for printf */
#include <string.h>
#include <math.h>	/* for fabs, floor */
#include <gtk/gtk.h>
#include <locale.h>
#include <glib/gi18n.h>
#include "ufraw.h"

/* Global information for the 'Save As' dialog */
typedef struct {
    ufraw_data *uf;
    GtkToggleButton *ppmButton;
    GtkToggleButton *tiffButton;
    GtkToggleButton *jpegButton;
} save_as_dialog_data;

#if GTK_CHECK_VERSION(2,6,0)
void ufraw_chooser_toggle(GtkToggleButton *button, GtkFileChooser *filechooser);
#endif

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

void ufraw_saver_set_type(GtkWidget *widget, save_as_dialog_data *data)
{
    char *filename, *type;
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
    type = strrchr(filename, '.');
    if (type==NULL) {
	g_free(filename);
	return;
    }
    if ( !strcmp(type,".ppm") && data->uf->conf->type!=ppm16_type ) {
	data->uf->conf->type = ppm8_type;
	gtk_toggle_button_set_active(data->ppmButton, TRUE);
    }
#ifdef HAVE_LIBTIFF
    if ( !strcmp(type,".tif") && data->uf->conf->type!=tiff16_type ) {
	data->uf->conf->type = tiff8_type;
	gtk_toggle_button_set_active(data->tiffButton, TRUE);
    }
#endif
#ifdef HAVE_LIBJPEG
    if ( !strcmp(type,".jpg") ) {
	data->uf->conf->type = jpeg_type;
	gtk_toggle_button_set_active(data->jpegButton, TRUE);
    }
#endif
    g_free(filename);
}

int ufraw_overwrite_dialog(char *filename, GtkWidget *widget)
{
    char message[max_path];
    int response;
    GtkWidget *dialog = gtk_dialog_new_with_buttons(_("File exists"),
	    GTK_WINDOW(gtk_widget_get_toplevel(widget)),
	    GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
	    GTK_STOCK_NO, GTK_RESPONSE_NO,
	    GTK_STOCK_YES, GTK_RESPONSE_YES, NULL);
    char *utf8 = g_filename_to_utf8(filename, -1, NULL, NULL, NULL);
    if (utf8==NULL) utf8 = g_strdup("Unknown file name");
    snprintf(message, max_path,
	    _("File '%s' already exists.\nOverwrite?"), utf8);
    g_free(utf8);
    GtkWidget *label = gtk_label_new(message);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), label);
    gtk_widget_show_all(dialog);
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return (response==GTK_RESPONSE_YES);
}

long ufraw_saver(void *widget, gpointer user_data)
{
    GtkFileChooser *fileChooser;
    GtkWidget *expander, *box, *table, *widg, *button, *align, *overwriteButton;
    GtkWidget *event, *label;
    GtkComboBox *idCombo, *confCombo;
#if defined(HAVE_LIBZ) && defined(HAVE_LIBTIFF)
    GtkWidget *losslessButton;
#endif
#ifdef HAVE_LIBJPEG
#if defined(HAVE_LIBEXIF) || defined(HAVE_EXIV2)
    GtkWidget *exifButton;
#endif
    GtkAdjustment *compressAdj;
#endif
    ufraw_data *uf = user_data;
    int status;
    save_as_dialog_data DialogData, *data = &DialogData;
    data->uf = uf;

    if (!strcmp(gtk_button_get_label(GTK_BUTTON(widget)), GTK_STOCK_SAVE)) {
	if ( !uf->conf->overwrite && uf->conf->createID!=only_id
	   && g_file_test(uf->conf->outputFilename, G_FILE_TEST_EXISTS) ) {
	    if ( !ufraw_overwrite_dialog(uf->conf->outputFilename, widget) )
		return UFRAW_ERROR;
	}
	status = ufraw_write_image(uf);
	return status;
    }
    fileChooser = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new(
	    _("Save image"), GTK_WINDOW(gtk_widget_get_toplevel(widget)),
	    GTK_FILE_CHOOSER_ACTION_SAVE,
	    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	    GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL));

    gtk_dialog_set_default_response(GTK_DIALOG(fileChooser), GTK_RESPONSE_OK);
    ufraw_focus(fileChooser, TRUE);
#if GTK_CHECK_VERSION(2,6,0)
    gtk_file_chooser_set_show_hidden(fileChooser, FALSE);
    GtkWidget *hidden_button = gtk_check_button_new_with_label( _("Show hidden files"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hidden_button), FALSE);
    g_signal_connect(G_OBJECT(hidden_button), "toggled",
	    G_CALLBACK(ufraw_chooser_toggle), fileChooser);
    gtk_file_chooser_set_extra_widget(fileChooser, hidden_button);
#endif
    char *absFilename = uf_file_set_absolute(uf->conf->outputFilename);
    char *path = g_path_get_dirname(absFilename);
    gtk_file_chooser_set_current_folder(fileChooser, path);
    g_free(path);
    g_free(absFilename);
    char *base = g_path_get_basename(uf->conf->outputFilename);
    gtk_file_chooser_set_current_name(fileChooser, base);
    g_free(base);

    GtkTooltips *tips = gtk_tooltips_new();
#if GTK_CHECK_VERSION(2,10,0)
    g_object_ref_sink(GTK_OBJECT(tips));
#else
    g_object_ref(tips);
    gtk_object_sink(GTK_OBJECT(tips));
#endif

    expander = gtk_expander_new(_("Output options"));
    gtk_expander_set_expanded(GTK_EXPANDER(expander), TRUE);
    gtk_file_chooser_set_extra_widget(fileChooser, expander);
    box = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(expander), box);

    if (uf->conf->interpolation==half_interpolation) {
	uf->conf->interpolation = ahd_interpolation;
	if (uf->conf->shrink<2) uf->conf->shrink = 2;
	ufraw_message(UFRAW_WARNING, _("Interpolation method set to AHD"));
    }

    gtk_box_pack_start(GTK_BOX(box), gtk_hseparator_new(), TRUE, TRUE, 0);
    button = gtk_radio_button_new_with_label(NULL, _("8-bit ppm"));
    data->ppmButton = GTK_TOGGLE_BUTTON(button);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
	    uf->conf->type==ppm8_type);
    g_object_set_data(G_OBJECT(button), "ButtonValue", (gpointer)ppm8_type);
    g_signal_connect(G_OBJECT(button), "toggled",
	    G_CALLBACK(ufraw_radio_button_update), &uf->conf->type);
    gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);

    button = gtk_radio_button_new_with_label_from_widget(
	    GTK_RADIO_BUTTON(button), _("16-bit ppm"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
	    uf->conf->type==ppm16_type);
    g_object_set_data(G_OBJECT(button), "ButtonValue", (gpointer)ppm16_type);
    g_signal_connect(G_OBJECT(button), "toggled",
	    G_CALLBACK(ufraw_radio_button_update), &uf->conf->type);
    gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);
#ifdef HAVE_LIBTIFF
    gtk_box_pack_start(GTK_BOX(box), gtk_hseparator_new(), TRUE, TRUE, 0);
    table = gtk_table_new(5, 1, FALSE);
    gtk_box_pack_start(GTK_BOX(box), table, TRUE, TRUE, 0);
    button = gtk_radio_button_new_with_label_from_widget(
	    GTK_RADIO_BUTTON(button), _("8-bit TIFF"));
    data->tiffButton = GTK_TOGGLE_BUTTON(button);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
	    uf->conf->type==tiff8_type);
    g_object_set_data(G_OBJECT(button), "ButtonValue", (gpointer)tiff8_type);
    g_signal_connect(G_OBJECT(button), "toggled",
	    G_CALLBACK(ufraw_radio_button_update), &uf->conf->type);
    align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
    gtk_container_add(GTK_CONTAINER(align), button);
    gtk_table_attach(GTK_TABLE(table), align, 0, 1, 0, 1, GTK_FILL,0, 0, 0);
    button = gtk_radio_button_new_with_label_from_widget(
	    GTK_RADIO_BUTTON(button), _("16-bit TIFF"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
	    uf->conf->type==tiff16_type);
    g_object_set_data(G_OBJECT(button), "ButtonValue", (gpointer)tiff16_type);
    g_signal_connect(G_OBJECT(button), "toggled",
	    G_CALLBACK(ufraw_radio_button_update), &uf->conf->type);
    gtk_table_attach(GTK_TABLE(table), button, 0, 1, 1, 2, 0, 0, 0, 0);
#ifdef HAVE_LIBZ
    losslessButton = gtk_check_button_new_with_label(_("ZIP Compress (lossless)"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(losslessButton),
	    uf->conf->losslessCompress);
    gtk_table_attach(GTK_TABLE(table), losslessButton, 1, 2, 2, 3, 0, 0, 0, 0);
#endif
#endif /*HAVE_LIBTIFF*/
#ifdef HAVE_LIBJPEG
    gtk_box_pack_start(GTK_BOX(box), gtk_hseparator_new(), TRUE, TRUE, 0);
    table = gtk_table_new(5, 1, FALSE);
    gtk_box_pack_start(GTK_BOX(box), table, TRUE, TRUE, 0);
    button = gtk_radio_button_new_with_label_from_widget(
	    GTK_RADIO_BUTTON(button), "JPEG");
    data->jpegButton = GTK_TOGGLE_BUTTON(button);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
	    uf->conf->type==jpeg_type);
    g_object_set_data(G_OBJECT(button), "ButtonValue", (gpointer)jpeg_type);
    g_signal_connect(G_OBJECT(button), "toggled",
	    G_CALLBACK(ufraw_radio_button_update), &uf->conf->type);
    gtk_table_attach(GTK_TABLE(table), button, 0, 1, 0, 1, 0, 0, 0, 0);
    widg = gtk_label_new(_("Compression level"));
    gtk_table_attach(GTK_TABLE(table), widg, 1, 2, 1, 2, 0, 0, 0, 0);
    compressAdj = GTK_ADJUSTMENT(gtk_adjustment_new(uf->conf->compression,
	    0, 100, 5, 10, 0));
    widg = gtk_hscale_new(compressAdj);
    gtk_scale_set_draw_value(GTK_SCALE(widg), FALSE);
    gtk_table_attach(GTK_TABLE(table), widg, 2, 3, 1, 2,
	    GTK_EXPAND|GTK_FILL, 0, 0, 0);
    widg = gtk_spin_button_new(compressAdj, 5, 0);
    gtk_table_attach(GTK_TABLE(table), widg, 3, 4, 1, 2, 0, 0, 0, 0);
#if defined(HAVE_LIBEXIF) || defined(HAVE_EXIV2)
    exifButton = gtk_check_button_new_with_label(_("Embed EXIF data"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(exifButton),
	    uf->conf->embedExif && uf->exifBuf!=NULL);
    gtk_widget_set_sensitive(exifButton, uf->exifBuf!=NULL);
    gtk_table_attach(GTK_TABLE(table), exifButton, 1, 2, 2, 3, 0, 0, 0, 0);
#endif
#endif /*HAVE_LIBJPEG*/

    gtk_box_pack_start(GTK_BOX(box), gtk_hseparator_new(), TRUE, TRUE, 0);
    table = gtk_table_new(1, 5, FALSE);
    gtk_box_pack_start(GTK_BOX(box), table, TRUE, TRUE, 0);
    label = gtk_label_new(_("Create ID file "));
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, 0, 0, 0, 0);
    idCombo = GTK_COMBO_BOX(gtk_combo_box_new_text());
    gtk_combo_box_append_text(idCombo, _("No"));
    gtk_combo_box_append_text(idCombo, _("Also"));
    gtk_combo_box_append_text(idCombo, _("Only"));
    gtk_combo_box_set_active(idCombo, uf->conf->createID);
    gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(idCombo), 1, 2, 0, 1,
	    0, 0, 0, 0);

    label = gtk_label_new(_("\tSave image defaults "));
    event = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(event), label);
    gtk_tooltips_set_tip(tips, event,
	    _("Save current image manipulation parameters as defaults.\n"
	    "The output parameters in this window are always saved."), NULL);
    gtk_table_attach(GTK_TABLE(table), event, 3, 4, 0, 1, 0, 0, 0, 0);
    confCombo = GTK_COMBO_BOX(gtk_combo_box_new_text());
    gtk_combo_box_append_text(confCombo, _("Never again"));
    gtk_combo_box_append_text(confCombo, _("Always"));
    gtk_combo_box_append_text(confCombo, _("Just this once"));
    gtk_combo_box_set_active(confCombo, uf->conf->saveConfiguration);
    gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(confCombo), 4, 5, 0, 1,
	    0, 0, 0, 0);

    gtk_box_pack_start(GTK_BOX(box), gtk_hseparator_new(), TRUE, TRUE, 0);
    overwriteButton = gtk_check_button_new_with_label(
	    _("Overwrite existing files without asking"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(overwriteButton),
	    uf->conf->overwrite);
    gtk_box_pack_start(GTK_BOX(box), overwriteButton, TRUE, TRUE, 0);
    gtk_widget_show_all(expander);

    if (strlen(uf->conf->inputFilename)>0) {
	char *cp = g_path_get_dirname(uf->conf->inputFilename);
	if (strcmp(cp,"."))
	    gtk_file_chooser_add_shortcut_folder(fileChooser, cp, NULL);
	g_free(cp);
    }
    g_signal_connect(G_OBJECT(fileChooser), "selection-changed",
	    G_CALLBACK(ufraw_saver_set_type), data);
    while(1) {
	status = gtk_dialog_run(GTK_DIALOG(fileChooser));
	if (status==GTK_RESPONSE_CANCEL) {
	    ufraw_focus(fileChooser, FALSE);
	    gtk_widget_destroy(GTK_WIDGET(fileChooser));
	    return UFRAW_CANCEL;
	}
	/* GTK_RESPONSE_OK - save file */
	char *filename = gtk_file_chooser_get_filename(fileChooser);
	uf->conf->createID = gtk_combo_box_get_active(idCombo);
	uf->conf->saveConfiguration = gtk_combo_box_get_active(confCombo);
	uf->conf->overwrite = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(overwriteButton));
#if defined(HAVE_LIBZ) && defined(HAVE_LIBTIFF)
	uf->conf->losslessCompress = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(losslessButton));
#endif
#ifdef HAVE_LIBJPEG
	uf->conf->compression = gtk_adjustment_get_value(compressAdj);
#if defined(HAVE_LIBEXIF) || defined(HAVE_EXIV2)
	uf->conf->embedExif = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(exifButton));
#endif
#endif
	if ( !uf->conf->overwrite && uf->conf->createID!=only_id
	   && g_file_test(filename, G_FILE_TEST_EXISTS) ) {
	    if ( !ufraw_overwrite_dialog(filename, GTK_WIDGET(fileChooser)) )
		continue;
	}
	g_strlcpy(uf->conf->outputFilename, filename, max_path);
	g_free(filename);
	status = ufraw_write_image(uf);
	if (status==UFRAW_SUCCESS) {
	    ufraw_focus(fileChooser, FALSE);
	    gtk_widget_destroy(GTK_WIDGET(fileChooser));
	    return UFRAW_SUCCESS;
	}
    }
}
