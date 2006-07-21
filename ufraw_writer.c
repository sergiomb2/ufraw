/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw_writer.c - functions to output image files in different formats.
 * Copyright 2004-2006 by Udi Fuchs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation. You should have received
 * a copy of the license along with this program.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>     /* for printf */
#include <errno.h>     /* for errno */
#include <string.h>
#include <glib.h>
#ifdef HAVE_LIBTIFF
#include <tiffio.h>
#endif
#ifdef HAVE_LIBJPEG
#include <jpeglib.h>
#include "iccjpeg.h"
#endif
#include "ufraw.h"

#ifdef HAVE_LIBTIFF
void ufraw_tiff_message(const char *module, const char *fmt, va_list ap)
{
    module = module;
    char buf[max_path];
    vsnprintf(buf, max_path, fmt, ap);
    ufraw_message(UFRAW_SET_WARNING, "%s\n", buf);
}
#endif /*HAVE_LIBTIFF*/

#ifdef HAVE_LIBJPEG
void ufraw_jpeg_warning(j_common_ptr cinfo)
{
    ufraw_message(UFRAW_SET_WARNING, cinfo->err->jpeg_message_table
            [cinfo->err->msg_code],
            cinfo->err->msg_parm.i[0],
            cinfo->err->msg_parm.i[1],
            cinfo->err->msg_parm.i[2],
            cinfo->err->msg_parm.i[3]);
}
void ufraw_jpeg_error(j_common_ptr cinfo)
{
    ufraw_message(UFRAW_SET_ERROR, cinfo->err->jpeg_message_table
            [cinfo->err->msg_code],
            cinfo->err->msg_parm.i[0],
            cinfo->err->msg_parm.i[1],
            cinfo->err->msg_parm.i[2],
            cinfo->err->msg_parm.i[3]);
}
#endif /*HAVE_LIBJPEG*/

int ufraw_write_image(ufraw_data *uf)
{
    void *out; /* out is a pointer to FILE or TIFF */
    int width, height, row, rowStride, i, status=UFRAW_SUCCESS;
    image_type *rawImage;
    guint8 *pixbuf8=NULL;
    guint16 *pixbuf16;
    char *confFilename=NULL;
    char *message=NULL;
    message = message;
    ufraw_message(UFRAW_RESET, NULL);

    if ( uf->conf->createID==only_id ||
         uf->conf->createID==also_id) {
        confFilename = uf_file_set_type(uf->conf->outputFilename, ".ufraw");
        if (!strcmp(confFilename, uf->conf->outputFilename)) {
            ufraw_message(UFRAW_ERROR, "Image filename can not be the "
                        "same as ID filename '%s'", confFilename);
            return UFRAW_ERROR;
        }
    }
    if (uf->conf->createID==only_id) {
        status = conf_save(uf->conf, confFilename, NULL);
        g_free(confFilename);
        return status;
    }
#ifdef HAVE_LIBTIFF
    if (uf->conf->type==tiff8_type || uf->conf->type==tiff16_type) {
        TIFFSetErrorHandler(ufraw_tiff_message);
        TIFFSetWarningHandler(ufraw_tiff_message);
	if (!strcmp(uf->conf->outputFilename, "-"))
            out = TIFFFdOpen(fileno((FILE *)stdout),
		    uf->conf->outputFilename, "w");
        else
            out = TIFFOpen(uf->conf->outputFilename, "w");
        if (out==NULL ) {
            message=ufraw_message(UFRAW_GET_WARNING, NULL);
            ufraw_message(UFRAW_ERROR, "Error creating file '%s'.\n%s",
                    uf->conf->outputFilename, message);
            return UFRAW_ERROR;
        }
    } else
#endif
    {
	if (!strcmp(uf->conf->outputFilename, "-")) {
            out = stdout;
	} else {
            if ( (out=fopen(uf->conf->outputFilename, "wb"))==NULL) {
                ufraw_message(UFRAW_ERROR, "Error creating file '%s': %s",
                        uf->conf->outputFilename, g_strerror(errno));
                return UFRAW_ERROR;
            }
        }
    }
    ufraw_convert_image(uf);
    width = uf->image.width;
    height = uf->image.height;
    rowStride = width;
    rawImage = uf->image.image;
    pixbuf16 = g_new(guint16, width*3);
    if (uf->conf->type==ppm8_type) {
        fprintf(out, "P6\n%d %d\n%d\n", width, height, 0xFF);
        pixbuf8 = g_new(guint8, width*3);
        for (row=0; row<height; row++) {
            if (row%100==99)
                preview_progress(uf->widget, "Saving image",
			0.5 + 0.5*row/height);
            develope(pixbuf8, rawImage[row*rowStride],
                    uf->developer, 8, pixbuf16, width);
            if ((int)fwrite(pixbuf8, 3, width, out)<width) {
                ufraw_message(UFRAW_ERROR, "Error creating file '%s': %s.",
                        uf->conf->outputFilename, g_strerror(errno));
                status = UFRAW_ABORT_SAVE;
                break;
            }
        }
    } else if (uf->conf->type==ppm16_type) {
        fprintf(out, "P6\n%d %d\n%d\n", width, height, 0xFFFF);
        for (row=0; row<height; row++) {
            if (row%100==99)
                preview_progress(uf->widget, "Saving image",
			0.5 + 0.5*row/height);
            develope(pixbuf16, rawImage[row*rowStride],
                    uf->developer, 16, pixbuf16, width);
            for (i=0; i<3*width; i++)
                pixbuf16[i] = g_htons(pixbuf16[i]);
            if ((int)fwrite(pixbuf16, 6, width, out)<width) {
                ufraw_message(UFRAW_ERROR, "Error creating file '%s': %s.",
                        uf->conf->outputFilename, g_strerror(errno));
                status = UFRAW_ABORT_SAVE;
                break;
            }
        }
#ifdef HAVE_LIBTIFF
    } else if (uf->conf->type==tiff8_type ||
            uf->conf->type==tiff16_type) {
        TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
        TIFFSetField(out, TIFFTAG_IMAGELENGTH, height);
        TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
        TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 3);
        if (uf->conf->type==tiff8_type)
            TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);
        else
            TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 16);
        TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
