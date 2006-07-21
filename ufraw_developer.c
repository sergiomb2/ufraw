/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw_developer.c - functions for developing images or more exactly pixels.
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
#include <math.h>
#include <lcms.h>
#include <glib.h>
#include "nikon_curve.h"
#include "ufraw.h"

developer_data *developer_init()
{
    int i;
    developer_data *d = g_new(developer_data,1);
    d->gamma = -1;
    d->linear = -1;
    d->saturation = -1;
    for (i=0; i<profile_types; i++) {
        d->profile[i] = NULL;
        strcpy(d->profileFile[i],"no such file");
    }
    memset(&d->baseCurveData, 0, sizeof(d->baseCurveData));
    d->baseCurveData.m_gamma = -1.0;
    memset(&d->luminosityCurveData, 0, sizeof(d->luminosityCurveData));
    d->luminosityCurveData.m_gamma = -1.0;
    d->intent = -1;
    d->updateTransform = TRUE;
    d->colorTransform = NULL;
    cmsSetErrorHandler((void *)ufraw_message);
    return d;
}

void developer_destroy(developer_data *d)
{
    int i;
    if (d==NULL) return;
    for (i=0; i<profile_types; i++)
        if (d->profile[i]!=NULL) cmsCloseProfile(d->profile[i]);
    if (d->colorTransform!=NULL)
        cmsDeleteTransform(d->colorTransform);
    g_free(d);
}

/* Update the profile in the developer
 * and init values in the profile if needed */
void developer_profile(developer_data *d, int type, profile_data *p)
{
    if (strcmp(p->file, d->profileFile[type])) {
        g_strlcpy(d->profileFile[type], p->file, max_path);
        if (d->profile[type]!=NULL) cmsCloseProfile(d->profile[type]);
        if (!strcmp(d->profileFile[type],""))
            d->profile[type] = cmsCreate_sRGBProfile();
        else {
            d->profile[type] =
                    cmsOpenProfileFromFile(d->profileFile[type], "r");
	    if (d->profile[type]==NULL)
                d->profile[type] = cmsCreate_sRGBProfile();
	}
        d->updateTransform = TRUE;
    }
    if (d->updateTransform) {
        if (d->profile[type]!=NULL)
            g_strlcpy(p->productName, cmsTakeProductName(d->profile[type]),
                        max_name);
        else
            strcpy(p->productName, "");
    }
}

