/*
   UFRaw - Unidentified Flying Raw
   Raw photo loader plugin for The GIMP
   by udi Fuchs,

   based on the gimp plug-in by Pawel T. Jochym jochym at ifj edu pl,

   based on the gimp plug-in by Dave Coffin
   http://www.cybercom.net/~dcoffin/

   UFRaw is licensed under the GNU General Public License.
   It uses "dcraw" code to do the actual raw decoding.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <glib.h>
#include "dcraw_api.h"
#include "nikon_curve.h"
#include "ufraw.h"

char *ufraw_message_buffer(char *buffer, char *message)
{
#ifdef UFRAW_DEBUG
    ufraw_batch_messenger(message, NULL);
#endif
    char *buf;
    if (buffer==NULL) return g_strdup(message);
    buf = g_strconcat(buffer, message, NULL);
    g_free(buffer);
    return buf;
}

void ufraw_batch_messenger(char *message, void *parentWindow)
{
    if (parentWindow!=NULL) {
        ufraw_messenger(message, parentWindow);
#ifdef UFRAW_DEBUG
        ufraw_batch_messenger(message, NULL);
#endif
        return;
    }
    fprintf(stderr, "ufraw: %s", message);
    if (message[strlen(message)-1]!='\n') fputc('\n', stderr);
    fflush(stderr);
}

char *ufraw_message(int code, const char *format, ...)
{
    static char *logBuffer = NULL;
    static char *errorBuffer = NULL;
    static gboolean errorFlag = FALSE;
    static void *parentWindow = NULL;
    char *message = NULL;
    void *saveParentWindow;

    if (code==UFRAW_SET_PARENT) {
        saveParentWindow = parentWindow;
        parentWindow = (void *)format;
        return saveParentWindow;
    }
    if (format!=NULL) {
        va_list ap;
        va_start(ap, format);
        message = g_strdup_vprintf (format, ap);
        va_end(ap);
    }
    switch (code) {
    case UFRAW_SET_ERROR:
                errorFlag = TRUE;
    case UFRAW_SET_WARNING:
                errorBuffer = ufraw_message_buffer(errorBuffer, message);
    case UFRAW_SET_LOG:
    case UFRAW_DCRAW_SET_LOG:
                logBuffer = ufraw_message_buffer(logBuffer, message);
                g_free(message);
                return NULL;
    case UFRAW_GET_ERROR:
                if (!errorFlag) return NULL;
    case UFRAW_GET_WARNING:
                return errorBuffer;
    case UFRAW_GET_LOG:
                return logBuffer;
    case UFRAW_CLEAN:
                g_free(logBuffer);
                logBuffer = NULL;
    case UFRAW_RESET:
                g_free(errorBuffer);
                errorBuffer = NULL;
                errorFlag = FALSE;
                return NULL;
    case UFRAW_BATCH_MESSAGE:
                if (parentWindow==NULL)
                    ufraw_batch_messenger(message, parentWindow);
                g_free(message);
                return NULL;
    case UFRAW_INTERACTIVE_MESSAGE:
                if (parentWindow!=NULL)
                    ufraw_batch_messenger(message, parentWindow);
                g_free(message);
                return NULL;
    case UFRAW_LCMS_WARNING:
    case UFRAW_LCMS_RECOVERABLE:
    case UFRAW_LCMS_ABORTED:
                ufraw_batch_messenger(message, parentWindow);
                g_free(message);
                return (char *)1; /* Tell lcms that we are handling the error */
    case UFRAW_REPORT:
                ufraw_batch_messenger(errorBuffer, parentWindow);
                return NULL;
    default:
                ufraw_batch_messenger(message, parentWindow);
                g_free(message);
                return NULL;
    }
}