#ifdef HAVE_LIBZ
        if (uf->conf->losslessCompress) {
            TIFFSetField(out, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
            TIFFSetField(out, TIFFTAG_ZIPQUALITY, 9);
        }
        else
#endif
            TIFFSetField(out, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
        /* Embed output profile if it is not the internal sRGB*/
        if (strcmp(uf->developer->profileFile[out_profile], "")) {
            char *buf;
            gsize len;
            if (g_file_get_contents(uf->developer->profileFile[out_profile],
                &buf, &len, NULL)) {
            TIFFSetField(out, TIFFTAG_ICCPROFILE, len, buf);
            g_free(buf);
            } else ufraw_message(UFRAW_WARNING,
                    "Failed to embed output profile '%s' in '%s'",
                    uf->developer->profileFile[out_profile],
		    uf->conf->outputFilename);
        }
        TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, 0));
        if (uf->conf->type==tiff8_type) {
            pixbuf8 = g_new(guint8, width*3);
            for (row=0; row<height; row++) {
                if (row%100==99)
                    preview_progress(uf->widget, "Saving image",
			    0.5+0.5*row/height);
                develope(pixbuf8, rawImage[row*rowStride],
                        uf->developer, 8, pixbuf16, width);
                if (TIFFWriteScanline(out, pixbuf8, row, 0)<0) {
                    message=ufraw_message(UFRAW_GET_WARNING, NULL);
                    ufraw_message(UFRAW_ERROR, "Error creating file '%s'.\n%s",
                            uf->conf->outputFilename, message);
                    status = UFRAW_ABORT_SAVE;
                    break;
                }
            }
        } else {
            for (row=0; row<height; row++) {
                if (row%100==99)
                    preview_progress(uf->widget, "Saving image",
			    0.5+0.5*row/height);
                develope(pixbuf16, rawImage[row*rowStride],
                        uf->developer, 16, pixbuf16, width);
                if (TIFFWriteScanline(out, pixbuf16, row, 0)<0) {
                    message=ufraw_message(UFRAW_GET_WARNING, NULL);
                    ufraw_message(UFRAW_ERROR, "Error creating file '%s'.\n%s",
                            uf->conf->outputFilename, message);
                    status = UFRAW_ABORT_SAVE;
                    break;
                }
            }
        }
