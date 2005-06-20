/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 * by Udi Fuchs
 *
 * ufraw_writer.c - functions to output image files in different formats.
 *
 * UFRaw is licensed under the GNU General Public License.
 * It uses DCRaw code to do the actual raw decoding.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
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

int ufraw_write_image(image_data *image)
{
    void *out; /* out is a pointer to FILE or TIFF */
    int row, rowStride, i, status=UFRAW_SUCCESS;
    image_type *rawImage;
    guint8 *pixbuf8=NULL;
    guint16 *pixbuf16;
    char *confFilename=NULL;
    char *message=NULL;
    message = message;
    ufraw_message(UFRAW_RESET, NULL);

    if ( image->cfg->createID==only_id ||
         image->cfg->createID==also_id) {
        confFilename = uf_file_set_type(image->cfg->outputFilename, ".ufraw");
        if (!strcmp(confFilename, image->cfg->outputFilename)) {
            ufraw_message(UFRAW_ERROR, "Image filename can not be the "
                        "same as ID filename '%s'", confFilename);
            return UFRAW_ERROR;
        }
    }
    if (image->cfg->createID==only_id) {
        status = save_configuration(image->cfg, image->developer,
		confFilename, NULL, 0);
        g_free(confFilename);
        return status;
    }
#ifdef HAVE_LIBTIFF
    if (image->cfg->type==tiff8_type || image->cfg->type==tiff16_type) {
        TIFFSetErrorHandler(ufraw_tiff_message);
        TIFFSetWarningHandler(ufraw_tiff_message);
	if (!strcmp(image->cfg->outputFilename, "-"))
            out = TIFFFdOpen(fileno((FILE *)stdout),
		    image->cfg->outputFilename, "w");
        else
            out = TIFFOpen(image->cfg->outputFilename, "w");
        if (out==NULL ) {
            message=ufraw_message(UFRAW_GET_WARNING, NULL);
            ufraw_message(UFRAW_ERROR, "Error creating file '%s'.\n%s",
                    image->cfg->outputFilename, message);
            return UFRAW_ERROR;
        }
    } else
#endif
    {
	if (!strcmp(image->cfg->outputFilename, "-")) {
            out = stdout;
	} else {
            if ( (out=fopen(image->cfg->outputFilename, "wb"))==NULL) {
                ufraw_message(UFRAW_ERROR, "Error creating file '%s': %s",
                        image->cfg->outputFilename, g_strerror(errno));
                return UFRAW_ERROR;
            }
        }
    }
    ufraw_convert_image(image, image);
    rowStride = image->width + 2*image->trim;
    rawImage = image->image + image->trim*rowStride + image->trim;
    pixbuf16 = g_new(guint16, image->width*3);
    if (image->cfg->type==ppm8_type) {
        fprintf(out, "P6\n%d %d\n%d\n", image->width, image->height, 0xFF);
        pixbuf8 = g_new(guint8, image->width*3);
        for (row=0; row<image->height; row++) {
            if (row%100==99)
                preview_progress(image->widget, "Saving image",
			0.5 + 0.5*row/image->height);
            develope(pixbuf8, rawImage[row*rowStride],
                    image->developer, 8, pixbuf16, image->width);
            if ((int)fwrite(pixbuf8, 3, image->width, out)<image->width) {
                ufraw_message(UFRAW_ERROR, "Error creating file '%s': %s.",
                        image->cfg->outputFilename, g_strerror(errno));
                status = UFRAW_ABORT_SAVE;
                break;
            }
        }
    } else if (image->cfg->type==ppm16_type) {
        fprintf(out, "P6\n%d %d\n%d\n", image->width, image->height, 0xFFFF);
        for (row=0; row<image->height; row++) {
            if (row%100==99)
                preview_progress(image->widget, "Saving image",
			0.5 + 0.5*row/image->height);
            develope(pixbuf16, rawImage[row*rowStride],
                    image->developer, 16, pixbuf16, image->width);
            for (i=0; i<3*image->width; i++)
                pixbuf16[i] = g_htons(pixbuf16[i]);
            if ((int)fwrite(pixbuf16, 6, image->width, out)<image->width) {
                ufraw_message(UFRAW_ERROR, "Error creating file '%s': %s.",
                        image->cfg->outputFilename, g_strerror(errno));
                status = UFRAW_ABORT_SAVE;
                break;
            }
        }
#ifdef HAVE_LIBTIFF
    } else if (image->cfg->type==tiff8_type ||
            image->cfg->type==tiff16_type) {
        TIFFSetField(out, TIFFTAG_IMAGEWIDTH, image->width);
        TIFFSetField(out, TIFFTAG_IMAGELENGTH, image->height);
        TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
        TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 3);
        if (image->cfg->type==tiff8_type)
            TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);
        else
            TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 16);
        TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
