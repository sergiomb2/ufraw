/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw_embedded.c - functions to output embedded preview image.
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
#ifdef HAVE_LIBJPEG
#include <jpeglib.h>
#endif
#include "dcraw_api.h"
#include "ufraw.h"

/* prototype for functions in ufraw_writer.c */
#ifdef HAVE_LIBJPEG
void ufraw_jpeg_warning(j_common_ptr cinfo);
void ufraw_jpeg_error(j_common_ptr cinfo);
#endif

int ufraw_read_embedded(ufraw_data *uf)
{
    int status=UFRAW_SUCCESS;
    dcraw_data *raw = uf->raw;
    ufraw_message(UFRAW_RESET, NULL);

#ifndef HAVE_LIBJPEG
    ufraw_message(UFRAW_ERROR, "Reading embedded image requires libjpeg.");
    return UFRAW_ERROR;
#endif
    if ( raw->thumbType==unknown_thumb_type) {
        ufraw_message(UFRAW_ERROR, "No embedded image found");
        return UFRAW_ERROR;
    }
    fseek(raw->ifp, raw->thumbOffset, SEEK_SET);

    if ( uf->conf->shrink<2 && uf->conf->size==0 && uf->conf->flip==0 &&
	 raw->thumbType==jpeg_thumb_type) {
        uf->thumb.buffer = g_new(unsigned char, raw->thumbBufferLength);
        fread(uf->thumb.buffer, 1, raw->thumbBufferLength, raw->ifp);
        uf->thumb.buffer[0] = 0xff;
    } else {
	unsigned srcHeight = uf->thumb.height, srcWidth = uf->thumb.width;
	int scaleNum = 1, scaleDenom = 1;

	if (uf->conf->size>0) {
	    int srcSize = MAX(srcHeight, srcWidth);
	    if (srcSize<uf->conf->size) {
		ufraw_message(UFRAW_WARNING, "Original size (%d) "
			"is smaller than the requested size (%d)",
			srcSize, uf->conf->size);
	    } else {
		    scaleNum = uf->conf->size;
		    scaleDenom = srcSize;
	    }
	} else if (uf->conf->shrink>1) {
	    scaleNum = 1;
	    scaleDenom = uf->conf->shrink;
	}
	if (raw->thumbType==ppm_thumb_type) {
	    if (srcHeight*srcWidth*3!=(unsigned)raw->thumbBufferLength) {
		ufraw_message(UFRAW_ERROR, "ppm thumb mismatch, "
			"height %d, width %d, while buffer %d.",
			srcHeight, srcWidth, raw->thumbBufferLength);
		return UFRAW_ERROR;
	    }
	    uf->thumb.buffer = g_new(JSAMPLE, raw->thumbBufferLength);
	    fread(uf->thumb.buffer, 1, raw->thumbBufferLength, raw->ifp);
	} else {
#ifdef HAVE_LIBJPEG
	    struct jpeg_decompress_struct srcinfo;
	    struct jpeg_error_mgr jsrcerr;
	    srcinfo.err = jpeg_std_error(&jsrcerr);
	    /* possible BUG: two messages in case of error? */
	    srcinfo.err->output_message = ufraw_jpeg_warning;
	    srcinfo.err->error_exit = ufraw_jpeg_error;

	    jpeg_create_decompress(&srcinfo);
	    jpeg_stdio_src(&srcinfo, raw->ifp);

	    jpeg_read_header(&srcinfo, TRUE);
	    if (srcinfo.image_height!=srcHeight) {
		ufraw_message(UFRAW_WARNING, "JPEG thumb height %d "
			"different than expected %d.",
			srcinfo.image_height, srcHeight);
		srcHeight = srcinfo.image_height;
	    }
	    if (srcinfo.image_width!=srcWidth) {
		ufraw_message(UFRAW_WARNING, "JPEG thumb width %d "
			"different than expected %d.",
			srcinfo.image_width, srcWidth);
		srcWidth = srcinfo.image_width;
	    }
	    srcinfo.scale_num = scaleNum;
	    srcinfo.scale_denom = scaleDenom;
	    jpeg_start_decompress(&srcinfo);
	    uf->thumb.buffer = g_new(JSAMPLE,
		    srcinfo.output_width * srcinfo.output_height *
		    srcinfo.output_components);
	    JSAMPROW buf;
	    while (srcinfo.output_scanline < srcinfo.output_height) {
		buf = uf->thumb.buffer + srcinfo.output_scanline *
			srcinfo.output_width * srcinfo.output_components;
		jpeg_read_scanlines(&srcinfo, &buf, srcinfo.rec_outbuf_height);
	    }
	    uf->thumb.width = srcinfo.output_width;
	    uf->thumb.height = srcinfo.output_height;
	    jpeg_finish_decompress(&srcinfo);
	    jpeg_destroy_decompress(&srcinfo);
	    char *message = ufraw_message(UFRAW_GET_ERROR, NULL);
	    if (message!=NULL) {
		ufraw_message(UFRAW_ERROR, "Error creating file '%s'.\n%s",
			uf->conf->outputFilename, message);
		status = UFRAW_ERROR;
	    } else if (ufraw_message(UFRAW_GET_WARNING, NULL)!=NULL) {
		ufraw_message(UFRAW_REPORT, NULL);
	    }
#endif /* HAVE_LIBJPEG */
    	}
    }
    return status;
}

