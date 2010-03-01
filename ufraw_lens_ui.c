/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw_lens_ui.c - User interface for interaction with lensfun,
 *                   a lens defect correction library.
 * Copyright 2007-2010 by Andrew Zabolotny, Udi Fuchs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "ufraw.h"
#include "uf_gtk.h"
#include "ufraw_ui.h"
#include <glib/gi18n.h>
#include <ctype.h>

#ifdef HAVE_LENSFUN
#include <lensfun.h>

static void delete_children(GtkWidget *widget, gpointer data)
{
    (void)data;
    gtk_widget_destroy(widget);
}

static void camera_set(preview_data *data)
{
    const char *maker = lf_mlstr_get(CFG->camera->Maker);
    const char *model = lf_mlstr_get(CFG->camera->Model);
    const char *variant = lf_mlstr_get(CFG->camera->Variant);

    if (model != NULL) {
	gchar *fm;
	if (maker != NULL)
	    fm = g_strdup_printf("%s, %s", maker, model);
	else
	    fm = g_strdup_printf("%s", model);
	gtk_entry_set_text(GTK_ENTRY(data->CameraModel), fm);
	g_free(fm);
    }
    char _variant[100];
    if (variant != NULL)
	snprintf(_variant, sizeof(_variant), " (%s)", variant);
    else
	_variant[0] = 0;

    gchar *fm = g_strdup_printf(_("Maker:\t\t%s\n"
			"Model:\t\t%s%s\n"
			"Mount:\t\t%s\n"
			"Crop factor:\t%.1f"),
			maker, model, _variant,
			CFG->camera->Mount, CFG->camera->CropFactor);
    uf_widget_set_tooltip(data->CameraModel, fm);
    g_free(fm);
}

static void camera_menu_select(GtkMenuItem *menuitem, gpointer user_data)
{
    preview_data *data = (preview_data *)user_data;
    lfCamera *cam = g_object_get_data(G_OBJECT(menuitem), "lfCamera");
    lf_camera_copy(CFG->camera, cam);
    camera_set(data);
}

static void camera_menu_fill(preview_data *data, const lfCamera *const *camlist)
{
    unsigned i;
    GPtrArray *makers, *submenus;

    if (data->CameraMenu) {
	gtk_widget_destroy(data->CameraMenu);
	data->CameraMenu = NULL;
    }

    /* Count all existing camera makers and create a sorted list */
    makers = g_ptr_array_new();
    submenus = g_ptr_array_new();
    for (i = 0; camlist[i]; i++) {
	GtkWidget *submenu, *item;
	const char *m = lf_mlstr_get(camlist[i]->Maker);
	int idx = ptr_array_find_sorted(makers, m,
		(GCompareFunc)g_utf8_collate);
	if (idx < 0) {
	    /* No such maker yet, insert it into the array */
	    idx = ptr_array_insert_sorted(makers, m,
		    (GCompareFunc)g_utf8_collate);
	    /* Create a submenu for cameras by this maker */
	    submenu = gtk_menu_new();
	    ptr_array_insert_index(submenus, submenu, idx);
	}

	submenu = g_ptr_array_index(submenus, idx);
	/* Append current camera name to the submenu */
	m = lf_mlstr_get(camlist[i]->Model);
	if (camlist[i]->Variant == NULL) {
	    item = gtk_menu_item_new_with_label(m);
	} else {
	    gchar *fm = g_strdup_printf("%s (%s)", m, camlist[i]->Variant);
	    item = gtk_menu_item_new_with_label(fm);
	    g_free(fm);
	}
	gtk_widget_show(item);
	g_object_set_data(G_OBJECT(item), "lfCamera", (void *)camlist[i]);
	g_signal_connect(G_OBJECT(item), "activate",
		G_CALLBACK(camera_menu_select), data);
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    }

    data->CameraMenu = gtk_menu_new();
    for (i = 0; i < makers->len; i++) {
	GtkWidget *item = gtk_menu_item_new_with_label(
		g_ptr_array_index(makers, i));
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(data->CameraMenu), item);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item),
		(GtkWidget *)g_ptr_array_index(submenus, i));
    }
    g_ptr_array_free(submenus, TRUE);
    g_ptr_array_free(makers, TRUE);
}