#endif /*HAVE_LIBTIFF*/
#ifdef HAVE_LIBJPEG
    } else if (uf->conf->type==jpeg_type) {
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;

        cinfo.err = jpeg_std_error(&jerr);
        /* possible BUG: two messages in case of error? */
        cinfo.err->output_message = ufraw_jpeg_warning;
        cinfo.err->error_exit = ufraw_jpeg_error;
        jpeg_create_compress(&cinfo);
        jpeg_stdio_dest(&cinfo, out);
        cinfo.image_width = width;
        cinfo.image_height = height;
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, uf->conf->compression, TRUE);

        jpeg_start_compress(&cinfo, TRUE);

        /* Embed output profile if it is not the internal sRGB*/
        if (strcmp(uf->developer->profileFile[out_profile], "")) {
            char *buf;
            gsize len;
            if (g_file_get_contents(uf->developer->profileFile[out_profile],
                    &buf, &len, NULL)) {
                write_icc_profile(&cinfo, (unsigned char *)buf, len);
                g_free(buf);
            } else ufraw_message(UFRAW_WARNING,
                    "Failed to embed output profile '%s' in '%s'",
                    uf->developer->profileFile[out_profile],
		    uf->conf->outputFilename);
        }
        if (uf->exifBuf!=NULL && uf->conf->embedExif) {
	     if (uf->exifBufLen>65533) {
		ufraw_message(UFRAW_SET_WARNING,
			"EXIF buffer length %d, too long, ignored.\n",
			uf->exifBufLen);
	    } else {
		jpeg_write_marker(&cinfo, JPEG_APP0+1,
			uf->exifBuf, uf->exifBufLen);
	    }
	}
        pixbuf8 = g_new(guint8, width*3);
        for (row=0; row<height; row++) {
            if (row%100==99)
                preview_progress(uf->widget, "Saving image",
			0.5 + 0.5*row/height);
            develope(pixbuf8, rawImage[row*rowStride],
                    uf->developer, 8, pixbuf16, width);
            jpeg_write_scanlines(&cinfo, &pixbuf8, 1);
            message=ufraw_message(UFRAW_GET_ERROR, NULL);
            if (message!=NULL ) break;
        }
        if (message==NULL) jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        if ( (message=ufraw_message(UFRAW_GET_ERROR, NULL))!=NULL) {
            ufraw_message(UFRAW_ERROR, "Error creating file '%s'.\n%s",
                    uf->conf->outputFilename, message);
            status = UFRAW_ABORT_SAVE;
        } else if (ufraw_message(UFRAW_GET_WARNING, NULL)!=NULL) {
            ufraw_message(UFRAW_REPORT, NULL);
        }
#endif /*HAVE_LIBJPEG*/
    } else {
        ufraw_message(UFRAW_ERROR,
                "Error creating file '%s'. Unknown file type %d.",
                uf->conf->outputFilename, uf->conf->type);
        status = UFRAW_ABORT_SAVE;
    }
    g_free(pixbuf16);
    g_free(pixbuf8);
#ifdef HAVE_LIBTIFF
    if (uf->conf->type==tiff8_type || uf->conf->type==tiff16_type) {
        TIFFClose(out);
        if ( (message=ufraw_message(UFRAW_GET_ERROR, NULL))!=NULL) {
            ufraw_message(UFRAW_ERROR, "Error creating file '%s'.\n%s",
                    uf->conf->outputFilename, message);
            status = UFRAW_ABORT_SAVE;
        }
    } else
#endif
	if (strcmp(uf->conf->outputFilename, "-"))
            fclose(out);
    if (uf->conf->createID==also_id) {
        conf_save(uf->conf, confFilename, NULL);
        g_free(confFilename);
    }
    return status;
}

int ufraw_batch_saver(ufraw_data *uf)
{
    if (strlen(uf->conf->outputFilename)==0) {
	/* If output filename wasn't specified use input filename */
        char *filename = uf_file_set_type(uf->filename,
			file_type[uf->conf->type]);
	if (strlen(uf->conf->outputPath)>0) {
	    char *cp = g_path_get_basename(filename);
	    g_free(filename);
	    filename = g_build_filename(uf->conf->outputPath, cp , NULL);
	    g_free(cp);
	}
        g_strlcpy(uf->conf->outputFilename, filename, max_path);
	g_free(filename);
    }
    if ( !uf->conf->overwrite && uf->conf->createID!=only_id
       && strcmp(uf->conf->outputFilename, "-")
       && g_file_test(uf->conf->outputFilename, G_FILE_TEST_EXISTS) ) {
        char ans[max_name];
        fprintf(stderr, "%s: overwrite '%s'? [y/N] ", ufraw_binary,
		uf->conf->outputFilename);
        fflush(stderr);
        fgets(ans, max_name, stdin);
        if (ans[0]!='y' && ans[0]!='Y') return UFRAW_CANCEL;
    }
    if (strcmp(uf->conf->outputFilename, "-")) {
	char *absname = uf_file_set_absolute(uf->conf->outputFilename);
	g_strlcpy(uf->conf->outputFilename, absname, max_path);
	g_free(absname);
    }
    return ufraw_write_image(uf);
}
