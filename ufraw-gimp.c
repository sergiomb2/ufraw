/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw-gimp.c - The GIMP plug-in.
 * Copyright 2004-2006 by Udi Fuchs
 *
 * based on the GIMP plug-in by Pawel T. Jochym jochym at ifj edu pl,
 *
 * based on the GIMP plug-in by Dave Coffin
 * http://www.cybercom.net/~dcoffin/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation. You should have received
 * a copy of the license along with this program.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#ifdef UFRAW_CINEPAINT
/* Bypass a bug in CinePaint header files */
#define RNH_FLOAT
#include <plugin_main.h>
#define GIMP_CONST
/* Fix some compatibility issues between CinePaint and GIMP */
#define GIMP_CHECK_VERSION(a,b,c) 0
typedef GimpRunModeType GimpRunMode;
#else
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#define GIMP_CONST const
/* Missing and irrelevant definitions in GIMP */
#define U16_RGB 0
#define U16_RGB_IMAGE 0
#endif
#include <glib/gi18n.h>
#include "ufraw.h"

void query();
void run(GIMP_CONST gchar *name,
	gint nparams,
	GIMP_CONST GimpParam *param,
	gint *nreturn_vals,
	GimpParam **return_vals);

long ufraw_save_gimp_image(GtkWidget *widget, ufraw_data *uf);

GimpPlugInInfo PLUG_IN_INFO = {
    NULL,  /* init_procedure */
    NULL,  /* quit_procedure */
    query, /* query_procedure */
    run,   /* run_procedure */
};

MAIN()