static void parse_maker_model(const char *txt, char *make, size_t sz_make,
	char *model, size_t sz_model)
{
    while (txt[0] != '\0' && isspace(txt[0]))
	txt++;
    const gchar *sep = strchr(txt, ',');
    if (sep != NULL) {
	size_t len = sep - txt + 1;
	len = MIN(len, sz_make);
	g_strlcpy(make, txt, len);

	while (*++sep && isspace(sep[0])) { }
	g_strlcpy(model, sep, sz_model);
    } else {
	g_strlcpy(model, txt, sz_model);
    }
}

static void camera_search_clicked(GtkWidget *button, preview_data *data)
{
    (void)button;
    char make[200], model[200];
    const gchar *txt = gtk_entry_get_text(GTK_ENTRY(data->CameraModel));
    parse_maker_model(txt, make, sizeof(make), model, sizeof(model));

    lfDatabase *lensdb = ufraw_lensfun_db();
    const lfCamera **camlist = lf_db_find_cameras_ext(lensdb, make, model, 0);
    if (camlist == NULL)
	return;
    camera_menu_fill(data, camlist);
    lf_free(camlist);

    gtk_menu_popup(GTK_MENU(data->CameraMenu), NULL, NULL, NULL, NULL,
	    0, gtk_get_current_event_time());
}

static void camera_list_clicked(GtkWidget *button, preview_data *data)
{
    (void)button;
    lfDatabase *lensdb = ufraw_lensfun_db();
    const lfCamera *const *camlist = lf_db_get_cameras(lensdb);
    if (camlist == NULL)
	return;

    camera_menu_fill(data, camlist);

    gtk_menu_popup(GTK_MENU(data->CameraMenu), NULL, NULL, NULL, NULL,
	    0, gtk_get_current_event_time());
}

static void combo_entry_new(UFObject *object, GtkWidget *box,
	const char *labelText, const char *tooltip)
{
    GtkWidget *label = gtk_label_new(labelText);
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 2);
    GtkWidget *combo = ufarray_combo_box_entry_new(object);
    gtk_box_pack_start(GTK_BOX(box), combo, TRUE, TRUE, 2);
    uf_widget_set_tooltip(label, tooltip);
}

static void lens_set(preview_data *data)
{
    UFObject *lensfun = ufgroup_element(CFG->ufobject, ufLensfun);
    lfLens *lens = ufraw_lensfun_transformation_lens(lensfun);
    gchar *fm;

    const char *maker = lf_mlstr_get(lens->Maker);
    const char *model = lf_mlstr_get(lens->Model);

    if (model != NULL) {
	if (maker != NULL)
	    fm = g_strdup_printf("%s, %s", maker, model);
	else
	    fm = g_strdup_printf("%s", model);
	gtk_entry_set_text(GTK_ENTRY(data->LensModel), fm);
	g_free(fm);
    }

    char focal[100], aperture[100], mounts[200];

    if (lens->MinFocal < lens->MaxFocal)
	snprintf(focal, sizeof(focal), "%g-%gmm",
		lens->MinFocal, lens->MaxFocal);
    else
	snprintf(focal, sizeof(focal), "%gmm", lens->MinFocal);
    if (lens->MinAperture < lens->MaxAperture)
	snprintf(aperture, sizeof(aperture), "%g-%g",
		lens->MinAperture, lens->MaxAperture);
    else
	snprintf(aperture, sizeof(aperture), "%g", lens->MinAperture);

    if (lens->Mounts != NULL) {
	mounts[0] = 0;
	unsigned i;
	for (i = 0; lens->Mounts[i] != NULL; i++) {
	    if (i > 0)
		g_strlcat(mounts, ", ", sizeof(mounts));
	    g_strlcat(mounts, lens->Mounts[i], sizeof(mounts));
	}
    }
    fm = g_strdup_printf(_("Maker:\t\t%s\n"
			   "Model:\t\t%s\n"
			   "Focal range:\t%s\n"
			   "Aperture:\t\t%s\n"
			   "Crop factor:\t%.1f\n"
			   "Type:\t\t%s\n"
			   "Mounts:\t\t%s"),
			    maker ? maker : "?", model ? model : "?",
			    focal, aperture, lens->CropFactor,
			    lf_get_lens_type_desc(lens->Type, NULL), mounts);
    uf_widget_set_tooltip(data->LensModel, fm);
    g_free(fm);

    /* Create the focal/aperture/distance combo boxes */
    gtk_container_foreach(GTK_CONTAINER(data->LensParamBox),
	    delete_children, NULL);

    UFObject *FocalLength = ufgroup_element(lensfun, ufFocalLength);
    combo_entry_new(FocalLength, data->LensParamBox,
	    _("Focal"), _("Focal length"));
    UFObject *Aperture = ufgroup_element(lensfun, ufAperture);
    combo_entry_new(Aperture, data->LensParamBox,
	    _("F"), _("F-number (Aperture)"));
    UFObject *Distance = ufgroup_element(lensfun, ufDistance);
    combo_entry_new(Distance, data->LensParamBox,
	    _("Distance"), _("Distance to subject in meters"));

    gtk_widget_show_all(data->LensParamBox);
}

