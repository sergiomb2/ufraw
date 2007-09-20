/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw_writer.c - functions to output image files in different formats.
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
#include <errno.h>	/* for errno */
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#ifdef HAVE_LIBTIFF
#include <tiffio.h>
#endif
#ifdef HAVE_LIBJPEG
#include <jpeglib.h>
#include <jerror.h>
#include "iccjpeg.h"
#endif
#ifdef HAVE_LIBPNG
#include <png.h>
#endif
#include "ufraw.h"

#ifdef HAVE_LIBCFITSIO
#include <unistd.h>
#include "fitsio.h"
#endif

#ifdef HAVE_LIBTIFF
// There seem to be no way to get the libtiff message without a static variable
// Therefore the folloing code is not thread-safe. 
static char ufraw_tiff_message[max_path];

ufraw_private_function void tiff_messenger(const char *module,
    const char *fmt, va_list ap)
{
    (void)module;
    vsnprintf(ufraw_tiff_message, max_path, fmt, ap);
}
#endif /*HAVE_LIBTIFF*/

#ifdef HAVE_LIBJPEG
ufraw_private_function void jpeg_warning_handler(j_common_ptr cinfo)
{
    ufraw_data *uf = cinfo->client_data;
    ufraw_set_warning(uf,
	    cinfo->err->jpeg_message_table[cinfo->err->msg_code],
	    cinfo->err->msg_parm.i[0],
	    cinfo->err->msg_parm.i[1],
	    cinfo->err->msg_parm.i[2],
	    cinfo->err->msg_parm.i[3]);
}

ufraw_private_function void jpeg_error_handler(j_common_ptr cinfo)
{
    /* We ignore the SOI error if second byte is 0xd8 since Minolta's
     * SOI is known to be wrong */
    ufraw_data *uf = cinfo->client_data;
    if (cinfo->err->msg_code==JERR_NO_SOI &&
	cinfo->err->msg_parm.i[1]==0xd8) {
	    ufraw_set_info(uf,
		    cinfo->err->jpeg_message_table[cinfo->err->msg_code],
		    cinfo->err->msg_parm.i[0],
		    cinfo->err->msg_parm.i[1],
		    cinfo->err->msg_parm.i[2],
		    cinfo->err->msg_parm.i[3]);
	return;
    }
    ufraw_set_error(uf,
	    cinfo->err->jpeg_message_table[cinfo->err->msg_code],
	    cinfo->err->msg_parm.i[0],
	    cinfo->err->msg_parm.i[1],
	    cinfo->err->msg_parm.i[2],
	    cinfo->err->msg_parm.i[3]);
}
#endif /*HAVE_LIBJPEG*/

#ifdef HAVE_LIBCFITSIO
ufraw_private_function int fits_set_keys(
    fitsfile *ff, ufraw_data *uf, char *chan, float max, float min, int *status)
{
    fits_update_key(ff, TSTRING, "CHANNEL", chan,
	    "red, green or blue channel", status);
    fits_update_key(ff, TFLOAT, "DATAMIN", &min,
	    "minimum data in the data set", status);
    fits_update_key(ff, TFLOAT, "DATAMAX", &max,
	    "maximum data in the data set", status);

    // EXIF properties
    fits_update_key(ff, TSTRING, "EXPOSURE", &uf->conf->shutterText,
	    "Exposure Time", status);
    fits_update_key(ff, TSTRING, "ISO", &uf->conf->isoText,
	    "ISO Speed", status);
    fits_update_key(ff, TSTRING, "APERTURE", &uf->conf->apertureText,
	    "Aperture", status);
    fits_update_key(ff, TSTRING, "FOCALLEN", &uf->conf->focalLenText,
	    "Focal Length", status);
    // TODO: This must be in special time format
    fits_update_key(ff, TSTRING, "DATE-OBS", &uf->conf->timestamp,
	    "Observation Date", status);
    fits_update_key(ff, TSTRING, "MANUFRAC", &uf->conf->make,
	    "Manufractor of the Camera", status);
    fits_update_key(ff, TSTRING, "INSTRUME", &uf->conf->model,
	    "Camera Model", status);

    // Creator Ufraw
    fits_update_key(ff, TSTRING, "CREATOR",  "UFRaw" VERSION,
	    "Creator Software", status);

    return *status;
}
#endif /* HAVE_LIBCFITSIO */

