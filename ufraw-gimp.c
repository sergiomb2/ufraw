/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw-gimp.c - The GIMP plug-in.
 * Copyright 2004-2010 by Udi Fuchs
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

#include "ufraw.h"
#include "uf_gtk.h"
#ifdef UFRAW_CINEPAINT
/* Bypass a bug in CinePaint header files */
#define RNH_FLOAT
#include <plugin_main.h>
#define GIMP_CONST
/* Fix some compatibility issues between CinePaint and GIMP */
#define GIMP_CHECK_VERSION(a,b,c) 0
typedef GimpRunModeType GimpRunMode;
#define PLUGIN_MODE 2
#else
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#define GIMP_CONST const
/* Missing and irrelevant definitions in GIMP */
#define U16_RGB 0
#define U16_RGB_IMAGE 0
#define PLUGIN_MODE 1
#endif
#include <glib/gi18n.h>
#include <string.h>

void query();
void run(GIMP_CONST gchar *name,
	gint nparams,
	GIMP_CONST GimpParam *param,
	gint *nreturn_vals,
	GimpParam **return_vals);

long ufraw_save_gimp_image(ufraw_data *uf, GtkWidget *widget);

GimpPlugInInfo PLUG_IN_INFO = {
    NULL,  /* init_procedure */
    NULL,  /* quit_procedure */
    query, /* query_procedure */
    run,   /* run_procedure */
};

MAIN()

void query()
{
    static GIMP_CONST GimpParamDef load_args[] = {
	{ GIMP_PDB_INT32,  "run_mode", "Interactive, non-interactive" },
	{ GIMP_PDB_STRING, "filename", "The name of the file to load" },
	{ GIMP_PDB_STRING, "raw_filename", "The name of the file to load" },
    };
    static GIMP_CONST GimpParamDef load_return_vals[] = {
	{ GIMP_PDB_IMAGE, "image", "Output image" },
    };
#if GIMP_CHECK_VERSION(2,2,0)
    static GIMP_CONST GimpParamDef thumb_args[] = {
	{ GIMP_PDB_STRING, "filename",     "The name of the file to load" },
	{ GIMP_PDB_INT32,  "thumb_size",   "Preferred thumbnail size" }
    };
    static GIMP_CONST GimpParamDef thumb_return_vals[] = {
	{ GIMP_PDB_IMAGE,  "image",        "Thumbnail image" },
	{ GIMP_PDB_INT32,  "image_width",  "Width of full-sized image" },
	{ GIMP_PDB_INT32,  "image_height", "Height of full-sized image" }
    };
#endif
    gimp_install_procedure("file_ufraw_load",
	    "Loads digital camera raw files",
	    "Loads digital camera raw files.",
	    "Udi Fuchs",
	    "Copyright 2003 by Dave Coffin\n"
	    "Copyright 2004 by Pawel Jochym\n"
	    "Copyright 2004-2010 by Udi Fuchs",
	    "ufraw-" VERSION,
#if GIMP_CHECK_VERSION(2,2,0)
	    "raw image",
#else
	    "<Load>/UFRaw",
#endif
	    NULL,
	    GIMP_PLUGIN,
	    G_N_ELEMENTS(load_args),
	    G_N_ELEMENTS(load_return_vals),
	    load_args,
	    load_return_vals);

    gimp_register_load_handler("file_ufraw_load", (char *)raw_ext, "");

#if GIMP_CHECK_VERSION(2,2,0)
    gimp_install_procedure ("file_ufraw_load_thumb",
	    "Loads thumbnails from digital camera raw files.",
	    "Loads thumbnails from digital camera raw files.",
	    "Udi Fuchs",
	    "Copyright 2004-2010 by Udi Fuchs",
	    "ufraw-" VERSION,
	    NULL,
	    NULL,
	    GIMP_PLUGIN,
	    G_N_ELEMENTS(thumb_args),
	    G_N_ELEMENTS(thumb_return_vals),
	    thumb_args, thumb_return_vals);

    gimp_register_thumbnail_loader("file_ufraw_load",
	    "file_ufraw_load_thumb");
#endif
}

char *ufraw_binary;