static void lens_menu_select(GtkMenuItem *menuitem, preview_data *data)
{
    lfLens *lens = (lfLens *)g_object_get_data(G_OBJECT(menuitem), "lfLens");
    UFObject *lensfun = ufgroup_element(CFG->ufobject, ufLensfun);
    ufraw_lensfun_interpolate(lensfun, lens);
    lens_set(data);
}

static void lens_menu_fill(preview_data *data, const lfLens *const *lenslist)
{
    unsigned i;
    if (data->LensMenu != NULL) {
	gtk_widget_destroy(data->LensMenu);
	data->LensMenu = NULL;
    }

    /* Count all existing lens makers and create a sorted list */
    GPtrArray *makers = g_ptr_array_new();
    GPtrArray *submenus = g_ptr_array_new();
    for (i = 0; lenslist[i]; i++) {
	GtkWidget *submenu, *item;
	const char *m = lf_mlstr_get(lenslist[i]->Maker);
	int idx = ptr_array_find_sorted(makers, m,
		(GCompareFunc)g_utf8_collate);
	if (idx < 0) {
	    /* No such maker yet, insert it into the array */
	    idx = ptr_array_insert_sorted(makers, m,
		    (GCompareFunc)g_utf8_collate);
	    /* Create a submenu for lenses by this maker */
	    submenu = gtk_menu_new();
	    ptr_array_insert_index(submenus, submenu, idx);
	}
	submenu = g_ptr_array_index(submenus, idx);
	/* Append current lens name to the submenu */
	item = gtk_menu_item_new_with_label(lf_mlstr_get(lenslist[i]->Model));
	gtk_widget_show(item);
	g_object_set_data(G_OBJECT(item), "lfLens", (void *)lenslist[i]);
	g_signal_connect(G_OBJECT(item), "activate",
		G_CALLBACK(lens_menu_select), data);
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    }

    data->LensMenu = gtk_menu_new();
    for (i = 0; i < makers->len; i++) {
	GtkWidget *item =
		gtk_menu_item_new_with_label(g_ptr_array_index(makers, i));
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(data->LensMenu), item);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item),
		(GtkWidget *)g_ptr_array_index(submenus, i));
    }
    g_ptr_array_free(submenus, TRUE);
    g_ptr_array_free(makers, TRUE);
}

static void lens_search_clicked(GtkWidget *button, preview_data *data)
{
    (void)button;
    char make[200], model[200];
    const gchar *txt = gtk_entry_get_text(GTK_ENTRY(data->LensModel));

    parse_maker_model(txt, make, sizeof(make), model, sizeof(model));
    lfDatabase *lensdb = ufraw_lensfun_db();
    const lfLens **lenslist = lf_db_find_lenses_hd(lensdb, CFG->camera,
	    make[0] ? make : NULL, model[0] ? model : NULL, 0);
    if (lenslist == NULL)
	return;
    lens_menu_fill(data, lenslist);
    lf_free(lenslist);

    gtk_menu_popup(GTK_MENU(data->LensMenu), NULL, NULL, NULL, NULL,
	    0, gtk_get_current_event_time());
}

