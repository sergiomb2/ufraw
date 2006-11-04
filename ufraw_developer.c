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
    d->luminosityProfile = NULL;
    LPGAMMATABLE *TransferFunction = (LPGAMMATABLE *)d->TransferFunction;
    TransferFunction[0] = cmsAllocGamma(0x100);
    TransferFunction[1] = TransferFunction[2] = cmsBuildGamma(0x100, 1.0);
    d->saturationProfile = NULL;
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
    cmsCloseProfile(d->luminosityProfile);
    cmsFreeGamma(d->TransferFunction[0]);
    cmsFreeGamma(d->TransferFunction[1]);
    cmsCloseProfile(d->saturationProfile);
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

int saturation_sampler(register WORD In[], register WORD Out[], register LPVOID Cargo){
    cmsCIELab LabIn, LabOut;
    cmsCIELCh LChIn, LChOut;
    double saturation = *(double *)Cargo;
    cmsLabEncoded2Float(&LabIn, In);
    cmsLab2LCh(&LChIn, &LabIn);
    // Do some adjusts on LCh
    LChOut.L = LChIn.L;
    LChOut.C = (1-pow(1-(LChIn.C/256.), saturation))*256;
    LChOut.h = LChIn.h;

    cmsLCh2Lab(&LabOut, &LChOut);
    // Back to encoded
    cmsFloat2LabEncoded(Out, &LabOut);

    return TRUE;
}