image_data *ufraw_open(char *filename)
{
    int status;
    image_data *image;
    dcraw_data *raw;
    ufraw_message(UFRAW_CLEAN, NULL);
    raw = g_new(dcraw_data, 1);
    status = dcraw_open(raw, filename);
    if ( status!=DCRAW_SUCCESS) {
        /* Hold the message without displaying it */
        ufraw_message(UFRAW_SET_WARNING, raw->message);
        g_free(raw);
        return NULL;
    }
    image = g_new0(image_data, 1);
    g_strlcpy(image->filename, filename, max_path);
    image->raw = raw;
    image->developer = developer_init();
    if (raw->fuji_width) {
        /* copied from dcraw's fuji_rotate() */
        image->width = raw->fuji_width / 0.5; /* sqrt(0.5); */
        image->height = (raw->height - raw->fuji_width) / 0.5; /* sqrt(0.5); */
    } else {
        image->height = raw->height;
        image->width = raw->width;
    }
    if (raw->flip & 4) {
        int tmp = image->height;
        image->height = image->width;
        image->width = tmp;
    }
    ufraw_message(UFRAW_SET_LOG, "ufraw_open: w:%d h:%d curvesize:%d\n",
        image->width, image->height, raw->toneCurveSize);
    return image;
}

int ufraw_config(image_data *image, cfg_data *cfg)
{
    int i;
    dcraw_data *raw=NULL;
    char *inPath, *oldInPath, *oldOutPath;

    if (cfg->cfgSize!=cfg_default.cfgSize || cfg->version!=cfg_default.version)
        load_configuration(cfg);

    if (cfg->wbLoad==load_default) cfg->wb = camera_wb;
    if (cfg->wbLoad==load_auto) cfg->wb = auto_wb;
    if (cfg->curveLoad==load_default)
        cfg->saturation = cfg_default.saturation;
    if (cfg->exposureLoad==load_default) {
        cfg->exposure = cfg_default.exposure;
        for (i=0; i<cfg->curveCount; i++)
	    CurveDataSetPoint(&cfg->curve[i], 0, 0, 0);
    }
    if (cfg->exposureLoad==load_auto) cfg->exposure = uf_nan();

    if (image==NULL) return UFRAW_SUCCESS;
    image->cfg = cfg;

    /* Guess the prefered output path */
    if (strlen(image->cfg->outputPath)==0) {
        inPath = g_path_get_dirname(image->filename);
        oldInPath = g_path_get_dirname(image->cfg->inputFilename);
        oldOutPath = g_path_get_dirname(image->cfg->outputFilename);
        if ( !strcmp(oldInPath, oldOutPath) || !strcmp(oldOutPath,".") )
            g_strlcpy(image->cfg->outputPath, inPath, max_path);
        else
            g_strlcpy(image->cfg->outputPath, oldOutPath, max_path);
        g_free(inPath);
        g_free(oldInPath);
        g_free(oldOutPath);
        if (!strcmp(image->cfg->outputPath,"."))
            strcpy(image->cfg->outputPath,"");
    }
    char *absname = uf_file_set_absolute(image->filename);
    g_strlcpy(cfg->inputFilename, absname, max_path);
    g_free(absname);

    raw = image->raw;
    if (ufraw_exif_from_raw(raw->ifp, image->filename, &image->exifBuf,
            &image->exifBufLen)!=UFRAW_SUCCESS) {
        ufraw_message(UFRAW_SET_LOG, "Error reading EXIF data from '%s'\n",
                image->filename);
    }
    if (raw->toneCurveSize!=0) {
        CurveData nc;
        long pos = ftell(raw->ifp);
        if (RipNikonNEFCurve(raw->ifp, raw->toneCurveOffset, &nc, NULL)
                !=UFRAW_SUCCESS) {
            ufraw_message(UFRAW_ERROR, "Error reading NEF curve\n");
            return UFRAW_WARNING;
        }
        fseek(raw->ifp, pos, SEEK_SET);
	if (nc.m_numAnchors<2) nc = cfg_default.curve[0];
	g_strlcpy(nc.name, cfg->curve[camera_curve].name, max_name);
        cfg->curve[camera_curve] = nc;
    } else {
	/* BUG? why -1 and not 0? */
        cfg->curve[camera_curve].m_numAnchors = -1;
        /* don't retain camera_curve if no cameraCurve */
        if (cfg->curveIndex==camera_curve)
            cfg->curveIndex = linear_curve;
    }
    return UFRAW_SUCCESS;
}