#ifdef HAVE_LIBZ
        if (image->cfg->losslessCompress) {
            TIFFSetField(out, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
            TIFFSetField(out, TIFFTAG_ZIPQUALITY, 9);
        }
        else
#endif
            TIFFSetField(out, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
        /* Embed output profile if it is not the internal sRGB*/
        if (strcmp(image->developer->profileFile[out_profile], "")) {
            char *buf;
            int len;
            if (g_file_get_contents(image->developer->profileFile[out_profile],
                &buf, &len, NULL)) {
            TIFFSetField(out, TIFFTAG_ICCPROFILE, len, buf);
            g_free(buf);
            } else ufraw_message(UFRAW_WARNING,
                    "Failed to embed output profile '%s' in '%s'",
                    image->developer->profileFile[out_profile],
		    image->cfg->outputFilename);
        }
        TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, 0));
        if (image->cfg->type==tiff8_type) {
            pixbuf8 = g_new(guint8, image->width*3);
            for (row=0; row<image->height; row++) {
                if (row%100==99)
                    preview_progress(image->widget, "Saving image",
			    0.5+0.5*row/image->height);
                develope(pixbuf8, rawImage[row*rowStride],
                        image->developer, 8, pixbuf16, image->width);
                if (TIFFWriteScanline(out, pixbuf8, row, 0)<0) {
                    message=ufraw_message(UFRAW_GET_WARNING, NULL);
                    ufraw_message(UFRAW_ERROR, "Error creating file '%s'.\n%s",
                            image->cfg->outputFilename, message);
                    status = UFRAW_ABORT_SAVE;
                    break;
                }
            }
        } else {
            for (row=0; row<image->height; row++) {
                if (row%100==99)
                    preview_progress(image->widget, "Saving image",
			    0.5+0.5*row/image->height);
                develope(pixbuf16, rawImage[row*rowStride],
                        image->developer, 16, pixbuf16, image->width);
                if (TIFFWriteScanline(out, pixbuf16, row, 0)<0) {
                    message=ufraw_message(UFRAW_GET_WARNING, NULL);
                    ufraw_message(UFRAW_ERROR, "Error creating file '%s'.\n%s",
                            image->cfg->outputFilename, message);
                    status = UFRAW_ABORT_SAVE;
                    break;
                }
            }
        }
#endif /*HAVE_LIBTIFF*/
#ifdef HAVE_LIBJPEG
    } else if (image->cfg->type==jpeg_type) {
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;

        cinfo.err = jpeg_std_error(&jerr);
        /* possible BUG: two messages in case of error? */
        cinfo.err->output_message = ufraw_jpeg_warning;
        cinfo.err->error_exit = ufraw_jpeg_error;
        jpeg_create_compress(&cinfo);
        jpeg_stdio_dest(&cinfo, out);
        cinfo.image_width = image->width;
        cinfo.image_height = image->height;
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, image->cfg->compression, TRUE);

        jpeg_start_compress(&cinfo, TRUE);

        /* Embed output profile if it is not the internal sRGB*/
        if (strcmp(image->developer->profileFile[out_profile], "")) {
            char *buf;
            int len;
            if (g_file_get_contents(image->developer->profileFile[out_profile],
                    &buf, &len, NULL)) {
                write_icc_profile(&cinfo, buf, len);
                g_free(buf);
            } else ufraw_message(UFRAW_WARNING,
                    "Failed to embed output profile '%s' in '%s'",
                    image->developer->profileFile[out_profile],
		    image->cfg->outputFilename);
        }
