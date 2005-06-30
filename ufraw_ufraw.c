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

ufraw_data *ufraw_open(char *filename)
{
    int status;
    ufraw_data *uf;
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
    uf = g_new0(ufraw_data, 1);
    g_strlcpy(uf->filename, filename, max_path);
    uf->image.image = NULL;
    uf->raw = raw;
    uf->colors = raw->colors;
    uf->use_coeff = raw->use_coeff;
    uf->developer = developer_init();
    uf->cfg = NULL;
    uf->widget = NULL;
    if (raw->fuji_width) {
        /* copied from dcraw's fuji_rotate() */
        uf->predictateWidth = raw->fuji_width / 0.5; /* sqrt(0.5); */
        uf->predictateHeight = (raw->height - raw->fuji_width) / 0.5; /* sqrt(0.5); */
    } else {
        uf->predictateHeight = raw->height;
        uf->predictateWidth = raw->width;
    }
    if (raw->flip & 4) {
        int tmp = uf->predictateHeight;
        uf->predictateHeight = uf->predictateWidth;
        uf->predictateWidth = tmp;
    }
    ufraw_message(UFRAW_SET_LOG, "ufraw_open: w:%d h:%d curvesize:%d\n",
        uf->predictateWidth, uf->predictateHeight, raw->toneCurveSize);
    return uf;
}