#ifdef UFRAW_CINEPAINT
/* There is no way to get the full executable path in Cinepaint.
 * It is only needed for windows, so no big deal. */
char *gimp_get_progname()
{
    return "ufraw-cinepaint";
}
#endif

gboolean sendToGimpMode;

void run(GIMP_CONST gchar *name,
	gint nparams,
	GIMP_CONST GimpParam *param,
	gint *nreturn_vals,
	GimpParam **return_vals)
{
    // TODO: Check if the static variable here is really needed.
    // In any case this should cause no issues with threads.
    static GimpParam values[4];
    GimpRunMode run_mode;
    char *filename;
    int size;
    ufraw_data *uf;
    conf_data rc;
    int status;

    ufraw_binary = g_path_get_basename(gimp_get_progname());
    uf_init_locale(gimp_get_progname());

    *nreturn_vals = 1;
    *return_vals = values;

    if (!strcmp(name, "file_ufraw_load_thumb")) {
	run_mode = 0;
	filename = param[0].data.d_string;
	size = param[1].data.d_int32;
    } else if (!strcmp(name, "file_ufraw_load")) {
	run_mode = (GimpRunMode)param[0].data.d_int32;
	filename = param[1].data.d_string;
	size = 0;
    } else {
	values[0].type = GIMP_PDB_STATUS;
	values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
	return;
    }
    gboolean loadThumbnail = size>0;
    char *gtkrcfile = g_build_filename(uf_get_home_dir(),
	".ufraw-gtkrc", NULL);
    gtk_rc_add_default_file(gtkrcfile);
    g_free(gtkrcfile);
    gimp_ui_init("ufraw-gimp", TRUE);

    uf = ufraw_open(filename);
    /* if UFRaw fails on jpg or tif then open with GIMP */
    if (uf==NULL) {
	if (!strcasecmp(filename + strlen(filename) - 4, ".jpg")) {
	    if ( loadThumbnail )
		*return_vals = gimp_run_procedure2 ("file_jpeg_load_thumb",
			nreturn_vals, nparams, param);
	    else
		*return_vals = gimp_run_procedure2 ("file_jpeg_load",
			nreturn_vals, nparams, param);
	    return;
	} else if (!strcasecmp(filename+strlen(filename)-4, ".tif")) {
	    if ( !loadThumbnail )
		*return_vals = gimp_run_procedure2 ("file_tiff_load",
			nreturn_vals, nparams, param);
	    else {
		/* There is no "file_tiff_load_thumb".
		 * The call to "file_ufraw_load" will handle the thumbnail */
		/* Following is another solution for tiff thumbnails
		GimpParam tiffParam[3];
		tiffParam[0].type = GIMP_PDB_INT32;
		tiffParam[0].data.d_int32 = GIMP_RUN_NONINTERACTIVE;
		tiffParam[1].type = GIMP_PDB_STRING;
		tiffParam[1].data.d_string = filename;
		tiffParam[2].type = GIMP_PDB_STRING;
		tiffParam[2].data.d_string = filename;
		*return_vals = gimp_run_procedure2 ("file_tiff_load",
		    	nreturn_vals, 3, tiffParam);
		*/
	    }
	    return;
	} else {
	    /* Don't issue a message on thumbnail failure, since ufraw-gimp
	     * will be called again with "file_ufraw_load" */
	    if ( loadThumbnail ) return;

	    ufraw_icons_init();
	    GtkWidget *dummyWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	    uf_window_set_icon_name(GTK_WINDOW(dummyWindow), "ufraw");
	    ufraw_message(UFRAW_SET_PARENT, (char *)dummyWindow);

	    ufraw_message(UFRAW_REPORT, NULL);
	    values[0].type = GIMP_PDB_STATUS;
	    /* With GIMP_PDB_CANCEL, Gimp won't issue a warning */
	    values[0].data.d_status = GIMP_PDB_CANCEL;

	    gtk_widget_destroy(dummyWindow);
	    return;
	}
    }
    /* Load $HOME/.ufrawrc */
    conf_load(&rc, NULL);

    ufraw_config(uf, &rc, NULL, NULL);
    sendToGimpMode = (uf->conf->createID==send_id);
#if GIMP_CHECK_VERSION(2,2,0)
    if ( loadThumbnail ) {
	uf->conf->size = size;
	uf->conf->embeddedImage = TRUE;
    }
#else
    if (run_mode==GIMP_RUN_NONINTERACTIVE) uf->conf->shrink = 8;
#endif
    /* UFRaw already issues warnings.
     * With GIMP_PDB_CANCEL, Gimp won't issue another one. */
    values[0].type = GIMP_PDB_STATUS;
    values[0].data.d_status = GIMP_PDB_CANCEL;
    /* BUG - what should be done with GIMP_RUN_WITH_LAST_VALS */
    if ( run_mode==GIMP_RUN_INTERACTIVE &&
	 !loadThumbnail && !sendToGimpMode ) {
	/* Show the preview in interactive mode, unless if we are
	 * in thumbnail mode or 'send to gimp' mode. */
	status = ufraw_preview(uf, &rc, PLUGIN_MODE, ufraw_save_gimp_image);
    } else {
	if ( sendToGimpMode ) {
	    char *text = g_strdup_printf(_("Loading raw file '%s'"),
		    uf->filename);
	    gimp_progress_init(text);
	    g_free(text);
	}
	if ( sendToGimpMode ) gimp_progress_update(0.1);
	status=ufraw_load_raw(uf);
	if ( status!=UFRAW_SUCCESS ) {
	    values[0].type = GIMP_PDB_STATUS;
	    values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
	    return;
	}
	if ( sendToGimpMode ) gimp_progress_update(0.3);
	ufraw_save_gimp_image(uf, NULL);
	if ( sendToGimpMode ) gimp_progress_update(1.0);
	ufraw_close(uf);
	/* To make sure we don't delete the raw file by mistake we check
	 * that the file is really an ID file. */
	if ( sendToGimpMode &&
	     strcasecmp(filename + strlen(filename) - 6, ".ufraw")==0 )
	    g_unlink(filename);
    }
    if ( status != UFRAW_SUCCESS || uf->gimpImage==-1 ) {
	values[0].type = GIMP_PDB_STATUS;
	if ( status==UFRAW_CANCEL )
	    values[0].data.d_status = GIMP_PDB_CANCEL;
	else
	    values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
	return;
    }
    *nreturn_vals = 2;
    values[0].type = GIMP_PDB_STATUS;
    values[0].data.d_status = GIMP_PDB_SUCCESS;
    values[1].type = GIMP_PDB_IMAGE;
    values[1].data.d_image = uf->gimpImage;
    if ( loadThumbnail ) {
	*nreturn_vals = 4;
	values[2].type = GIMP_PDB_INT32;
	values[2].data.d_int32 = uf->initialWidth;
	values[3].type = GIMP_PDB_INT32;
	values[3].data.d_int32 = uf->initialHeight;
    }
}

