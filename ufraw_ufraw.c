/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw_ufraw.c - program interface to all the components
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> /* for fstat() */
#include <math.h>
#include <time.h>
#include <errno.h>
#include <glib.h>
#include <glib/gi18n.h>
#include "dcraw_api.h"
#include "nikon_curve.h"
#include "ufraw.h"

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
    char *fname, *hostname;

    fname = g_filename_from_uri(filename, &hostname, NULL);
    if (fname!=NULL) {
	if (hostname!=NULL) {
	    ufraw_message(UFRAW_SET_ERROR, _("Remote URI is not supported"));
	    g_free(hostname);
	    g_free(fname);
	    return NULL;
	}
	g_strlcpy(filename, fname, max_path);
	g_free(fname);
    }
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
    uf->thumb.buffer = NULL;
    uf->raw = raw;
    uf->colors = raw->colors;
    uf->raw_color = raw->raw_color;
    uf->developer = developer_init();
    uf->widget = NULL;
    uf->RawLumHistogram = NULL;
    if (raw->fuji_width) {
        /* Copied from DCRaw's fuji_rotate() */
        uf->predictedWidth = raw->fuji_width / raw->fuji_step;
        uf->predictedHeight = (raw->height - raw->fuji_width) / raw->fuji_step;
    } else {
        uf->predictedHeight = raw->height * raw->ymag;
        uf->predictedWidth = raw->width;
    }
    if (raw->flip & 4) {
        int tmp = uf->predictedHeight;
        uf->predictedHeight = uf->predictedWidth;
        uf->predictedWidth = tmp;
    }
    ufraw_message(UFRAW_SET_LOG, "ufraw_open: w:%d h:%d curvesize:%d\n",
        uf->predictedWidth, uf->predictedHeight, raw->toneCurveSize);
    return uf;
}

ufraw_data *ufraw_load_darkframe(char *darkframeFile)
{
    ufraw_data *uf;
    if (strlen(darkframeFile)>0) {
        uf = ufraw_open(darkframeFile);
        if (!uf){
            ufraw_message(UFRAW_ERROR,
		    _("darkframe error: %s is not a raw file\n"),
		    darkframeFile);
            return NULL;
        }
	uf->conf = g_new(conf_data, 1);
	*uf->conf = conf_default;
	/* disable all auto settings on darkframe */
	uf->conf->autoExposure = disabled_state;
	uf->conf->autoBlack = disabled_state;
        if (ufraw_load_raw(uf)==UFRAW_SUCCESS){
            ufraw_message(UFRAW_BATCH_MESSAGE, _("using darkframe '%s'\n"),
		    uf->filename);
            return uf;
        }
        else
            ufraw_message(UFRAW_ERROR, _("error loading darkframe '%s'\n"),
		    uf->filename);
    }
    return NULL;
}