int ufraw_config(ufraw_data *uf, cfg_data *cfg)
{
    int i;
    dcraw_data *raw=NULL;
    char *inPath, *oldInPath, *oldOutPath;

    if (cfg->cfgSize!=cfg_default.cfgSize || cfg->version!=cfg_default.version)
        load_configuration(cfg);

    if (cfg->wbLoad==load_default) cfg->wb = camera_wb;
    if (cfg->wbLoad==load_auto) cfg->wb = auto_wb;
    if (cfg->curveLoad==load_default) {
        for (i=0; i<cfg->curveCount; i++)
	    CurveDataSetPoint(&cfg->curve[i], 0, 0, 0);
        cfg->saturation = cfg_default.saturation;
    }
    if (cfg->exposureLoad==load_default) {
        cfg->exposure = cfg_default.exposure;
    }
    if (cfg->exposureLoad==load_auto) {
	cfg->autoExposure = TRUE;
	cfg->autoBlack = TRUE;
    }
    if (cfg->autoExposure==TRUE) cfg->exposure = uf_nan();
    if (cfg->autoBlack==TRUE) cfg->black = uf_nan();

    if (uf==NULL) return UFRAW_SUCCESS;
    uf->cfg = cfg;

    /* Guess the prefered output path */
    if (strlen(uf->cfg->outputPath)==0) {
        inPath = g_path_get_dirname(uf->filename);
        oldInPath = g_path_get_dirname(uf->cfg->inputFilename);
        oldOutPath = g_path_get_dirname(uf->cfg->outputFilename);
        if ( !strcmp(oldInPath, oldOutPath) || !strcmp(oldOutPath,".") )
            g_strlcpy(uf->cfg->outputPath, inPath, max_path);
        else
            g_strlcpy(uf->cfg->outputPath, oldOutPath, max_path);
        g_free(inPath);
        g_free(oldInPath);
        g_free(oldOutPath);
        if (!strcmp(uf->cfg->outputPath,"."))
            strcpy(uf->cfg->outputPath,"");
    }
    char *absname = uf_file_set_absolute(uf->filename);
    g_strlcpy(cfg->inputFilename, absname, max_path);
    g_free(absname);

    raw = uf->raw;
    if (ufraw_exif_from_raw(raw->ifp, uf->filename, &uf->exifBuf,
            &uf->exifBufLen)!=UFRAW_SUCCESS) {
        ufraw_message(UFRAW_SET_LOG, "Error reading EXIF data from '%s'\n",
                uf->filename);
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
    uf->useMatrix = ( cfg->profile[0][cfg->profileIndex[0]].useMatrix
	    && uf->use_coeff ) || uf->colors==4;

    return UFRAW_SUCCESS;
}

int ufraw_load_raw(ufraw_data *uf)
{
    int status,  c;
    dcraw_data *raw = uf->raw;

    if ( (status=dcraw_load_raw(raw))!=DCRAW_SUCCESS ) {
        ufraw_message(status, raw->message);
        return status;
    }
    uf->rgbMax = raw->rgbMax;
    for (c=0; c<4; c++) uf->preMul[c] = raw->pre_mul[c];
    if (uf->colors<4) uf->preMul[3] = 0;
    memcpy(uf->coeff, raw->coeff, sizeof uf->coeff);
    return UFRAW_SUCCESS;
}

void ufraw_close(ufraw_data *uf)
{
    dcraw_close(uf->raw);
    g_free(uf->raw);
    g_free(uf->image.image);
    developer_destroy(uf->developer);
    ufraw_message(UFRAW_CLEAN, NULL);
}

/* Convert rawImage to standard rgb image */
int ufraw_convert_image(ufraw_data *uf)
{
    int c;
    dcraw_data *raw = uf->raw;
    cfg_data *cfg = uf->cfg;
    dcraw_image_data final;

    preview_progress(uf->widget, "Loading image", 0.1);
    if ( cfg->interpolation==half_interpolation ||
         ( cfg->size==0 && cfg->shrink>1 ) ||
         ( cfg->size>0 &&
	   cfg->size<MAX(raw->raw.height, raw->raw.width) )  ) {
        dcraw_finalize_shrink(&final, raw, MAX(cfg->shrink,2));
 
        uf->image.height = final.height;
        uf->image.width = final.width;
	g_free(uf->image.image);
        uf->image.image = final.image;
        ufraw_set_wb(uf);
        if (isnan(uf->cfg->exposure)) ufraw_auto_expose(uf);
        if (isnan(uf->cfg->black)) ufraw_auto_black(uf);
        developer_prepare(uf->developer, uf->rgbMax,
                pow(2,cfg->exposure), cfg->unclip,
                cfg->temperature, cfg->green,
		uf->preMul, uf->coeff, uf->colors, uf->useMatrix,
                &cfg->profile[0][cfg->profileIndex[0]],
                &cfg->profile[1][cfg->profileIndex[1]], cfg->intent,
                cfg->saturation,
                &cfg->curve[cfg->curveIndex]);
    } else {
        if ( isnan(uf->cfg->exposure) || isnan(uf->cfg->black) ) {
            int shrinkSave = uf->cfg->shrink;
            int sizeSave = uf->cfg->size;
            uf->cfg->shrink = 8;
            uf->cfg->size = 0;
            ufraw_convert_image(uf);
            uf->cfg->shrink = shrinkSave;
            uf->cfg->size = sizeSave;
        } else
            ufraw_set_wb(uf);
        developer_prepare(uf->developer, uf->rgbMax,
                pow(2,cfg->exposure), cfg->unclip,
                cfg->temperature, cfg->green,
		uf->preMul, uf->coeff, uf->colors, uf->useMatrix,
                &cfg->profile[0][cfg->profileIndex[0]],
                &cfg->profile[1][cfg->profileIndex[1]], cfg->intent,
                cfg->saturation,
                &cfg->curve[cfg->curveIndex]);
        dcraw_finalize_interpolate(&final, raw,
		cfg->interpolation==quick_interpolation,
                cfg->interpolation==four_color_interpolation,
		uf->developer->rgbWB);
	uf->developer->rgbMax = 0xFFFF;
        for (c=0; c<4; c++)
	    uf->developer->rgbWB[c] = uf->developer->rgbMax;
	g_free(uf->image.image);
        uf->image.image = final.image;
    }
    if (cfg->size>0) {
        if ( cfg->size>MAX(final.height, final.width) ) {
            ufraw_message(UFRAW_ERROR, "Can not downsize from %d to %d.",
                    MAX(final.height, final.width), cfg->size);
        } else {
            dcraw_image_resize(&final, cfg->size);
	}
    }
    preview_progress(uf->widget, "Loading image", 0.4);
    dcraw_flip_image(&final, raw->flip);
    uf->image.trim = final.trim;
    uf->image.height = final.height - 2 * final.trim;
    uf->image.width = final.width - 2 * final.trim;
    preview_progress(uf->widget, "Loading image", 0.5);
    return UFRAW_SUCCESS;
}

int ufraw_set_wb(ufraw_data *uf)
{
    dcraw_data *raw = uf->raw;
    double rgbWB[3];
    int status, c;

    if (uf->cfg->wb==manual_wb) return UFRAW_SUCCESS;
    if (uf->cfg->wb==auto_wb || uf->cfg->wb==camera_wb) {
        if ( (status=dcraw_set_color_scale(raw, uf->cfg->wb==auto_wb,
                uf->cfg->wb==camera_wb))!=DCRAW_SUCCESS ) {
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
                uf->cfg->wb = auto_wb;
                status=dcraw_set_color_scale(raw, TRUE, FALSE);
            }
            if (status!=DCRAW_SUCCESS)
		return status;
        }
        for (c=0; c<raw->colors; c++)
            rgbWB[c] = raw->pre_mul[c]/raw->post_mul[c];
    } else if (uf->cfg->wb<wb_preset_count) {
        rgbWB[0] = 1/wb_preset[uf->cfg->wb].red;
        rgbWB[1] = 1;
        rgbWB[2] = 1/wb_preset[uf->cfg->wb].blue;
    } else return UFRAW_ERROR;
    RGB_to_temperature(rgbWB, &uf->cfg->temperature, &uf->cfg->green);
    return UFRAW_SUCCESS;
}