int gimp_row_writer(ufraw_data *uf, void *volatile out, void *pixbuf,
    int row, int width, int height, int grayscale, int bitDepth)
{
    (void)uf;
    (void)grayscale;
    (void)bitDepth;

    gimp_pixel_rgn_set_rect(out, pixbuf, 0, row, width, height);

    return UFRAW_SUCCESS;
}

long ufraw_save_gimp_image(ufraw_data *uf, GtkWidget *widget)
{
    GimpDrawable *drawable;
    GimpPixelRgn pixel_region;
    gint32 layer;
    UFRectangle Crop;
    int depth, tile_height, row, nrows;
    (void)widget;

    uf->gimpImage = -1;

    if (uf->conf->embeddedImage) {
	if (ufraw_convert_embedded(uf)!=UFRAW_SUCCESS)
	    return UFRAW_ERROR;
	Crop.height = uf->thumb.height;
	Crop.width = uf->thumb.width;
	Crop.y = 0;
	Crop.x = 0;
	depth = 3;
    } else {
	if (ufraw_convert_image(uf)!=UFRAW_SUCCESS)
	    return UFRAW_ERROR;
	ufraw_get_scaled_crop(uf, &Crop);
#ifdef UFRAW_CINEPAINT
	if ( uf->conf->profile[out_profile]
			[uf->conf->profileIndex[out_profile]].BitDepth==16 )
	    depth = 6;
	else
	    depth = 3;
#else
	depth = 3;
#endif
    }
    uf->gimpImage = gimp_image_new(Crop.width, Crop.height,
	    depth==3 ? GIMP_RGB : U16_RGB );
    if (uf->gimpImage== -1) {
	ufraw_message(UFRAW_ERROR, _("Can't allocate new image."));
	return UFRAW_ERROR;
    }
    gimp_image_set_filename(uf->gimpImage, uf->filename);

    /* Create the "background" layer to hold the image... */
    layer = gimp_layer_new(uf->gimpImage, _("Background"), Crop.width,
	    Crop.height, depth==3 ? GIMP_RGB_IMAGE : U16_RGB_IMAGE,
	    100.0, GIMP_NORMAL_MODE);
    gimp_image_add_layer(uf->gimpImage, layer, 0);

    /* Get the drawable and set the pixel region for our load... */
    drawable = gimp_drawable_get(layer);
    gimp_pixel_rgn_init(&pixel_region, drawable, 0, 0, drawable->width,
	    drawable->height, TRUE, FALSE);
    tile_height = gimp_tile_height();

    if (uf->conf->embeddedImage) {
	for (row = 0; row < Crop.height; row += tile_height) {
	    nrows = MIN(Crop.height-row, tile_height);
	    gimp_pixel_rgn_set_rect(&pixel_region,
		    uf->thumb.buffer+3*row*Crop.width, 0, row, Crop.width, nrows);
	}
    } else {
	ufraw_write_image_data(uf, &pixel_region, &Crop, depth==3 ? 8 : 16, 0,
		gimp_row_writer);
    }
    gimp_drawable_flush(drawable);
    gimp_drawable_detach(drawable);

    if (uf->conf->embeddedImage) return UFRAW_SUCCESS;

#ifdef UFRAW_CINEPAINT
/* No parasites in CinePaint.
 * We need to disable EXIF export.
 * ICC profile export works differently. */
    if ( strcmp(uf->developer->profileFile[out_profile], "")==0) {
	gimp_image_set_srgb_profile(uf->gimpImage);
    } else {
	char *buf;
	gsize len;
	if (g_file_get_contents(uf->developer->profileFile[out_profile],
		&buf, &len, NULL))
	{
	    gimp_image_set_icc_profile_by_mem(uf->gimpImage, len, buf,
		    ICC_IMAGE_PROFILE);
	    g_free(buf);
	} else {
	    ufraw_message(UFRAW_WARNING,
		    _("Failed to embed output profile '%s' in image."),
		    uf->developer->profileFile[out_profile]);
	}
    }
#else
    ufraw_exif_prepare_output(uf);
    if (uf->outputExifBuf!=NULL) {
	if (uf->outputExifBufLen>65533) {
	    ufraw_message(UFRAW_SET_WARNING,
		    _("EXIF buffer length %d, too long, ignored."),
		    uf->outputExifBufLen);
	} else {
	    GimpParasite *exif_parasite;
	    exif_parasite = gimp_parasite_new ("exif-data",
		    GIMP_PARASITE_PERSISTENT, uf->outputExifBufLen, uf->outputExifBuf);
	    gimp_image_parasite_attach (uf->gimpImage, exif_parasite);
	    gimp_parasite_free (exif_parasite);
	}
    }
    /* Create "icc-profile" parasite from output profile
     * if it is not the internal sRGB.*/
    if (strcmp(uf->developer->profileFile[out_profile], "")) {
	char *buf;
	gsize len;
	if (g_file_get_contents(uf->developer->profileFile[out_profile],
		&buf, &len, NULL))
	{
	    GimpParasite *icc_parasite;
	    icc_parasite = gimp_parasite_new ("icc-profile",
		GIMP_PARASITE_PERSISTENT, len, buf);
	    gimp_image_parasite_attach (uf->gimpImage, icc_parasite);
	    gimp_parasite_free (icc_parasite);
	    g_free(buf);
	} else {
	    ufraw_message(UFRAW_WARNING,
		    _("Failed to embed output profile '%s' in image."),
		    uf->developer->profileFile[out_profile]);
	}
    }
#endif
    return UFRAW_SUCCESS;
}

