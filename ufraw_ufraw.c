/*
   UFRaw - Unidentified Flying Raw
   Raw photo loader plugin for The GIMP
   by Udi Fuchs,

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
#include "blackbody.h"

char *ufraw_message_buffer(char *buffer, char *message)
{
#ifdef UFRAW_DEBUG
    ufraw_batch_messenger(message);
#endif
    char *buf;
    if (buffer==NULL) return g_strdup(message);
    buf = g_strconcat(buffer, message, NULL);
    g_free(buffer);
    return buf;
}

void ufraw_batch_messenger(char *message)
{
    /* Print the 'ufraw:' header only if there are no newlines in the message
     * (not including possibly one at the end).
     * Otherwise, the header will be printed only for the first line. */
    if (g_strstr_len(message, strlen(message)-1, "\n")==NULL)
	fprintf(stderr, "%s: ", ufraw_binary);
    fprintf(stderr, "%s", message);
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
                    ufraw_messenger(message, parentWindow);
                g_free(message);
                return NULL;
    case UFRAW_INTERACTIVE_MESSAGE:
                if (parentWindow!=NULL)
		    ufraw_messenger(message, parentWindow);
                g_free(message);
                return NULL;
    case UFRAW_LCMS_WARNING:
    case UFRAW_LCMS_RECOVERABLE:
    case UFRAW_LCMS_ABORTED:
                ufraw_messenger(message, parentWindow);
                g_free(message);
                return (char *)1; /* Tell lcms that we are handling the error */
    case UFRAW_REPORT:
                ufraw_messenger(errorBuffer, parentWindow);
                return NULL;
    default:
                ufraw_messenger(message, parentWindow);
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
    conf_data *conf = NULL;

    /* First handle ufraw ID files */
    if (!strcasecmp(filename + strlen(filename) - 6, ".ufraw")) {
	conf = g_new(conf_data, 1);
	status = conf_load(conf, filename);
	if (status!=UFRAW_SUCCESS) {
	    g_free(conf);
	    return NULL;
	}
	if (conf->createID==only_id) conf->createID = also_id;
	filename = conf->inputFilename;
    }
    raw = g_new(dcraw_data, 1);
    status = dcraw_open(raw, filename);
    if ( status!=DCRAW_SUCCESS) {
        /* Hold the message without displaying it */
        ufraw_message(UFRAW_SET_WARNING, raw->message);
        g_free(raw);
        return NULL;
    }
    uf = g_new0(ufraw_data, 1);
    uf->conf = conf;
    g_strlcpy(uf->filename, filename, max_path);
    uf->image.image = NULL;
    uf->raw = raw;
    uf->colors = raw->colors;
    uf->use_coeff = raw->use_coeff;
    uf->developer = developer_init();
    uf->widget = NULL;
    if (raw->fuji_width) {
        /* copied from dcraw's fuji_rotate() */
        uf->predictateWidth = raw->fuji_width / raw->fuji_step;
        uf->predictateHeight = (raw->height - raw->fuji_width) / raw->fuji_step;
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

int ufraw_config(ufraw_data *uf, conf_data *rc, conf_data *conf, conf_data *cmd)
{
    dcraw_data *raw=NULL;
    int status;

    if (rc->wb!=spot_wb) rc->chanMul[0] = -1.0;
    if (rc->autoExposure==enabled_state) rc->autoExposure = apply_state;
    if (rc->autoBlack==enabled_state) rc->autoBlack = apply_state;

    /* Check if we are loading an ID file */
    if (uf!=NULL) {
	if (uf->conf!=NULL) {
	    conf_data tmp = *rc;
	    conf_copy_image(&tmp, uf->conf);
	    conf_copy_save(&tmp, uf->conf);
	    *uf->conf = tmp;
	} else {
	    uf->conf = g_new(conf_data, 1);
	    *uf->conf = *rc;
	}
	rc = uf->conf;
    }
    if (conf!=NULL && conf->version!=0) {
	conf_copy_image(rc, conf);
	conf_copy_save(rc, conf);
	if (rc->wb!=spot_wb) rc->chanMul[0] = -1.0;
	if (rc->autoExposure==enabled_state) rc->autoExposure = apply_state;
	if (rc->autoBlack==enabled_state) rc->autoBlack = apply_state;
    }
    if (cmd!=NULL) {
	status = conf_set_cmd(rc, cmd);
	if (status!=UFRAW_SUCCESS) return status;
    }
    if (uf==NULL) return UFRAW_SUCCESS;

    char *absname = uf_file_set_absolute(uf->filename);
    g_strlcpy(uf->conf->inputFilename, absname, max_path);
    g_free(absname);

    raw = uf->raw;
    if (ufraw_exif_from_raw(raw->ifp, uf->filename, &uf->exifBuf,
            &uf->exifBufLen)!=UFRAW_SUCCESS) {
        ufraw_message(UFRAW_SET_LOG, "Error reading EXIF data from %s\n",
                uf->filename);
    }
    /* Always set useMatrix if colors=4
     * Always reset useMatrix if !use_coeff */
    uf->useMatrix = ( uf->conf->profile[0][uf->conf->profileIndex[0]].useMatrix
	    && uf->use_coeff ) || uf->colors==4;

    if (raw->toneCurveSize!=0) {
        CurveData nc;
        long pos = ftell(raw->ifp);
        if (RipNikonNEFCurve(raw->ifp, raw->toneCurveOffset, &nc, NULL)
                !=UFRAW_SUCCESS) {
            ufraw_message(UFRAW_ERROR, "Error reading NEF curve");
            return UFRAW_WARNING;
        }
        fseek(raw->ifp, pos, SEEK_SET);
	if (nc.m_numAnchors<2) nc = conf_default.curve[0];
	g_strlcpy(nc.name, uf->conf->curve[camera_curve].name, max_name);
        uf->conf->curve[camera_curve] = nc;
    } else {
        uf->conf->curve[camera_curve].m_numAnchors = -1;
        /* don't retain camera_curve if no cameraCurve */
        if (uf->conf->curveIndex==camera_curve)
            uf->conf->curveIndex = linear_curve;
    }
    return UFRAW_SUCCESS;
}

int ufraw_load_raw(ufraw_data *uf)
{
    int status;
    dcraw_data *raw = uf->raw;

    if ( (status=dcraw_load_raw(raw))!=DCRAW_SUCCESS ) {
        ufraw_message(status, raw->message);
        return status;
    }
    uf->rgbMax = raw->rgbMax;
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
    conf_data *conf = uf->conf;
    dcraw_image_data final;

    preview_progress(uf->widget, "Loading image", 0.1);
    if ( conf->interpolation==half_interpolation ||
         ( conf->size==0 && conf->shrink>1 ) ||
         ( conf->size>0 &&
	   conf->size<MAX(raw->raw.height, raw->raw.width) )  ) {
        dcraw_finalize_shrink(&final, raw, MAX(conf->shrink,2));
 
        uf->image.height = final.height;
        uf->image.width = final.width;
	g_free(uf->image.image);
        uf->image.image = final.image;
        if (uf->conf->chanMul[0]<0) ufraw_set_wb(uf);
        if (uf->conf->autoExposure==apply_state) ufraw_auto_expose(uf);
        if (uf->conf->autoBlack==apply_state) ufraw_auto_black(uf);
        developer_prepare(uf->developer, uf->rgbMax,
                pow(2,conf->exposure), conf->unclip,
		uf->conf->chanMul, uf->coeff, uf->colors, uf->useMatrix,
                &conf->profile[0][conf->profileIndex[0]],
                &conf->profile[1][conf->profileIndex[1]], conf->intent,
                conf->saturation,
                &conf->curve[conf->curveIndex]);
    } else {
        if ( uf->conf->autoExposure==apply_state ||
	     uf->conf->autoBlack==apply_state ) {
            int shrinkSave = uf->conf->shrink;
            int sizeSave = uf->conf->size;
            uf->conf->shrink = 8;
            uf->conf->size = 0;
            ufraw_convert_image(uf);
            uf->conf->shrink = shrinkSave;
            uf->conf->size = sizeSave;
        } else {
	    if (uf->conf->chanMul[0]<0) ufraw_set_wb(uf);
	}
        developer_prepare(uf->developer, uf->rgbMax,
                pow(2,conf->exposure), conf->unclip,
		conf->chanMul, uf->coeff, uf->colors, uf->useMatrix,
                &conf->profile[0][conf->profileIndex[0]],
                &conf->profile[1][conf->profileIndex[1]], conf->intent,
                conf->saturation,
                &conf->curve[conf->curveIndex]);
        dcraw_finalize_interpolate(&final, raw,
		conf->interpolation==quick_interpolation,
                conf->interpolation==four_color_interpolation,
		uf->developer->rgbWB, uf->developer->max);
	uf->developer->rgbMax = 0xFFFF;
        for (c=0; c<4; c++)
	    uf->developer->rgbWB[c] = uf->developer->rgbMax;
	g_free(uf->image.image);
        uf->image.image = final.image;
    }
    if (conf->size>0) {
        if ( conf->size>MAX(final.height, final.width) ) {
            ufraw_message(UFRAW_ERROR, "Can not downsize from %d to %d.",
                    MAX(final.height, final.width), conf->size);
        } else {
            dcraw_image_resize(&final, conf->size);
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
    double rgbWB[3], rbRatio;
    int status, c, l, r, m;

    /* For manual_wb we calculate chanMul from the temperature/green. */
    /* For all other it is the other way around. */
    if (uf->conf->wb==manual_wb) {
	int i = uf->conf->temperature/10-200;
	for (c=0; c<raw->colors; c++)
	    uf->conf->chanMul[c] = bbWB[i][1] / bbWB[i][c==3?1:c] *
		    raw->pre_mul[c] * (c==1||c==3?uf->conf->green:1.0);
	/* Normalize chanMul[] so that MIN(chanMul[]) will be 1.0 */
	double min = uf->conf->chanMul[0];
	for (c=1; c<raw->colors; c++)
	   if (uf->conf->chanMul[c] < min) min = uf->conf->chanMul[c];
	if (min==0.0) min = 1.0; /* should never happen, just to be safe */
	for (c=0; c<raw->colors; c++) uf->conf->chanMul[c] /= min;
	if (raw->colors<4) uf->conf->chanMul[3] = 0.0;
	return UFRAW_SUCCESS;
    }
    if (uf->conf->wb==spot_wb) {
	/* do nothing */
    } else if (uf->conf->wb==auto_wb || uf->conf->wb==camera_wb) {
        if ( (status=dcraw_set_color_scale(raw, uf->conf->wb==auto_wb,
                uf->conf->wb==camera_wb))!=DCRAW_SUCCESS ) {
            if (status==DCRAW_NO_CAMERA_WB) {
                ufraw_message(UFRAW_BATCH_MESSAGE,
                    "Cannot use camera white balance, "
                    "reverting to auto white balance.");
                ufraw_message(UFRAW_INTERACTIVE_MESSAGE,
                    "Cannot use camera white balance, "
                    "reverting to auto white balance.");
                uf->conf->wb = auto_wb;
                status=dcraw_set_color_scale(raw, TRUE, FALSE);
            }
            if (status!=DCRAW_SUCCESS)
		return status;
        }
        for (c=0; c<raw->colors; c++)
            uf->conf->chanMul[c] = raw->post_mul[c];
    } else if (uf->conf->wb<wb_preset_count) {
        for (c=0; c<raw->colors; c++)
            uf->conf->chanMul[c] = raw->pre_mul[c];
        uf->conf->chanMul[0] *= wb_preset[uf->conf->wb].red;
        uf->conf->chanMul[2] *= wb_preset[uf->conf->wb].blue;
    } else return UFRAW_ERROR;
    /* Normalize chanMul[] so that MIN(chanMul[]) will be 1.0 */
    double min = uf->conf->chanMul[0];
    for (c=1; c<raw->colors; c++)
	if (uf->conf->chanMul[c] < min) min = uf->conf->chanMul[c];
    if (min==0.0) min = 1.0; /* should never happen, just to be safe */
    for (c=0; c<raw->colors; c++) uf->conf->chanMul[c] /= min;
    if (raw->colors<4) uf->conf->chanMul[3] = 0.0;

    /* rgbWB holds the normalized channel values */
    for (c=0; c<raw->colors; c++)
        rgbWB[c] = raw->pre_mul[c]/uf->conf->chanMul[c];

    /* From these values we calculate temperature, green values */
    rbRatio = rgbWB[0]/rgbWB[2];
    for (l=0, r=sizeof(bbWB)/(sizeof(float)*3), m=(l+r)/2; r-l>1 ; m=(l+r)/2) {
	if (bbWB[m][0]/bbWB[m][2] > rbRatio) l = m;
	else r = m;
    }
    uf->conf->temperature = m*10+2000;
    uf->conf->green = (bbWB[m][1]/bbWB[m][0])/(rgbWB[1]/rgbWB[0]);

    return UFRAW_SUCCESS;
}

void ufraw_auto_expose(ufraw_data *uf)
{
    int sum, stop, wp, i, c;
    int raw_histogram[0x10000];

    /* set cutoff at 1/256/4 of the histogram */
    stop = uf->image.width*uf->image.height*3/256/4;
    uf->conf->exposure = 0;
    developer_prepare(uf->developer, uf->rgbMax,
            pow(2,uf->conf->exposure), uf->conf->unclip,
	    uf->conf->chanMul, uf->coeff, uf->colors, uf->useMatrix,
            &uf->conf->profile[0][uf->conf->profileIndex[0]],
            &uf->conf->profile[1][uf->conf->profileIndex[1]],
            uf->conf->intent,
            uf->conf->saturation,
            &uf->conf->curve[uf->conf->curveIndex]);

    /* First calculate the exposure */
    memset(raw_histogram, 0, sizeof(raw_histogram));
    for (i=0; i<uf->image.width*uf->image.height; i++)
        for (c=0; c<3; c++)
            raw_histogram[MIN((guint64)uf->image.image[i][c] *
                    uf->developer->rgbWB[c] / 0x10000, 0xFFFF)]++;
    for (wp=0xFFFF, sum=0; wp>0 && sum<stop; wp--)
        sum += raw_histogram[wp];
    uf->conf->exposure = -log((double)wp/uf->developer->rgbMax)/log(2);
    uf->conf->autoExposure = enabled_state;
    if (!uf->conf->unclip)
        uf->conf->exposure = MAX(uf->conf->exposure,0);
    ufraw_message(UFRAW_SET_LOG, "ufraw_auto_expose: "
	    "Exposure %f (white point %d)\n", uf->conf->exposure, wp);
}

void ufraw_auto_black(ufraw_data *uf)
{
    int sum, stop, bp, i, j;
    int preview_histogram[0x100];
    guint8 *p8 = g_new(guint8, 3*uf->image.width);
    guint16 *pixtmp = g_new(guint16, 3*uf->image.width);

    stop = uf->image.width*uf->image.height*3/256/4;
    CurveDataSetPoint(&uf->conf->curve[uf->conf->curveIndex], 0, 0, 0);
    developer_prepare(uf->developer, uf->rgbMax,
            pow(2,uf->conf->exposure), uf->conf->unclip,
	    uf->conf->chanMul, uf->coeff, uf->colors, uf->useMatrix,
            &uf->conf->profile[0][uf->conf->profileIndex[0]],
            &uf->conf->profile[1][uf->conf->profileIndex[1]],
            uf->conf->intent,
            uf->conf->saturation,
            &uf->conf->curve[uf->conf->curveIndex]);
    memset(preview_histogram, 0, sizeof(preview_histogram));
    for (i=0; i<uf->image.height; i++) {
        develope(p8, uf->image.image[i*uf->image.width], uf->developer,
		8, pixtmp, uf->image.width);
        for (j=0; j<3*uf->image.width; j++) preview_histogram[p8[j]]++;
    }
    for (bp=0, sum=0; bp<0x100 && sum<stop; bp++)
        sum += preview_histogram[bp];
    CurveDataSetPoint(&uf->conf->curve[uf->conf->curveIndex],
	    0, (double)bp/256, 0);
    uf->conf->autoBlack = enabled_state;
    g_free(p8);
    g_free(pixtmp);
    ufraw_message(UFRAW_SET_LOG, "ufraw_auto_black: "
	    "Black %f (black point %d)\n",
            uf->conf->curve[uf->conf->curveIndex].m_anchors[0].x, bp);
}

/* ufraw_auto_curve sets the black-point and then distribute the (step-1)
 * parts of the histogram with the weights: w_i = pow(decay,i). */
void ufraw_auto_curve(ufraw_data *uf)
{
    int sum, stop, steps=8, bp, i, j;
    int preview_histogram[0x100];
    guint8 *p8 = g_new(guint8, 3*uf->image.width);
    guint16 *pixtmp = g_new(guint16, 3*uf->image.width);
    CurveData *curve = &uf->conf->curve[uf->conf->curveIndex];
    double decay = 0.9;
    double norm = (1-pow(decay,steps))/(1-decay);

    CurveDataReset(curve);
    developer_prepare(uf->developer, uf->rgbMax,
            pow(2,uf->conf->exposure), uf->conf->unclip,
	    uf->conf->chanMul, uf->coeff, uf->colors, uf->useMatrix,
            &uf->conf->profile[0][uf->conf->profileIndex[0]],
            &uf->conf->profile[1][uf->conf->profileIndex[1]],
            uf->conf->intent,
            uf->conf->saturation,
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
