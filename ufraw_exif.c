/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw_exif.c - read the EXIF data from the RAW file.
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

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#ifdef HAVE_LIBEXIF
#include <libexif/exif-data.h>
#include <libexif/exif-utils.h>
#endif
#ifdef HAVE_LIBTIFF
#include <tiffio.h>
#endif
#include <glib.h>
#include "ufraw.h"

#ifdef HAVE_EXIV2
int ufraw_exif_from_exiv2(ufraw_data *uf);
#endif

void tiff_message(const char *module, const char *fmt, va_list ap)
{
    module = module;
    char buf[max_path];
    vsnprintf(buf, max_path, fmt, ap);
    ufraw_message(UFRAW_SET_WARNING, "%s\n", buf);
}

#if defined(HAVE_LIBEXIF) && defined(HAVE_LIBTIFF) && defined(USE_LIBEXIF_LIBTIFF_HACK)
static ExifData* ufraw_exif_from_tiff(char *filename)
{
   TIFF *tiff;
//    unsigned short tiffOrientation;
    int tiffFd, exifOffset;
    ExifData *exifData = NULL;
    ExifEntry *entry;
    char *buffer;
    struct stat tiffStat;

    TIFFSetErrorHandler(tiff_message);
    TIFFSetWarningHandler(tiff_message);
    /* It seems more elegant to use the same FILE * as DCRaw,
     * but it does not work for me at the moment */
//    if ((tiff = TIFFFdOpen(fileno((FILE *)ifp), filename, "r")) == NULL)

    /* Open the NEF TIFF file */
    if ((tiff = TIFFOpen(filename, "r")) == NULL)
        return NULL;

    /* Look for the EXIF tag */
    /* EXIFIFD became an official libtiff tag since version 3.7.3 */
    /* that requires different handling. */ 
#ifdef TIFFTAG_EXIFIFD
    if (TIFFGetField(tiff, TIFFTAG_EXIFIFD, &exifOffset)!= 1) {
        TIFFClose(tiff);
        return NULL;
    }
#else
    short tiffCount;
    int *tiffData;
    if (TIFFGetField(tiff, 34665, &tiffCount, &tiffData)!= 1 || tiffCount != 1) {
        TIFFClose(tiff);
        return NULL;
    }
    
    /* Get the offset of the EXIF data in the TIFF */
    exifOffset = *tiffData;
#endif

    /* Get the underlying file descriptor for the TIFF file */
    if ((tiffFd = TIFFFileno(tiff)) < 0) {
        TIFFClose(tiff);
        return NULL;
    }
    
    /* Get the size of the TIFF file through the filesystem */
    if (fstat(tiffFd, &tiffStat) < 0) {
        TIFFClose(tiff);
        return NULL;
    }
    
    /* Creat a new EXIF data structure */
    exifData = exif_data_new();

    /* Some EXIF tags appear only as TIFF tags and not separately
       in the EXIF section, so process those first */

    /* If there is camera make information */
    if (TIFFGetField(tiff, TIFFTAG_MAKE, &buffer) == 1) {
        entry = exif_entry_new();
        entry->tag = EXIF_TAG_MAKE;
        entry->format = EXIF_FORMAT_ASCII;

        entry->components = strlen(buffer)+1;
        entry->size = exif_format_get_size (entry->format) * entry->components;
        entry->data = (unsigned char *)strdup(buffer);
        exif_content_add_entry(exifData->ifd[EXIF_IFD_0], entry);
    }
    /* If there is camera model information */
    if (TIFFGetField(tiff, TIFFTAG_MODEL, &buffer) == 1) {
        entry = exif_entry_new();
        entry->tag = EXIF_TAG_MODEL;
        entry->format = EXIF_FORMAT_ASCII;
        entry->components = strlen(buffer)+1;
        entry->size = exif_format_get_size (entry->format) * entry->components;
        entry->data = (unsigned char *)strdup(buffer);
        exif_content_add_entry(exifData->ifd[EXIF_IFD_0], entry);
    }
    /* If there is camera orientation information */

    /* Orientation is ignored at the moment since UFRaw rotates the image
     * by itself. Orientation aware software like gqview will rotate the
     * image again if we set this tag.
     * This is a temporary solution. The real solution is to read this tag
     * and change it later as needed. */
    /*
    if (TIFFGetField(tiff, TIFFTAG_ORIENTATION, &tiffOrientation) == 1) {
        entry = exif_entry_new();
        entry->tag = EXIF_TAG_ORIENTATION;
        entry->format = EXIF_FORMAT_SHORT;
        entry->components = 1;
        entry->size = exif_format_get_size (entry->format) * entry->components;
        entry->data = malloc(sizeof(unsigned short));
        exif_set_short(entry->data, exif_data_get_byte_order(exifData),
                        tiffOrientation);
        exif_content_add_entry(exifData->ifd[EXIF_IFD_0], entry);
    }
    */
    /* If there is software information */
    if (TIFFGetField(tiff, TIFFTAG_SOFTWARE, &buffer) == 1) {
        entry = exif_entry_new();
        entry->tag = EXIF_TAG_SOFTWARE;
        entry->format = EXIF_FORMAT_ASCII;
        entry->components = strlen(buffer)+1;
        entry->size = exif_format_get_size (entry->format) * entry->components;
        entry->data = (unsigned char *)strdup(buffer);
        exif_content_add_entry(exifData->ifd[EXIF_IFD_0], entry);
    }

    if (TIFFGetField(tiff, TIFFTAG_IMAGEDESCRIPTION, &buffer) == 1) {
        entry = exif_entry_new();
        entry->tag = EXIF_TAG_IMAGE_DESCRIPTION;
        entry->format = EXIF_FORMAT_ASCII;
        entry->components = strlen(buffer)+1;
        entry->size = exif_format_get_size (entry->format) * entry->components;
        entry->data = (unsigned char *)strdup(buffer);
        exif_content_add_entry(exifData->ifd[EXIF_IFD_0], entry);
    }

    /* Allocate a buffer to read the entire file */
    buffer = malloc(tiffStat.st_size + 9);

    /* Seek to the beginning of the file */
    lseek(tiffFd, 0, SEEK_SET);

    /* Slurp the entire file into the buffer, leaving 9 bytes at the
       beginning for the fake JPEG/EXIF header that will be inserted */
    read(tiffFd, buffer+9, tiffStat.st_size);

    /* Close the TIFF file */
    TIFFClose(tiff);

    /* Construct a fake JPEG/EXIF header at the beginning of the buffer
       so that the libexif routines can be fooled into parsing the TIFF */

    /* JPEG APP1 marker */
    buffer[0x0] = 0xe1;

    /* Size field appears to be ignored by libexif (it inspects
       the "number of entries" field in the IFD insted) */
    buffer[0x1] = 0x00;
    buffer[0x2] = 0x00;

    /* EXIF marker, endianness field and fixed value 0x002a */
    buffer[0x3] = 0x45;
    buffer[0x4] = 0x78;
    buffer[0x5] = 0x69;
    buffer[0x6] = 0x66;
    buffer[0x7] = 0x00;
    buffer[0x8] = 0x00;
    buffer[0x9] = 0x4d;
    buffer[0xa] = 0x4d;
    buffer[0xb] = 0x00;
    buffer[0xc] = 0x2a;

    /* Offset of the EXIF data in the TIFF file buffer */
    buffer[0xd] = (exifOffset >> 24) & 0xff;
    buffer[0xe] = (exifOffset >> 16) & 0xff;
    buffer[0xf] = (exifOffset >> 8) & 0xff;
    buffer[0x10] = exifOffset & 0xff;

    /* Have libexif read in the EXIF data */
    exif_data_load_data(exifData, (unsigned char *)buffer, tiffStat.st_size+9);

    /* Free the TIFF file buffer */
    free(buffer);

    return exifData;
}
#endif