void ufraw_auto_expose(ufraw_data *uf)
{
    int sum, stop, wp, i, c;
    int raw_histogram[0x10000];

    /* set cutoff at 1/256/4 of the histogram */
    stop = uf->image.width*uf->image.height*3/256/4;
    uf->cfg->exposure = 0;
    developer_prepare(uf->developer, uf->rgbMax,
            pow(2,uf->cfg->exposure), uf->cfg->unclip,
            uf->cfg->temperature, uf->cfg->green,
	    uf->preMul, uf->coeff, uf->colors, uf->useMatrix,
            &uf->cfg->profile[0][uf->cfg->profileIndex[0]],
            &uf->cfg->profile[1][uf->cfg->profileIndex[1]],
            uf->cfg->intent,
            uf->cfg->saturation,
            &uf->cfg->curve[uf->cfg->curveIndex]);

    /* First calculate the exposure */
    memset(raw_histogram, 0, sizeof(raw_histogram));
    for (i=0; i<uf->image.width*uf->image.height; i++)
        for (c=0; c<3; c++)
            raw_histogram[MIN((guint64)uf->image.image[i][c] *
                    uf->developer->rgbWB[c] / 0x10000, 0xFFFF)]++;
    for (wp=0xFFFF, sum=0; wp>0 && sum<stop; wp--)
        sum += raw_histogram[wp];
    uf->cfg->exposure = -log((double)wp/uf->developer->rgbMax)/log(2);
    if (!uf->cfg->unclip)
        uf->cfg->exposure = MAX(uf->cfg->exposure,0);
    ufraw_message(UFRAW_SET_LOG, "ufraw_auto_expose: "
	    "Exposure %f (white point %d)\n", uf->cfg->exposure, wp);
}