/* Based on lcms' cmsCreateBCHSWabstractProfile() */
cmsHPROFILE create_saturation_profile(double saturation)
{
    cmsHPROFILE hICC;
    LPLUT Lut;

    hICC = _cmsCreateProfilePlaceholder();
    if (hICC==NULL) return NULL;// can't allocate

    cmsSetDeviceClass(hICC, icSigAbstractClass);
    cmsSetColorSpace(hICC, icSigLabData);
    cmsSetPCS(hICC, icSigLabData);
    cmsSetRenderingIntent(hICC, INTENT_PERCEPTUAL);

    // Creates a LUT with 3D grid only
    Lut = cmsAllocLUT();
    cmsAlloc3DGrid(Lut, 7, 3, 3);
    if (!cmsSample3DGrid(Lut, saturation_sampler, &saturation , 0)) {
        // Shouldn't reach here
        cmsFreeLUT(Lut);
        cmsCloseProfile(hICC);
        return NULL;
    }
    // Create tags
    cmsAddTag(hICC, icSigMediaWhitePointTag, (LPVOID) cmsD50_XYZ());
    cmsAddTag(hICC, icSigAToB0Tag, (LPVOID) Lut);
    // LUT is already on virtual profile
    cmsFreeLUT(Lut);
    return hICC;
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

    if (d->useMatrix)
	for (i=0; i<3; i++)
	    for (c=0; c<d->colors; c++)
		d->colorMatrix[i][c] = rgb_cam[i][c]*0x10000;
    /* Check if base curve data has changed. */
    if (in->gamma!=d->gamma || in->linear!=d->linear ||
	    memcmp(baseCurve, &d->baseCurveData, sizeof(CurveData)) ) {
        double a, b, c, g;
	CurveSample *cs = CurveSampleInit(0x10000, 0x10000);
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
    if (curve==NULL) {
	/* Reset the luminosity profile and make sure it gets recalculated */
	cmsCloseProfile(d->luminosityProfile);
	d->luminosityProfile = NULL;
	d->luminosityCurveData.m_gamma = -1.0;
        d->updateTransform = TRUE;
    } else {
	/* Check if curve data has changed. */
	if (memcmp(curve, &d->luminosityCurveData, sizeof(CurveData))) {
	    d->luminosityCurveData = *curve;
	    /* Trivial curve does not require a profile */
	    if (curve->m_anchors[0].x==0 && curve->m_anchors[0].y==0 &&
		curve->m_anchors[1].x==1 && curve->m_anchors[1].y==1 ) {
		d->luminosityProfile = NULL;
	    } else {
		cmsCloseProfile(d->luminosityProfile);
		CurveSample *cs = CurveSampleInit(0x100, 0x10000);
		ufraw_message(UFRAW_RESET, NULL);
		if (CurveDataSample(curve, cs)!=UFRAW_SUCCESS) {
		    ufraw_message(UFRAW_REPORT, NULL);
		    d->luminosityProfile = NULL;
		} else {
		    LPGAMMATABLE *TransferFunction =
			(LPGAMMATABLE *)d->TransferFunction;
		    for (i=0; i<0x100; i++)
			TransferFunction[0]->GammaTable[i] = cs->m_Samples[i];
		    d->luminosityProfile = cmsCreateLinearizationDeviceLink(
			    icSigLabData, TransferFunction);
		    cmsSetDeviceClass(d->luminosityProfile, icSigAbstractClass);
		}
		CurveSampleFree(cs);
	    }
	    d->updateTransform = TRUE;
	}
    }
    if (saturation!=d->saturation) {
        d->saturation = saturation;
	cmsCloseProfile(d->saturationProfile);
	if (saturation==1)
	    d->saturationProfile = NULL;
	else
	    d->saturationProfile = create_saturation_profile(d->saturation);
        d->updateTransform = TRUE;
    }
    if (d->updateTransform) {
        if (d->colorTransform!=NULL)
            cmsDeleteTransform(d->colorTransform);
        if (!strcmp(d->profileFile[0],"") && !strcmp(d->profileFile[1],"") &&
	    d->luminosityProfile==NULL && d->saturationProfile==NULL) {
            d->colorTransform = NULL;
#if defined(LCMS_VERSION) && LCMS_VERSION <= 113 /* Bypass a lcms 1.13 bug. */
        } else if (d->luminosityProfile==NULL && d->saturationProfile==NULL) {
	    d->colorTransform = cmsCreateTransform(d->profile[0],
		    TYPE_RGB_16, d->profile[1], TYPE_RGB_16, d->intent, 0);
#endif
        } else {
	    cmsHPROFILE prof[4];
	    i = 0;
	    prof[i++] = d->profile[0];
	    if (d->luminosityProfile!=NULL)
		prof[i++] = d->luminosityProfile;
	    if (d->saturationProfile!=NULL)
		prof[i++] = d->saturationProfile;
	    prof[i++] = d->profile[1];
	    d->colorTransform = cmsCreateMultiprofileTransform(prof, i,
		    TYPE_RGB_16, TYPE_RGB_16, d->intent, 0);
	}
    }
}

extern const double xyz_rgb[3][3];
//const double xyz_rgb[3][3] = {                  /* XYZ from RGB */
//  { 0.412453, 0.357580, 0.180423 },
//  { 0.212671, 0.715160, 0.072169 },
//  { 0.019334, 0.119193, 0.950227 } };
const double rgb_xyz[3][3] = {			/* RGB from XYZ */
    { 3.24048, -1.53715, -0.498536 },
    { -0.969255, 1.87599, 0.0415559 },
    { 0.0556466, -0.204041, 1.05731 } };

void rgb_to_cielch(gint64 rgb[3], float lch[3])
{
    int c, cc, i;
    float r, xyz[3], lab[3];
    static gboolean firstRun = TRUE;
    static float cbrt[0x10000];

    if (firstRun) {
	for (i=0; i < 0x10000; i++) {
	    r = i / 65535.0;
	cbrt[i] = r > 0.008856 ? pow(r,1/3.0) : 7.787*r + 16/116.0;
	}
	firstRun = FALSE;
    } else {
	xyz[0] = xyz[1] = xyz[2] = 0.5;
	for (c=0; c<3; c++)
	    for (cc=0; cc<3; cc++)
		xyz[cc] += xyz_rgb[cc][c] * rgb[c];
	for (c=0; c<3; c++)
	    xyz[c] = cbrt[MAX(MIN((int)xyz[c], 0xFFFF), 0)];
	lab[0] = 116 * xyz[1] - 16;
	lab[1] = 500 * (xyz[0] - xyz[1]);
	lab[2] = 200 * (xyz[1] - xyz[2]);

	lch[0] = lab[0];
	lch[1] = sqrt(lab[1]*lab[1]+lab[2]*lab[2]);
	lch[2] = atan2(lab[2], lab[1]);
    }
}

void cielch_to_rgb(float lch[3], gint64 rgb[3])
{
    int c, cc;
    float xyz[3], fx, fy, fz, xr, yr, zr, kappa, epsilon, tmpf, lab[3];
    epsilon = 0.008856; kappa = 903.3;
    lab[0] = lch[0];
    lab[1] = lch[1] * cos(lch[2]);
    lab[2] = lch[1] * sin(lch[2]);
    yr = (lab[0]<=kappa*epsilon) ? 
	(lab[0]/kappa) : (pow((lab[0]+16.0)/116.0, 3.0));
    fy = (yr<=epsilon) ? ((kappa*yr+16.0)/116.0) : ((lab[0]+16.0)/116.0);
    fz = fy - lab[2]/200.0;
    fx = lab[1]/500.0 + fy;
    zr = (pow(fz, 3.0)<=epsilon) ? ((116.0*fz-16.0)/kappa) : (pow(fz, 3.0));
    xr = (pow(fx, 3.0)<=epsilon) ? ((116.0*fx-16.0)/kappa) : (pow(fx, 3.0));

    xyz[0] = xr*65535.0 - 0.5;
    xyz[1] = yr*65535.0 - 0.5;
    xyz[2] = zr*65535.0 - 0.5;

    for (c=0; c<3; c++) {
	tmpf = 0;
	for (cc=0; cc<3; cc++)
	    tmpf += rgb_xyz[c][cc] * xyz[cc];
	rgb[c] = MAX(tmpf, 0);
    }
}

void MaxMidMin(gint64 p[3], int *maxc, int *midc, int *minc)
{
    if (p[0] > p[1] && p[0] > p[2]) {
        *maxc = 0;
        if (p[1] > p[2]) { *midc = 1; *minc = 2; }
        else { *midc = 2; *minc = 1; }
    } else if (p[1] > p[2]) {
        *maxc = 1;
        if (p[0] > p[2]) { *midc = 0; *minc = 2; }
        else { *midc = 2; *minc = 0; }
    } else {
        *maxc = 2;
        if (p[0] > p[1]) { *midc = 0; *minc = 1; }
        else { *midc = 1; *minc = 0; }
    }
}

inline void develope(void *po, guint16 pix[4], developer_data *d, int mode,
    guint16 *buf, int count)
{
    guint8 *p8 = po;
    guint16 *p16 = po, c, cc;
    gint64 tmp, tmppix[4];
    int i;
    gboolean clipped;
    for (i=0; i<count; i++) {
	clipped = FALSE;
	for (c=0; c<d->colors; c++) {
	    tmppix[c] = (guint64)pix[i*4+c] * d->rgbWB[c] / 0x10000;
	    if (d->unclip) {
		if (tmppix[c] > d->max / 0x10000)
		    clipped = TRUE;
	    } else {
		tmppix[c] = MIN(tmppix[c], d->max);
	    }
	    tmppix[c] = tmppix[c] * d->exposure / d->max;
	}
	if ( clipped ) {
	    gint64 unclippedPix[3], clippedPix[3];
	    if ( d->useMatrix ) {
		for (cc=0; cc<3; cc++) {
		    for (c=0, tmp=0; c<d->colors; c++)
			tmp += tmppix[c] * d->colorMatrix[cc][c];
		    unclippedPix[cc] = MAX(tmp / 0x10000, 0);
		}
	    } else {
		for (c=0; c<3; c++) unclippedPix[c] = tmppix[c];
	    }
	    for (c=0; c<3; c++) tmppix[c] = MIN(tmppix[c], d->exposure);
	    if ( d->useMatrix ) {
		for (cc=0; cc<3; cc++) {
		    for (c=0, tmp=0; c<d->colors; c++)
			tmp += tmppix[c] * d->colorMatrix[cc][c];
		    clippedPix[cc] = MAX(tmp/0x10000, 0);
		}
	    } else {
		for (c=0; c<3; c++) clippedPix[c] = tmppix[c];
	    }
#ifdef LCH_BLENDING
	    float lch[3], clippedLch[3], unclippedLch[3];
	    rgb_to_cielch(unclippedPix, unclippedLch);
	    rgb_to_cielch(clippedPix, clippedLch);
	    lch[0] = clippedLch[0] + (unclippedLch[0]-clippedLch[0]) * 2/2;
	    lch[1] = clippedLch[1];
	    lch[2] = clippedLch[2];
	    cielch_to_rgb(lch, tmppix);
#else
	    int maxc, midc, minc;
	    MaxMidMin(unclippedPix, &maxc, &midc, &minc);
	    gint64 unclippedLum = unclippedPix[maxc];
	    gint64 clippedLum = clippedPix[maxc];
	    gint64 unclippedSat = 0x10000 -
		    unclippedPix[minc] * 0x10000 / unclippedPix[maxc];
	    gint64 clippedSat = 0x10000 -
		    clippedPix[minc] * 0x10000 / clippedPix[maxc];
	    gint64 clippedHue;
	    if ( clippedPix[maxc]==clippedPix[minc] ) clippedHue = 0;
	    else clippedHue =
		    (clippedPix[midc]-clippedPix[minc])*0x10000 /
		    (clippedPix[maxc]-clippedPix[minc]);
	    gint64 unclippedHue;
	    if ( unclippedPix[maxc]==unclippedPix[minc] )
		unclippedHue = clippedHue;
	    else
		unclippedHue =
		    (unclippedPix[midc]-unclippedPix[minc])*0x10000 /
		    (unclippedPix[maxc]-unclippedPix[minc]);
	    /* Here we decide how to mix the clipped and unclipped values.
	     * The general equation is clipped + (unclipped - clipped) * x,
	     * where x is between 0 and 1. */
	    /* For lum we set x=1/2. This way hightlights are not too bright. */
	    gint64 lum = clippedLum + (unclippedLum - clippedLum) * 1/2;
	    /* For sat we should set x=0 to prevent color artifacts. */
	    gint64 sat = clippedSat + (unclippedSat - clippedSat) * 0/1 ;
	    /* For hue we set x=1. This doesn't seem to have much effect. */
	    gint64 hue = unclippedHue;

	    tmppix[maxc] = lum;
	    tmppix[minc] = lum * (0x10000-sat) / 0x10000;
	    tmppix[midc] = lum * (0x10000-sat + sat*hue/0x10000) / 0x10000;
#endif /*LCH_BLENDING*/
	    for (c=0; c<3; c++)
		buf[i*3+c] = d->gammaCurve[MIN(tmppix[c], 0xFFFF)];
	} else {
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
    }
    if (d->colorTransform!=NULL)
        cmsDoTransform(d->colorTransform, buf, buf, count);

    if (mode==16) for (i=0; i<3*count; i++) p16[i] = buf[i];
    else for (i=0; i<3*count; i++) p8[i] = buf[i] >> 8;
}
