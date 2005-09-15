/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 * by Udi Fuchs
 *
 * ufraw_exif.c - read the EXIF data from the RAW file.
 *
 * The EXIF code is base on a patch from Berend Ozceri.
 *
 * UFRaw is licensed under the GNU General Public License.
 * It uses DCRaw code to do the actual raw decoding.
 */

// uncomment the next two lines to compile ufraw_exif as a stand-alone tool.
//#define _STAND_ALONE_
//#define HAVE_LIBEXIF

#ifdef HAVE_CONFIG_H
#include <config.h>
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
#include <tiffio.h>
#endif

#ifdef _STAND_ALONE_
#include <stdarg.h>
#include <jpeglib.h>
#define UFRAW_SET_LOG 0
#define UFRAW_SET_WARNING 1
#define UFRAW_RESET 0
#define UFRAW_SUCCESS 0
#define UFRAW_ERROR 1
void ufraw_message(int code, const char *format, ...)
{
    if (format==NULL) return;
    va_list ap;
    va_start(ap, format);
    if (code==UFRAW_SET_LOG) {
        vfprintf(stdout, format, ap);
        fflush(stdout);
    } else {\
        vfprintf(stderr, format, ap);\
        fflush(stderr);\
    }
    va_end(ap);
}
#define max_name 80
#define max_path 256
#else
#include <glib.h>
#include "ufraw.h"
#endif

void tiff_message(const char *module, const char *fmt, va_list ap)
{
    module = module;
    char buf[max_path];
    vsnprintf(buf, max_path, fmt, ap);
    ufraw_message(UFRAW_SET_WARNING, "%s\n", buf);
}

int ufraw_exif_from_raw(void *ifp, char *filename,
    unsigned char **exifBuf, unsigned int *exifBufLen)
{
#ifdef HAVE_LIBEXIF
    TIFF *tiff;
    short tiffCount;
    unsigned short tiffOrientation;
    int *tiffData, tiffFd, exifOffset;
    ExifData *exifData;
    ExifEntry *entry;
    char *buffer;
    struct stat tiffStat;

    *exifBuf = NULL;
    *exifBufLen = 0;

    TIFFSetErrorHandler(tiff_message);
    TIFFSetWarningHandler(tiff_message);
    ufraw_message(UFRAW_RESET, NULL);
    /* It seems more elegant to use the same FILE * as dcraw,
     * but it does not work for me at the moment */
//    if ((tiff = TIFFFdOpen(fileno((FILE *)ifp), filename, "r")) == NULL)
    ifp = ifp;

    /* Open the NEF TIFF file */
    if ((tiff = TIFFOpen(filename, "r")) == NULL)
        return UFRAW_ERROR;

    /* Look for the EXIF tag */
    if (TIFFGetField(tiff, 34665, &tiffCount, &tiffData)!= 1 || tiffCount != 1)
        return UFRAW_ERROR;

    /* Get the offset of the EXIF data in the TIFF */
    exifOffset = *tiffData;

    /* Get the underlying file descriptor for the TIFF file */
    if ((tiffFd = TIFFFileno(tiff)) < 0)
        return UFRAW_ERROR;

    /* Get the size of the TIFF file through the filesystem */
    if (fstat(tiffFd, &tiffStat) < 0)
        return UFRAW_ERROR;

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
        entry->data = strdup(buffer);
        exif_content_add_entry(exifData->ifd[EXIF_IFD_0], entry);
    }
    /* If there is camera model information */
    if (TIFFGetField(tiff, TIFFTAG_MODEL, &buffer) == 1) {
        entry = exif_entry_new();
        entry->tag = EXIF_TAG_MODEL;
        entry->format = EXIF_FORMAT_ASCII;
        entry->components = strlen(buffer)+1;
        entry->size = exif_format_get_size (entry->format) * entry->components;
        entry->data = strdup(buffer);
        exif_content_add_entry(exifData->ifd[EXIF_IFD_0], entry);
    }
    /* If there is camera orientation information */
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
    /* If there is software information */
    if (TIFFGetField(tiff, TIFFTAG_SOFTWARE, &buffer) == 1) {
        entry = exif_entry_new();
        entry->tag = EXIF_TAG_SOFTWARE;
        entry->format = EXIF_FORMAT_ASCII;
        entry->components = strlen(buffer)+1;
        entry->size = exif_format_get_size (entry->format) * entry->components;
        entry->data = strdup(buffer);
        exif_content_add_entry(exifData->ifd[EXIF_IFD_0], entry);
    }

    if (TIFFGetField(tiff, TIFFTAG_IMAGEDESCRIPTION, &buffer) == 1) {
        entry = exif_entry_new();
        entry->tag = EXIF_TAG_IMAGE_DESCRIPTION;
        entry->format = EXIF_FORMAT_ASCII;
        entry->components = strlen(buffer)+1;
        entry->size = exif_format_get_size (entry->format) * entry->components;
        entry->data = strdup(buffer);
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
       so that the libexiff routines can be fooled into parsing the TIFF */

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

    /* Have libexiff read in the EXIF data */
    exif_data_load_data(exifData, buffer, tiffStat.st_size+9);

    /* Free the TIFF file buffer */
    free(buffer);

    exif_data_save_data(exifData, exifBuf, exifBufLen);

    char value[max_name];
    int i,j;
    for (i=0; i<EXIF_IFD_COUNT; i++) {
        ExifContent *content = exifData->ifd[i];
        if (content==NULL) continue;
        for (j=0; j<(int)content->count; j++) {
            ExifEntry *entry = content->entries[j];
            ufraw_message(UFRAW_SET_LOG, "EXIF tag 0x%04x: %s : %s\n",
                    entry->tag,
                    exif_tag_get_title(entry->tag),
                    exif_entry_get_value(entry, value, max_name));
        }
    }
/*  Interesting tags:
    int tag;
    tag = EXIF_TAG_EXPOSURE_TIME;
    if ( (entry=exif_content_get_entry(exifData->ifd[EXIF_IFD_0], tag))!=NULL) {
        ufraw_message(UFRAW_SET_LOG, "%s : %s\n", exif_tag_get_title(tag),
                        exif_entry_get_value(entry, value, max_name));
    }
    tag = EXIF_TAG_FNUMBER;
    tag = EXIF_TAG_ISO_SPEED_RATINGS;
    tag = EXIF_TAG_DATE_TIME_ORIGINAL;
    tag = EXIF_TAG_APERTURE_VALUE;
    tag = EXIF_TAG_FOCAL_LENGTH;
    tag = EXIF_TAG_SUBJECT_DISTANCE;
*/
    ExifMnoteData *notes = exif_data_get_mnote_data (exifData);
    if (notes==NULL) {
        ufraw_message(UFRAW_SET_LOG, "EXIF unknown MakerNote format.\n");
    } else {
        int c = exif_mnote_data_count (notes);
        ufraw_message(UFRAW_SET_LOG, "EXIF MakerNote contains %i values:\n", c);
        for (i = 0; i < c; i++)
            ufraw_message(UFRAW_SET_LOG, "EXIF tag 0x%04x: %s : %s\n",
                    i, exif_mnote_data_get_title (notes, i),
                    exif_mnote_data_get_value (notes, i, value, max_name));
    }
    exif_data_unref(exifData);

    return UFRAW_SUCCESS;
#else
    ifp = ifp;
    filename = filename;
    exifBuf = exifBuf;
    exifBufLen = exifBufLen;
    ufraw_message(UFRAW_SET_LOG, "ufraw built without EXIF support\n");
    return UFRAW_ERROR;
#endif /*HAVE_LIBEXIF*/
}

