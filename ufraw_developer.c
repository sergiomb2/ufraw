/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 * by udi Fuchs,
 *
 * ufraw_developer.c - functions for developing images or more exactly pixels.
 *
 * UFRaw is licensed under the GNU General Public License.
 * It uses "dcraw" code to do the actual raw decoding.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <lcms.h>
#include <glib.h>
#include "nikon_curve.h"
#include "ufraw.h"
#include "blackbody.h"

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
    d->toneCurveData.m_gamma = -1.0;
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
	int unclip, double temperature, double green,
	float pre_mul[4], profile_data *in, profile_data *out, int intent,
        double saturation, CurveData *curve)
{
    int c, i;

    d->rgbMax = rgbMax;
    i = temperature/10-200;
    for (c=0; c<4; c++)
        d->rgbWB[c] = bbWB[i][1]/bbWB[i][c]*pre_mul[c]*
                        (c==1||c==3?green:1.0)*0x10000;
    if (unclip) d->max = 0xFFFF;
    else d->max = MIN(exposure*0x10000, 0xFFFF);
    for (c=0; c<4; c++) d->rgbWB[c] = d->rgbWB[c] * exposure;
    if (in->gamma!=d->gamma || in->linear!=d->linear) {
        double a, b, c, g;
        d->gamma = in->gamma;
        d->linear = in->linear;
        if (d->linear<1.0) {
            g = d->gamma*(1.0-d->linear);
            a = 1.0/(1.0+d->linear*(g-1));
            b = d->linear*(g-1)*a;
            c = pow(a*d->linear+b, g)/d->linear;
        } else {
            a = b = g = 0.0;
            c = 1.0;
        }
        for (i=0; i<0x10000*d->linear; i++) d->gammaCurve[i] = MIN(c*i, 0xFFFF);
        for (; i<0x10000; i++) d->gammaCurve[i] =
            MIN(pow(a*i/0x10000+b, g)*0x10000, 0xFFFF);
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
    /* Check if curve data has changed. Changes in contrast, saturation
     * will cause a redundant call to CurveDataSample() */
    if (memcmp(curve,&d->toneCurveData, sizeof(CurveData))) {
        d->toneCurveData = *curve;
	CurveSample *cs = CurveSampleInit(0x10000, 0xFFFF);
        if (CurveDataSample(curve, cs)!=UFRAW_SUCCESS) {
            ufraw_message(UFRAW_ERROR, "Error sampling Nikon curve");
            for (i=0; i<0x10000; i++) d->toneCurve[i] = i;
        } else
            for (i=0; i<0x10000; i++) d->toneCurve[i] = cs->m_Samples[i];
        CurveSampleFree(cs);
    }
    int norm = 0x10000/(1.0-curve->m_black);
    for (i=0; i<0x10000; i++)  {
        double val = (double)d->toneCurve[i]/0x10000;
        d->correctionCurve[i] = MIN( floor( MAX(
                ( curve->m_contrast==0 ? val :
                    log(1+curve->m_contrast*val) / log(1+curve->m_contrast) ) -
                curve->m_black, 0) * norm), 0xFFFF);
    }
}

inline void develope(void *po, guint16 pix[4], developer_data *d, int mode,
    guint16 *buf, int count)
{
    guint8 *p8 = po;
    guint16 *p16 = po, *p, tmp, maxc, midc, minc, c;
    unsigned sat, hue;
    int i;

    for (i=0; i<count; i++)
        for (c=0; c<3; c++) {
	    tmp = pix[i*4+c];
            tmp = MIN((guint64)d->rgbWB[c]*tmp/d->rgbMax,(guint64)d->max);
	    buf[i*3+c] = d->gammaCurve[tmp];
	}
    if (d->colorTransform!=NULL)
        cmsDoTransform(d->colorTransform, buf, buf, count);
    if (d->saturation!=1) for (i=0, p=buf; i<count; i++, p+=3) {
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
            p[minc] = p[maxc]*(0x10000-sat)/0x10000;
            p[midc] = p[maxc]*(0x10000-sat+sat*hue/0x10000)/0x10000;
            /* It is also possible to define oldSaturation = max-min */
        }
    }
    if (mode==16) for (i=0; i<3*count; i++) p16[i] = d->correctionCurve[buf[i]];
    else for (i=0; i<3*count; i++) p8[i] = d->correctionCurve[buf[i]] >> 8;
}