void query()
{
    static GimpParamDef load_args[] = {
	{ GIMP_PDB_INT32,  "run_mode", "Interactive, non-interactive" },
	{ GIMP_PDB_STRING, "filename", "The name of the file to load" },
	{ GIMP_PDB_STRING, "raw_filename", "The name of the file to load" },
    };
    static GimpParamDef load_return_vals[] = {
	{ GIMP_PDB_IMAGE, "image", "Output image" },
    };
#if GIMP_CHECK_VERSION(2,2,0)
    static GimpParamDef thumb_args[] = {
	{ GIMP_PDB_STRING, "filename",     "The name of the file to load" },
	{ GIMP_PDB_INT32,  "thumb_size",   "Preferred thumbnail size" }
    };
    static GimpParamDef thumb_return_vals[] = {
	{ GIMP_PDB_IMAGE,  "image",        "Thumbnail image" },
	{ GIMP_PDB_INT32,  "image_width",  "Width of full-sized image" },
	{ GIMP_PDB_INT32,  "image_height", "Height of full-sized image" }
    };
#endif
    gimp_install_procedure("file_ufraw_load",
	    "Loads raw digital camera files",
	    "This plug-in loads raw digital camera files.",
	    "Udi Fuchs",
	    "Copyright 2003 by Dave Coffin\n"
	    "Copyright 2004 by Pawel Jochym\n"
	    "Copyright 2004 by Udi Fuchs",
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
	    "Loads a small preview from a raw image",
	    "",
	    "",
	    "Copyright 2004 by Udi Fuchs",
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

void run(GIMP_CONST gchar *name,
	gint nparams,
	GIMP_CONST GimpParam *param,
	gint *nreturn_vals,
	GimpParam **return_vals)
{
    static GimpParam values[4];
    GimpRunMode run_mode;
    char *filename;
    int size;
    ufraw_data *uf;
    conf_data conf;
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

    gimp_ui_init("ufraw-gimp", TRUE);

    uf = ufraw_open(filename);
    /* if UFRaw fails on jpg or tif then open with GIMP */
    if (uf==NULL) {
	if (!strcasecmp(filename + strlen(filename) - 4, ".jpg")) {
	    if (size==0)
		*return_vals = gimp_run_procedure2 ("file_jpeg_load",
			nreturn_vals, nparams, param);
	    else
		*return_vals = gimp_run_procedure2 ("file_jpeg_load_thumb",
			nreturn_vals, nparams, param);
	    return;
	} else if (!strcasecmp(filename+strlen(filename)-4, ".tif")) {
	    if (size==0)
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
	    if (size>0) return;

	    ufraw_icons_init();
	    GtkWidget *dummyWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
#if GTK_CHECK_VERSION(2,6,0)
	    gtk_window_set_icon_name(GTK_WINDOW(dummyWindow), "ufraw");
#else
	    gtk_window_set_icon(GTK_WINDOW(dummyWindow),
		    gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
			"ufraw", 48, GTK_ICON_LOOKUP_USE_BUILTIN, NULL));
#endif
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
    conf_load(&conf, NULL);

    ufraw_config(uf, &conf, NULL, NULL);
    conf_copy_save(uf->conf, &conf_default);
#if GIMP_CHECK_VERSION(2,2,0)
    if (size>0) {
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
    if (size==0 && run_mode==GIMP_RUN_INTERACTIVE) {
	status = ufraw_preview(uf, TRUE, ufraw_save_gimp_image);
	gimp_set_data("plug_in_ufraw", &conf, sizeof(conf));
    } else {
	status=ufraw_load_raw(uf);
	if ( status!=UFRAW_SUCCESS ) {
	    values[0].type = GIMP_PDB_STATUS;
	    values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
	    return;
	}
	ufraw_save_gimp_image(NULL, uf);
	ufraw_close(uf);
    }
    if ( status != UFRAW_SUCCESS || uf->gimpImage==-1 ) {
	values[0].type = GIMP_PDB_STATUS;
	values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
	return;
    }
    *nreturn_vals = 2;
    values[0].type = GIMP_PDB_STATUS;
    values[0].data.d_status = GIMP_PDB_SUCCESS;
    values[1].type = GIMP_PDB_IMAGE;
    values[1].data.d_image = uf->gimpImage;
    if (size>0) {
	*nreturn_vals = 4;
	values[2].type = GIMP_PDB_INT32;
	values[2].data.d_int32 = uf->predictedWidth;
	values[3].type = GIMP_PDB_INT32;
	values[3].data.d_int32 = uf->predictedHeight;
    }
}

long ufraw_save_gimp_image(GtkWidget *widget, ufraw_data *uf)
{
    GtkWindow *window;
    GimpDrawable *drawable;
    GimpPixelRgn pixel_region;
    gint32 layer;
    guint8 *pixbuf;
    guint16 *pixtmp;
    int height, width, depth, tile_height, row, nrows, rowStride, y;
    image_type *rawImage;

    uf->gimpImage = -1;

    if (uf->conf->embeddedImage) {
	if (ufraw_convert_embedded(uf)!=UFRAW_SUCCESS)
	    return UFRAW_ERROR;
	height = uf->thumb.height;
	width = uf->thumb.width;
	depth = 3;
    } else {
	if (ufraw_convert_image(uf)!=UFRAW_SUCCESS)
	    return UFRAW_ERROR;
	height = uf->image.height;
	width = uf->image.width;
#ifdef UFRAW_CINEPAINT
	depth = 6;
#else
	depth = 3;
#endif
    }
    uf->gimpImage = gimp_image_new(width, height,
	    depth==3 ? GIMP_RGB : U16_RGB );
    if (uf->gimpImage== -1) {
	ufraw_message(UFRAW_ERROR, _("Can't allocate new image."));
	return UFRAW_ERROR;
    }
    gimp_image_set_filename(uf->gimpImage, uf->filename);

    /* Create the "background" layer to hold the image... */
    layer = gimp_layer_new(uf->gimpImage, "Background", width,
	    height, depth==3 ? GIMP_RGB_IMAGE : U16_RGB_IMAGE,
	    100, GIMP_NORMAL_MODE);
    gimp_image_add_layer(uf->gimpImage, layer, 0);

    /* Get the drawable and set the pixel region for our load... */
    drawable = gimp_drawable_get(layer);
    gimp_pixel_rgn_init(&pixel_region, drawable, 0, 0, drawable->width,
	    drawable->height, TRUE, FALSE);
    tile_height = gimp_tile_height();

    if (uf->conf->embeddedImage) {
	for (row = 0; row < height; row += tile_height) {
	    nrows = MIN(height-row, tile_height);
	    gimp_pixel_rgn_set_rect(&pixel_region,
		    uf->thumb.buffer+3*row*width, 0, row, width, nrows);
	}
    } else {
	pixbuf = g_new(guint8, tile_height * width * depth);
	pixtmp = g_new(guint16, tile_height * width * 3);
	rowStride = width;
	rawImage = uf->image.image;
	for (row = 0; row < height; row += tile_height) {
	    preview_progress(widget, _("Loading image"),
		    0.5 + 0.5*row/height);
	    nrows = MIN(height-row, tile_height);
	    for (y=0 ; y<nrows; y++)
		develope(&pixbuf[y*width*depth], rawImage[(row+y)*rowStride],
			uf->developer, depth==3 ? 8 : 16, pixtmp, width);
	    gimp_pixel_rgn_set_rect(&pixel_region, pixbuf, 0, row,
		    width, nrows);
	}
	g_free(pixbuf);
	g_free(pixtmp);
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
    if (uf->exifBuf!=NULL) {
	if (uf->exifBufLen>65533) {
	    ufraw_message(UFRAW_SET_WARNING,
		    _("EXIF buffer length %d, too long, ignored.\n"),
		    uf->exifBufLen);
	} else {
	    GimpParasite *exif_parasite;
	    exif_parasite = gimp_parasite_new ("exif-data",
		    GIMP_PARASITE_PERSISTENT, uf->exifBufLen, uf->exifBuf);
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
    if (widget!=NULL) {
	window = GTK_WINDOW(gtk_widget_get_toplevel(widget));
	g_object_set_data(G_OBJECT(window), "WindowResponse",
		(gpointer)GTK_RESPONSE_OK);
	gtk_main_quit();
    }
    return UFRAW_SUCCESS;
}