void developer_prepare(developer_data *d, int rgbMax, double exposure,
	int unclip, 
	double chanMul[4], float rgb_cam[3][4], int colors, int useMatrix,
	profile_data *in, profile_data *out, int intent,
        double saturation, CurveData *baseCurve, CurveData *curve)
{
    unsigned c, i;

    d->rgbMax = rgbMax;
    d->colors = colors;
    d->useMatrix = useMatrix;
    d->unclip = unclip;
    if (exposure>=1.0) d->unclip = FALSE;
    d->exposure = exposure * 0x10000;

    double max = 0;
    for (c=0; c<d->colors; c++) max = MAX(max, chanMul[c]);
    d->max = 0x10000 / max;
    /* rgbWB is used in dcraw_finalized_interpolation() before the Bayer
     * Interpolation. It is normalized to guaranty that values do not
     * exceed 0xFFFF */
    for (c=0; c<d->colors; c++) d->rgbWB[c] = chanMul[c] * d->max
	    * 0xFFFF / d->rgbMax;
    /* rgbNorm is used in the unclipping algorithm */
    for (c=0; c<d->colors; c++) d->rgbNorm[c] = chanMul[c] * d->exposure;

    if (d->useMatrix)
	for (i=0; i<3; i++)
	    for (c=0; c<d->colors; c++)
		d->colorMatrix[i][c] = rgb_cam[i][c]*0x10000;
    /* Check if base curve data has changed. */
    if (in->gamma!=d->gamma || in->linear!=d->linear ||
	    memcmp(baseCurve, &d->baseCurveData, sizeof(CurveData)) ) {
        double a, b, c, g;
	CurveSample *cs = CurveSampleInit(0x10000, 0xFFFF);
        d->baseCurveData = *baseCurve;
        ufraw_message(UFRAW_RESET, NULL);
        if (CurveDataSample(baseCurve, cs)!=UFRAW_SUCCESS) {
            ufraw_message(UFRAW_REPORT, NULL);
            for (i=0; i<0x10000; i++) cs->m_Samples[i] = i;
	}
        d->gamma = in->gamma;
        d->linear = in->linear;
	/* The parameters of the linearized gamma curve are set in a way that
	 * keeps the curve continuous and smooth at the connecting point.
	 * d->linear also changes the real gamma used for the curve (g) in
	 * a way that keeps the derivative at i=0x10000 constant.
	 * This way changing the linearity changes the curve behaviour in
	 * the shadows, but has a minimal effect on the rest of the range. */ 
        if (d->linear<1.0) {
            g = d->gamma*(1.0-d->linear)/(1.0-d->gamma*d->linear);
            a = 1.0/(1.0+d->linear*(g-1));
            b = d->linear*(g-1)*a;
            c = pow(a*d->linear+b, g)/d->linear;
        } else {
            a = b = g = 0.0;
            c = 1.0;
        }
        for (i=0; i<0x10000 && cs->m_Samples[i]<0x10000*d->linear ; i++)
	    d->gammaCurve[i] = MIN(c*cs->m_Samples[i], 0xFFFF);
        for (; i<0x10000; i++) d->gammaCurve[i] =
            MIN(pow(a*cs->m_Samples[i]/0x10000+b, g)*0x10000, 0xFFFF);
        CurveSampleFree(cs);
    }
    developer_profile(d, 0, in);
    developer_profile(d, 1, out);
    if (intent!=d->intent) {
        d->intent = intent;
        d->updateTransform = TRUE;
    }
    if (d->updateTransform) {
        if (d->colorTransform!=NULL)
            cmsDeleteTransform(d->colorTransform);
        if (!strcmp(d->profileFile[0],"") && !strcmp(d->profileFile[1],""))
            d->colorTransform = NULL;
        else
            d->colorTransform = cmsCreateTransform(
                    d->profile[0], TYPE_RGB_16,
                    d->profile[1], TYPE_RGB_16, d->intent, 0);
    }
    if (saturation!=d->saturation) {
        d->saturation = saturation;
        for (i=0; i<0x10000; i++) d->saturationCurve[i] = MAX(MIN(
            pow((double)i/0x10000, saturation)*0x10000,0xFFFF),1);
    }
    if (curve==NULL) {
	/* Reset the luminosity curve and make sure it gets recalculated */
        for (i=0; i<0x10000; i++) d->luminosityCurve[i] = i;
	d->luminosityCurveData.m_gamma = -1.0;
    } else {
	/* Check if curve data has changed. */
	if (memcmp(curve, &d->luminosityCurveData, sizeof(CurveData))) {
	    d->luminosityCurveData = *curve;
	    CurveSample *cs = CurveSampleInit(0x10000, 0xFFFF);
	    ufraw_message(UFRAW_RESET, NULL);
	    if (CurveDataSample(curve, cs)!=UFRAW_SUCCESS) {
		ufraw_message(UFRAW_REPORT, NULL);
		for (i=0; i<0x10000; i++) d->luminosityCurve[i] = i;
	    } else {
		for (i=0; i<0x10000; i++)
		    d->luminosityCurve[i] = cs->m_Samples[i];
	    }
	    CurveSampleFree(cs);
	}
    }
}