int ufraw_config(ufraw_data *uf, conf_data *rc, conf_data *conf, conf_data *cmd)
{
    int status;
    gboolean loadingID = FALSE;

    if (strcmp(rc->wb, spot_wb)) rc->chanMul[0] = -1.0;
    if (rc->autoExposure==enabled_state) rc->autoExposure = apply_state;
    if (rc->autoBlack==enabled_state) rc->autoBlack = apply_state;

    /* Check if we are loading an ID file */
    if (uf!=NULL) {
	if (uf->conf!=NULL) {
	    loadingID = TRUE;
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
	if (strcmp(rc->wb, spot_wb)) rc->chanMul[0] = -1.0;
	if (rc->autoExposure==enabled_state) rc->autoExposure = apply_state;
	if (rc->autoBlack==enabled_state) rc->autoBlack = apply_state;
    }
    if (cmd!=NULL) {
	status = conf_set_cmd(rc, cmd);
	if (status!=UFRAW_SUCCESS) return status;
    }
    if (uf==NULL) return UFRAW_SUCCESS;

    dcraw_data *raw = uf->raw;
    char *absname = uf_file_set_absolute(uf->filename);
    g_strlcpy(uf->conf->inputFilename, absname, max_path);
    g_free(absname);
    if (!loadingID) {
	g_snprintf(uf->conf->inputURI, max_path, "file://%s",
		uf->conf->inputFilename);
	struct stat s;
	fstat(fileno(raw->ifp), &s);
	g_snprintf(uf->conf->inputModTime, max_name, "%d", (int)s.st_mtime);
    }
    if (ufraw_exif_from_raw(uf)!=UFRAW_SUCCESS) {
        ufraw_message(UFRAW_SET_LOG, "Error reading EXIF data from %s\n",
                uf->filename);
    }
    /* If we switched cameras, ignore channel multipliers and
     * change spot_wb to manual_wb */
    if ( !loadingID &&
	 ( strcmp(uf->conf->make, raw->make)!=0 ||
	   strcmp(uf->conf->model, raw->model)!=0 ) ) {
	uf->conf->chanMul[0] = -1.0;
	if (strcmp(uf->conf->wb, spot_wb)==0)
	    g_strlcpy(uf->conf->wb, manual_wb, max_name);
    }
    /* Set the EXIF data */
#ifdef __MINGW32__
    /* For MinG32 we don't have the thread safe ctime_r */
    g_strlcpy(uf->conf->timestamp, ctime(&raw->timestamp), max_name);
#elif defined(SOLARIS)
    ctime_r(&raw->timestamp, uf->conf->timestamp, sizeof(uf->conf->timestamp));
#else
    ctime_r(&raw->timestamp, uf->conf->timestamp);
#endif
    if (uf->conf->timestamp[strlen(uf->conf->timestamp)-1]=='\n')
	uf->conf->timestamp[strlen(uf->conf->timestamp)-1] = '\0';
    g_strlcpy(uf->conf->make, raw->make, max_name);
    g_strlcpy(uf->conf->model, raw->model, max_name);
    uf->conf->flip = raw->flip;
    if (uf->exifBuf==NULL) {
	g_strlcpy(uf->conf->exifSource, "DCRaw", max_name);
	uf->conf->iso_speed = raw->iso_speed;
	g_snprintf(uf->conf->isoText, max_name, "%d", (int)uf->conf->iso_speed);
	uf->conf->shutter = raw->shutter;
	if (uf->conf->shutter>0 && uf->conf->shutter<1)
	    g_snprintf(uf->conf->shutterText, max_name, "1/%0.1f s",
		    1/uf->conf->shutter);
	else
	    g_snprintf(uf->conf->shutterText, max_name, "%0.1f s",
		    uf->conf->shutter);
	uf->conf->aperture = raw->aperture;
	g_snprintf(uf->conf->apertureText, max_name, "F/%0.1f",
		uf->conf->aperture);
	uf->conf->focal_len = raw->focal_len;
	g_snprintf(uf->conf->focalLenText, max_name, "%d mm",
		(int)uf->conf->focal_len);
    }
    /* Always set useMatrix if colors=4
     * Always reset useMatrix if 'raw_color' */
    uf->useMatrix = ( uf->conf->profile[0][uf->conf->profileIndex[0]].useMatrix
	    && !uf->raw_color ) || uf->colors==4;

    /* If there is an embeded curve we "turn on" the custom/camera curve
     * mechanism */
    if (raw->toneCurveSize!=0) {
	CurveData nc;
	long pos = ftell(raw->ifp);
	if (RipNikonNEFCurve(raw->ifp, raw->toneCurveOffset, &nc, NULL)
		!=UFRAW_SUCCESS) {
	    ufraw_message(UFRAW_ERROR, _("Error reading NEF curve"));
	    return UFRAW_WARNING;
	}
	fseek(raw->ifp, pos, SEEK_SET);
	if (nc.m_numAnchors<2) nc = conf_default.BaseCurve[0];

	g_strlcpy(nc.name, uf->conf->BaseCurve[custom_curve].name, max_name);
	uf->conf->BaseCurve[custom_curve] = nc;

	int use_custom_curve = 0;
	if (raw->toneModeSize) {
	    // "AUTO    " "HIGH    " "CS      " "MID.L   " "MID.H   "NORMAL  " "LOW     "
	    long pos = ftell(raw->ifp);
	    char buf[9];
	    fseek(raw->ifp, raw->toneModeOffset, SEEK_SET);
	    // read it in.
	    fread(&buf, 9, 1, raw->ifp);
	    fseek(raw->ifp, pos, SEEK_SET);

	    if (!strncmp(buf, "CS      ", sizeof(buf)))  use_custom_curve=1;

	    // down the line, we need to translate the other values into
	    // tone curves!
	}

	if (use_custom_curve) {
	    uf->conf->BaseCurve[camera_curve] =
		uf->conf->BaseCurve[custom_curve];
	    g_strlcpy(uf->conf->BaseCurve[camera_curve].name,
		    conf_default.BaseCurve[camera_curve].name, max_name);
	} else {
	    uf->conf->BaseCurve[camera_curve] =
		conf_default.BaseCurve[camera_curve];
	}
    } else {
	/* If there is no embeded curve we "turn off" the custom/camera curve
	 * mechanism */
	uf->conf->BaseCurve[camera_curve].m_numAnchors = 0;
	uf->conf->BaseCurve[custom_curve].m_numAnchors = 0;
	if (uf->conf->BaseCurveIndex==custom_curve ||
		uf->conf->BaseCurveIndex==camera_curve)
	    uf->conf->BaseCurveIndex = linear_curve;
    }
    /* Using DarkframeFile from the configuration is disabled since we still
     * cannot open two raw files simultaniously */
    /*
    if (strcmp(uf->conf->darkframeFile, cmd->darkframeFile)!=0)
	uf->conf->darkframe = ufraw_load_darkframe(uf->conf->darkframeFile);
    */
    return UFRAW_SUCCESS;
}

void ufraw_substract_darkframe(ufraw_data *uf)
{
    dcraw_data *df = uf->conf->darkframe->raw;
    dcraw_data *org = uf->raw;
    int i, cl;

    if (org->raw.width!=df->raw.width &&
        org->raw.width!=df->raw.width &&
        org->raw.colors!=df->raw.colors){

        ufraw_message(UFRAW_SET_WARNING,
		_("Darkframe is incompatible with main image"));
        return;
    }

    for( i=0; i<org->raw.height*org->raw.width; i++ ) {
        for( cl=0; cl<org->raw.colors; cl++ ){
            org->raw.image[i][cl] =
		    org->raw.image[i][cl] >= df->raw.image[i][cl] ?
                    org->raw.image[i][cl] - df->raw.image[i][cl] :
                    0;
        }
    }
    org->black = org->black >= df->black ?
	    org->black - df->black :
	    0;
    ufraw_message(UFRAW_SET_LOG, "subtracted darkframe '%s'\n",
	    uf->conf->darkframeFile);
}

int ufraw_load_raw(ufraw_data *uf)
{
    int status;
    dcraw_data *raw = uf->raw;

    if (uf->conf->embeddedImage) {
	dcraw_image_data thumb;
	if ( (status=dcraw_load_thumb(raw, &thumb))!=DCRAW_SUCCESS ) {
	    ufraw_message(status, raw->message);
	    return status;
	}
	uf->thumb.height = thumb.height;
	uf->thumb.width = thumb.width;
	return ufraw_read_embedded(uf);
    }
    if ( (status=dcraw_load_raw(raw))!=DCRAW_SUCCESS ) {
        ufraw_message(status, raw->message);
        return status;
    }
    /* raw->black can change in ufraw_subtract_darkframe() so we must
     * calculate uf->rgbMax first */
    uf->rgbMax = raw->rgbMax - raw->black;
    if (uf->conf->darkframe!=NULL)
        ufraw_substract_darkframe(uf);
    memcpy(uf->rgb_cam, raw->rgb_cam, sizeof uf->rgb_cam);

    if (uf->conf->chanMul[0]<0) ufraw_set_wb(uf);
    ufraw_auto_expose(uf);
    ufraw_auto_black(uf);
    return UFRAW_SUCCESS;
}

void ufraw_close(ufraw_data *uf)
{
    dcraw_close(uf->raw);
    g_free(uf->raw);
    g_free(uf->exifBuf);
    g_free(uf->image.image);
    g_free(uf->thumb.buffer);
    developer_destroy(uf->developer);
    g_free(uf->RawLumHistogram);
    ufraw_message(UFRAW_CLEAN, NULL);
}

/* Convert rawImage to standard rgb image */
int ufraw_convert_image(ufraw_data *uf)
{
    int c;
    dcraw_data *raw = uf->raw;
    dcraw_image_data final;
    int shrink = 1;

//    preview_progress(uf->widget, "Loading image", 0.1);
    developer_prepare(uf->developer, uf->rgbMax,
	    pow(2,uf->conf->exposure), uf->conf->unclip,
	    uf->conf->chanMul, uf->rgb_cam, uf->colors, uf->useMatrix,
	    &uf->conf->profile[0][uf->conf->profileIndex[0]],
	    &uf->conf->profile[1][uf->conf->profileIndex[1]], uf->conf->intent,
	    uf->conf->saturation,
	    &uf->conf->BaseCurve[uf->conf->BaseCurveIndex],
	    &uf->conf->curve[uf->conf->curveIndex]);
    /* We can do a simple interpolation in the following cases:
     * We shrink by an integer value.
     * If there is a ymag (D1X) shrink must be at least 4.
     * Wanted size is smaller than raw size (size is after a raw->shrink).
     * There are no filters (Foveon). */
    if ( uf->conf->interpolation==half_interpolation ||
         ( uf->conf->size==0 && uf->conf->shrink/raw->ymag>1 ) ||
         ( uf->conf->size>0 &&
	   uf->conf->size<MAX(raw->raw.height, raw->raw.width) ) ||
	 ( raw->filters==0 )  ) {
	if (uf->conf->size==0 && uf->conf->shrink>=2 &&
		uf->conf->shrink%raw->ymag==0)
	    shrink = uf->conf->shrink / raw->ymag;
	else if (uf->conf->interpolation==half_interpolation)
	    shrink = 2;
	else if (raw->filters!=0)
	    shrink = 2;
        dcraw_finalize_shrink(&final, raw, shrink);

        uf->image.height = final.height;
        uf->image.width = final.width;
	g_free(uf->image.image);
        uf->image.image = final.image;
    } else {
        dcraw_finalize_interpolate(&final, raw, uf->conf->interpolation,
		uf->developer->rgbWB);
	uf->developer->rgbMax = uf->developer->max;
        for (c=0; c<4; c++)
	    uf->developer->rgbWB[c] = 0x10000;
	g_free(uf->image.image);
        uf->image.image = final.image;
    }
    if (raw->ymag==2) {
	dcraw_image_stretch(&final, raw->ymag);
	if (uf->conf->size==0 && uf->conf->shrink>1)
            dcraw_image_resize(&final,
		    shrink*MAX(final.height, final.width)/uf->conf->shrink);
    }
    if (uf->conf->size>0) {
        if ( uf->conf->size>MAX(final.height, final.width) ) {
            ufraw_message(UFRAW_ERROR, _("Can not downsize from %d to %d."),
                    MAX(final.height, final.width), uf->conf->size);
        } else {
            dcraw_image_resize(&final, uf->conf->size);
	}
    }
//    preview_progress(uf->widget, "Loading image", 0.4);
    uf->image.image = final.image;
    dcraw_flip_image(&final, raw->flip);
    uf->image.height = final.height;
    uf->image.width = final.width;
//    preview_progress(uf->widget, "Loading image", 0.5);
    return UFRAW_SUCCESS;
}

int ufraw_set_wb(ufraw_data *uf)
{
    dcraw_data *raw = uf->raw;
    double rgbWB[3];
    int status, c, cc, i;

    /* For manual_wb we calculate chanMul from the temperature/green. */
    /* For all other it is the other way around. */
    if (!strcmp(uf->conf->wb, manual_wb)) {
	Temperature_to_RGB(uf->conf->temperature, rgbWB);
	rgbWB[1] = rgbWB[1] / uf->conf->green;
	/* Suppose we shot a white card at some temperature:
	 * rgbWB[3] = rgb_cam[3][4] * preMul[4] * camWhite[4]
	 * Now we want to make it white (1,1,1), so we replace preMul
	 * with chanMul, which is defined as:
	 * chanMul[4][4] = cam_rgb[4][3] * (1/rgbWB[3][3]) * rgb_cam[3][4]
	 *		* preMul[4][4]
	 * We "upgraded" preMul, chanMul and rgbWB to diagonal matrices.
	 * This allows for the manipulation:
	 * (1/chanMul)[4][4] = (1/preMul)[4][4] * cam_rgb[4][3] * rgbWB[3][3]
	 *		* rgb_cam[3][4]
	 * We use the fact that rgb_cam[3][4] * (1,1,1,1) = (1,1,1) and get:
	 * (1/chanMul)[4] = (1/preMul)[4][4] * cam_rgb[4][3] * rgbWB[3]
	 */
	if (uf->raw_color) {
	    /* If there is no color matrix it is simple */
	    for (c=0; c<3; c++) {
		uf->conf->chanMul[c] = raw->pre_mul[c] / rgbWB[c];
	    }
	} else {
	    for (c=0; c<raw->colors; c++) {
		double chanMulInv = 0;
		for (cc=0; cc<3; cc++)
		    chanMulInv += 1/raw->pre_mul[c] * raw->cam_rgb[c][cc]
			    * rgbWB[cc];
		uf->conf->chanMul[c] = 1/chanMulInv;
	    }
	}
	/* Normalize chanMul[] so that MIN(chanMul[]) will be 1.0 */
	double min = uf->conf->chanMul[0];
	for (c=1; c<raw->colors; c++)
	   if (uf->conf->chanMul[c] < min) min = uf->conf->chanMul[c];
	if (min==0.0) min = 1.0; /* should never happen, just to be safe */
	for (c=0; c<raw->colors; c++) uf->conf->chanMul[c] /= min;
	if (raw->colors<4) uf->conf->chanMul[3] = 0.0;
	uf->conf->WBTuning = 0;
	return UFRAW_SUCCESS;
    }
    if (!strcmp(uf->conf->wb, spot_wb)) {
	/* do nothing */
	uf->conf->WBTuning = 0;
    } else if ( !strcmp(uf->conf->wb, auto_wb) ) {
	int p;
	/* Build a raw channel histogram */
	image_type *histogram;
	histogram = g_new0(image_type, uf->rgbMax+1);
	for (i=0; i<raw->raw.height*raw->raw.width; i++) {
	    gboolean countPixel = TRUE;
	    /* The -25 bound was copied from dcraw */
	    for (c=0; c<raw->raw.colors; c++)
		if (raw->raw.image[i][c] > uf->rgbMax+raw->black-25)
		    countPixel = FALSE;
	    if (countPixel) {
		for (c=0; c<raw->raw.colors; c++) {
		    p = MIN(MAX(raw->raw.image[i][c]-raw->black,0),uf->rgbMax);
		    histogram[p][c]++;
		}
	    }
	}
        gint64 sum;
        for (c=0; c<uf->colors; c++) {
            sum = 0;
            for (i=0; i<uf->rgbMax+1; i++)
                sum += (gint64)i*histogram[i][c];
            uf->conf->chanMul[c] = 1.0/sum;
        }
	g_free(histogram);
	uf->conf->WBTuning = 0;
    } else if ( !strcmp(uf->conf->wb, camera_wb) ) {
        if ( (status=dcraw_set_color_scale(raw,
		!strcmp(uf->conf->wb, auto_wb),
		!strcmp(uf->conf->wb, camera_wb)))!=DCRAW_SUCCESS ) {
            if (status==DCRAW_NO_CAMERA_WB) {
                ufraw_message(UFRAW_BATCH_MESSAGE,
                    _("Cannot use camera white balance, "
                    "reverting to auto white balance."));
                ufraw_message(UFRAW_INTERACTIVE_MESSAGE,
                    _("Cannot use camera white balance, "
                    "reverting to auto white balance."));
		g_strlcpy(uf->conf->wb, auto_wb, max_name);
		return ufraw_set_wb(uf);
            }
            if (status!=DCRAW_SUCCESS)
		return status;
        }
        for (c=0; c<raw->colors; c++)
            uf->conf->chanMul[c] = raw->post_mul[c];
	uf->conf->WBTuning = 0;
    } else {
	int lastTuning = -1;
	char model[max_name];
	if ( strcmp(uf->conf->make,"MINOLTA")==0 &&
		( strncmp(uf->conf->model, "ALPHA", 5)==0 ||
		  strncmp(uf->conf->model, "MAXXUM", 6)==0 ) ) {
	    /* Canonize Minolta model names (copied from dcraw) */
	    g_snprintf(model, max_name, "DYNAX %s",
	    uf->conf->model+6+(uf->conf->model[0]=='M'));
	} else {
	    g_strlcpy(model, uf->conf->model, max_name);
	}
	for (i=0; i<wb_preset_count; i++) {
	    if (!strcmp(uf->conf->wb, wb_preset[i].name) &&
		!strcmp(uf->conf->make, wb_preset[i].make) &&
		!strcmp(model, wb_preset[i].model) ) {
		if (uf->conf->WBTuning == wb_preset[i].tuning) {
		    for (c=0; c<raw->colors; c++)
			uf->conf->chanMul[c] = wb_preset[i].channel[c];
		    break;
		} else if (uf->conf->WBTuning < wb_preset[i].tuning) {
		    if (lastTuning == -1) {
			/* WBTuning was set to a value smaller than possible */
			uf->conf->WBTuning = wb_preset[i].tuning;
			for (c=0; c<raw->colors; c++)
			    uf->conf->chanMul[c] = wb_preset[i].channel[c];
			break;
		    } else {
			/* Extrapolate WB tuning values:
			 * f(x) = f(a) + (x-a)*(f(b)-f(a))/(b-a) */
			for (c=0; c<raw->colors; c++)
			    uf->conf->chanMul[c] = wb_preset[i].channel[c] +
				(uf->conf->WBTuning - wb_preset[i].tuning) *
				(wb_preset[lastTuning].channel[c] -
				 wb_preset[i].channel[c]) /
				(wb_preset[lastTuning].tuning -
				 wb_preset[i].tuning);
			break;
		    }
		} else if (uf->conf->WBTuning > wb_preset[i].tuning) {
			lastTuning = i;
		}
	    } else if (lastTuning!=-1) {
		/* WBTuning was set to a value larger than possible */
		uf->conf->WBTuning = wb_preset[lastTuning].tuning;
		for (c=0; c<raw->colors; c++)
		    uf->conf->chanMul[c] = wb_preset[lastTuning].channel[c];
		break;
	    }
	}
	if (i==wb_preset_count) {
	    if (lastTuning!=-1) {
		/* WBTuning was set to a value larger than possible */
		uf->conf->WBTuning = wb_preset[lastTuning].tuning;
		for (c=0; c<raw->colors; c++)
		    uf->conf->chanMul[c] = wb_preset[lastTuning].channel[c];
	    } else {
		g_strlcpy(uf->conf->wb, manual_wb, max_name);
		ufraw_set_wb(uf);
		return UFRAW_WARNING;
	    }
	}
    }
    /* Normalize chanMul[] so that MIN(chanMul[]) will be 1.0 */
    double min = uf->conf->chanMul[0];
    for (c=1; c<raw->colors; c++)
	if (uf->conf->chanMul[c] < min) min = uf->conf->chanMul[c];
    if (min==0.0) min = 1.0; /* should never happen, just to be safe */
    for (c=0; c<raw->colors; c++) uf->conf->chanMul[c] /= min;
    if (raw->colors<4) uf->conf->chanMul[3] = 0.0;

    /* (1/chanMul)[4] = (1/preMul)[4][4] * cam_rgb[4][3] * rgbWB[3]
     * Therefore:
     * rgbWB[3] = rgb_cam[3][4] * preMul[4][4] * (1/chanMul)[4]
     */
    if (uf->raw_color) {
	/* If there is no color matrix it is simple */
	for (c=0; c<3; c++) {
	    rgbWB[c] = raw->pre_mul[c] / uf->conf->chanMul[c];
	}
    } else {
	for (c=0; c<3; c++) {
	    rgbWB[c] = 0;
	    for (cc=0; cc<raw->colors; cc++)
		rgbWB[c] += raw->rgb_cam[c][cc] * raw->pre_mul[cc]
		    / uf->conf->chanMul[cc];
	}
    }
    /* From these values we calculate temperature, green values */
    RGB_to_Temperature(rgbWB, &uf->conf->temperature, &uf->conf->green);

    return UFRAW_SUCCESS;
}

void ufraw_build_raw_luminosity_histogram(ufraw_data *uf)
{
    int i, c;
    gint64 max;
    double maxChan = 0;
    dcraw_data *raw = uf->raw;
    gboolean updateHistogram = FALSE;

    if (uf->RawLumHistogram==NULL) {
	uf->RawLumHistogram = g_new(int, uf->rgbMax+1);
	updateHistogram = TRUE;
    }
    for (c=0; c<uf->colors; c++) maxChan = MAX(uf->conf->chanMul[c], maxChan);
    for (c=0; c<uf->colors; c++) {
	int tmp = floor(uf->conf->chanMul[c]/maxChan*0x10000);
	if (uf->RawLumChanMul[c]!=tmp) {
	    updateHistogram = TRUE;
	    uf->RawLumChanMul[c] = tmp;
	}
    }
    if (!updateHistogram) return;

    if (uf->colors==3) uf->RawLumChanMul[3] = uf->RawLumChanMul[1];
    memset(uf->RawLumHistogram, 0, (uf->rgbMax+1)*sizeof(int));
    uf->RawLumCount = raw->raw.height*raw->raw.width;
    for (i=0; i<uf->RawLumCount; i++) {
        for (c=0, max=0; c<raw->raw.colors; c++) {
            max = MAX( (gint64)(raw->raw.image[i][c]-raw->black) *
			uf->RawLumChanMul[c], max);
        }
        uf->RawLumHistogram[MIN(max/0x10000,uf->rgbMax)]++;
    }
}

void ufraw_auto_expose(ufraw_data *uf)
{
    int sum, stop, wp, c, pMax, pMin, p;
    image_type pix;
    guint16 p16[3], pixtmp[3];

    if (uf->conf->autoExposure!=apply_state) return;

    /* Reset the exposure and luminosityCurve */
    developer_prepare(uf->developer, uf->rgbMax,
	    pow(2,0) /*exposure*/, uf->conf->unclip,
	    uf->conf->chanMul, uf->rgb_cam, uf->colors, uf->useMatrix,
	    &uf->conf->profile[0][uf->conf->profileIndex[0]],
	    &uf->conf->profile[1][uf->conf->profileIndex[1]],
	    uf->conf->intent,
	    uf->conf->saturation,
	    &uf->conf->BaseCurve[uf->conf->BaseCurveIndex],
	    NULL /*luminosityCurve*/);
    /* Find the grey value that gives 99% luminosity */
    double maxChan = 0;
    for (c=0; c<uf->colors; c++) maxChan = MAX(uf->conf->chanMul[c], maxChan);
    for (pMax=uf->rgbMax, pMin=0, p=(pMax+pMin)/2; pMin<pMax-1; p=(pMax+pMin)/2)
    {
	for (c=0; c<uf->colors; c++)
	    pix[c] = MIN (p * maxChan/uf->conf->chanMul[c], uf->rgbMax);
	develope(p16, pix, uf->developer, 16, pixtmp, 1);
	for (c=0, wp=0; c<3; c++) wp = MAX(wp, p16[c]);
	if (wp < 0x10000 * 99/100) pMin = p;
	else pMax = p;
    }
    /* set cutoff at 99% of the histogram */
    ufraw_build_raw_luminosity_histogram(uf);
    stop = uf->RawLumCount * 1/100;
    /* Calculate the white point */
    for (wp=uf->rgbMax, sum=0; wp>1 && sum<stop; wp--)
                sum += uf->RawLumHistogram[wp];
    /* Set 99% of the luminosity values with luminosity below 99% */
    uf->conf->exposure = log((double)p/wp)/log(2);
    uf->conf->autoExposure = enabled_state;
//    ufraw_message(UFRAW_SET_LOG, "ufraw_auto_expose: "
//	    "Exposure %f (white point %d/%d)\n", uf->conf->exposure, wp, p);
}

void ufraw_auto_black(ufraw_data *uf)
{
    int sum, stop, bp, c;
    image_type pix;
    guint16 p16[3], pixtmp[3];

    if (uf->conf->autoBlack==disabled_state) return;

    /* Reset the luminosityCurve */
    developer_prepare(uf->developer, uf->rgbMax,
	    pow(2,uf->conf->exposure), uf->conf->unclip,
	    uf->conf->chanMul, uf->rgb_cam, uf->colors, uf->useMatrix,
	    &uf->conf->profile[0][uf->conf->profileIndex[0]],
	    &uf->conf->profile[1][uf->conf->profileIndex[1]],
	    uf->conf->intent,
	    uf->conf->saturation,
	    &uf->conf->BaseCurve[uf->conf->BaseCurveIndex],
	    NULL /*luminosityCurve*/);
    /* Calculate the black point */
    ufraw_build_raw_luminosity_histogram(uf);
    stop = uf->RawLumCount/256/4;
    for (bp=0, sum=0; bp<uf->rgbMax && sum<stop; bp++)
                sum += uf->RawLumHistogram[bp];
    double maxChan = 0;
    for (c=0; c<uf->colors; c++) maxChan = MAX(uf->conf->chanMul[c], maxChan);
    for (c=0; c<uf->colors; c++)
	pix[c] = MIN (bp * maxChan/uf->conf->chanMul[c], uf->rgbMax);
    develope(p16, pix, uf->developer, 16, pixtmp, 1);
    for (c=0, bp=0; c<3; c++) bp = MAX(bp, p16[c]);

    CurveDataSetPoint(&uf->conf->curve[uf->conf->curveIndex],
            0, (double)bp/0x10000, 0);

    uf->conf->autoBlack = enabled_state;
//    ufraw_message(UFRAW_SET_LOG, "ufraw_auto_black: "
//	    "Black %f (black point %d)\n",
//	    uf->conf->curve[uf->conf->curveIndex].m_anchors[0].x, bp);
}

/* ufraw_auto_curve sets the black-point and then distribute the (step-1)
 * parts of the histogram with the weights: w_i = pow(decay,i). */
void ufraw_auto_curve(ufraw_data *uf)
{
    int sum, stop, steps=8, bp, p, i, j, c;
    image_type pix;
    guint16 p16[3], pixtmp[3];
    CurveData *curve = &uf->conf->curve[uf->conf->curveIndex];
    double decay = 0.90;
    double norm = (1-pow(decay,steps))/(1-decay);

    CurveDataReset(curve);
    developer_prepare(uf->developer, uf->rgbMax,
	    pow(2,uf->conf->exposure), uf->conf->unclip,
	    uf->conf->chanMul, uf->rgb_cam, uf->colors, uf->useMatrix,
	    &uf->conf->profile[0][uf->conf->profileIndex[0]],
	    &uf->conf->profile[1][uf->conf->profileIndex[1]],
	    uf->conf->intent,
	    uf->conf->saturation,
	    &uf->conf->BaseCurve[uf->conf->BaseCurveIndex],
	    NULL /*luminosityCurve*/);
    /* Calculate curve points */
    ufraw_build_raw_luminosity_histogram(uf);
    stop = uf->RawLumCount/256/4;
    double maxChan = 0;
    for (c=0; c<uf->colors; c++) maxChan = MAX(uf->conf->chanMul[c], maxChan);
    for (bp=0, sum=0, p=0, i=j=0; i<steps && bp<uf->rgbMax && p<0xFFFF; i++) {
	for (; bp<uf->rgbMax && sum<stop; bp++)
            sum += uf->RawLumHistogram[bp];
	for (c=0; c<uf->colors; c++)
	    pix[c] = MIN (bp * maxChan/uf->conf->chanMul[c], uf->rgbMax);
	develope(p16, pix, uf->developer, 16, pixtmp, 1);
	for (c=0, p=0; c<3; c++) p = MAX(p, p16[c]);
	stop += uf->RawLumCount * pow(decay,i) / norm;
	/* Skeep adding point if slope is too big (more than 4) */
	if (j>0 && p - curve->m_anchors[j-1].x*0x10000 < (i+1-j)*0x04000/steps)
	    continue;
	curve->m_anchors[j].x = (double)p/0x10000;
	curve->m_anchors[j].y = (double)i/steps;
	j++;
    }
    if (bp==0x10000) {
	curve->m_numAnchors = j;
    } else {
	curve->m_anchors[j].x = 1.0;
	/* The last point can be up to twice the hight of a linear
	 * interpolation of the last two points */
	if (j>1) {
	    curve->m_anchors[j].y = curve->m_anchors[j-1].y +
		    2 * (1.0 - curve->m_anchors[j-1].x) *
		    (curve->m_anchors[j-1].y - curve->m_anchors[j-2].y) /
		    (curve->m_anchors[j-1].x - curve->m_anchors[j-2].x);
	    if (curve->m_anchors[j].y > 1.0) curve->m_anchors[j].y = 1.0;
	} else {
	    curve->m_anchors[j].y = 1.0;
	}
	curve->m_numAnchors = j+1;
    }
}