#ifdef HAVE_LIBPNG
ufraw_private_function void png_error_handler(png_structp png,
    png_const_charp error_msg)
{
    ufraw_data *uf = png_get_error_ptr(png);
    ufraw_set_error(uf, "%s: %s.", error_msg, g_strerror(errno));
    longjmp(png_jmpbuf(png), 1);
}

ufraw_private_function void png_warning_handler(png_structp png,
    png_const_charp warning_msg)
{
    ufraw_data *uf = png_get_error_ptr(png);
    ufraw_set_warning(uf, "%s.", warning_msg);
}

ufraw_private_function void PNGwriteRawProfile(png_struct *ping,
    png_info *ping_info, char *profile_type, guint8 *profile_data,
    png_uint_32 length);
#endif /*HAVE_LIBPNG*/

int ufraw_write_image(ufraw_data *uf)
{
    /* 'volatile' supresses clobbering warning */
    void * volatile out; /* out is a pointer to FILE or TIFF */
#ifdef HAVE_LIBCFITSIO
    fitsfile *fitsFile;
#endif
    int width, height, row, rowStride, i;
    int left, top;
    image_type *rawImage;
    guint8 *pixbuf8=NULL;
    guint16 *pixbuf16;
    char * volatile confFilename=NULL;
    ufraw_message_reset(uf);

    if ( uf->conf->createID==only_id ||
	    uf->conf->createID==also_id) {
	confFilename = uf_file_set_type(uf->conf->outputFilename, ".ufraw");
	if (!strcmp(confFilename, uf->conf->outputFilename)) {
	    ufraw_set_error(uf, _("Image filename can not be the "
		    "same as ID filename '%s'"), confFilename);
	    g_free(confFilename);
	    return ufraw_get_status(uf);
	}
    }
    if (uf->conf->createID==only_id) {
	int status = conf_save(uf->conf, confFilename, NULL);
	g_free(confFilename);
	return status;
    }
#ifdef HAVE_LIBTIFF
    if (uf->conf->type==tiff8_type || uf->conf->type==tiff16_type) {
	TIFFSetErrorHandler(tiff_messenger);
	TIFFSetWarningHandler(tiff_messenger);
	ufraw_tiff_message[0] = '\0';
	if (!strcmp(uf->conf->outputFilename, "-"))
	    out = TIFFFdOpen(fileno((FILE *)stdout),
		    uf->conf->outputFilename, "w");
	else
	    out = TIFFOpen(uf->conf->outputFilename, "w");
	if (out==NULL ) {
	    ufraw_set_error(uf, _("Error creating file."));
	    ufraw_set_error(uf, ufraw_tiff_message);
	    ufraw_set_error(uf, g_strerror(errno));
	    ufraw_tiff_message[0] = '\0';
	    return ufraw_get_status(uf);
	}
    } else
#endif
#ifdef HAVE_LIBCFITSIO
    if ( uf->conf->type==fits_type ) {
	if ( strcmp(uf->conf->outputFilename, "-")!=0 ) {
	    if ( g_file_test(uf->conf->outputFilename, G_FILE_TEST_EXISTS) ) {
		if ( unlink(uf->conf->outputFilename) ) {
		    ufraw_set_error(uf, _("Error creating file '%s'."),
			    uf->conf->outputFilename);
		    ufraw_set_error(uf, g_strerror(errno));
		    return ufraw_get_status(uf);
		}
	    }
	}
	int status;
        fits_create_file(&fitsFile, uf->conf->outputFilename, &status);
        if ( status ) {
	    ufraw_set_error(uf, _("Error creating file '%s'."),
		    uf->conf->outputFilename);
	    char errBuffer[max_name];
	    fits_get_errstatus(status, errBuffer);
	    ufraw_set_error(uf, errBuffer);
	    while (fits_read_errmsg(errBuffer))
		ufraw_set_error(uf, errBuffer);
	    return ufraw_get_status(uf);
        }
    } else
#endif
    {
	if (!strcmp(uf->conf->outputFilename, "-")) {
	    out = stdout;
	} else {
	    if ( (out=fopen(uf->conf->outputFilename, "wb"))==NULL) {
		ufraw_set_error(uf, _("Error creating file '%s'."),
			uf->conf->outputFilename);
		ufraw_set_error(uf, g_strerror(errno));
		return ufraw_get_status(uf);
	    }
	}
    }
    // TODO: error handling
    ufraw_convert_image(uf);
    left = uf->conf->CropX1 * uf->image.width / uf->initialWidth;
    top = uf->conf->CropY1 * uf->image.height / uf->initialHeight;
    width = (uf->conf->CropX2 - uf->conf->CropX1)
	    * uf->image.width / uf->initialWidth;
    height = (uf->conf->CropY2 - uf->conf->CropY1)
	    * uf->image.height / uf->initialHeight;
    rowStride = uf->image.width;
    rawImage = uf->image.image;
    pixbuf16 = g_new(guint16, width*3);
    if (uf->conf->type==ppm8_type) {
	fprintf(out, "P6\n%d %d\n%d\n", width, height, 0xFF);
	pixbuf8 = g_new(guint8, width*3);
	for (row=0; row<height; row++) {
	    if (row%100==99)
		preview_progress(uf->widget, _("Saving image"),
			0.5 + 0.5*row/height);
	    develope(pixbuf8, rawImage[(top+row)*rowStride+left],
		    uf->developer, 8, pixbuf16, width);
	    if ((int)fwrite(pixbuf8, 3, width, out)<width) {
		ufraw_set_error(uf, _("Error creating file '%s'."),
			uf->conf->outputFilename);
		ufraw_set_error(uf, g_strerror(errno));
		break;
	    }
	}
    } else if (uf->conf->type==ppm16_type) {
	fprintf(out, "P6\n%d %d\n%d\n", width, height, 0xFFFF);
	for (row=0; row<height; row++) {
	    if (row%100==99)
		preview_progress(uf->widget, _("Saving image"),
			0.5 + 0.5*row/height);
	    develope(pixbuf16, rawImage[(top+row)*rowStride+left],
		    uf->developer, 16, pixbuf16, width);
	    for (i=0; i<3*width; i++)
		pixbuf16[i] = g_htons(pixbuf16[i]);
	    if ((int)fwrite(pixbuf16, 6, width, out)<width) {
		ufraw_set_error(uf, _("Error creating file '%s'."),
			uf->conf->outputFilename);
		ufraw_set_error(uf, g_strerror(errno));
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
	    if ( g_file_get_contents(uf->developer->profileFile[out_profile],
			&buf, &len, NULL)) {
		TIFFSetField(out, TIFFTAG_ICCPROFILE, len, buf);
		g_free(buf);
	    } else {
		ufraw_set_warning(uf,
			_("Failed to embed output profile '%s' in '%s'."),
			uf->developer->profileFile[out_profile],
			uf->conf->outputFilename);
	    }
	}
	TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, 0));
	if (uf->conf->type==tiff8_type) {
	    pixbuf8 = g_new(guint8, width*3);
	    for (row=0; row<height; row++) {
		if (row%100==99) preview_progress(uf->widget, _("Saving image"),
			    0.5+0.5*row/height);
		develope(pixbuf8, rawImage[(top+row)*rowStride+left],
			uf->developer, 8, pixbuf16, width);
		if (TIFFWriteScanline(out, pixbuf8, row, 0)<0) {
		    // 'errno' does seem to contain useful information
		    ufraw_set_error(uf, _("Error creating file."));
		    ufraw_set_error(uf, ufraw_tiff_message);
		    ufraw_tiff_message[0] = '\0';
		    break;
		}
	    }
	} else {
	    for (row=0; row<height; row++) {
		if (row%100==99) preview_progress(uf->widget, _("Saving image"),
			    0.5+0.5*row/height);
		develope(pixbuf16, rawImage[(top+row)*rowStride+left],
			uf->developer, 16, pixbuf16, width);
		if (TIFFWriteScanline(out, pixbuf16, row, 0)<0) {
		    // 'errno' does seem to contain useful information
		    ufraw_set_error(uf, _("Error creating file."));
		    ufraw_set_error(uf, ufraw_tiff_message);
		    ufraw_tiff_message[0] = '\0';
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
	cinfo.err->output_message = jpeg_warning_handler;
	cinfo.err->error_exit = jpeg_error_handler;
	cinfo.client_data = uf;
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, out);
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, uf->conf->compression, TRUE);
	if (uf->conf->progressiveJPEG)
	    jpeg_simple_progression(&cinfo);

	jpeg_start_compress(&cinfo, TRUE);

	/* Embed output profile if it is not the internal sRGB*/
	if (strcmp(uf->developer->profileFile[out_profile], "")) {
	    char *buf;
	    gsize len;
	    if (g_file_get_contents(uf->developer->profileFile[out_profile],
		    &buf, &len, NULL)) {
		write_icc_profile(&cinfo, (unsigned char *)buf, len);
		g_free(buf);
	    } else {
		ufraw_set_warning(uf,
			_("Failed to embed output profile '%s' in '%s'."),
			uf->developer->profileFile[out_profile],
			uf->conf->outputFilename);
	    }
	}
	if (uf->exifBuf!=NULL && uf->conf->embedExif) {
	    if (uf->exifBufLen>65533) {
		ufraw_set_warning(uf,
			_("EXIF buffer length %d, too long, ignored."),
			uf->exifBufLen);
	    } else {
		jpeg_write_marker(&cinfo, JPEG_APP0+1,
			uf->exifBuf, uf->exifBufLen);
	    }
	}
	pixbuf8 = g_new(guint8, width*3);
	for (row=0; row<height; row++) {
	    if (row%100==99) preview_progress(uf->widget, _("Saving image"),
			0.5 + 0.5*row/height);
	    develope(pixbuf8, rawImage[(top+row)*rowStride+left],
		    uf->developer, 8, pixbuf16, width);
	    jpeg_write_scanlines(&cinfo, &pixbuf8, 1);
	    if ( ufraw_is_error(uf) ) break;
	}
	if ( ufraw_is_error(uf) ) {
	    char *message = g_strdup(ufraw_get_message(uf));
	    ufraw_message_reset(uf);
	    ufraw_set_error(uf, _("Error creating file '%s'."),
		    uf->conf->outputFilename);
	    ufraw_set_error(uf, message);
	    g_free(message);
	} else
	    jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
#endif /*HAVE_LIBJPEG*/
#ifdef HAVE_LIBPNG
    } else if (uf->conf->type==png8_type || uf->conf->type==png16_type) {
	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
		uf, png_error_handler, png_warning_handler);
	png_infop info = png_create_info_struct(png);
	if ( setjmp(png_jmpbuf(png)) ) {
	    char *message = g_strdup(ufraw_get_message(uf));
	    ufraw_message_reset(uf);
	    ufraw_set_error(uf, _("Error creating file '%s'."),
		    uf->conf->outputFilename);
	    ufraw_set_error(uf, message);
	    g_free(message);
	    png_destroy_write_struct(&png, &info);
	} else {
	    png_init_io(png, out);
	    int bit_depth = uf->conf->type==png8_type ? 8 : 16;
	    png_set_IHDR(png, info, width, height, bit_depth,
		    PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		    PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	    png_set_compression_level(png, Z_BEST_COMPRESSION);
	    png_text text[2];
	    text[0].compression = PNG_TEXT_COMPRESSION_NONE;
	    text[0].key = "Software";
	    text[0].text = "UFRaw";
	    text[1].compression = PNG_TEXT_COMPRESSION_NONE;
	    text[1].key = "Source";
	    text[1].text = g_strdup_printf("%s%s",
		    uf->conf->make, uf->conf->model);
	    png_set_text(png, info, text, 2);
	    g_free(text[1].text);
	    /* Embed output profile if it is not the internal sRGB*/
	    if (strcmp(uf->developer->profileFile[out_profile], "")) {
		char *buf;
		gsize len;
		if (g_file_get_contents(uf->developer->profileFile[out_profile],
			&buf, &len, NULL)) {
		    png_set_iCCP(png, info,
			    uf->developer->profileFile[out_profile],
			    PNG_COMPRESSION_TYPE_BASE,
			    buf, len);
		    g_free(buf);
		} else {
		    ufraw_set_warning(uf,
			_("Failed to embed output profile '%s' in '%s'."),
			uf->developer->profileFile[out_profile],
			uf->conf->outputFilename);
		}
	    }
	    if (uf->exifBuf!=NULL && uf->conf->embedExif)
		PNGwriteRawProfile(png, info, "exif",
			uf->exifBuf, uf->exifBufLen);
	    png_write_info(png, info);
	    if (uf->conf->type==png8_type) {
		pixbuf8 = g_new(guint8, width*3);
		for (row=0; row<height; row++) {
		    if (row%100==99) preview_progress(uf->widget,
			    _("Saving image"), 0.5+0.5*row/height);
		    develope(pixbuf8, rawImage[(top+row)*rowStride+left],
			    uf->developer, 8, pixbuf16, width);
		    png_write_row(png, pixbuf8);
		}
	    } else {
		if ( G_BYTE_ORDER==G_LITTLE_ENDIAN )
		    png_set_swap(png); // Swap byte order to big-endian
		for (row=0; row<height; row++) {
		    if (row%100==99) preview_progress(uf->widget,
			    _("Saving image"), 0.5+0.5*row/height);
		    develope(pixbuf16, rawImage[(top+row)*rowStride+left],
			    uf->developer, 16, pixbuf16, width);
		    png_write_row(png, (guint8 *)pixbuf16);
		}
	    }
	    png_write_end(png, NULL);
	    png_destroy_write_struct(&png, &info);
	}
#endif /*HAVE_LIBPNG*/
#ifdef HAVE_LIBCFITSIO
    } else if ( uf->conf->type==fits_type ) {

        // image data and min/max values
        guint16 *fr, *fg, *fb;
        guint16 rmax, rmin, gmax, gmin, bmax, bmin;
        rmin = gmin = bmin = 65535;
        rmax = gmax = bmax = 0 ;

        fr   = (guint16 *) malloc (width * height * sizeof(guint16));
        fg   = (guint16 *) malloc (width * height * sizeof(guint16));
        fb   = (guint16 *) malloc (width * height * sizeof(guint16));

        for (row=0; row<height; row++) {
            if (row%100==99)
                preview_progress(uf->widget, _("Saving image"),
                    0.5 + 0.5*row/height);
            develope(pixbuf16, rawImage[(top+row)*rowStride+left],
                    uf->developer, 16, pixbuf16, width);
            for (i=0; i < width; i++)
            {
		develop_linear(rawImage[(top+row)*rowStride+left+i], pixbuf16,
			uf->developer);
                // red channel
                fr[row*width + i] = pixbuf16[0];
		rmax = MAX(pixbuf16[0], rmax);
		rmin = MIN(pixbuf16[0], rmin);

                // green channel
                fg[row*width + i] = pixbuf16[1];
		gmax = MAX(pixbuf16[1], gmax);
		gmin = MIN(pixbuf16[1], gmin);

                // blue channel
                fb[row*width + i] = pixbuf16[2];
		bmax = MAX(pixbuf16[2], bmax);
		bmin = MIN(pixbuf16[2], bmin);
            }
        }
        // FITS Header (taken from cookbook.c)
        int bitpix = USHORT_IMG;        // Use float format
        int naxis  = 2;                 // 2-dimensional image
        long naxes[2]  = { width, height };
        int status = 0;                 // status variable for fitsio

        fits_create_img(fitsFile, bitpix, naxis, naxes, &status);
        fits_set_keys(fitsFile, uf, "RED", rmax, rmin, &status);
        fits_write_img(fitsFile, TUSHORT, 1, width*height, fr, &status);

        fits_create_img(fitsFile, bitpix, naxis, naxes, &status);
        fits_set_keys(fitsFile, uf, "GREEN", gmax, gmin, &status);
        fits_write_img(fitsFile, TUSHORT, 1, width*height, fg, &status);

        fits_create_img(fitsFile, bitpix, naxis, naxes, &status);
        fits_set_keys(fitsFile, uf, "BLUE", bmax, bmin, &status);
        fits_write_img(fitsFile, TUSHORT, 1, width*height, fb, &status);

        fits_close_file(fitsFile, &status);

        if ( status ) {
	    ufraw_set_error(uf, _("Error creating file '%s'."),
		    uf->conf->outputFilename);
	    char errBuffer[max_name];
	    fits_get_errstatus(status, errBuffer);
	    ufraw_set_error(uf, errBuffer);
	    while (fits_read_errmsg(errBuffer))
		ufraw_set_error(uf, errBuffer);
	    return ufraw_get_status(uf);
        }
#endif /* HAVE_LIBCFITSIO */
    } else {
	ufraw_set_error(uf, _("Error creating file '%s'."),
		uf->conf->outputFilename);
	ufraw_set_error(uf, _("Unknown file type %d."), uf->conf->type);
    }
    g_free(pixbuf16);
    g_free(pixbuf8);
#ifdef HAVE_LIBTIFF
    if (uf->conf->type==tiff8_type || uf->conf->type==tiff16_type) {
	TIFFClose(out);
	if ( ufraw_tiff_message[0]!='\0' ) {
	    if ( !ufraw_is_error(uf) ) { // Error was not already set before
		ufraw_set_error(uf, _("Error creating file."));
		ufraw_set_error(uf, ufraw_tiff_message);
	    }
	    ufraw_tiff_message[0] = '\0';
	}
    } else
#endif
#ifdef HAVE_LIBCFITSIO
    // Dummy to prevent fclose
    if ( uf->conf->type==fits_type ) {}
    else
#endif
    {
	if (strcmp(uf->conf->outputFilename, "-"))
	    if ( fclose(out)!=0 ) {
		if ( !ufraw_is_error(uf) ) { // Error was not already set before
		    ufraw_set_error(uf, _("Error creating file '%s'."),
			    uf->conf->outputFilename);
		    ufraw_set_error(uf, g_strerror(errno));
		}
	    }
    }
    if (uf->conf->createID==also_id) {
	// TODO: error handling
	conf_save(uf->conf, confFilename, NULL);
	g_free(confFilename);
    }
    return ufraw_get_status(uf);
}