int ufraw_exif_from_raw(ufraw_data *uf)
{
    uf->exifBuf = NULL;
    uf->exifBufLen = 0;
#ifdef HAVE_EXIV2
    int status = ufraw_exif_from_exiv2(uf);
    if (status==UFRAW_SUCCESS)
	return status;
#ifdef HAVE_LIBEXIF
    ufraw_message(UFRAW_SET_LOG,
	    "Can not read EXIF data using exiv2. Trying libexif.\n");
#else
    return status;
#endif
#endif
#ifdef HAVE_LIBEXIF
    ExifData *exifData = NULL;

    ufraw_message(UFRAW_RESET, NULL);

    /* Try "native" libexif first */
    exifData = exif_data_new_from_file(uf->filename);
    if (exifData != NULL) {
	if (exifData->size == 0) {
	    exif_data_free(exifData);
	    exifData = NULL;
	} else {
	    g_strlcpy(uf->conf->exifSource, "libexif", max_name);
	}
    }
#if defined(HAVE_LIBTIFF) && defined(USE_LIBEXIF_LIBTIFF_HACK)
    if (exifData == NULL) {
	exifData = ufraw_exif_from_tiff(uf->filename);
	if (exifData != NULL)
	    g_strlcpy(uf->conf->exifSource, "libexif/libtiff", max_name);
    }
#endif
    if (exifData == NULL)
	return UFRAW_ERROR;
    
    exif_data_save_data(exifData, &uf->exifBuf, &uf->exifBufLen);

    ExifEntry *entry;
    int tag, i;
    tag = EXIF_TAG_EXPOSURE_TIME;
    for (i=0, entry=NULL; i<EXIF_IFD_COUNT && entry==NULL; i++)
	entry=exif_content_get_entry(exifData->ifd[i], tag);
    if ( entry!=NULL) {
        exif_entry_get_value(entry, uf->conf->shutterText, max_name);
    }
    tag = EXIF_TAG_APERTURE_VALUE;
    for (i=0, entry=NULL; i<EXIF_IFD_COUNT && entry==NULL; i++)
	entry=exif_content_get_entry(exifData->ifd[i], tag);
    if ( entry!=NULL) {
        exif_entry_get_value(entry, uf->conf->apertureText, max_name);
    } else {
	tag = EXIF_TAG_FNUMBER;
	for (i=0, entry=NULL; i<EXIF_IFD_COUNT && entry==NULL; i++)
	    entry=exif_content_get_entry(exifData->ifd[i], tag);
	if ( entry!=NULL) {
	    exif_entry_get_value(entry, uf->conf->apertureText, max_name);
	}
    }
    tag = EXIF_TAG_ISO_SPEED_RATINGS;
    for (i=0, entry=NULL; i<EXIF_IFD_COUNT && entry==NULL; i++)
	entry=exif_content_get_entry(exifData->ifd[i], tag);
    if ( entry!=NULL) {
        exif_entry_get_value(entry, uf->conf->isoText, max_name);
    }
    tag = EXIF_TAG_FOCAL_LENGTH;
    for (i=0, entry=NULL; i<EXIF_IFD_COUNT && entry==NULL; i++)
	entry=exif_content_get_entry(exifData->ifd[i], tag);
    if ( entry!=NULL) {
        exif_entry_get_value(entry, uf->conf->focalLenText, max_name);
    }
    exif_data_unref(exifData);

    return UFRAW_SUCCESS;
#endif /*HAVE_LIBEXIF*/
    uf = uf;
    ufraw_message(UFRAW_SET_LOG, "ufraw built without EXIF support\n");
    return UFRAW_ERROR;
}