int ufraw_load_raw(image_data *image, int interactive)
{
    int status, half, c;
    dcraw_data *raw = image->raw;

    half = !interactive && (image->cfg->interpolation==half_interpolation ||
                ( image->cfg->size==0 && image->cfg->shrink%2==0) ||
                ( image->cfg->size>0 &&
                        image->cfg->size<MAX(image->height, image->width)/2 ));
    if ( (status=dcraw_load_raw(raw, half))!=DCRAW_SUCCESS ) {
        ufraw_message(status, raw->message);
        return status;
    }
    image->rgbMax = raw->rgbMax;
    for (c=0; c<4; c++) image->preMul[c] = raw->pre_mul[c];
    if (raw->colors<4) image->preMul[3] = 0;
    return UFRAW_SUCCESS;
}

void ufraw_close(image_data *image)
{
    dcraw_close(image->raw);
    g_free(image->raw);
    developer_destroy(image->developer);
    ufraw_message(UFRAW_CLEAN, NULL);
}

/* Convert rawImage to standard rgb image */
int ufraw_convert_image(image_data *image, image_data *rawImage)
{
    int c;
    dcraw_data *raw = rawImage->raw;
    dcraw_data *rawCopy = image->raw;
    cfg_data *cfg = rawImage->cfg;

    preview_progress("Loading image", 0.1);
    if ( cfg->interpolation==half_interpolation ||
         ( cfg->size==0 && cfg->shrink>1 ) ||
         ( cfg->size>0 && cfg->size<MAX(image->height, image->width)/2 )  ) {
        if (image!=rawImage) {
            rawCopy = image->raw = g_new(dcraw_data, 1);
            image->developer = rawImage->developer;
            image->cfg = rawImage->cfg;
        }
        dcraw_copy_shrink(rawCopy, raw, MAX(cfg->shrink,2));
        dcraw_fuji_rotate(rawCopy);
        if (cfg->size>0) {
            if ( cfg->size>MAX(rawCopy->height, rawCopy->width) ) {
                ufraw_message(UFRAW_ERROR, "Can not downsize from %d to %d.",
                        MAX(rawCopy->height, rawCopy->width), cfg->size);
            } else
                dcraw_size(rawCopy, cfg->size);
        }
        image->rgbMax = rawCopy->rgbMax;
        image->height = rawCopy->height;
        image->width = rawCopy->width;
        image->image = rawCopy->rawImage;
        for (c=0; c<4; c++) image->preMul[c] = rawCopy->pre_mul[c];
	if (rawCopy->colors<4) image->preMul[3] = 0;
	// Small BUG if use_coeff preMul is lost
        if (rawCopy->use_coeff) {
            int preMul[4];
            for (c=0; c<4; c++)
                preMul[c] = rawCopy->pre_mul[c]*0x10000;
            dcraw_scale_colors(rawCopy, preMul);
            image->rgbMax = rawCopy->rgbMax;
            for (c=0; c<4; c++) image->preMul[c] = 1.0;
        }
        ufraw_set_wb(image);
        if (isnan(image->cfg->exposure)) {
            ufraw_auto_expose(image);
            ufraw_auto_black(image);
	}
        developer_prepare(image->developer, image->rgbMax,
                pow(2,cfg->exposure), cfg->unclip,
                cfg->temperature, cfg->green, image->preMul,
                &cfg->profile[0][cfg->profileIndex[0]],
                &cfg->profile[1][cfg->profileIndex[1]], cfg->intent,
                cfg->saturation,
                &cfg->curve[cfg->curveIndex]);
    } else {
        if (isnan(image->cfg->exposure)) {
            image_data tmpImage;
            int shrinkSave = image->cfg->shrink;
            int sizeSave = image->cfg->size;
            image->cfg->shrink = 8;
            image->cfg->size = 0;
            ufraw_convert_image(&tmpImage, image);
            image->cfg->shrink = shrinkSave;
            image->cfg->size = sizeSave;
            tmpImage.developer = NULL;
            ufraw_close(&tmpImage);
        } else
            ufraw_set_wb(image);
        if (raw->use_coeff) {
            int preMul[4];
            for (c=0; c<4; c++)
                preMul[c] = raw->pre_mul[c]*0x10000;
            dcraw_scale_colors(raw, preMul);
            image->rgbMax = raw->rgbMax;
            for (c=0; c<4; c++) image->preMul[c] = 1.0;
            developer_prepare(image->developer, image->rgbMax,
                    pow(2,cfg->exposure), cfg->unclip,
                    cfg->temperature, cfg->green, image->preMul,
                    &cfg->profile[0][cfg->profileIndex[0]],
                    &cfg->profile[1][cfg->profileIndex[1]], cfg->intent,
                    cfg->saturation,
                    &cfg->curve[cfg->curveIndex]);
        } else {
            developer_prepare(image->developer, image->rgbMax,
                    pow(2,cfg->exposure), cfg->unclip,
                    cfg->temperature, cfg->green, image->preMul,
                    &cfg->profile[0][cfg->profileIndex[0]],
                    &cfg->profile[1][cfg->profileIndex[1]], cfg->intent,
                    cfg->saturation,
                    &cfg->curve[cfg->curveIndex]);
            dcraw_scale_colors(raw, image->developer->rgbWB);
            image->rgbMax = raw->rgbMax;
            for (c=0; c<4; c++)
            image->developer->rgbWB[c] = image->developer->rgbMax;
        }
        dcraw_interpolate(raw, cfg->interpolation==quick_interpolation,
                cfg->interpolation==four_color_interpolation);
        dcraw_fuji_rotate(raw);
        if (cfg->size>0) {
            if ( cfg->size>MAX(raw->height, raw->width) ) {
                ufraw_message(UFRAW_ERROR, "Can not downsize from %d to %d.",
                        MAX(raw->height, raw->width), cfg->size);
            } else
                dcraw_size(raw, cfg->size);
        }
    }
    dcraw_convert_to_rgb(rawCopy);
    preview_progress("Loading image", 0.4);
    dcraw_flip_image(rawCopy);
    preview_progress("Loading image", 0.5);
    image->trim = rawCopy->trim;
    image->height = rawCopy->height - 2*image->trim;
    image->width = rawCopy->width - 2*image->trim;
    image->image = rawCopy->rawImage;
    return UFRAW_SUCCESS;
}