static void lens_list_clicked(GtkWidget *button, preview_data *data)
{
    (void)button;

    lfDatabase *lensdb = ufraw_lensfun_db();
    if (CFG->camera != NULL) {
	const lfLens **lenslist = lf_db_find_lenses_hd(lensdb,
		CFG->camera, NULL, NULL, 0);
	if (lenslist == NULL)
	    return;
	lens_menu_fill(data, lenslist);
	lf_free(lenslist);
    } else {
	const lfLens *const *lenslist = lf_db_get_lenses(lensdb);
	if (lenslist == NULL)
	    return;
	lens_menu_fill(data, lenslist);
    }
    gtk_menu_popup(GTK_MENU(data->LensMenu), NULL, NULL, NULL, NULL,
	    0, gtk_get_current_event_time());
}

/* --- TCA correction page --- */

static void tca_model_changed(GtkComboBox *widget, preview_data *data)
{
    (void)widget;
    gtk_container_foreach(GTK_CONTAINER(data->LensTCATable),
	    delete_children, NULL);

    UFObject *lensfun = ufgroup_element(CFG->ufobject, ufLensfun);
    UFObject *tca = ufgroup_element(lensfun, ufTCA);
    UFObject *model = ufgroup_element(tca, ufobject_string_value(tca));
    lfTCAModel tcaModel = ufarray_index(tca);
    const char *details;
    const lfParameter **params;
    if (!lf_get_tca_model_desc(tcaModel, &details, &params))
	return; /* should never happen */
    int i;
    if (params != NULL) {
	for (i = 0; params[i] != NULL; i++) {
	    UFObject *param = ufgroup_element(model, params[i]->Name);
	    ufnumber_adjustment_scale(param, GTK_TABLE(data->LensTCATable),
		    0, i, params[i]->Name, NULL);
	    GtkWidget *reset = ufobject_reset_button_new(NULL);
	    ufobject_reset_button_add(reset, param);
	    gtk_table_attach(GTK_TABLE(data->LensTCATable), reset,
		    7, 8, i, i+1, 0, 0, 0, 0);
	}
    }
    gtk_label_set_text(GTK_LABEL(data->LensTCADesc), details);
    gtk_widget_show_all(data->LensTCATable);
}