int ufraw_convert_embedded(ufraw_data *uf)
{
    if ( uf->thumb.buffer==NULL) {
        ufraw_message(UFRAW_ERROR, "No embedded image read");
        return UFRAW_ERROR;
    }
    unsigned srcHeight = uf->thumb.height, srcWidth = uf->thumb.width;
    int scaleNum = 1, scaleDenom = 1;

    if (uf->conf->size>0) {
	int srcSize = MAX(srcHeight, srcWidth);
	if (srcSize>uf->conf->size) {
	    scaleNum = uf->conf->size;
	    scaleDenom = srcSize;
	}
    } else if (uf->conf->shrink>1) {
	scaleNum = 1;
	scaleDenom = uf->conf->shrink;
    }
    unsigned dstWidth = srcWidth * scaleNum / scaleDenom;
    unsigned dstHeight = srcHeight * scaleNum / scaleDenom;
    if (dstWidth != srcWidth || dstHeight != srcHeight) {
	/* libjpeg only shrink by 1,2,4 or 8. Here we finish the work */
	unsigned r, nr, c, nc;
	int m;
	for (r=0; r<srcHeight; r++) {
	    nr = r * dstHeight / srcHeight;
	    for (c=0; c<srcWidth; c++) {
		nc = c * dstWidth / srcWidth;
		for (m=0; m<3; m++) 
		    uf->thumb.buffer[(nr*dstWidth+nc) * 3 + m] =
			    uf->thumb.buffer[(r*srcWidth+c) * 3 + m];
	    }
	}	
    }
    if (uf->conf->flip!=0) {
	unsigned r, nr, c, nc, tmp;
	int m;
	unsigned height = dstHeight;
	unsigned width = dstWidth;
	if (uf->conf->flip & 4) {
	    tmp = height;
	    height = width;
	    width = tmp;
	}
	unsigned char *newBuffer = g_new(unsigned char, width * height * 3);
	for (r=0; r<dstHeight; r++) {
	    if (uf->conf->flip & 2) nr = dstHeight - r - 1;
	    else nr = r;
	    for (c=0; c<dstWidth; c++) {
		if (uf->conf->flip & 1) nc = dstWidth - c -1;
		else nc = c;
		if (uf->conf->flip & 4) tmp = nc*width+nr;
		else tmp = nr*width+nc;
		for (m=0; m<3; m++) {
		    newBuffer[tmp * 3 + m] =
			    uf->thumb.buffer[(r*dstWidth+c) * 3 + m];
		}
	    }
	}
	g_free(uf->thumb.buffer);
	uf->thumb.buffer = newBuffer;
	if (uf->conf->flip & 4) {
	    dstHeight = height;
	    dstWidth = width;
	}
    }
    uf->thumb.height = dstHeight;
    uf->thumb.width = dstWidth;
    return UFRAW_SUCCESS;
}

int ufraw_write_embedded(ufraw_data *uf)
{
    int status=UFRAW_SUCCESS;
    dcraw_data *raw = uf->raw;
    FILE *out = NULL;
    ufraw_message(UFRAW_RESET, NULL);

    if ( uf->conf->type!=embedded_jpeg_type ) {
        ufraw_message(UFRAW_ERROR,
                "Error creating file '%s'. Unknown file type %d.",
                uf->conf->outputFilename, uf->conf->type);
        return UFRAW_ERROR;
    }
    if ( uf->thumb.buffer==NULL) {
        ufraw_message(UFRAW_ERROR, "No embedded image read");
        return UFRAW_ERROR;
    }
    if (!strcmp(uf->conf->outputFilename, "-")) {
	out = stdout;
    } else {
	if ( (out=fopen(uf->conf->outputFilename, "wb"))==NULL) {
	    ufraw_message(UFRAW_ERROR, "Error creating file '%s': %s",
		    uf->conf->outputFilename, g_strerror(errno));
	    return UFRAW_ERROR;
	}
    }
    if ( uf->conf->shrink<2 && uf->conf->size==0 && uf->conf->flip==0 ) {
        fwrite(uf->thumb.buffer, 1, raw->thumbBufferLength, out);
    } else {
#ifdef HAVE_LIBJPEG
	struct jpeg_compress_struct dstinfo;
	struct jpeg_error_mgr jdsterr;
	dstinfo.err = jpeg_std_error(&jdsterr);
	/* possible BUG: two messages in case of error? */
	dstinfo.err->output_message = ufraw_jpeg_warning;
	dstinfo.err->error_exit = ufraw_jpeg_error;

	jpeg_create_compress(&dstinfo);
	dstinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&dstinfo);
	jpeg_set_quality(&dstinfo, uf->conf->compression, TRUE);
	dstinfo.input_components = 3;
	jpeg_default_colorspace(&dstinfo);
	dstinfo.image_width = uf->thumb.width;
	dstinfo.image_height = uf->thumb.height;

	jpeg_stdio_dest(&dstinfo, out);
	jpeg_start_compress(&dstinfo, TRUE);
	JSAMPROW buf;
	while (dstinfo.next_scanline < dstinfo.image_height) {
	    buf = uf->thumb.buffer + dstinfo.next_scanline *
		    dstinfo.image_width * dstinfo.input_components;
	    jpeg_write_scanlines(&dstinfo, &buf, 1);
	}
	jpeg_finish_compress(&dstinfo);
	jpeg_destroy_compress(&dstinfo);
        char *message = ufraw_message(UFRAW_GET_ERROR, NULL);
	if ( message!=NULL ) {
	    ufraw_message(UFRAW_ERROR, "Error creating file '%s'.\n%s",
		    uf->conf->outputFilename, message);
	    status = UFRAW_ERROR;
	} else if (ufraw_message(UFRAW_GET_WARNING, NULL)!=NULL) {
	    ufraw_message(UFRAW_REPORT, NULL);
	}
#endif /*HAVE_LIBJPEG*/
    }
    if ( strcmp(uf->conf->outputFilename, "-") )
        fclose(out);
 
    return status;
}