#ifdef HAVE_LIBEXIF
        if (image->exifBuf!=NULL && image->cfg->embedExif)
            jpeg_write_marker(&cinfo, JPEG_APP0+1,
                    image->exifBuf, image->exifBufLen);
#endif
        pixbuf8 = g_new(guint8, image->width*3);
        for (row=0; row<image->height; row++) {
            if (row%100==99)
                preview_progress(image->widget, "Saving image",
			0.5 + 0.5*row/image->height);
            develope(pixbuf8, rawImage[row*rowStride],
                    image->developer, 8, pixbuf16, image->width);
            jpeg_write_scanlines(&cinfo, &pixbuf8, 1);
            message=ufraw_message(UFRAW_GET_ERROR, NULL);
            if (message!=NULL ) break;
        }
        if (message==NULL) jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        if ( (message=ufraw_message(UFRAW_GET_ERROR, NULL))!=NULL) {
            ufraw_message(UFRAW_ERROR, "Error creating file '%s'.\n%s",
                    image->cfg->outputFilename, message);
            status = UFRAW_ABORT_SAVE;
        } else if (ufraw_message(UFRAW_GET_WARNING, NULL)!=NULL) {
            ufraw_message(UFRAW_REPORT, NULL);
        }
#endif /*HAVE_LIBJPEG*/
    } else {
        ufraw_message(UFRAW_ERROR,
                "Error creating file '%s'. Unknown file type %d.",
                image->cfg->outputFilename, image->cfg->type);
        status = UFRAW_ABORT_SAVE;
    }
    g_free(pixbuf16);
    g_free(pixbuf8);
#ifdef HAVE_LIBTIFF
    if (image->cfg->type==tiff8_type || image->cfg->type==tiff16_type) {
        TIFFClose(out);
        if ( (message=ufraw_message(UFRAW_GET_ERROR, NULL))!=NULL) {
            ufraw_message(UFRAW_ERROR, "Error creating file '%s'.\n%s",
                    image->cfg->outputFilename, message);
            status = UFRAW_ABORT_SAVE;
        }
    } else
#endif
	if (strcmp(image->cfg->outputFilename, "-"))
            fclose(out);
    if (image->cfg->createID==also_id) {
        save_configuration(image->cfg, image->developer, confFilename, NULL, 0);
        g_free(confFilename);
    }
    return status;
}

int ufraw_batch_saver(image_data *image)
{
    if (strlen(image->cfg->outputFilename)==0) {
	/* If output filename wasn't specified use input filename */
        char *filename = uf_file_set_type(image->filename,
			file_type[image->cfg->type]);
	if (strlen(image->cfg->outputPath)>0) {
	    char *cp = g_path_get_basename(filename);
	    g_free(filename);
	    filename = g_build_filename(image->cfg->outputPath, cp , NULL);
	    g_free(cp);
	}
        g_strlcpy(image->cfg->outputFilename, filename, max_path);
	g_free(filename);
    }
    if ( !image->cfg->overwrite && image->cfg->createID!=only_id
       && strcmp(image->cfg->outputFilename, "-")
       && g_file_test(image->cfg->outputFilename, G_FILE_TEST_EXISTS) ) {
        char ans[max_name];
        fprintf(stderr, "ufraw: overwrite '%s'? [y/N] ",
		image->cfg->outputFilename);
        fflush(stderr);
        fgets(ans, max_name, stdin);
        if (ans[0]!='y' && ans[0]!='Y') return UFRAW_CANCEL;
    }
    if (strcmp(image->cfg->outputFilename, "-")) {
	char *absname = uf_file_set_absolute(image->cfg->outputFilename);
	g_strlcpy(image->cfg->outputFilename, absname, max_path);
	g_free(absname);
    }
    return ufraw_write_image(image);
}