#ifdef _STAND_ALONE_

char usage[] = "usage: ufraw-exif input-file.raw [output-file.jpg]\n\n"
"ufraw-exif dumps the EXIF data from the RAW file to stdout.\n"
"If a JPEG file is specified, the EXIF data is copied to it.\n"
"This overwrites the original JPEG, so make sure you have a backup for it.\n";

int main(int argc, char **argv)
{
    unsigned char *buf;
    unsigned int len;
    if (argc<2 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help") ) {
        ufraw_message(UFRAW_SET_WARNING, usage);
        exit(0);
    }
    if (ufraw_exif_from_raw(NULL, argv[1], &buf, &len)!=UFRAW_SUCCESS)
        exit(1);

    if (argc<3)
        exit(0);

    struct jpeg_decompress_struct srcinfo;
    struct jpeg_compress_struct dstinfo;
    struct jpeg_error_mgr jsrcerr, jdsterr;
    jvirt_barray_ptr *coef;
    FILE *in, *out;

    srcinfo.err = jpeg_std_error(&jsrcerr);
    jpeg_create_decompress(&srcinfo);
    dstinfo.err = jpeg_std_error(&jdsterr);
    jpeg_create_compress(&dstinfo);
    if ( (in=fopen(argv[2], "rb"))==NULL) {
        ufraw_message(UFRAW_SET_WARNING, "Error reading JPEG file %s: %s\n",
                argv[2], strerror(errno));
        exit(1);
    }
    jpeg_stdio_src(&srcinfo, in);
    jpeg_read_header(&srcinfo, TRUE);
    coef = jpeg_read_coefficients(&srcinfo);
    jpeg_copy_critical_parameters(&srcinfo, &dstinfo);
    if ( (out=fopen(argv[2], "wb"))==NULL) {
        ufraw_message(UFRAW_SET_WARNING, "Error writing JPEG file %s: %s\n",
                argv[2], strerror(errno));
        exit(1);
    }
    jpeg_stdio_dest(&dstinfo, out);
    jpeg_write_coefficients(&dstinfo, coef);
    jpeg_write_marker(&dstinfo, JPEG_APP0+1, buf, len);
    jpeg_finish_compress(&dstinfo);
    jpeg_destroy_compress(&dstinfo);
    fclose(out);
    jpeg_finish_decompress(&srcinfo);
    jpeg_destroy_decompress(&srcinfo);
    fclose(in);

    exit(0);
}
#endif
