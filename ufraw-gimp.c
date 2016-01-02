/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw-gimp.c - The GIMP plug-in.
 * Copyright 2004-2016 by Udi Fuchs
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
#if HAVE_GIMP_2_9
#include <gegl.h>
#endif
#include <libgimpbase/gimpbase.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <glib/gi18n.h>
#include <string.h>

void query();
void run(const gchar *name,
         gint nparams,
         const GimpParam *param,
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
    static const GimpParamDef load_args[] = {
        { GIMP_PDB_INT32,  "run_mode", "Interactive, non-interactive" },
        { GIMP_PDB_STRING, "filename", "The name of the file to load" },
        { GIMP_PDB_STRING, "raw_filename", "The name of the file to load" },
    };
    static const GimpParamDef load_return_vals[] = {
        { GIMP_PDB_IMAGE, "image", "Output image" },
    };
    static const GimpParamDef thumb_args[] = {
        { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
        { GIMP_PDB_INT32,  "thumb_size",   "Preferred thumbnail size" }
    };
    static const GimpParamDef thumb_return_vals[] = {
        { GIMP_PDB_IMAGE,  "image",        "Thumbnail image" },
        { GIMP_PDB_INT32,  "image_width",  "Width of full-sized image" },
        { GIMP_PDB_INT32,  "image_height", "Height of full-sized image" }
    };
    gimp_install_procedure("file_ufraw_load",
                           "Loads digital camera raw files",
                           "Loads digital camera raw files.",
                           "Udi Fuchs",
                           "Copyright 2003 by Dave Coffin\n"
                           "Copyright 2004 by Pawel Jochym\n"
                           "Copyright 2004-2016 by Udi Fuchs",
                           "ufraw-" VERSION,
                           "raw image",
                           NULL,
                           GIMP_PLUGIN,
                           G_N_ELEMENTS(load_args),
                           G_N_ELEMENTS(load_return_vals),
                           load_args,
                           load_return_vals);

#if HAVE_GIMP_2_9
    gimp_register_magic_load_handler("file_ufraw_load",
                                     (char *)raw_ext,
                                     "",
                                     "0,string,II*\\0,"
                                     "0,string,MM\\0*,"
                                     "0,string,<?xml");
#else
    gimp_register_load_handler("file_ufraw_load", (char *)raw_ext, "");
#endif

    gimp_install_procedure("file_ufraw_load_thumb",
                           "Loads thumbnails from digital camera raw files.",
                           "Loads thumbnails from digital camera raw files.",
                           "Udi Fuchs",
                           "Copyright 2004-2016 by Udi Fuchs",
                           "ufraw-" VERSION,
                           NULL,
                           NULL,
                           GIMP_PLUGIN,
                           G_N_ELEMENTS(thumb_args),
                           G_N_ELEMENTS(thumb_return_vals),
                           thumb_args, thumb_return_vals);

    gimp_register_thumbnail_loader("file_ufraw_load",
                                   "file_ufraw_load_thumb");
}

char *ufraw_binary;
gboolean sendToGimpMode;

void run(const gchar *name,
         gint nparams,
         const GimpParam *param,
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

#if !GLIB_CHECK_VERSION(2,31,0)
    g_thread_init(NULL);
#endif
#ifndef _WIN32
    gdk_threads_init();
    gdk_threads_enter();
#endif
    ufraw_binary = g_path_get_basename(gimp_get_progname());
    uf_init_locale(gimp_get_progname());
#if HAVE_GIMP_2_9
    gegl_init(NULL, NULL);
#endif

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
#ifndef _WIN32
        gdk_threads_leave();
#endif
        return;
    }
    gboolean loadThumbnail = size > 0;
    char *gtkrcfile = g_build_filename(uf_get_home_dir(),
                                       ".ufraw-gtkrc", NULL);
    gtk_rc_add_default_file(gtkrcfile);
    g_free(gtkrcfile);
    gimp_ui_init("ufraw-gimp", TRUE);

    uf = ufraw_open(filename);
    /* if UFRaw fails on jpg/jpeg or tif/tiff then open with GIMP */
    if (uf == NULL) {
        if (!strcasecmp(filename + strlen(filename) - 4, ".jpg") ||
                !strcasecmp(filename + strlen(filename) - 5, ".jpeg")) {
            if (loadThumbnail)
                *return_vals = gimp_run_procedure2("file_jpeg_load_thumb",
                                                   nreturn_vals, nparams, param);
            else
                *return_vals = gimp_run_procedure2("file_jpeg_load",
                                                   nreturn_vals, nparams, param);
#ifndef _WIN32
            gdk_threads_leave();
#endif
            return;
        } else if (!strcasecmp(filename + strlen(filename) - 4, ".tif") ||
                   !strcasecmp(filename + strlen(filename) - 5, ".tiff")) {
            if (!loadThumbnail)
                *return_vals = gimp_run_procedure2("file_tiff_load",
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
#ifndef _WIN32
            gdk_threads_leave();
#endif
            return;
        } else {
            /* Don't issue a message on thumbnail failure, since ufraw-gimp
             * will be called again with "file_ufraw_load" */
            if (loadThumbnail) {
#ifndef _WIN32
                gdk_threads_leave();
#endif
                return;
            }
            ufraw_icons_init();
            GtkWidget *dummyWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
            gtk_window_set_icon_name(GTK_WINDOW(dummyWindow), "ufraw");
            ufraw_message(UFRAW_SET_PARENT, (char *)dummyWindow);

            ufraw_message(UFRAW_REPORT, NULL);
            values[0].type = GIMP_PDB_STATUS;
            /* With GIMP_PDB_CANCEL, Gimp won't issue a warning */
            values[0].data.d_status = GIMP_PDB_CANCEL;

            gtk_widget_destroy(dummyWindow);
#ifndef _WIN32
            gdk_threads_leave();
#endif
            return;
        }
    }
    /* Load $HOME/.ufrawrc */
    conf_load(&rc, NULL);

    ufraw_config(uf, &rc, NULL, NULL);
    sendToGimpMode = (uf->conf->createID == send_id);
#if !HAVE_GIMP_2_9
    if (loadThumbnail) {
        uf->conf->size = size;
        uf->conf->embeddedImage = TRUE;
    }
#else
    if (run_mode == GIMP_RUN_NONINTERACTIVE) uf->conf->shrink = 8;
#endif
    /* UFRaw already issues warnings.
     * With GIMP_PDB_CANCEL, Gimp won't issue another one. */
    values[0].type = GIMP_PDB_STATUS;
    values[0].data.d_status = GIMP_PDB_CANCEL;
    /* BUG - what should be done with GIMP_RUN_WITH_LAST_VALS */
    if (run_mode == GIMP_RUN_INTERACTIVE &&
            !loadThumbnail && !sendToGimpMode) {
        /* Show the preview in interactive mode, unless if we are
         * in thumbnail mode or 'send to gimp' mode. */
        status = ufraw_preview(uf, &rc, ufraw_gimp_plugin, ufraw_save_gimp_image);
    } else {
        if (sendToGimpMode) {
            char *text = g_strdup_printf(_("Loading raw file '%s'"),
                                         uf->filename);
            gimp_progress_init(text);
            g_free(text);
        }
        if (sendToGimpMode) gimp_progress_update(0.1);
        status = ufraw_load_raw(uf);
        if (status != UFRAW_SUCCESS) {
            values[0].type = GIMP_PDB_STATUS;
            values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
#ifndef _WIN32
            gdk_threads_leave();
#endif
            return;
        }
        if (sendToGimpMode) gimp_progress_update(0.3);
        ufraw_save_gimp_image(uf, NULL);
        if (sendToGimpMode) gimp_progress_update(1.0);
        ufraw_close_darkframe(uf->conf);
        ufraw_close(uf);
        /* To make sure we don't delete the raw file by mistake we check
         * that the file is really an ID file. */
        if (sendToGimpMode &&
                strcasecmp(filename + strlen(filename) - 6, ".ufraw") == 0)
            g_unlink(filename);
    }
    if (status != UFRAW_SUCCESS || uf->gimpImage == -1) {
        values[0].type = GIMP_PDB_STATUS;
        if (status == UFRAW_CANCEL)
            values[0].data.d_status = GIMP_PDB_CANCEL;
        else
            values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
#ifndef _WIN32
        gdk_threads_leave();
#endif
        return;
    }
    *nreturn_vals = 2;
    values[0].type = GIMP_PDB_STATUS;
    values[0].data.d_status = GIMP_PDB_SUCCESS;
    values[1].type = GIMP_PDB_IMAGE;
    values[1].data.d_image = uf->gimpImage;
    if (loadThumbnail) {
        *nreturn_vals = 4;
        values[2].type = GIMP_PDB_INT32;
        values[2].data.d_int32 = uf->initialWidth;
        values[3].type = GIMP_PDB_INT32;
        values[3].data.d_int32 = uf->initialHeight;
    }
#ifndef _WIN32
    gdk_threads_leave();
#endif
    return;
}

int gimp_row_writer(ufraw_data *uf, void *volatile out, void *pixbuf,
                    int row, int width, int height, int grayscale, int bitDepth)
{
    (void)uf;
    (void)grayscale;
    (void)bitDepth;

#if HAVE_GIMP_2_9
    gegl_buffer_set(out, GEGL_RECTANGLE(0, row, width, height),
                    0, NULL, pixbuf,
                    GEGL_AUTO_ROWSTRIDE);
#else
    gimp_pixel_rgn_set_rect(out, pixbuf, 0, row, width, height);
#endif

    return UFRAW_SUCCESS;
}

long ufraw_save_gimp_image(ufraw_data *uf, GtkWidget *widget)
{
#if HAVE_GIMP_2_9
    GeglBuffer *buffer;
#else
    GimpDrawable *drawable;
    GimpPixelRgn pixel_region;
    int tile_height, row, nrows;
#endif
    gint32 layer;
    UFRectangle Crop;
    int depth;
    (void)widget;

    uf->gimpImage = -1;

    if (uf->conf->embeddedImage) {
        if (ufraw_convert_embedded(uf) != UFRAW_SUCCESS)
            return UFRAW_ERROR;
        Crop.height = uf->thumb.height;
        Crop.width = uf->thumb.width;
        Crop.y = 0;
        Crop.x = 0;
        depth = 3;
    } else {
        if (ufraw_convert_image(uf) != UFRAW_SUCCESS)
            return UFRAW_ERROR;
        ufraw_get_scaled_crop(uf, &Crop);
#if HAVE_GIMP_2_9
        if (uf->conf->profile[out_profile]
                [uf->conf->profileIndex[out_profile]].BitDepth == 16)
            depth = 6;
        else
            depth = 3;
#else
        depth = 3;
#endif
    }
#if HAVE_GIMP_2_9
    uf->gimpImage =
        gimp_image_new_with_precision(Crop.width, Crop.height, GIMP_RGB,
                                      depth == 3 ? GIMP_PRECISION_U8_GAMMA :
                                      GIMP_PRECISION_U16_GAMMA);
#else
    uf->gimpImage = gimp_image_new(Crop.width, Crop.height, GIMP_RGB);
#endif
    if (uf->gimpImage == -1) {
        ufraw_message(UFRAW_ERROR, _("Can't allocate new image."));
        return UFRAW_ERROR;
    }
    gimp_image_set_filename(uf->gimpImage, uf->filename);

    /* Create the "background" layer to hold the image... */
    layer = gimp_layer_new(uf->gimpImage, _("Background"), Crop.width,
                           Crop.height, GIMP_RGB_IMAGE, 100.0,
                           GIMP_NORMAL_MODE);
#if defined(GIMP_CHECK_VERSION) && GIMP_CHECK_VERSION(2,7,3)
    gimp_image_insert_layer(uf->gimpImage, layer, 0, 0);
#else
    gimp_image_add_layer(uf->gimpImage, layer, 0);
#endif

    /* Get the drawable and set the pixel region for our load... */
#if HAVE_GIMP_2_9
    buffer = gimp_drawable_get_buffer(layer);
#else
    drawable = gimp_drawable_get(layer);
    gimp_pixel_rgn_init(&pixel_region, drawable, 0, 0, drawable->width,
                        drawable->height, TRUE, FALSE);
    tile_height = gimp_tile_height();
#endif

    if (uf->conf->embeddedImage) {
#if HAVE_GIMP_2_9
        gegl_buffer_set(buffer,
                        GEGL_RECTANGLE(0, 0, Crop.width, Crop.height),
                        0, NULL, uf->thumb.buffer,
                        GEGL_AUTO_ROWSTRIDE);
#else
        for (row = 0; row < Crop.height; row += tile_height) {
            nrows = MIN(Crop.height - row, tile_height);
            gimp_pixel_rgn_set_rect(&pixel_region,
                                    uf->thumb.buffer + 3 * row * Crop.width, 0, row, Crop.width, nrows);
        }
#endif
    } else {
#if HAVE_GIMP_2_9
        ufraw_write_image_data(uf, buffer, &Crop, depth == 3 ? 8 : 16, 0,
                               gimp_row_writer);
#else
        ufraw_write_image_data(uf, &pixel_region, &Crop, depth == 3 ? 8 : 16, 0,
                               gimp_row_writer);
#endif
    }
#if HAVE_GIMP_2_9
    gegl_buffer_flush(buffer);
#else
    gimp_drawable_flush(drawable);
    gimp_drawable_detach(drawable);
#endif

    if (uf->conf->embeddedImage) return UFRAW_SUCCESS;

    ufraw_exif_prepare_output(uf);
    if (uf->outputExifBuf != NULL) {
        if (uf->outputExifBufLen > 65533) {
            ufraw_message(UFRAW_SET_WARNING,
                          _("EXIF buffer length %d, too long, ignored."),
                          uf->outputExifBufLen);
        } else {
            GimpParasite *exif_parasite;

            exif_parasite = gimp_parasite_new("exif-data",
                                              GIMP_PARASITE_PERSISTENT, uf->outputExifBufLen, uf->outputExifBuf);
#if defined(GIMP_CHECK_VERSION) && GIMP_CHECK_VERSION(2,8,0)
            gimp_image_attach_parasite(uf->gimpImage, exif_parasite);
#else
            gimp_image_parasite_attach(uf->gimpImage, exif_parasite);
#endif
            gimp_parasite_free(exif_parasite);

#if defined(GIMP_CHECK_VERSION) && GIMP_CHECK_VERSION(2,8,0)
            {
                GimpParam    *return_vals;
                gint          nreturn_vals;
                return_vals = gimp_run_procedure("plug-in-metadata-decode-exif",
                                                 &nreturn_vals,
                                                 GIMP_PDB_IMAGE, uf->gimpImage,
                                                 GIMP_PDB_INT32, 7,
                                                 GIMP_PDB_INT8ARRAY, "unused",
                                                 GIMP_PDB_END);
                if (return_vals[0].data.d_status != GIMP_PDB_SUCCESS) {
                    g_warning("UFRaw Exif -> XMP Merge failed");
                }
            }
#endif
        }
    }
    /* Create "icc-profile" parasite from output profile
     * if it is not the internal sRGB.*/
    if (strcmp(uf->developer->profileFile[out_profile], "")) {
        char *buf;
        gsize len;
        if (g_file_get_contents(uf->developer->profileFile[out_profile],
                                &buf, &len, NULL)) {
            GimpParasite *icc_parasite;
            icc_parasite = gimp_parasite_new("icc-profile",
                                             GIMP_PARASITE_PERSISTENT, len, buf);
#if defined(GIMP_CHECK_VERSION) && GIMP_CHECK_VERSION(2,8,0)
            gimp_image_attach_parasite(uf->gimpImage, icc_parasite);
#else
            gimp_image_parasite_attach(uf->gimpImage, icc_parasite);
#endif
            gimp_parasite_free(icc_parasite);
            g_free(buf);
        } else {
            ufraw_message(UFRAW_WARNING,
                          _("Failed to embed output profile '%s' in image."),
                          uf->developer->profileFile[out_profile]);
        }
    }
    return UFRAW_SUCCESS;
}