/* Write EXIF data to PNG file.
 * Code copied from DigiKam's libs/dimg/loaders/pngloader.cpp.
 * The EXIF embeding is defined by ImageMagicK.
 * It is documented in the ExifTool page:
 * http://www.sno.phy.queensu.ca/~phil/exiftool/TagNames/PNG.html
 */

#ifdef HAVE_LIBPNG
ufraw_private_function void PNGwriteRawProfile(png_struct *ping,
    png_info *ping_info, char *profile_type, guint8 *profile_data,
    png_uint_32 length)
{
    png_textp text;
    long i;
    guint8 *sp;
    png_charp dp;
    png_uint_32 allocated_length, description_length;

    const guint8 hex[16] =
	    {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
    text = png_malloc(ping, sizeof(png_text));
    description_length = strlen(profile_type);
    allocated_length = length*2 + (length >> 5) + 20 + description_length;

    text[0].text = png_malloc(ping, allocated_length);
    text[0].key = png_malloc(ping, 80);
    text[0].key[0] = '\0';

    g_strlcat(text[0].key, "Raw profile type ", 80);
    g_strlcat(text[0].key, profile_type, 80);

    sp = profile_data;
    dp = text[0].text;
    *dp++='\n';

    g_strlcpy(dp, profile_type, allocated_length);

    dp += description_length;
    *dp++='\n';

    g_snprintf(dp, allocated_length-strlen(text[0].text), "%8lu ", length);

    dp += 8;

    for (i=0; i < (long) length; i++)
    {
        if (i%36 == 0)
            *dp++='\n';

        *(dp++) = hex[((*sp >> 4) & 0x0f)];
        *(dp++) = hex[((*sp++ ) & 0x0f)];
    }

    *dp++='\n';
    *dp='\0';
    text[0].text_length = (dp-text[0].text);
    text[0].compression = -1;

    if (text[0].text_length <= allocated_length)
	png_set_text(ping, ping_info,text, 1);

    png_free(ping, text[0].text);
    png_free(ping, text[0].key);
    png_free(ping, text);
}
#endif /*HAVE_LIBPNG*/
