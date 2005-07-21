/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 * by Udi Fuchs,
 *
 * ufraw-gimp.c - The GIMP plug-in.
 *
 * based on the gimp plug-in by Pawel T. Jochym jochym at ifj edu pl,
 *
 * based on the gimp plug-in by Dave Coffin
 * http://www.cybercom.net/~dcoffin/
 *
 * UFRaw is licensed under the GNU General Public License.
 * It uses "dcraw" code to do the actual raw decoding.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <locale.h>
#include "ufraw.h"
#include "ufraw_icon.h"

void query();
void run(const gchar *name,
        gint nparams,
        const GimpParam *param,
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
    static gint num_load_args = sizeof load_args / sizeof load_args[0];
    static gint num_load_return_vals =
        sizeof load_return_vals / sizeof load_return_vals[0];

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
            num_load_args,
            num_load_return_vals,
            load_args,
            load_return_vals);

    gimp_register_load_handler("file_ufraw_load", raw_ext, "");
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

void run(const gchar *name,
        gint nparams,
        const GimpParam *param,
        gint *nreturn_vals,
        GimpParam **return_vals)
{
    static GimpParam values[2];
    GimpRunMode run_mode;
    char *filename;
    int size;
    ufraw_data *uf;
    conf_data conf;
    int status;
    const char *locale;

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
    locale = setlocale(LC_ALL, "");
    if ( locale!=NULL &&
        (!strncmp(locale, "he", 2) || !strncmp(locale, "iw", 2) ||
        !strncmp(locale, "ar", 2) ||
        !strncmp(locale, "Hebrew", 6) || !strncmp(locale, "Arabic", 6) ) ) {
        /* I'm not sure why the following doesn't work (on Windows at least) */
/*        setlocale(LC_ALL, "en_US");
        gtk_disable_setlocale(); */
        /* so I'm using setenv */
        g_setenv("LC_ALL", "en_US", TRUE);
    }
    gimp_ui_init("ufraw-gimp", TRUE);

    uf = ufraw_open(filename);
    /* if UFRaw fails on jpg or tif then open with Gimp */
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
		/* There is no "file_tiff_load_thumb", so we need to use
		 * "file_tiff_load". */
		GimpParam tiffParam[3];
		tiffParam[0].type = GIMP_PDB_INT32;
		tiffParam[0].data.d_int32 = GIMP_RUN_NONINTERACTIVE;
		tiffParam[1].type = GIMP_PDB_STRING;
		tiffParam[1].data.d_string = filename;
		tiffParam[2].type = GIMP_PDB_STRING;
		tiffParam[2].data.d_string = filename;
		*return_vals = gimp_run_procedure2 ("file_tiff_load",
		    	nreturn_vals, 3, tiffParam);
	    }
            return;
        } else {
	    /* Don't issue a message on thumbnail failure, since ufraw-gimp
	     * will be called again with "file_ufraw_load" */
	    if (size>0) return;

            GtkWidget *dummyWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
            gtk_window_set_icon(GTK_WINDOW(dummyWindow),
                    gdk_pixbuf_new_from_inline(-1, ufraw_icon, FALSE, NULL));
            ufraw_message(UFRAW_SET_PARENT, (char *)dummyWindow);

            ufraw_message(UFRAW_REPORT, NULL);
            values[0].type = GIMP_PDB_STATUS;
            /* With GIMP_PDB_CANCEL, Gimp won't issue a warning */
            values[0].data.d_status = GIMP_PDB_CANCEL;

            gtk_widget_destroy(dummyWindow);
            return;
        }
    }
    conf.confSize = 0;
    if ( gimp_get_data_size("plug_in_ufraw")==sizeof(conf) )
        gimp_get_data("plug_in_ufraw", &conf);
    if (conf.confSize!=conf_default.confSize ||
	            conf.version!=conf_default.version) {
	/* Load $HOME/.ufrawrc */
	conf_load(&conf, NULL);
    }
    ufraw_config(uf, &conf, NULL, NULL);
    conf_copy_save(uf->conf, &conf_default);
#if GIMP_CHECK_VERSION(2,2,0)
    if (size>0) uf->conf->size = size;
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
        ufraw_save_gimp_image(NULL, uf);
        ufraw_close(uf);
    }
    if (status != UFRAW_SUCCESS) return;
    if (uf->gimpImage==-1) {
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
	values[2].data.d_int32 = uf->predictateWidth;
	values[3].type = GIMP_PDB_INT32;
	values[3].data.d_int32 = uf->predictateHeight;
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
    int tile_height, row, nrows, rowStride, y;
    image_type *rawImage;

    if (ufraw_convert_image(uf)!=UFRAW_SUCCESS) {
        uf->gimpImage = -1;
        return UFRAW_ERROR;
    }
    uf->gimpImage = gimp_image_new(uf->image.width, uf->image.height,GIMP_RGB);
    if (uf->gimpImage== -1) {
        ufraw_message(UFRAW_ERROR, "Can't allocate new image.");
        return UFRAW_ERROR;
    }
    gimp_image_set_filename(uf->gimpImage, uf->filename);

    /* Create the "background" layer to hold the image... */
    layer = gimp_layer_new(uf->gimpImage, "Background", uf->image.width,
            uf->image.height, GIMP_RGB_IMAGE, 100, GIMP_NORMAL_MODE);
    gimp_image_add_layer(uf->gimpImage, layer, 0);

    /* Get the drawable and set the pixel region for our load... */
    drawable = gimp_drawable_get(layer);
    gimp_pixel_rgn_init(&pixel_region, drawable, 0, 0, drawable->width,
            drawable->height, TRUE, FALSE);
    tile_height = gimp_tile_height();
    pixbuf = g_new(guint8, tile_height * uf->image.width * 3);
    pixtmp = g_new(guint16, tile_height * uf->image.width * 3);

    rowStride = uf->image.width + 2*uf->image.trim;
    rawImage = uf->image.image + uf->image.trim*rowStride + uf->image.trim;
    for (row = 0; row < uf->image.height; row += tile_height) {
        preview_progress(widget, "Loading image",
		0.5 + 0.5*row/uf->image.height);
        nrows = MIN(uf->image.height-row, tile_height);
        for (y=0 ; y<nrows; y++)
            develope(&pixbuf[3*y*uf->image.width], rawImage[(row+y)*rowStride],
                    uf->developer, 8, pixtmp, uf->image.width);
        gimp_pixel_rgn_set_rect(&pixel_region, pixbuf, 0, row,
                uf->image.width, nrows);
    }
    g_free(pixbuf);
    g_free(pixtmp);
    gimp_drawable_flush(drawable);
    gimp_drawable_detach(drawable);
    if (uf->exifBuf!=NULL) {
        GimpParasite *exif_parasite;
        exif_parasite = gimp_parasite_new ("exif-data",
                GIMP_PARASITE_PERSISTENT, uf->exifBufLen, uf->exifBuf);
        gimp_image_parasite_attach (uf->gimpImage, exif_parasite);
        gimp_parasite_free (exif_parasite);
        g_free (uf->exifBuf);
        uf->exifBuf = NULL;
    }
    if (widget!=NULL) {
        window = GTK_WINDOW(gtk_widget_get_toplevel(widget));
        g_object_set_data(G_OBJECT(window), "WindowResponse",
                (gpointer)GTK_RESPONSE_OK);
        gtk_main_quit();
    }
    return UFRAW_SUCCESS;
}

