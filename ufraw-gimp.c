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

long ufraw_save_gimp_image(GtkWidget *widget, image_data *image);

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
            "<Load>/UFRaw",
            NULL,
            GIMP_PLUGIN,
            num_load_args,
            num_load_return_vals,
            load_args,
            load_return_vals);

    gimp_register_load_handler("file_ufraw_load", raw_ext, "");
}

void run(const gchar *name,
        gint nparams,
        const GimpParam *param,
        gint *nreturn_vals,
        GimpParam **return_vals)
{
    static GimpParam values[2];
    GimpRunMode run_mode = (GimpRunMode)param[0].data.d_int32;
    char *filename = param[1].data.d_string;
    image_data *image;
    cfg_data cfg;
    int status;
    const char *locale;

    *nreturn_vals = 1;
    *return_vals = values;

    if (strcmp(name, "file_ufraw_load")) {
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
    image = ufraw_open(filename);
    /* if UFRaw fails on jpg or tif then open with Gimp */
    if (image==NULL) {
        if (!strcasecmp(filename + strlen(filename) - 4, ".jpg")) {
            *return_vals = gimp_run_procedure2 ("file_jpeg_load",
                    nreturn_vals, nparams, param);
            return;
        } else if (!strcasecmp(filename+strlen(filename)-4, ".tif")) {
            *return_vals = gimp_run_procedure2 ("file_tiff_load",
                    nreturn_vals, nparams, param);
            return;
        } else {
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
    cfg.size = 0;
    if ( gimp_get_data_size("plug_in_ufraw")==sizeof(cfg) )
        gimp_get_data("plug_in_ufraw", &cfg);
    ufraw_config(image, &cfg);
    strcpy(cfg.outputFilename, "");
    if (run_mode==GIMP_RUN_NONINTERACTIVE) cfg.shrink = 8;
    else cfg.shrink = 1;
    cfg.size = 0;
    /* UFRaw already issues warnings.
     * With GIMP_PDB_CANCEL, Gimp won't issue another one. */
    values[0].type = GIMP_PDB_STATUS;
    values[0].data.d_status = GIMP_PDB_CANCEL;
    if (run_mode==GIMP_RUN_INTERACTIVE) {
        status = ufraw_preview(image, TRUE, ufraw_save_gimp_image);
        gimp_set_data("plug_in_ufraw", &cfg, sizeof(cfg));
    } else {
        status=ufraw_load_raw(image, FALSE);
        ufraw_save_gimp_image(NULL, image);
        ufraw_close(image);
    }
    if (status != UFRAW_SUCCESS) return;
    if (image->gimpImage==-1) {
        values[0].type = GIMP_PDB_STATUS;
        values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
        return;
    }
    *nreturn_vals = 2;
    values[0].type = GIMP_PDB_STATUS;
    values[0].data.d_status = GIMP_PDB_SUCCESS;
    values[1].type = GIMP_PDB_IMAGE;
    values[1].data.d_image = image->gimpImage;
}

long ufraw_save_gimp_image(GtkWidget *widget, image_data *image)
{
    GtkWindow *window;
    GimpDrawable *drawable;
    GimpPixelRgn pixel_region;
    gint32 layer;
    guint8 *pixbuf;
    guint16 *pixtmp;
    int tile_height, row, nrows, rowStride, y;
    image_type *rawImage;

    if (ufraw_convert_image(image, image)!=UFRAW_SUCCESS) {
        image->gimpImage = -1;
        return UFRAW_ERROR;
    }
    image->gimpImage = gimp_image_new(image->width, image->height,GIMP_RGB);
    if (image->gimpImage== -1) {
        ufraw_message(UFRAW_ERROR, "Can't allocate new image.");
        return UFRAW_ERROR;
    }
    gimp_image_set_filename(image->gimpImage, image->filename);

    /* Create the "background" layer to hold the image... */
    layer = gimp_layer_new(image->gimpImage, "Background", image->width,
            image->height, GIMP_RGB_IMAGE, 100, GIMP_NORMAL_MODE);
    gimp_image_add_layer(image->gimpImage, layer, 0);

    /* Get the drawable and set the pixel region for our load... */
    drawable = gimp_drawable_get(layer);
    gimp_pixel_rgn_init(&pixel_region, drawable, 0, 0, drawable->width,
            drawable->height, TRUE, FALSE);
    tile_height = gimp_tile_height();
    pixbuf = g_new(guint8, tile_height * image->width * 3);
    pixtmp = g_new(guint16, tile_height * image->width * 3);

    rowStride = image->width + 2*image->trim;
    rawImage = image->image + image->trim*rowStride + image->trim;
    for (row = 0; row < image->height; row += tile_height) {
        preview_progress(widget, "Loading image", 0.5 + 0.5*row/image->height);
        nrows = MIN(image->height-row, tile_height);
        for (y=0 ; y<nrows; y++)
            develope(&pixbuf[3*y*image->width], rawImage[(row+y)*rowStride],
                    image->developer, 8, pixtmp, image->width);
        gimp_pixel_rgn_set_rect(&pixel_region, pixbuf, 0, row,
                image->width, nrows);
    }
    g_free(pixbuf);
    g_free(pixtmp);
    gimp_drawable_flush(drawable);
    gimp_drawable_detach(drawable);
    if (image->exifBuf!=NULL) {
        GimpParasite *exif_parasite;
        exif_parasite = gimp_parasite_new ("exif-data",
                GIMP_PARASITE_PERSISTENT, image->exifBufLen, image->exifBuf);
        gimp_image_parasite_attach (image->gimpImage, exif_parasite);
        gimp_parasite_free (exif_parasite);
        g_free (image->exifBuf);
        image->exifBuf = NULL;
    }
    if (widget!=NULL) {
        window = GTK_WINDOW(gtk_widget_get_toplevel(widget));
        g_object_set_data(G_OBJECT(window), "WindowResponse",
                (gpointer)GTK_RESPONSE_OK);
        gtk_main_quit();
    }
    return UFRAW_SUCCESS;
}