static void fill_tca_page(preview_data *data, GtkWidget *page)
{
    GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 0);

    /* Add the model combobox */
    GtkWidget *label = gtk_label_new(_("Model:"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

    UFObject *lensfun = ufgroup_element(CFG->ufobject, ufLensfun);
    UFObject *tca = ufgroup_element(lensfun, ufTCA);
    GtkWidget *combo = ufarray_combo_box_new(tca);
    gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
    uf_widget_set_tooltip(combo, _("Chromatic Aberrations mathematical model"));
    g_signal_connect_after(G_OBJECT(combo), "changed",
            G_CALLBACK(tca_model_changed), data);

    data->LensTCATable = gtk_table_new(10, 1, FALSE);
    GtkWidget *f = gtk_frame_new(_("Parameters"));
    gtk_box_pack_start(GTK_BOX(page), f, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(f), data->LensTCATable);

    data->LensTCADesc = gtk_label_new("");
    gtk_label_set_line_wrap(GTK_LABEL(data->LensTCADesc), TRUE);
    gtk_label_set_ellipsize(GTK_LABEL(data->LensTCADesc), PANGO_ELLIPSIZE_END);
    gtk_label_set_selectable(GTK_LABEL(data->LensTCADesc), TRUE);
    gtk_box_pack_start(GTK_BOX(page), data->LensTCADesc, FALSE, FALSE, 0);
}

/* --- Vignetting correction page --- */

static void vignetting_model_changed(GtkComboBox *widget, preview_data *data)
{
    (void)widget;
    gtk_container_foreach(GTK_CONTAINER(data->LensVignettingTable),
	    delete_children, NULL);

    UFObject *lensfun = ufgroup_element(CFG->ufobject, ufLensfun);
    UFObject *vignetting = ufgroup_element(lensfun, ufVignetting);
    UFObject *model = ufgroup_element(vignetting,
	    ufobject_string_value(vignetting));
    lfVignettingModel vignettingModel = ufarray_index(vignetting);
    const char *details;
    const lfParameter **params;
    if (!lf_get_vignetting_model_desc(vignettingModel, &details, &params))
	return; /* should never happen */
    int i;
    if (params != NULL) {
	for (i = 0; params[i] != NULL; i++) {
	    UFObject *param = ufgroup_element(model, params[i]->Name);
	    ufnumber_adjustment_scale(param,
		    GTK_TABLE(data->LensVignettingTable), 0, i,
		    params[i]->Name, NULL);
	    GtkWidget *reset = ufobject_reset_button_new(NULL);
	    ufobject_reset_button_add(reset, param);
	    gtk_table_attach(GTK_TABLE(data->LensVignettingTable), reset,
		    7, 8, i, i+1, 0, 0, 0, 0);
	}
    }
    gtk_label_set_text(GTK_LABEL(data->LensVignettingDesc), details);
    gtk_widget_show_all(data->LensVignettingTable);
}

static void fill_vignetting_page(preview_data *data, GtkWidget *page)
{
    GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 0);

    /* Add the model combobox */
    GtkWidget *label = gtk_label_new(_("Model:"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

    UFObject *lensfun = ufgroup_element(CFG->ufobject, ufLensfun);
    UFObject *vignetting = ufgroup_element(lensfun, ufVignetting);
    GtkWidget *combo = ufarray_combo_box_new(vignetting);
    gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
    uf_widget_set_tooltip(combo, _("Optical vignetting mathematical model"));
    g_signal_connect_after(G_OBJECT(combo), "changed",
            G_CALLBACK(vignetting_model_changed), data);

    data->LensVignettingTable = gtk_table_new(10, 1, FALSE);
    GtkWidget *f = gtk_frame_new(_("Parameters"));
    gtk_box_pack_start(GTK_BOX(page), f, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(f), data->LensVignettingTable);

    data->LensVignettingDesc = gtk_label_new("");
    gtk_label_set_line_wrap(GTK_LABEL(data->LensVignettingDesc), TRUE);
    gtk_label_set_ellipsize(GTK_LABEL(data->LensVignettingDesc),
	    PANGO_ELLIPSIZE_END);
    gtk_label_set_selectable(GTK_LABEL(data->LensVignettingDesc), TRUE);
    gtk_box_pack_start(GTK_BOX(page), data->LensVignettingDesc,
	    FALSE, FALSE, 0);
}

/* --- Distortion correction page --- */

static void distortion_model_changed(GtkComboBox *widget, preview_data *data)
{
    (void)widget;
    gtk_container_foreach(GTK_CONTAINER(data->LensDistortionTable),
	    delete_children, NULL);

    UFObject *lensfun = ufgroup_element(CFG->ufobject, ufLensfun);
    UFObject *distortion = ufgroup_element(lensfun, ufDistortion);
    UFObject *model = ufgroup_element(distortion,
	    ufobject_string_value(distortion));
    lfDistortionModel distortionModel = ufarray_index(distortion);
    const char *details;
    const lfParameter **params;
    if (!lf_get_distortion_model_desc(distortionModel, &details, &params))
	return; // should never happen
    int i;
    if (params != NULL) {
	for (i = 0; params[i] != NULL; i++) {
	    UFObject *param = ufgroup_element(model, params[i]->Name);
	    ufnumber_adjustment_scale(param,
		    GTK_TABLE(data->LensDistortionTable), 0, i,
		    params[i]->Name, NULL);
	    GtkWidget *reset = ufobject_reset_button_new(NULL);
	    ufobject_reset_button_add(reset, param);
	    gtk_table_attach(GTK_TABLE(data->LensDistortionTable), reset,
		    7, 8, i, i+1, 0, 0, 0, 0);
	}
    }
    gtk_label_set_text(GTK_LABEL(data->LensDistortionDesc), details);
    gtk_widget_show_all(data->LensDistortionTable);
}

static void fill_distortion_page(preview_data *data, GtkWidget *page)
{
    GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 0);

    /* Add the model combobox */
    GtkWidget *label = gtk_label_new(_("Model:"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

    UFObject *lensfun = ufgroup_element(CFG->ufobject, ufLensfun);
    UFObject *distortion = ufgroup_element(lensfun, ufDistortion);
    GtkWidget *combo = ufarray_combo_box_new(distortion);
    gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
    uf_widget_set_tooltip(combo, _("Lens distortion mathematical model"));
    g_signal_connect_after(G_OBJECT(combo), "changed",
	    G_CALLBACK(distortion_model_changed), data);

    data->LensDistortionTable = gtk_table_new(10, 1, FALSE);
    GtkWidget *f = gtk_frame_new(_("Parameters"));
    gtk_box_pack_start(GTK_BOX(page), f, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(f), data->LensDistortionTable);

    data->LensDistortionDesc = gtk_label_new("");
    gtk_label_set_line_wrap(GTK_LABEL(data->LensDistortionDesc), TRUE);
    gtk_label_set_ellipsize(GTK_LABEL(data->LensDistortionDesc),
	    PANGO_ELLIPSIZE_END);
    gtk_label_set_selectable(GTK_LABEL(data->LensDistortionDesc), TRUE);
    gtk_box_pack_start(GTK_BOX(page),
	    data->LensDistortionDesc, FALSE, FALSE, 0);
}

/* --- Lens geometry page --- */

static void geometry_model_changed(GtkComboBox *widget, GtkLabel *label)
{
    lfLensType type = gtk_combo_box_get_active(widget);

    const char *details;
    if (!lf_get_lens_type_desc(type, &details))
	return; // should never happen

    gtk_label_set_text(label, details);
}

static GtkWidget *fill_geometry_page(UFObject *ufobject)
{
    UFObject *lensfun = ufgroup_element(ufobject, ufLensfun);
    GtkTable *geometryTable = GTK_TABLE(gtk_table_new(10, 1, FALSE));

    /* Lens geometry combobox */
    GtkWidget *label = gtk_label_new(_("Lens geometry:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(geometryTable, label, 0, 1, 0, 1, GTK_FILL, 0, 5, 0);
    UFObject *lensGeometry = ufgroup_element(lensfun, ufLensGeometry);
    GtkWidget *combo = ufarray_combo_box_new(lensGeometry);
    uf_widget_set_tooltip(combo,
	    _("The geometry of the lens used to make the shot"));
    gtk_table_attach(geometryTable, combo, 1, 2, 0, 1,
	    GTK_EXPAND | GTK_FILL, 0, 0, 0);

    GtkWidget *description = gtk_label_new("");
    gtk_label_set_line_wrap(GTK_LABEL(description), TRUE);
    gtk_label_set_ellipsize(GTK_LABEL(description), PANGO_ELLIPSIZE_END);
    gtk_label_set_selectable(GTK_LABEL(description), TRUE);
    gtk_misc_set_alignment(GTK_MISC(description), 0.5, 0.5);
    gtk_table_attach(geometryTable, description, 0, 2, 1, 2,
	    GTK_EXPAND | GTK_FILL, 0, 0, 10);
    g_signal_connect(G_OBJECT(combo), "changed",
	    G_CALLBACK(geometry_model_changed), description);

    /* Target lens geometry combobox */
    label = gtk_label_new(_("Target geometry:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(geometryTable, label, 0, 1, 2, 3, GTK_FILL, 0, 5, 0);
    UFObject *targetLensGeometry =
	    ufgroup_element(lensfun, ufTargetLensGeometry);
    combo = ufarray_combo_box_new(targetLensGeometry);
    uf_widget_set_tooltip(combo, _("The target geometry for output image"));
    gtk_table_attach(geometryTable, combo, 1, 2, 2, 3,
	    GTK_EXPAND | GTK_FILL, 0, 0, 0);

    description = gtk_label_new("");
    gtk_label_set_line_wrap(GTK_LABEL(description), TRUE);
    gtk_label_set_ellipsize(GTK_LABEL(description), PANGO_ELLIPSIZE_END);
    gtk_label_set_selectable(GTK_LABEL(description), TRUE);
    gtk_misc_set_alignment(GTK_MISC(description), 0.5, 0.5);
    gtk_table_attach(geometryTable, description, 0, 2, 3, 4,
	    GTK_EXPAND | GTK_FILL, 0, 0, 10);
    g_signal_connect(G_OBJECT(combo), "changed",
	    G_CALLBACK(geometry_model_changed), description);

    return GTK_WIDGET(geometryTable);
}

/**
 * Fill the "lens correction" page in the main notebook.
 */
void lens_fill_interface(preview_data *data, GtkWidget *page)
{
    GtkWidget *label, *button, *subpage;

    /* Camera selector */
    GtkTable *table = GTK_TABLE(gtk_table_new(10, 10, FALSE));
    gtk_box_pack_start(GTK_BOX(page), GTK_WIDGET(table), FALSE, FALSE, 0);

    label = gtk_label_new(_("Camera"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(table, label, 0, 1, 0, 1, GTK_FILL, 0, 2, 0);

    data->CameraModel = gtk_entry_new();
    gtk_table_attach(table, data->CameraModel, 1, 2, 0, 1,
	    GTK_EXPAND|GTK_FILL, 0, 2, 0);

    button = stock_icon_button(GTK_STOCK_FIND,
	    _("Search for camera using a pattern\n"
	      "Format: [Maker, ][Model]"),
	    G_CALLBACK(camera_search_clicked), data);
    gtk_table_attach(table, button, 2, 3, 0, 1, 0, 0, 0, 0);

    button = stock_icon_button(GTK_STOCK_INDEX,
	    _("Choose camera from complete list"),
	    G_CALLBACK(camera_list_clicked), data);
    gtk_table_attach(table, button, 3, 4, 0, 1, 0, 0, 0, 0);

    /* Lens selector */
    label = gtk_label_new(_("Lens"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(table, label, 0, 1, 1, 2, GTK_FILL, 0, 2, 0);

    data->LensModel = gtk_entry_new();
    //gtk_entry_set_text(GTK_ENTRY(data->LensModel), "");
    gtk_table_attach(table, data->LensModel, 1, 2, 1, 2,
	    GTK_EXPAND|GTK_FILL, 0, 2, 0);

    button = stock_icon_button(GTK_STOCK_FIND,
	    _("Search for lens using a pattern\n"
	      "Format: [Maker, ][Model]"),
	    G_CALLBACK(lens_search_clicked), data);
    gtk_table_attach(table, button, 2, 3, 1, 2, 0, 0, 0, 0);

    button = stock_icon_button(GTK_STOCK_INDEX,
	    _("Choose lens from list of possible variants"),
	    G_CALLBACK(lens_list_clicked), data);
    gtk_table_attach(table, button, 3, 4, 1, 2, 0, 0, 0, 0);

    data->LensParamBox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(page), data->LensParamBox, FALSE, FALSE, 2);

    GtkNotebook *subnb = GTK_NOTEBOOK(gtk_notebook_new());
    gtk_box_pack_start(GTK_BOX(page), GTK_WIDGET(subnb), TRUE, TRUE, 0);
    gtk_notebook_set_tab_pos(subnb, GTK_POS_LEFT);

    /* Create a default lens & camera */
    camera_set(data);
    lens_set(data);

    subpage = notebook_page_new(subnb,
	    _("Lateral chromatic aberration"), "tca");
    fill_tca_page(data, subpage);
    tca_model_changed(NULL, data);

    subpage = notebook_page_new(subnb, _("Optical vignetting"), "vignetting");
    fill_vignetting_page(data, subpage);
    vignetting_model_changed(NULL, data);

    subpage = notebook_page_new(subnb, _("Lens distortion"), "distortion");
    fill_distortion_page(data, subpage);
    distortion_model_changed(NULL, data);
    gtk_widget_show_all(subpage); // Need to show the page for set page to work.
    int pageNum = gtk_notebook_page_num(subnb, page);
    gtk_notebook_set_current_page(subnb, pageNum);

    subpage = notebook_page_new(subnb, _("Lens geometry"), "geometry");
    gtk_box_pack_start(GTK_BOX(subpage), fill_geometry_page(CFG->ufobject),
	    TRUE, TRUE, 0);
}

#endif /* HAVE_LENSFUN */