void ufraw_auto_black(ufraw_data *uf)
{
    int sum, stop, bp, i, j;
    int preview_histogram[0x100];
    guint8 *p8 = g_new(guint8, 3*uf->image.width);
    guint16 *pixtmp = g_new(guint16, 3*uf->image.width);

    stop = uf->image.width*uf->image.height*3/256/4;
    CurveDataSetPoint(&uf->cfg->curve[uf->cfg->curveIndex], 0, 0, 0);
    developer_prepare(uf->developer, uf->rgbMax,
            pow(2,uf->cfg->exposure), uf->cfg->unclip,
            uf->cfg->temperature, uf->cfg->green,
	    uf->preMul, uf->coeff, uf->colors, uf->useMatrix,
            &uf->cfg->profile[0][uf->cfg->profileIndex[0]],
            &uf->cfg->profile[1][uf->cfg->profileIndex[1]],
            uf->cfg->intent,
            uf->cfg->saturation,
            &uf->cfg->curve[uf->cfg->curveIndex]);
    memset(preview_histogram, 0, sizeof(preview_histogram));
    for (i=0; i<uf->image.height; i++) {
        develope(p8, uf->image.image[i*uf->image.width], uf->developer,
		8, pixtmp, uf->image.width);
        for (j=0; j<3*uf->image.width; j++) preview_histogram[p8[j]]++;
    }
    for (bp=0, sum=0; bp<0x100 && sum<stop; bp++)
        sum += preview_histogram[bp];
    /* Notice that the value of cfg->black is not important. It is only
     * relevant that it is not is_nan() indicating that it was calculated */
    uf->cfg->black = (double)bp/256;
    CurveDataSetPoint(&uf->cfg->curve[uf->cfg->curveIndex],
	    0, uf->cfg->black, 0);
    g_free(p8);
    g_free(pixtmp);
    ufraw_message(UFRAW_SET_LOG, "ufraw_auto_black: "
	    "Black %f (black point %d)\n",
            uf->cfg->curve[uf->cfg->curveIndex].m_anchors[0].x, bp);
}

/* ufraw_auto_curve sets the black-point and then distribute the (step-1)
 * parts of the histogram with the weights: w_i = pow(decay,i). */
void ufraw_auto_curve(ufraw_data *uf)
{
    int sum, stop, steps=8, bp, i, j;
    int preview_histogram[0x100];
    guint8 *p8 = g_new(guint8, 3*uf->image.width);
    guint16 *pixtmp = g_new(guint16, 3*uf->image.width);
    CurveData *curve = &uf->cfg->curve[uf->cfg->curveIndex];
    double decay = 0.9;
    double norm = (1-pow(decay,steps))/(1-decay);

    CurveDataReset(curve);
    developer_prepare(uf->developer, uf->rgbMax,
            pow(2,uf->cfg->exposure), uf->cfg->unclip,
            uf->cfg->temperature, uf->cfg->green,
	    uf->preMul, uf->coeff, uf->colors, uf->useMatrix,
            &uf->cfg->profile[0][uf->cfg->profileIndex[0]],
            &uf->cfg->profile[1][uf->cfg->profileIndex[1]],
            uf->cfg->intent,
            uf->cfg->saturation,
            curve);
    /* Collect histogram data */
    memset(preview_histogram, 0, sizeof(preview_histogram));
    for (i=0; i<uf->image.height; i++) {
        develope(p8, uf->image.image[i*uf->image.width], uf->developer,
		8, pixtmp, uf->image.width);
        for (j=0; j<3*uf->image.width; j++) preview_histogram[p8[j]]++;
    }
    /* Calculate curve points */
    stop = uf->image.width*uf->image.height*3/256/4;
    for (bp=0, i=0; i<steps && bp<0x100; i++) {
	for (bp=0, sum=0; bp<0x100 && sum<stop; bp++)
	    sum += preview_histogram[bp];
	curve->m_anchors[i].x = (double)bp/256;
	curve->m_anchors[i].y = (double)i/steps;
	stop += (uf->image.width*uf->image.height*3) * pow(decay,i) / norm;
    }
    if (bp==0x100) {
	curve->m_numAnchors = i;
    } else {
	curve->m_anchors[i].x = 1.0;
	curve->m_anchors[i].y = 1.0;
	curve->m_numAnchors = i+1;
    }
    g_free(p8);
    g_free(pixtmp);
}