inline void develope(void *po, guint16 pix[4], developer_data *d, int mode,
    guint16 *buf, int count)
{
    guint8 *p8 = po;
    guint16 *p16 = po, *p, maxc, midc, minc, c, cc;
    gint64 tmp, tmppix[4];
    guint min; // max;
    unsigned sat, hue;
    int i;
    gboolean clipped;

    for (i=0; i<count; i++) {
	clipped = FALSE;
//	max = 0;
	for (c=0; c<d->colors; c++) {
	    tmppix[c] = (guint64)pix[i*4+c] * d->rgbWB[c] / 0x10000;
	    if (tmppix[c] > d->max) {
		if (d->unclip) {
		    clipped = TRUE;
		} else {
		    tmppix[c] = d->max;
		}
	    }
	    tmppix[c] = tmppix[c] * d->exposure / d->max;
//	    if (tmppix[c] > max) max = tmppix[c];
	}
	if (clipped) {
//	    max = 0;
	    for (c=0; c<d->colors; c++) {
		if (tmppix[c] > d->exposure) {
		    /* This channel requires unclipping.
		     * If another channel is saturated, then this channel can
		     * not have a value larger than the saturated channel
		     * rgbWB[]. Therefore, if the clipped value is larger
		     * than rgbWB[] we do a linear interpolation to make sure
		     * that the clipping process is continuous. */
		    min = tmppix[c];
		    const int unclip_strength = 1;
		    for (cc=0; cc<d->colors; cc++) {
			if (tmppix[c] > d->rgbNorm[cc]) {
			    tmp = d->rgbNorm[cc] + (tmppix[c]-d->rgbNorm[cc]) *
				MIN(unclip_strength * (d->rgbMax-
					MIN(pix[i*4+cc],d->rgbMax)),d->rgbMax)
				/ d->rgbMax;
			    if (tmp<min) min = tmp;
			}
		    }
		    tmppix[c] = min;
		}
//		if (tmppix[c] > max) max = tmppix[c];
	    }
	}
//	if (max>0xFFFF) { /* We need to restore color */
//	    
//	    for (c=0; c<d->colors; c++) {
//		tmppix[c] = tmppix[c] * ( (max-0xFFFF)*tmppix[c]+(gint64)0xFFFF*0xFFFF )
//		    / max / 0xFFFF;
////		tmppix[c] = tmppix[c] * ( (max-0xFFFF)*tmppix[c]+(gint64)0xFFFF*d->rgbWB[c] )
////		    / max / d->rgbWB[c] * 0xFFFF / d->rgbWB[c];
//	    }
//	}
	if (d->useMatrix) {
	    for (cc=0; cc<3; cc++) {
		for (c=0, tmp=0; c<d->colors; c++)
		    tmp += tmppix[c] * d->colorMatrix[cc][c];
		tmp /= 0x10000;
		buf[i*3+cc] = d->gammaCurve[MIN(MAX(tmp, 0), 0xFFFF)];
	    }
	} else {
	    for (c=0; c<3; c++)
		buf[i*3+c] = d->gammaCurve[MIN(tmppix[c], 0xFFFF)];
	}
    }
    if (d->colorTransform!=NULL)
        cmsDoTransform(d->colorTransform, buf, buf, count);
//    if (d->saturation!=1)
    for (i=0, p=buf; i<count; i++, p+=3) {
        if (p[0] > p[1] && p[0] > p[2]) {
            maxc = 0;
            if (p[1] > p[2]) { midc = 1; minc = 2; }
            else { midc = 2; minc = 1; }
        } else if (p[1] > p[2]) {
            maxc = 1;
            if (p[0] > p[2]) { midc = 0; minc = 2; }
            else { midc = 2; minc = 0; }
        } else {
            maxc = 2;
            if (p[0] > p[1]) { midc = 0; minc = 1; }
            else { midc = 1; minc = 0; }
        }
        if (p[maxc]!=p[minc]) {
            /* oldSaturation = (max-min) / max */
            /* newSaturation = 1 - pow(1 - oldSaturation, saturate) */
            sat = 0x10000 - (unsigned)d->saturationCurve[p[minc]]*0x10000 /
                    d->saturationCurve[p[maxc]];
            hue = (unsigned)(p[midc]-p[minc])*0x10000/(p[maxc]-p[minc]);
	    p[maxc] = d->luminosityCurve[p[maxc]];
            p[minc] = p[maxc]*(0x10000-sat)/0x10000;
            p[midc] = p[maxc]*(0x10000-sat+sat*hue/0x10000)/0x10000;
            /* It is also possible to define oldSaturation = max-min */
        } else {
	    for (c=0; c<3; c++) p[c] = d->luminosityCurve[p[c]];
	}
    }
    if (mode==16) for (i=0; i<3*count; i++) p16[i] = buf[i];
    else for (i=0; i<3*count; i++) p8[i] = buf[i] >> 8;
}