int ufraw_set_wb(image_data *image)
{
    dcraw_data *raw = image->raw;
    double rgbWB[3];
    int status, c;

    if (image->cfg->wb==preserve_wb) return UFRAW_SUCCESS;
    if (image->cfg->wb==auto_wb || image->cfg->wb==camera_wb) {
        if ( (status=dcraw_set_color_scale(raw, image->cfg->wb==auto_wb,
                image->cfg->wb==camera_wb))!=DCRAW_SUCCESS ) {
            if (status==DCRAW_NO_CAMERA_WB) {
                ufraw_message(UFRAW_BATCH_MESSAGE,
                    "Cannot use camera white balance, "
                    "reverting to auto white balance.\n");
                ufraw_message(UFRAW_INTERACTIVE_MESSAGE,
                    "Cannot use camera white balance, "
                    "reverting to auto white balance.\n"
                    "You can set 'Auto WB' as the initial setting "
                    "in 'Preferences' to avoid getting this "
                    "message in the future.\n");
                image->cfg->wb = auto_wb;
                status=dcraw_set_color_scale(raw, TRUE, FALSE);
            }
            if (status!=DCRAW_SUCCESS)
		return status;
        }
        for (c=0; c<raw->colors; c++)
            rgbWB[c] = raw->pre_mul[c]/raw->post_mul[c];
    } else if (image->cfg->wb<wb_preset_count) {
        rgbWB[0] = 1/wb_preset[image->cfg->wb].red;
        rgbWB[1] = 1;
        rgbWB[2] = 1/wb_preset[image->cfg->wb].blue;
    } else return UFRAW_ERROR;
    RGB_to_temperature(rgbWB, &image->cfg->temperature, &image->cfg->green);
    return UFRAW_SUCCESS;
}

void ufraw_auto_expose(image_data *image)
{
    int sum, stop, wp, i, c;
    int raw_histogram[0x10000];

    /* set cutoff at 1/256/4 of the histogram */
    stop = image->width*image->height*3/256/4;
    image->cfg->exposure = 0;
    developer_prepare(image->developer, image->rgbMax,
            pow(2,image->cfg->exposure), image->cfg->unclip,
            image->cfg->temperature, image->cfg->green, image->preMul,
            &image->cfg->profile[0][image->cfg->profileIndex[0]],
            &image->cfg->profile[1][image->cfg->profileIndex[1]],
            image->cfg->intent,
            image->cfg->saturation,
            &image->cfg->curve[image->cfg->curveIndex]);

    /* First calculate the exposure */
    memset(raw_histogram, 0, sizeof(raw_histogram));
    for (i=0; i<image->width*image->height; i++)
        for (c=0; c<3; c++)
            raw_histogram[MIN((guint64)image->image[i][c] *
                    image->developer->rgbWB[c] / 0x10000, 0xFFFF)]++;
    for (wp=0xFFFF, sum=0; wp>0 && sum<stop; wp--)
        sum += raw_histogram[wp];
    image->cfg->exposure = -log((double)wp/image->developer->rgbMax)/log(2);
    if (!image->cfg->unclip)
        image->cfg->exposure = MAX(image->cfg->exposure,0);
    ufraw_message(UFRAW_SET_LOG, "ufraw_auto_expose: "
	    "Exposure %f (white point %d)\n", image->cfg->exposure, wp);
}

void ufraw_auto_black(image_data *image)
{
    int sum, stop, bp, i, j;
    int preview_histogram[0x100];
    guint8 *p8 = g_new(guint8, 3*image->width);
    guint16 *pixtmp = g_new(guint16, 3*image->width);

    stop = image->width*image->height*3/256/4;
    CurveDataSetPoint(&image->cfg->curve[image->cfg->curveIndex], 0, 0, 0);
    developer_prepare(image->developer, image->rgbMax,
            pow(2,image->cfg->exposure), image->cfg->unclip,
            image->cfg->temperature, image->cfg->green, image->preMul,
            &image->cfg->profile[0][image->cfg->profileIndex[0]],
            &image->cfg->profile[1][image->cfg->profileIndex[1]],
            image->cfg->intent,
            image->cfg->saturation,
            &image->cfg->curve[image->cfg->curveIndex]);
    memset(preview_histogram, 0, sizeof(preview_histogram));
    for (i=0; i<image->height; i++) {
        develope(p8, image->image[i*image->width], image->developer, 8,
                pixtmp, image->width);
        for (j=0; j<3*image->width; j++) preview_histogram[p8[j]]++;
    }
    for (bp=0, sum=0; bp<0x100 && sum<stop; bp++)
        sum += preview_histogram[bp];
    CurveDataSetPoint(&image->cfg->curve[image->cfg->curveIndex],
	    0, (double)bp/256, 0);
    g_free(p8);
    g_free(pixtmp);
    ufraw_message(UFRAW_SET_LOG, "ufraw_auto_black: "
	    "Black %f (black point %d)\n",
            image->cfg->curve[image->cfg->curveIndex].m_anchors[0].x, bp);
}
