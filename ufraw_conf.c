/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw_conf.c - handle configuration issues
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

#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/stat.h> /* needed for fstat() */
#include <getopt.h>
#include <glib.h>
#include <glib/gi18n.h>
#include "ufraw.h"

const conf_data conf_default = {
    /* Internal data */
    sizeof(conf_data), 7, /* confSize, version */

    /* Image manipulation settings */
    camera_wb, 0, /* wb, WBTuning */
    6500, 1.0, /* temperature, green */
    { -1.0, -1.0, -1.0, -1.0 }, /* chanMul[] */
    0.0, 1.0, 0.0, /* exposure, saturation, black */
    TRUE /* Unclip highlights */,
    disabled_state, /* autoExposure */
    disabled_state, /* autoBlack */
    camera_curve, camera_curve+1, /* BaseCurveIndex, BaseCurveCount */
    /* BaseCurve data defaults */
    { { N_("Manual curve"), TONE_CURVE, 0.0, 1.0, 0.0, 1.0, 1.0,
	  2 , { { 0.0, 0.0 }, { 1.0, 1.0 } } },
      { N_("Linear curve"), TONE_CURVE, 0.0, 1.0, 0.0, 1.0, 1.0,
	  2 , { { 0.0, 0.0 }, { 1.0, 1.0 } } },
      { N_("Custom curve"), TONE_CURVE, 0.0, 1.0, 0.0, 1.0, 1.0,
	  2 , { { 0.0, 0.0 }, { 1.0, 1.0 } } },
      { N_("Camera curve"), TONE_CURVE, 0.0, 1.0, 0.0, 1.0, 1.0,
	  2 , { { 0.0, 0.0 }, { 1.0, 1.0 } } }
    },
    linear_curve, linear_curve+1, /* curveIndex, curveCount */
    /* Curve data defaults */
    { { N_("Manual curve"), TONE_CURVE, 0.0, 1.0, 0.0, 1.0, 1.0,
	  2 , { { 0.0, 0.0 }, { 1.0, 1.0 } } },
      { N_("Linear curve"), TONE_CURVE, 0.0, 1.0, 0.0, 1.0, 1.0,
	  2 , { { 0.0, 0.0 }, { 1.0, 1.0 } } }
    },
    { 0, 0 } , { 1, 1 }, /* profileIndex[], profileCount[] */
    /* Profile data defaults */
    { { { N_("sRGB"), "", "", 0.45, 0.1, TRUE },
        { "Some ICC Profile", "", "", 0.45, 0.0, FALSE } },
      { { N_("sRGB"), "", "", 0.0, 0.0, FALSE },
        { "Some ICC Profile", "", "", 0.0, 0.0, FALSE } } },
    0, /* intent */
    ahd_interpolation, /* interpolation */
    "", NULL, /* darkframeFile, darkframe */
    /* Save options */
    "", "", "", /* inputFilename, outputFilename, outputPath */
    "", "", /* inputURI, inputModTime */
    ppm8_type, 85, no_id, /* type, compression, createID */
    TRUE, /* embedExif */
    1, 0, /* shrink, size */
    FALSE, /* overwrite exsisting files without asking */
    FALSE, /* losslessCompress */
    FALSE, /* load embedded preview image */

    /* GUI settings */
    25.0, 4, /* Zoom, Scale */
    enabled_state, /* saveConfiguration */
    rgb_histogram, /* histogram */
    linear_histogram, 128, /* liveHistogramScale, liveHistogramHeight */
    linear_histogram, 128, /* rawHistogramScale, rawHistogramHeight */
    { TRUE, TRUE }, /* expander[] */
    FALSE, /* overExp indicator */
    FALSE, /* underExp indicator */
    "", "", /* curvePath, profilePath */

    /* EXIF data */
    0, /* flip */
    0.0, 0.0, 0.0, 0.0, /* iso_speed, shutter, aperture, focal_len */
    "", "", "", "", /* exifSource, isoText, shutterText, apertureText */
    "", "", "", /* focalLenText, focalLen35Text, lensText */
    "", "", "" /* timestamp, make, model */
};

const char *interpolationNames[] =
    { "ahd", "vng", "four-color", "bilinear", "half" };

int conf_find_name(const char name[],const char *namesList[])
{
    int i;
    for (i=0; namesList[i]!=NULL; i++) {
	if (!strcmp(name, namesList[i])) return i;
    }
    return -1;	
}

const char *conf_get_name(const char *namesList[], int index)
{
    int i;
    for (i=0; namesList[i]!=NULL; i++)
	if (i==index) return namesList[i];
    return "Error";
}

void conf_parse_start(GMarkupParseContext *context, const gchar *element,
    const gchar **names, const gchar **values, gpointer user, GError **error)
{
    conf_data *c = user;
    int int_value;
    GQuark ufrawQuark = g_quark_from_static_string("UFRaw");

    context = context;
    while (*names!=NULL) {
        if (!strcasecmp(*values, "yes"))
            int_value = 1;
        if (!strcasecmp(*values, "no"))
            int_value = 0;
        else
            sscanf(*values, "%d", &int_value);
        if (!strcmp(element, "UFRaw") && !strcmp(*names, "Version")) {
            if (int_value==3) {
                ufraw_message(UFRAW_WARNING,
                    _("Trying to convert .ufrawrc from UFRaw-0.4 or earlier"));
                c->version = int_value;
            }
	    /* In version 7 temperature calculation has changed */
            if (int_value==5) {
                ufraw_message(UFRAW_WARNING,
                   _("Trying to convert .ufrawrc from UFRaw-0.6 or earlier"));
                c->version = int_value;
            }
            if (int_value!=c->version)
                g_set_error(error, ufrawQuark, UFRAW_RC_VERSION,
                    _("UFRaw version in .ufrawrc is not supported"));
        }
        if (!strcmp(*names,"Current") && int_value!=0) {
            if (!strcmp("BaseManualCurve", element))
                c->BaseCurveIndex = manual_curve;
            if (!strcmp("BaseLinearCurve", element))
                c->BaseCurveIndex = linear_curve;
            if (!strcmp("BaseCustomCurve", element))
                c->BaseCurveIndex = custom_curve;
            if (!strcmp("BaseCameraCurve", element))
                c->BaseCurveIndex = camera_curve;
            if (!strcmp("BaseCurve", element))
                c->BaseCurveIndex = c->BaseCurveCount;
            if (!strcmp("ManualCurve", element))
                c->curveIndex = manual_curve;
            if (!strcmp("LinearCurve", element))
                c->curveIndex = linear_curve;
            if (!strcmp("Curve", element))
                c->curveIndex = c->curveCount;
            if (!strcmp("sRGBInputProfile", element))
                c->profileIndex[0] = 0;
            if (!strcmp("sRGBOutputProfile", element))
                c->profileIndex[1] = 0;
            if (!strcmp("InputProfile", element))
                c->profileIndex[0] = c->profileCount[0];
            if (!strcmp("OutputProfile", element))
                c->profileIndex[1] = c->profileCount[1];
        }
        names++;
        values++;
    }
    /* The default curve/profile count is always larger than 0,
     * threfore we can treat 0 as a negative number.
     * m_numAnchors==0 marks that no anchors where read from the XML. */
    if (!strcmp("BaseManualCurve", element)) {
	c->BaseCurveCount = - manual_curve;
	c->BaseCurve[-c->BaseCurveCount].m_numAnchors = 0;
    }
    if (!strcmp("BaseLinearCurve", element)) {
	c->BaseCurveCount = - linear_curve;
	c->BaseCurve[-c->BaseCurveCount].m_numAnchors = 0;
    }
    if (!strcmp("BaseCustomCurve", element)) {
	c->BaseCurveCount = - custom_curve;
	c->BaseCurve[-c->BaseCurveCount].m_numAnchors = 0;
    }
    if (!strcmp("BaseCameraCurve", element)) {
	c->BaseCurveCount = - camera_curve;
	c->BaseCurve[-c->BaseCurveCount].m_numAnchors = 0;
    }
    if (!strcmp("ManualCurve", element)) {
	c->curveCount = - manual_curve;
	c->curve[-c->curveCount].m_numAnchors = 0;
    }
    if (!strcmp("LinearCurve", element)) {
	c->curveCount = - linear_curve;
	c->curve[-c->curveCount].m_numAnchors = 0;
    }
    if (!strcmp("sRGBInputProfile", element)) c->profileCount[0] = - 0;
    if (!strcmp("sRGBOutputProfile", element)) c->profileCount[1] = - 0;
}

void conf_parse_end(GMarkupParseContext *context, const gchar *element,
         gpointer user, GError **error)
{
    conf_data *c = user;
    context = context;
    error = error;
    if ( c->BaseCurveCount<=0 &&
	 ( !strcmp("BaseManualCurve", element) ||
	   !strcmp("BaseLinearCurve", element) ||
	   !strcmp("BaseCustomCurve", element) ||
	   !strcmp("BaseCameraCurve", element) ) ) {
	if (c->BaseCurve[-c->BaseCurveCount].m_numAnchors==0)
	    c->BaseCurve[-c->BaseCurveCount].m_numAnchors = 2;
	c->BaseCurveCount = camera_curve+1;
    }
    if (c->BaseCurveCount<=0 && !strcmp("BaseCurve", element)) {
	if (c->BaseCurve[-c->BaseCurveCount].m_numAnchors==0)
	    c->BaseCurve[-c->BaseCurveCount].m_numAnchors = 2;
        c->BaseCurveCount = - c->BaseCurveCount + 1;
    }
    if ( c->curveCount<=0 &&
	 ( !strcmp("ManualCurve", element) ||
	   !strcmp("LinearCurve", element) ) ) {
	if (c->curve[-c->curveCount].m_numAnchors==0)
	    c->curve[-c->curveCount].m_numAnchors = 2;
	c->curveCount = linear_curve+1;
    }
    if (c->curveCount<=0 && !strcmp("Curve", element)) {
	if (c->curve[-c->curveCount].m_numAnchors==0)
	    c->curve[-c->curveCount].m_numAnchors = 2;
        c->curveCount = - c->curveCount + 1;
    }
    if (!strcmp("sRGBInputProfile", element)) c->profileCount[0] = 1;
    if (!strcmp("sRGBOutputProfile", element)) c->profileCount[1] = 1;
    if (c->profileCount[0]<=0 && !strcmp("InputProfile", element))
        c->profileCount[0] = - c->profileCount[0] + 1;
    if (c->profileCount[1]<=0 && !strcmp("OutputProfile", element))
        c->profileCount[1] = - c->profileCount[1] + 1;
}

void conf_parse_text(GMarkupParseContext *context, const gchar *text, gsize len,
        gpointer user, GError **error)
{
    conf_data *c = user;
    const gchar *element = g_markup_parse_context_get_element(context);
    char temp[max_path];
    int i;
    error = error;
    for(; len>0 && g_ascii_isspace(*text); len--, text++);
    for(; len>0 && g_ascii_isspace(text[len-1]); len--);
    if (len==0) return;
    if (len>max_path-1) len=max_path-1;
    strncpy(temp, text, len);
    temp[len] = '\0';
    if (c->curveCount<=0) {
        i = - c->curveCount;
        /* for backward compatibility with ufraw-0.4 */
        if (!strcmp("File", element)) {
            /* Ignore this curve if curve_load() fails */
            if (curve_load(&c->curve[i], temp)!=UFRAW_SUCCESS)
                    c->curveCount = - c->curveCount;
	}
        if (!strcmp("MinXY", element))
            sscanf(temp, "%lf %lf", &c->curve[i].m_min_x, &c->curve[i].m_min_y);
        if (!strcmp("MaxXY", element))
            sscanf(temp, "%lf %lf", &c->curve[i].m_max_x, &c->curve[i].m_max_y);
        if (!strcmp("AnchorXY", element)) {
	    if (c->curve[i].m_numAnchors==max_anchors) {
		ufraw_message(UFRAW_WARNING,
			_("Too many anchors for curve '%s'"), c->curve[i].name);
		/* We try to keep the last anchor point */
		c->curve[i].m_numAnchors--;
	    }
	    /* If one anchor is supplied then all anchors should be supplied */
            sscanf(temp, "%lf %lf",
                    &c->curve[i].m_anchors[c->curve[i].m_numAnchors].x,
                    &c->curve[i].m_anchors[c->curve[i].m_numAnchors].y);
            c->curve[i].m_numAnchors++;
        }
        return;
    }

    if (c->BaseCurveCount<=0) {
        i = - c->BaseCurveCount;
        if (!strcmp("MinXY", element))
            sscanf(temp, "%lf %lf", &c->BaseCurve[i].m_min_x, &c->BaseCurve[i].m_min_y);
        if (!strcmp("MaxXY", element))
            sscanf(temp, "%lf %lf", &c->BaseCurve[i].m_max_x, &c->BaseCurve[i].m_max_y);
        if (!strcmp("AnchorXY", element)) {
	    /* If one anchor is supplied then all anchors should be supplied */
            sscanf(temp, "%lf %lf",
                    &c->BaseCurve[i].m_anchors[c->BaseCurve[i].m_numAnchors].x,
                    &c->BaseCurve[i].m_anchors[c->BaseCurve[i].m_numAnchors].y);
            c->BaseCurve[i].m_numAnchors++;
        }
        return;
    }

    if (c->profileCount[0]<=0) {
        i = - c->profileCount[0];
        if (!strcmp("File", element))
            g_strlcpy(c->profile[0][i].file, temp, max_path);
        if (!strcmp("ProductName", element))
            g_strlcpy(c->profile[0][i].productName, temp, max_path);
        if (!strcmp("Gamma", element))
            sscanf(temp, "%lf", &c->profile[0][i].gamma);
        if (!strcmp("Linearity", element))
            sscanf(temp, "%lf", &c->profile[0][i].linear);
        if (!strcmp("UseColorMatrix", element))
            sscanf(temp, "%d", &c->profile[0][i].useMatrix);
        return;
    }
    if (c->profileCount[1]<=0) {
        i = - c->profileCount[1];
        if (!strcmp("File", element)) {
	    char *utf8 = g_filename_from_utf8(temp, -1, NULL, NULL, NULL);
	    if (utf8==NULL) utf8 = g_strdup("Unknown file name");
            g_strlcpy(c->profile[1][i].file, utf8, max_path);
	    g_free(utf8);
	}
        if (!strcmp("ProductName", element))
            g_strlcpy(c->profile[0][i].productName, temp, max_path);
        return;
    }
    if (!strcmp("BaseCurve", element)) {
        c->BaseCurveCount = - c->BaseCurveCount;
        i = - c->BaseCurveCount;
        c->BaseCurve[i] = conf_default.BaseCurve[0];
        g_strlcpy(c->BaseCurve[i].name, temp, max_name);
	/* m_numAnchors==0 marks that no anchors where read from the XML */
	c->BaseCurve[-c->BaseCurveCount].m_numAnchors = 0;
    }
    if (!strcmp("Curve", element)) {
        c->curveCount = - c->curveCount;
        i = - c->curveCount;
        c->curve[i] = conf_default.curve[0];
        g_strlcpy(c->curve[i].name, temp, max_name);
	/* m_numAnchors==0 marks that no anchors where read from the XML */
	c->curve[-c->curveCount].m_numAnchors = 0;
    }
    if (!strcmp("InputProfile", element)) {
        c->profileCount[0] = - c->profileCount[0];
        i = - c->profileCount[0];
        c->profile[0][i] = conf_default.profile[0][1];
        g_strlcpy(c->profile[0][i].name, temp, max_name);
    }
    if (!strcmp("OutputProfile", element)) {
        c->profileCount[1] = - c->profileCount[1];
        i = - c->profileCount[1];
        c->profile[1][i] = conf_default.profile[1][0];
        g_strlcpy(c->profile[1][i].name, temp, max_name);
    }
    if (!strcmp("InputFilename", element)) {
	char *utf8 = g_filename_from_utf8(temp, -1, NULL, NULL, NULL);
	if (utf8!=NULL)
            g_strlcpy(c->inputFilename, utf8, max_path);
	g_free(utf8);
    }
    if (!strcmp("OutputFilename", element)) {
	char *utf8 = g_filename_from_utf8(temp, -1, NULL, NULL, NULL);
	if (utf8!=NULL)
            g_strlcpy(c->outputFilename, utf8, max_path);
	g_free(utf8);
    }
    if (!strcmp("OutputPath", element)) {
	char *utf8 = g_filename_from_utf8(temp, -1, NULL, NULL, NULL);
	if (utf8!=NULL)
            g_strlcpy(c->outputPath, utf8, max_path);
	g_free(utf8);
    }
    if (!strcmp("SaveConfiguration", element))
	sscanf(temp, "%d", &c->saveConfiguration);
    if (!strcmp("Interpolation", element)) {
	/* Keep compatebility with old numbers from ufraw-0.5 */
        if (sscanf(temp, "%d", &c->interpolation)==1) {
	    switch (c->interpolation) {
		case 0: c->interpolation = vng_interpolation; break;
		case 1: c->interpolation = four_color_interpolation; break;
		case 2: c->interpolation = bilinear_interpolation; break;
		case 3: c->interpolation = half_interpolation; break;
		default: c->interpolation = ahd_interpolation;
	    }
	} else {
	    c->interpolation = conf_find_name(temp, interpolationNames);
	    if (c->interpolation<0) c->interpolation = ahd_interpolation;
	}
    }
    if (!strcmp("RawExpander", element))
            sscanf(temp, "%d", &c->expander[raw_expander]);
    if (!strcmp("LiveExpander", element))
            sscanf(temp, "%d", &c->expander[live_expander]);
    if (!strcmp("Histogram", element)) sscanf(temp, "%d", &c->histogram);
    if (!strcmp("LiveHistogramScale", element))
	    sscanf(temp, "%d", &c->liveHistogramScale);
    if (!strcmp("LiveHistogramheight", element))
	    sscanf(temp, "%d", &c->liveHistogramHeight);
    if (!strcmp("RawHistogramScale", element))
	    sscanf(temp, "%d", &c->rawHistogramScale);
    if (!strcmp("RawHistogramheight", element))
	    sscanf(temp, "%d", &c->rawHistogramHeight);
    if (!strcmp("OverExposure", element)) sscanf(temp, "%d", &c->overExp);
    if (!strcmp("UnderExposure", element)) sscanf(temp, "%d", &c->underExp);
    if (!strcmp("WB", element)) {
	/* Keep compatebility with old numbers from ufraw-0.6 */
        if (sscanf(temp, "%d", &i)==1) {
	    switch (i) {
		case -1: g_strlcpy(c->wb, spot_wb, max_name); break;
		case 0: g_strlcpy(c->wb, manual_wb, max_name); break;
		case 1: g_strlcpy(c->wb, camera_wb, max_name); break;
		case 2: g_strlcpy(c->wb, auto_wb, max_name); break;
		case 3: g_strlcpy(c->wb, "Incandescent", max_name); break;
		case 4: g_strlcpy(c->wb, "Fluorescent", max_name); break;
		case 5: g_strlcpy(c->wb, "Direct sunlight", max_name);break;
		case 6: g_strlcpy(c->wb, "Flash", max_name); break;
		case 7: g_strlcpy(c->wb, "Cloudy", max_name); break;
		case 8: g_strlcpy(c->wb, "Shade", max_name); break;
		default: g_strlcpy(c->wb, "", max_name);
	    }
	} else {
	    g_strlcpy(c->wb, temp, max_name);
	}
    }
    if (!strcmp("WBFineTuning", element)) sscanf(temp, "%lf", &c->WBTuning);
    if (!strcmp("Temperature", element)) sscanf(temp, "%lf", &c->temperature);
    if (!strcmp("Green", element)) sscanf(temp, "%lf", &c->green);
    if (!strcmp("ChannelMultipliers", element)) {
	i = sscanf(temp, "%lf %lf %lf %lf",
		&c->chanMul[0], &c->chanMul[1], &c->chanMul[2], &c->chanMul[3]);
	if (i<4) c->chanMul[3] = 0.0;
    }
    if (!strcmp("Exposure", element)) sscanf(temp, "%lf", &c->exposure);
    if (!strcmp("Saturation", element)) sscanf(temp, "%lf", &c->saturation);
    if (!strcmp("Unclip", element)) sscanf(temp, "%d", &c->unclip);
    if (!strcmp("AutoExposure", element)) sscanf(temp, "%d", &c->autoExposure);
    if (!strcmp("AutoBlack", element)) sscanf(temp, "%d", &c->autoBlack);
    if (!strcmp("CurvePath", element)) {
	char *utf8 = g_filename_from_utf8(temp, -1, NULL, NULL, NULL);
	if (utf8!=NULL)
            g_strlcpy(c->curvePath, utf8, max_path);
	g_free(utf8);
    }
    if (!strcmp("Intent", element)) sscanf(temp, "%d", &c->intent);
    if (!strcmp("Make", element)) g_strlcpy(c->make, temp, max_name);
    if (!strcmp("Model", element)) g_strlcpy(c->model, temp, max_name);
    if (!strcmp("DarkframeFile", element))
	g_strlcpy(c->darkframeFile, temp, max_path);
    if (!strcmp("ProfilePath", element)) {
	char *utf8 = g_filename_from_utf8(temp, -1, NULL, NULL, NULL);
	if (utf8!=NULL)
            g_strlcpy(c->profilePath, utf8, max_path);
	g_free(utf8);
    }
    if (!strcmp("Shrink", element)) sscanf(temp, "%d", &c->shrink);
    if (!strcmp("Size", element)) sscanf(temp, "%d", &c->size);
    if (!strcmp("OutputType", element)) sscanf(temp, "%d", &c->type);
    if (!strcmp("CreateID", element)) sscanf(temp, "%d", &c->createID);
    if (!strcmp("EmbedExif", element)) sscanf(temp, "%d", &c->embedExif);
    if (!strcmp("Compression", element)) sscanf(temp, "%d", &c->compression);
    if (!strcmp("Overwrite", element)) sscanf(temp, "%d", &c->overwrite);
    if (!strcmp("LosslessCompression", element))
            sscanf(temp, "%d", &c->losslessCompress);
}

int conf_load(conf_data *c, const char *IDFilename)
{
    char *confFilename, line[max_path], *locale;
    const char *hd;
    FILE *in;
    GMarkupParser parser={&conf_parse_start, &conf_parse_end,
            &conf_parse_text, NULL, NULL};
    GMarkupParseContext *context;
    GError *err = NULL;
    int i;

    *c = conf_default;
    if (IDFilename==NULL) {
        hd = uf_get_home_dir();
        confFilename = g_build_filename(hd, ".ufrawrc", NULL);
        in=fopen(confFilename, "r");
	/* We don't mind if ~/.ufrawrc does not exist. */
        if (in==NULL) {
	    g_free(confFilename);
            return UFRAW_SUCCESS;
	}
    } else {
        if ( (in=fopen(IDFilename, "r"))==NULL ) {
            ufraw_message(UFRAW_SET_ERROR,
                    _("Can't open ID file %s for reading\n%s\n"),
                    IDFilename, strerror(errno) );
            return UFRAW_ERROR;
        }
	confFilename = g_strdup(IDFilename);
    }
    g_snprintf(c->inputURI, max_path, "file://%s", confFilename);
    struct stat s;
    fstat(fileno(in), &s);
    g_snprintf(c->inputModTime, max_name, "%d", (int)s.st_mtime);

    locale = uf_set_locale_C();
    context = g_markup_parse_context_new(&parser, 0, c, NULL);
    line[max_path-1] = '\0';
    fgets(line, max_path-1, in);
    while (!feof(in)) {
        if (!g_markup_parse_context_parse(context, line, strlen(line), &err)) {
            ufraw_message(UFRAW_ERROR, _("Error parsing '%s'\n%s"),
                    confFilename, err->message);
	    g_markup_parse_context_free(context);
	    uf_reset_locale(locale);
	    g_free(confFilename);
	    fclose(in);
	    // We could if needed check explicitly for a version mismatch error
	    //GQuark ufrawQuark = g_quark_from_static_string("UFRaw");
            //if (g_error_matches(err, ufrawQuark, UFRAW_RC_VERSION))
	    g_error_free(err);
            return UFRAW_ERROR;
        }
        fgets(line, max_path, in);
    }
    g_markup_parse_context_end_parse(context, NULL);
    g_markup_parse_context_free(context);
    uf_reset_locale(locale);
    g_free(confFilename);
    fclose(in);

    if (c->version==3) {
        c->version = conf_default.version;
        /* Don't add linear part to existing profile curves (except sRGB) */
        for (i=2; i<c->profileCount[0]; i++)
            c->profile[0][i].linear = 0.0;
    }
    if (c->version==5) {
        c->version = conf_default.version;
    }
    /* a few consistency settings */
    if (c->curveIndex>=c->curveCount) c->curveIndex = conf_default.curveIndex;
    return UFRAW_SUCCESS;
}

int conf_save(conf_data *c, char *IDFilename, char **confBuffer)
{
    char *buf=NULL, *type, *current;
    int i, j;

    char *locale = uf_set_locale_C();

    buf = uf_markup_buf(buf, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
    buf = uf_markup_buf(buf, "<UFRaw Version='%d'>\n", c->version);
    if (strlen(c->inputFilename)>0 && IDFilename!=NULL) {
	char *utf8 = g_filename_to_utf8(c->inputFilename, -1, NULL, NULL, NULL);
	if (utf8==NULL) utf8 = g_strdup("Unknown file name");
        buf = uf_markup_buf(buf, "<InputFilename>%s</InputFilename>\n", utf8);
	g_free(utf8);
    }
    if (strlen(c->outputFilename)>0 && IDFilename!=NULL) {
	char *utf8=g_filename_to_utf8(c->outputFilename, -1, NULL, NULL, NULL);
	if (utf8==NULL) utf8 = g_strdup("Unknown file name");
        buf = uf_markup_buf(buf, "<OutputFilename>%s</OutputFilename>\n", utf8);
	g_free(utf8);
    }
    if (strlen(c->outputPath)>0) {
	char *utf8=g_filename_to_utf8(c->outputPath, -1, NULL, NULL, NULL);
	if (utf8!=NULL)
	    buf = uf_markup_buf(buf, "<OutputPath>%s</OutputPath>\n", utf8);
	g_free(utf8);
    } else {
	/* Guess outputPath */
        char *inPath = g_path_get_dirname(c->inputFilename);
	char *outPath = g_path_get_dirname(c->outputFilename);
	if ( strcmp(outPath,".") && strcmp(inPath, outPath) ) {
	    g_strlcpy(c->outputPath, outPath, max_path);
	    char *utf8=g_filename_to_utf8(outPath, -1, NULL, NULL, NULL);
	    if (utf8!=NULL)
		buf = uf_markup_buf(buf, "<OutputPath>%s</OutputPath>\n", utf8);
	    g_free(utf8);
	}
	g_free(outPath);
	g_free(inPath);
    }
					    
    /* GUI settings are only saved to .ufrawrc */
    if (IDFilename==NULL) {
        if (c->saveConfiguration!=conf_default.saveConfiguration)
            buf = uf_markup_buf(buf,
		    "<SaveConfiguration>%d</SaveConfiguration>\n",
		    c->saveConfiguration);
        if (c->expander[raw_expander]!=conf_default.expander[raw_expander])
            buf = uf_markup_buf(buf, "<RawExpander>%d</RawExpander>\n",
                    c->expander[raw_expander]);
        if (c->expander[live_expander]!=conf_default.expander[live_expander])
            buf = uf_markup_buf(buf, "<LiveExpander>%d</LiveExpander>\n",
                    c->expander[live_expander]);
        if (c->histogram!=conf_default.histogram)
            buf = uf_markup_buf(buf,
		    "<Histogram>%d</Histogram>\n", c->histogram);
        if (c->liveHistogramScale!=conf_default.liveHistogramScale)
            buf = uf_markup_buf(buf,
		    "<LiveHistogramScale>%d</LiveHistogramScale>\n",
		    c->liveHistogramScale);
        if (c->liveHistogramHeight!=conf_default.liveHistogramHeight)
            buf = uf_markup_buf(buf,
		    "<LiveHistogramHeight>%d</LiveHistogramHeight>\n",
		    c->liveHistogramHeight);
        if (c->rawHistogramScale!=conf_default.rawHistogramScale)
            buf = uf_markup_buf(buf,
		    "<RawHistogramScale>%d</RawHistogramScale>\n",
		    c->rawHistogramScale);
        if (c->rawHistogramHeight!=conf_default.rawHistogramHeight)
            buf = uf_markup_buf(buf,
		    "<RawHistogramHeight>%d</RawHistogramHeight>\n",
		    c->rawHistogramHeight);
        if (c->overExp!=conf_default.overExp)
            buf = uf_markup_buf(buf,
		    "<OverExposure>%d</OverExposure>\n", c->overExp);
        if (c->underExp!=conf_default.underExp)
            buf = uf_markup_buf(buf,
		    "<UnderExposure>%d</UnderExposure>\n", c->underExp);
	if (strlen(c->curvePath)>0) {
	    char *utf8 = g_filename_to_utf8(c->curvePath, -1, NULL, NULL, NULL);
	    if (utf8!=NULL)
		buf = uf_markup_buf(buf, "<CurvePath>%s</CurvePath>\n", utf8);
	    g_free(utf8);
	}
	if (strlen(c->profilePath)>0) {
	    char *utf8 = g_filename_to_utf8(c->profilePath, -1,NULL,NULL,NULL);
	    if (utf8!=NULL)
		buf = uf_markup_buf(buf,
			"<ProfilePath>%s</ProfilePath>\n", utf8);
	    g_free(utf8);
	}
    }
    if (c->interpolation!=conf_default.interpolation)
        buf = uf_markup_buf(buf, "<Interpolation>%s</Interpolation>\n",
		conf_get_name(interpolationNames, c->interpolation));
    if (strcmp(c->wb, conf_default.wb))
	buf = uf_markup_buf(buf, "<WB>%s</WB>\n", c->wb);
    if (c->WBTuning!=conf_default.WBTuning)
	buf = uf_markup_buf(buf, "<WBFineTuning>%d</WBFineTuning>\n",
		(int)floor(c->WBTuning));
    buf = uf_markup_buf(buf,
	    "<Temperature>%d</Temperature>\n", (int)floor(c->temperature));
    buf = uf_markup_buf(buf,
	    "<Green>%lf</Green>\n", c->green);
    if (c->chanMul[0]>0.0) {
	if (c->chanMul[3]==0.0) {
	    buf = uf_markup_buf(buf,
		    "<ChannelMultipliers>%f %f %f</ChannelMultipliers>\n",
		    c->chanMul[0], c->chanMul[1], c->chanMul[2]);
	} else {
	    buf = uf_markup_buf(buf,
		    "<ChannelMultipliers>%f %f %f %f</ChannelMultipliers>\n",
		    c->chanMul[0], c->chanMul[1], c->chanMul[2], c->chanMul[3]);
	}
    }
    if (c->exposure!=conf_default.exposure)
        buf = uf_markup_buf(buf, "<Exposure>%lf</Exposure>\n", c->exposure);
    if (c->unclip!=conf_default.unclip)
        buf = uf_markup_buf(buf, "<Unclip>%d</Unclip>\n", c->unclip);
    if (c->autoExposure!=conf_default.autoExposure)
        buf = uf_markup_buf(buf,
		"<AutoExposure>%d</AutoExposure>\n", c->autoExposure);
    if (c->autoBlack!=conf_default.autoBlack)
        buf = uf_markup_buf(buf, "<AutoBlack>%d</AutoBlack>\n", c->autoBlack);
    if (c->saturation!=conf_default.saturation)
        buf = uf_markup_buf(buf,
		"<Saturation>%lf</Saturation>\n", c->saturation);
    if (c->size!=conf_default.size)
        buf = uf_markup_buf(buf, "<Size>%d</Size>\n", c->size);
    if (c->shrink!=conf_default.shrink)
        buf = uf_markup_buf(buf, "<Shrink>%d</Shrink>\n", c->shrink);
    if (c->type!=conf_default.type)
        buf = uf_markup_buf(buf, "<OutputType>%d</OutputType>\n", c->type);
    if (c->createID!=conf_default.createID)
        buf = uf_markup_buf(buf, "<CreateID>%d</CreateID>\n", c->createID);
    if (c->embedExif!=conf_default.embedExif)
        buf = uf_markup_buf(buf, "<EmbedExif>%d</EmbedExif>\n", c->embedExif);
    if (c->compression!=conf_default.compression)
        buf = uf_markup_buf(buf,
		"<Compression>%d</Compression>\n", c->compression);
    if (c->overwrite!=conf_default.overwrite)
        buf = uf_markup_buf(buf, "<Overwrite>%d</Overwrite>\n", c->overwrite);
    if (c->losslessCompress!=conf_default.losslessCompress)
        buf = uf_markup_buf(buf,
		"<LosslessCompression>%d</LosslessCompression>\n",
                c->losslessCompress);
    for (i=0; i<c->BaseCurveCount; i++) {
        char *curveBuf = curve_buffer(&c->BaseCurve[i]);
        /* Write curve if it is non-default and we are not writing to .ufraw */
	/* But ALWAYS write the current curve */
        if ( c->BaseCurveIndex==i || (curveBuf!=NULL && IDFilename==NULL) ) {
	    if (curveBuf==NULL) curveBuf = g_strdup("");
            current = i==c->BaseCurveIndex?"yes":"no";
            switch (i) {
                case manual_curve:
                        buf = uf_markup_buf(buf,
				"<BaseManualCurve Current='%s'>\n", current);
			buf = uf_markup_buf(buf, curveBuf);
			buf = uf_markup_buf(buf, "</BaseManualCurve>\n");
                        break;
                case linear_curve:
                        buf = uf_markup_buf(buf,
				"<BaseLinearCurve Current='%s'>\n", current);
			buf = uf_markup_buf(buf, curveBuf);
			buf = uf_markup_buf(buf, "</BaseLinearCurve>\n");
                        break;
                case custom_curve:
                        buf = uf_markup_buf(buf,
				"<BaseCustomCurve Current='%s'>\n", current);
			buf = uf_markup_buf(buf, curveBuf);
			buf = uf_markup_buf(buf, "</BaseCustomCurve>\n");
                        break;
                case camera_curve:
                        buf = uf_markup_buf(buf,
				"<BaseCameraCurve Current='%s'>\n", current);
			buf = uf_markup_buf(buf, curveBuf);
			buf = uf_markup_buf(buf, "</BaseCameraCurve>\n");
                        break;
                default:
                        buf = uf_markup_buf(buf,
				"<BaseCurve Current='%s'>%s\n", current,
				c->BaseCurve[i].name);
			buf = uf_markup_buf(buf, curveBuf);
			buf = uf_markup_buf(buf, "</BaseCurve>\n");
            }
        }
        g_free(curveBuf);
    }
    for (i=0; i<c->curveCount; i++) {
        char *curveBuf = curve_buffer(&c->curve[i]);
        /* Write curve if it is non-default and we are not writing to .ufraw */
	/* But ALWAYS write the current curve */
        if ( c->curveIndex==i || (curveBuf!=NULL && IDFilename==NULL) ) {
	    if (curveBuf==NULL) curveBuf = g_strdup("");
            current = i==c->curveIndex?"yes":"no";
            switch (i) {
                case manual_curve:
                        buf = uf_markup_buf(buf,
				"<ManualCurve Current='%s'>\n", current);
			buf = uf_markup_buf(buf, curveBuf);
			buf = uf_markup_buf(buf, "</ManualCurve>\n");
                        break;
                case linear_curve:
                        buf = uf_markup_buf(buf,
				"<LinearCurve Current='%s'>\n", current);
			buf = uf_markup_buf(buf, curveBuf);
			buf = uf_markup_buf(buf, "</LinearCurve>\n");
                        break;
                default:
                        buf = uf_markup_buf(buf,
				"<Curve Current='%s'>%s\n", current,
				c->curve[i].name);
			buf = uf_markup_buf(buf, curveBuf);
			buf = uf_markup_buf(buf, "</Curve>\n");
            }
        }
        g_free(curveBuf);
    }
    for (j=0; j<profile_types; j++) {
        type = j==in_profile ? "InputProfile" :
                j==out_profile ? "OutputProfile" : "Error";
	/* The default sRGB profile is conf_default.profile[j][0] */
        if ( c->profileIndex[j]==0 || ( IDFilename==NULL &&
             ( c->profile[j][0].gamma!=conf_default.profile[0][0].gamma ||
               c->profile[j][0].linear!=conf_default.profile[0][0].linear ||
               c->profile[j][0].useMatrix!=
		   conf_default.profile[0][0].useMatrix ) ) ) {
            current = c->profileIndex[j]==0 ? "yes" : "no";
            buf = uf_markup_buf(buf, "<sRGB%s Current='%s'>\n", type, current);
            if (c->profile[j][0].gamma!=conf_default.profile[j][0].gamma)
                buf = uf_markup_buf(buf,
			"\t<Gamma>%lf</Gamma>\n", c->profile[j][0].gamma);
            if (c->profile[j][0].linear!=conf_default.profile[j][0].linear)
                buf = uf_markup_buf(buf, "\t<Linearity>%lf</Linearity>\n",
                        c->profile[j][0].linear);
            if (c->profile[j][0].useMatrix!=
		    conf_default.profile[j][0].useMatrix)
                buf = uf_markup_buf(buf,
			"\t<UseColorMatrix>%d</UseColorMatrix>\n",
                        c->profile[j][0].useMatrix);
            buf = uf_markup_buf(buf, "</sRGB%s>\n", type);
        }
	/* While the default ICC profile is conf_default.profile[j][1] */
        for (i=1; i<c->profileCount[j]; i++) {
	    if (IDFilename!=NULL && i!=c->profileIndex[j])
		continue;
            current = i==c->profileIndex[j]?"yes":"no";
            buf = uf_markup_buf(buf, "<%s Current='%s'>%s\n",
		    type, current, c->profile[j][i].name);
	    char *utf8 = g_filename_to_utf8(c->profile[j][i].file,
		    -1, NULL, NULL, NULL);
	    if (utf8==NULL) utf8 = g_strdup("Unknown file name");
            buf = uf_markup_buf(buf, "\t<File>%s</File>\n", utf8);
	    g_free(utf8);
            buf = uf_markup_buf(buf, "\t<ProductName>%s</ProductName>\n",
                    c->profile[j][i].productName);
            if (c->profile[j][i].gamma!=conf_default.profile[j][1].gamma)
                buf = uf_markup_buf(buf,
			"\t<Gamma>%lf</Gamma>\n", c->profile[j][i].gamma);
            if (c->profile[j][i].linear!=conf_default.profile[j][1].linear)
                buf = uf_markup_buf(buf, "\t<Linearity>%lf</Linearity>\n",
                        c->profile[j][i].linear);
            if (c->profile[j][i].useMatrix!=
		    conf_default.profile[j][1].useMatrix)
                buf = uf_markup_buf(buf,
			"\t<UseColorMatrix>%d</UseColorMatrix>\n",
                        c->profile[j][i].useMatrix);
            buf = uf_markup_buf(buf, "</%s>\n", type);
        }
    }
    if (c->intent!=conf_default.intent)
        buf = uf_markup_buf(buf, "<Intent>%d</Intent>\n", c->intent);
    /* We always write the Make and Mode information
     * to know if the WB setting is relevant */
    buf = uf_markup_buf(buf, "<Make>%s</Make>\n", c->make);
    buf = uf_markup_buf(buf, "<Model>%s</Model>\n", c->model);
    if (IDFilename!=NULL) {
	if (strcmp(c->darkframeFile, conf_default.darkframeFile)!=0)
	    buf = uf_markup_buf(buf,
		    "<DarkframeFile>%s</DarkframeFile>\n", c->darkframeFile);
	buf = uf_markup_buf(buf, "<Timestamp>%s</Timestamp>\n", c->timestamp);
	buf = uf_markup_buf(buf, "<ISOSpeed>%s</ISOSpeed>\n", c->isoText);
	buf = uf_markup_buf(buf, "<Shutter>%s</Shutter>\n", c->shutterText);
	buf = uf_markup_buf(buf, "<Aperture>%s</Aperture>\n", c->apertureText);
	buf = uf_markup_buf(buf, "<FocalLength>%s</FocalLength>\n",
		c->focalLenText);
	buf = uf_markup_buf(buf, "<FocalLength35>%s</FocalLength35>\n",
		c->focalLen35Text);
	if (strlen(c->lensText)>0)
	    buf = uf_markup_buf(buf, "<Lens>%s</Lens>\n", c->lensText);
	buf = uf_markup_buf(buf, "<EXIFSource>%s</EXIFSource>\n",
		c->exifSource);
	char *log = ufraw_message(UFRAW_GET_LOG, NULL);
	if (log!=NULL) {
	    char *utf8 = g_filename_to_utf8(log, -1, NULL, NULL, NULL);
	    if (utf8!=NULL)
		buf = uf_markup_buf(buf, "<Log>\n%s</Log>\n", utf8);
	    g_free(utf8);
	}
	/* As long as darkframe is not in the GUI we save it only to ID files.*/
    }
    buf = uf_markup_buf(buf, "</UFRaw>\n");
    uf_reset_locale(locale);
    if (confBuffer==NULL) {
	char *confFilename;
	FILE *out;
	if (IDFilename==NULL) {
            const char *hd = uf_get_home_dir();
            confFilename = g_build_filename(hd, ".ufrawrc", NULL);
	} else
	    confFilename = g_strdup(IDFilename);
        if ( (out=fopen(confFilename, "w"))==NULL ) {
            ufraw_message(UFRAW_ERROR,
                    _("Can't open file %s for writing\n%s\n"),
                    confFilename, strerror(errno) );
	    g_free(confFilename);
	    g_free(buf);
            return UFRAW_ERROR;
        }
	fputs(buf, out);
	fclose(out);
        g_free(confFilename);
	g_free(buf);
    } else {
	*confBuffer = buf;
    }
    return UFRAW_SUCCESS;
}

/* Copy the image manipulation options from *src to *dst */
void conf_copy_image(conf_data *dst, const conf_data *src)
{
    int i, j;

    dst->interpolation = src->interpolation;
    g_strlcpy(dst->wb, src->wb, max_name);
    dst->WBTuning = src->WBTuning;
    dst->temperature = src->temperature;
    dst->green = src->green;
    for (i=0; i<4; i++) dst->chanMul[i] = src->chanMul[i];
    /* make and model are 'part of' ChanMul,
     * since on different make and model ChanMul are meaningless */
    g_strlcpy(dst->make, src->make, max_name);
    g_strlcpy(dst->model, src->model, max_name);
    dst->exposure = src->exposure;
    dst->saturation = src->saturation;
    dst->black = src->black;
    dst->autoExposure = src->autoExposure;
    dst->autoBlack = src->autoBlack;
    dst->unclip = src->unclip;
    g_strlcpy(dst->darkframeFile, src->darkframeFile, max_path);
    /* We only copy the current BaseCurve */
    if (src->BaseCurveIndex<=camera_curve) {
        dst->BaseCurveIndex = src->BaseCurveIndex;
	if (src->BaseCurveIndex==manual_curve)
	    dst->BaseCurve[manual_curve] = src->BaseCurve[manual_curve];
    } else {
	/* For non-standard curves we look for a curve with the same name
	 * and override it, assuming it is the same curve. */
	for (i=camera_curve+1; i<dst->BaseCurveCount; i++) {
	    if (!strcmp(dst->BaseCurve[i].name,
			src->BaseCurve[src->BaseCurveIndex].name)) {
		dst->BaseCurve[i] = src->BaseCurve[src->BaseCurveIndex];
		dst->BaseCurveIndex = i;
		break;
	    }
	}
	/* If the curve was not found we add it. */
	if (i==dst->BaseCurveCount) {
	    /* If there is no more room we throw away the last curve. */
	    if (dst->BaseCurveCount==max_curves) dst->BaseCurveCount--;
	    dst->BaseCurve[dst->BaseCurveCount] =
		src->BaseCurve[src->BaseCurveIndex];
	    dst->BaseCurveIndex = dst->BaseCurveCount;
	    dst->BaseCurveCount++;
	}
    }
    /* We only copy the current curve */
    if (src->curveIndex<=linear_curve) {
        dst->curveIndex = src->curveIndex;
	if (src->curveIndex==manual_curve)
	    dst->curve[manual_curve] = src->curve[manual_curve];
    } else {
	/* For non-standard curves we look for a curve with the same name
	 * and override it, assuming it is the same curve. */
	for (i=camera_curve+1; i<dst->curveCount; i++) {
	    if (!strcmp(dst->curve[i].name, src->curve[src->curveIndex].name)) {
		dst->curve[i] = src->curve[src->curveIndex];
		dst->curveIndex = i;
		break;
	    }
	}
	/* If the curve was not found we add it. */
	if (i==dst->curveCount) {
	    /* If there is no more room we throw away the last curve. */
	    if (dst->curveCount==max_curves) dst->curveCount--;
	    dst->curve[dst->curveCount] = src->curve[src->curveIndex];
	    dst->curveIndex = dst->curveCount;
	    dst->curveCount++;
	}
    }
    /* We only copy the current input/output profile */
    for (j=0; j<2; j++) {
	if (src->profileIndex[j]==0) {
	    dst->profileIndex[j] = src->profileIndex[j];
	    dst->profile[j][0] = src->profile[j][0];
	} else {
	    /* For non-standard profiles we look for a profile with the same
	     * name and override it, assuming it is the same profile. */
	    for (i=1; i<dst->profileCount[j]; i++) {
		if (!strcmp(dst->profile[j][i].name,
			    src->profile[j][src->profileIndex[j]].name)) {
		    dst->profile[j][i] = src->profile[j][src->profileIndex[j]];
		    dst->profileIndex[j] = i;
		    break;
		}
	    }
	    /* If the profile was not found we add it. */
	    if (i==dst->profileCount[j]) {
		/* If there is no more room we throw away the last profile. */
		if (dst->profileCount[j]==max_profiles) dst->profileCount[j]--;
		dst->profile[j][dst->profileCount[j]] =
		    src->profile[j][src->profileIndex[j]];
		dst->profileIndex[j] = dst->profileCount[j];
		dst->profileCount[j]++;
	    }
	}
    }
    dst->intent = src->intent;
}

/* Copy the 'save options' from *src to *dst */
void conf_copy_save(conf_data *dst, const conf_data *src)
{
    /* Filenames get special treatment and are not simply copied
    g_strlcpy(dst->inputFilename, src->inputFilename, max_path);
    g_strlcpy(dst->outputFilename, src->outputFilename, max_path);
    g_strlcpy(dst->outputPath, src->outputPath, max_path);
    */
    g_strlcpy(dst->inputURI, src->inputURI, max_path);
    g_strlcpy(dst->inputModTime, src->inputModTime, max_name);
    dst->type = src->type;
    dst->compression = src->compression;
    dst->createID = src->createID;
    dst->embedExif = src->embedExif;
    dst->shrink = src->shrink;
    dst->size = src->size;
    dst->overwrite = src->overwrite;
    dst->losslessCompress = src->losslessCompress;
    dst->embeddedImage = src->embeddedImage;
}

int conf_set_cmd(conf_data *conf, const conf_data *cmd)
{
    if (cmd->overwrite!=-1) conf->overwrite = cmd->overwrite;
    if (cmd->unclip!=-1) conf->unclip = cmd->unclip;
    if (cmd->losslessCompress!=-1)
	conf->losslessCompress = cmd->losslessCompress;
    if (cmd->embedExif!=-1) conf->embedExif = cmd->embedExif;
    if (cmd->embeddedImage!=-1) conf->embeddedImage = cmd->embeddedImage;
    if (cmd->compression!=NULLF) conf->compression = cmd->compression;
    if (cmd->autoExposure) {
	conf->autoExposure = cmd->autoExposure;
    }
    if (cmd->exposure!=NULLF) {
	conf->exposure = cmd->exposure;
	conf->autoExposure = disabled_state;
    }
    if (strlen(cmd->wb)>0) g_strlcpy(conf->wb, cmd->wb, max_name);
    if (cmd->temperature!=NULLF) conf->temperature = cmd->temperature;
    if (cmd->green!=NULLF) conf->green = cmd->green;
    if (cmd->temperature!=NULLF || cmd->green!=NULLF)
	g_strlcpy(conf->wb, manual_wb, max_name);
    if (cmd->profile[0][0].gamma!=NULLF)
	conf->profile[0][conf->profileIndex[0]].gamma =
		cmd->profile[0][0].gamma;
    if (cmd->profile[0][0].linear!=NULLF)
	conf->profile[0][conf->profileIndex[0]].linear =
		cmd->profile[0][0].linear;
    if (cmd->saturation!=NULLF)
	conf->saturation=cmd->saturation;
    if (cmd->BaseCurveIndex>=0) conf->BaseCurveIndex = cmd->BaseCurveIndex;
    if (cmd->curveIndex>=0) conf->curveIndex = cmd->curveIndex;
    if (cmd->autoBlack) {
	conf->autoBlack = cmd->autoBlack;
    }
    if (cmd->black!=NULLF) {
	CurveDataSetPoint(&conf->curve[conf->curveIndex],
		0, cmd->black, 0);
	conf->autoBlack = disabled_state;
    }
    if (cmd->interpolation>=0) conf->interpolation = cmd->interpolation;
    if (cmd->shrink!=NULLF) {
        conf->shrink = cmd->shrink;
        conf->size = 0;
        if (conf->interpolation==half_interpolation)
            conf->interpolation = ahd_interpolation;
    }
    if (cmd->size!=NULLF) {
        conf->size = cmd->size;
        conf->shrink = 1;
        if (conf->interpolation==half_interpolation)
            conf->interpolation = ahd_interpolation;
    }
    if (cmd->type>=0) conf->type = cmd->type;
    if (cmd->createID>=0) conf->createID = cmd->createID;
    if (strlen(cmd->darkframeFile)>0)
	g_strlcpy(conf->darkframeFile, cmd->darkframeFile, max_path);
    if (cmd->darkframe!=NULL)
	conf->darkframe = cmd->darkframe;
    if (strlen(cmd->outputPath)>0)
	g_strlcpy(conf->outputPath, cmd->outputPath, max_path);
    if (strlen(cmd->outputFilename)>0) {
        if ( conf->createID!=no_id && !strcmp(cmd->outputFilename,"-") &&
	     !cmd->embeddedImage ) {
            ufraw_message(UFRAW_ERROR, _("cannot --create-id with stdout"));
            return UFRAW_ERROR;
        }
	g_strlcpy(conf->outputFilename, cmd->outputFilename, max_path);
    }
    return UFRAW_SUCCESS;
}

/* Following are global strings and functions used by both 'ufraw' and
 * 'ufraw-batch'. ufraw_conf.c is a good home for them since they are
 * closely related to the other configuration functions. */


char *helpText[] = {
N_("UFRaw "), VERSION, N_(" - Unidentified Flying Raw converter for digital camera images.\n"),
"\n",
N_("Usage: ufraw [ options ... ] [ raw-image-files ... ]\n"),
N_("       ufraw-batch [ options ... ] [ raw-image-files ... ]\n"),
N_("       ufraw [ options ... ] [ default-directory ]\n"),
"\n",
N_("By default 'ufraw' displays a preview window for each raw image allowing\n"
"the user to tweak the image parameters before saving. If no raw images\n"
"are given at the command line, UFRaw will display a file chooser dialog.\n"
"To process the images with no questions asked (and no preview) use\n"
"'ufraw-batch'.\n"),
"\n",
N_("The input files can be either raw images or ufraw's ID files. ID file\n"
"contain a raw image filename and the parameters for handling the image.\n"
"One can also use an ID file with the option:\n"),
"\n",
N_("--conf=ID-file        Apply the parameters in ID-file to other raw images.\n"),
"\n",
N_("The rest of the options are separated into two groups.\n"),
N_("The options which are related to the image manipulation are:\n"),
"\n",
N_("--wb=camera|auto      White balance setting.\n"),
N_("--temperature=TEMP    Color temperature in Kelvins (2000 - 7000).\n"),
N_("--green=GREEN         Green color normalization.\n"),
N_("--base-curve=manual|linear|camera|custom|CURVE\n"
"                      Type of base tone curve to use. CURVE can be any\n"
"                      curve that was previously loaded in the GUI.\n"
"                      (default camera if such exsists, linear otherwise).\n"),
N_("--base-curve-file=file\n"
"                      Use base tone curve included in specified file.\n"
"                      Overrides --base-curve option.\n"),
N_("--curve=manual|linear|CURVE\n"
"                      Type of luminosity curve to use. CURVE can be any\n"
"                      curve that was previously loaded in the GUI.\n"
"                      (default linear).\n"),
N_("--curve-file=file     Use luminosity curve included in specified file.\n"
"                      Overrides --curve option.\n"),
N_("--[un]clip            Unclip [clip] highlights (default clip).\n"),
N_("--gamma=GAMMA         Gamma adjustment of the base curve (default 0.45).\n"),
N_("--linearity=LINEARITY Linearity of the base curve (default 0.10).\n"),
N_("--saturation=SAT      Saturation adjustment (default 1.0, 0 for B&W output).\n"),
N_("--exposure=auto|EXPOSURE\n"
"                      Auto exposure or exposure correction in EV (default 0).\n"),
N_("--black-point=auto|BLACK\n"
"                      Auto black-point or black-point value (default 0).\n"),
N_("--interpolation=ahd|vng|four-color|bilinear\n"
"                      Interpolation algorithm to use (default ahd).\n"),
"\n",
N_("The options which are related to the final output are:\n"),
"\n",
N_("--shrink=FACTOR       Shrink the image by FACTOR (default 1).\n"),
N_("--size=SIZE           Downsize max(height,width) to SIZE.\n"),
N_("--out-type=ppm8|ppm16|tiff8|tiff16|jpeg\n"
"                      Output file format (default ppm8).\n"),
N_("--create-id=no|also|only\n"
"                      Create no|also|only ID file (default no).\n"),
N_("--compression=VALUE   JPEG compression (0-100, default 85).\n"),
N_("--[no]exif            Embed exif in output JPEG (default embed exif).\n"),
N_("--[no]zip             Enable [disable] TIFF zip compression (default nozip).\n"),
N_("--embedded-image      Extract the preview image embedded in the raw file\n"
"                      instead of converting the raw image.\n"),
N_("--out-path=PATH       PATH for output file (default use input file's path).\n"),
N_("--output=FILE         Output file name, use '-' to output to stdout.\n"),
N_("--darkframe=FILE      Use FILE for raw darkframe subtraction.\n"),
N_("--overwrite           Overwrite existing files without asking (default no).\n"),
"\n",
N_("UFRaw first reads the setting from the resource file $HOME/.ufrawrc.\n"
"Then, if an ID file is specified it setting are read. Next, the setting from\n"
"the --conf option are taken, ignoring input/output filenames in the ID file.\n"
"Lastly, the options from the command line are set. In batch mode, the second\n"
"group of options is NOT read from the resource file.\n"),
"\n",
N_("Last, but not least, --version displays the version number and compilation\n"
"options for ufraw and --help displays this help message and exits.\n"),
"END"
};

char versionText[]= 
"%s " VERSION "\n" 
 
"EXIF " 
#ifdef HAVE_LIBEXIF 
"enabled.\n" 
#else 
"disabled.\n" 
#endif 
 
"EXIV2 " 
#ifdef HAVE_EXIV2 
"enabled.\n" 
#else 
"disabled.\n" 
#endif 
 
"JPEG "  
#ifdef HAVE_LIBJPEG 
"enabled.\n" 
#else 
"disabled.\n" 
#endif 
 
"TIFF "  
#ifdef HAVE_LIBTIFF 
"enabled.\n" 
#else 
"disabled.\n" 
#endif 
 
"ZIP "  
#ifdef HAVE_LIBZ 
"enabled.\n" 
#else 
"disabled.\n" 
#endif 
""; 

/* ufraw_process_args returns values:
 * -1     : an error occurred.
 * 0      : --help or --version text were printed.
 * optint : the index in argv of the first argv-element that is not an option.
 */
int ufraw_process_args(int *argc, char ***argv, conf_data *cmd, conf_data *rc)
{
    int index=0, c, i;
    char *base;
    char *wbName=NULL;
    char *baseCurveName=NULL, *baseCurveFile=NULL,
	 *curveName=NULL, *curveFile=NULL, *outTypeName=NULL,
         *createIDName=NULL, *outPath=NULL, *output=NULL, *conf=NULL,
         *interpolationName=NULL, *darkframeFile=NULL;
    static struct option options[] = {
        {"wb", 1, 0, 'w'},
        {"temperature", 1, 0, 't'},
        {"green", 1, 0, 'g'},
        {"base-curve", 1, 0, 'B'},
        {"base-curve-file", 1, 0, 'S'},
        {"curve", 1, 0, 'c'},
        {"curve-file", 1, 0, 'f'},
        {"gamma", 1, 0, 'G'},
        {"linearity", 1, 0, 'L'},
        {"saturation", 1, 0, 's'},
        {"exposure", 1, 0, 'e'},
        {"black-point", 1, 0, 'k'},
        {"interpolation", 1, 0, 'i'},
        {"shrink", 1, 0, 'x'},
        {"size", 1, 0, 'X'},
        {"compression", 1, 0, 'j'},
        {"out-type", 1, 0, 'T'},
        {"create-id", 1, 0, 'I'},
        {"out-path", 1, 0, 'p'},
        {"output", 1, 0, 'o'},
        {"darkframe",1,0,'a'},
        {"conf", 1, 0, 'C'},
/* Binary flags that don't have a value are here at the end */
        {"unclip", 0, 0, 'u'},
        {"clip", 0, 0, 'U'},
        {"zip", 0, 0, 'z'},
        {"nozip", 0, 0, 'Z'},
        {"overwrite", 0, 0, 'O'},
	{"exif", 0, 0, 'E'},
	{"noexif", 0, 0, 'F'},
	{"embedded-image", 0, 0, 'm'},
        {"help", 0, 0, 'h'},
	{"version", 0, 0, 'v'},
	{"batch", 0, 0, 'b'},
        {0, 0, 0, 0}
    };
    void *optPointer[] = { &wbName, &cmd->temperature, &cmd->green,
	&baseCurveName, &baseCurveFile, &curveName, &curveFile,
        &cmd->profile[0][0].gamma, &cmd->profile[0][0].linear,
	&cmd->saturation,
        &cmd->exposure, &cmd->black, &interpolationName,
	&cmd->shrink, &cmd->size, &cmd->compression,
	&outTypeName, &createIDName, &outPath, &output, &darkframeFile, &conf };
    cmd->autoExposure = disabled_state;
    cmd->autoBlack = disabled_state;
    cmd->unclip=-1;
    cmd->losslessCompress=-1;
    cmd->overwrite=-1;
    cmd->embedExif=-1;
    cmd->embeddedImage=FALSE;
    cmd->profile[0][0].gamma=NULLF;
    cmd->profile[0][0].linear=NULLF;
    cmd->saturation=NULLF;
    cmd->black=NULLF;
    cmd->exposure=NULLF;
    cmd->temperature=NULLF;
    cmd->green=NULLF;
    cmd->shrink = NULLF;
    cmd->size = NULLF;
    cmd->compression=NULLF;

    while (1) {
        c = getopt_long (*argc, *argv, "h", options, &index);
        if (c == -1) break;
        switch (c) {
        case 'e':
	    if (!strcmp(optarg, "auto")) {
		cmd->autoExposure = apply_state;
		break;
	    }
        case 'k':
	    if (!strcmp(optarg, "auto")) {
		cmd->autoBlack = apply_state;
		break;
	    }
        case 't':
        case 'g':
        case 'G':
        case 'L':
        case 's':
        case 'd':
            if (sscanf(optarg, "%lf", (double *)optPointer[index])==0) {
                ufraw_message(UFRAW_ERROR,
			_("'%s' is not a valid value for the --%s option."),
			optarg, options[index].name);
                return -1;
            }
            break;
        case 'x':
        case 'X':
        case 'j':
            if (sscanf(optarg, "%d", (int *)optPointer[index])==0) {
                ufraw_message(UFRAW_ERROR,
			_("'%s' is not a valid value for the --%s option."),
			optarg, options[index].name);
                return -1;
            }
            break;
        case 'w':
        case 'B':
        case 'S':
        case 'c':
        case 'f':
        case 'i':
        case 'T':
        case 'I':
        case 'p':
        case 'o':
        case 'a':
        case 'C':
            *(char **)optPointer[index] = optarg;
            break;
        case 'u': cmd->unclip = TRUE; break;
        case 'U': cmd->unclip = FALSE; break;
        case 'O': cmd->overwrite = TRUE; break;
	case 'm': cmd->embeddedImage = TRUE; break;
        case 'z':
#ifdef HAVE_LIBZ
            cmd->losslessCompress = TRUE; break;
#else
            ufraw_message(UFRAW_ERROR,
                _("ufraw was build without ZIP support."));
            return -1;
#endif
        case 'Z': cmd->losslessCompress = FALSE; break;
        case 'E': cmd->embedExif = TRUE; break;
        case 'F': cmd->embedExif = FALSE; break;
        case 'h':
	    for (i=0; strcmp(helpText[i],"END")!=0; i++)
		ufraw_message(UFRAW_SET_WARNING, _(helpText[i]));
	    ufraw_message(UFRAW_WARNING,
		    ufraw_message(UFRAW_GET_WARNING, NULL));
            return 0;
	case 'v':
	    base = g_path_get_basename(*argv[0]);
            ufraw_message(UFRAW_WARNING, versionText, base);
	    g_free(base);
            return 0;
	case 'b':
            ufraw_message(UFRAW_ERROR,
                _("--batch is obselete. use ufraw-batch instead."));
            return -1;
        case '?': /* invalid option. Warning printed by getopt() */
            return -1;
        default:
            ufraw_message(UFRAW_ERROR, "getopt returned "
                       "character code 0%o ??", c);
            return -1;
        }
    }
    g_strlcpy(cmd->wb, "", max_name);
    if (wbName!=NULL) {
        if (!strcmp(wbName, "camera"))
	    g_strlcpy(cmd->wb, camera_wb, max_name);
        else if (!strcmp(wbName, "auto"))
	    g_strlcpy(cmd->wb, auto_wb, max_name);
        else {
            ufraw_message(UFRAW_ERROR,
                    _("'%s' is not a valid white balance setting."), wbName);
            return -1;
        }
	if (cmd->temperature!=NULLF || cmd->green!=NULLF) {
            ufraw_message(UFRAW_WARNING,
		    _("--temperature and --green options override the --wb=%s option."), wbName);
	    g_strlcpy(cmd->wb, "", max_name);
	}
    }
    cmd->BaseCurveIndex = -1;
    if (baseCurveFile!=NULL) {
        if (cmd->BaseCurveCount == max_curves) {
            ufraw_message(UFRAW_ERROR,
		    _("failed to load curve from %s, too many configured base curves"), baseCurveFile);
            return -1;
        }
        cmd->BaseCurveIndex = cmd->BaseCurveCount;
        if (curve_load(&(rc->BaseCurve[cmd->BaseCurveIndex]), baseCurveFile)
		== UFRAW_ERROR) {
            ufraw_message(UFRAW_ERROR,
                    _("failed to load curve from %s"), baseCurveFile);
            return -1;
        }
        cmd->BaseCurveCount++;
    } else if (baseCurveName!=NULL) {
        if (!strcmp(baseCurveName, "manual"))
	    cmd->BaseCurveIndex=manual_curve;
        else if (!strcmp(baseCurveName, "linear"))
	    cmd->BaseCurveIndex=linear_curve;
        else if (!strcmp(baseCurveName, "custom"))
	    cmd->BaseCurveIndex=custom_curve;
        else if (!strcmp(baseCurveName, "camera"))
	    cmd->BaseCurveIndex=camera_curve;
        else {
            cmd->BaseCurveIndex = -1;
            for( i = camera_curve + 1; i < rc->BaseCurveCount; i++ ) {
                if( !strcmp(baseCurveName, rc->BaseCurve[i].name )) {
                    cmd->BaseCurveIndex = i;
                    break;
                }
            }
            if( cmd->BaseCurveIndex == -1 ) {
                ufraw_message(UFRAW_ERROR,
                        _("'%s' is not a valid base curve name."), baseCurveName);
                return -1;
            }
        }
    }
    cmd->curveIndex = -1;
    if (curveFile!=NULL) {
        if (cmd->curveCount == max_curves) {
            ufraw_message(UFRAW_ERROR,
		    _("failed to load curve from %s, too many configured curves"), curveFile);
            return -1;
        }
        cmd->curveIndex = cmd->curveCount;
        if (curve_load(&(rc->curve[cmd->curveIndex]), curveFile) == UFRAW_ERROR)
	{
            ufraw_message(UFRAW_ERROR,
                    _("failed to load curve from %s"), curveFile);
            return -1;
        }
        cmd->curveCount++;
    } else if (curveName!=NULL) {
        if (!strcmp(curveName, "manual")) cmd->curveIndex=manual_curve;
        else if (!strcmp(curveName, "linear")) cmd->curveIndex=linear_curve;
        else {
            cmd->curveIndex = -1;
            for( i = linear_curve + 1; i < rc->curveCount; i++ ) {
                if( !strcmp( curveName, rc->curve[i].name )) {
                    cmd->curveIndex = i;
                    break;
                }
            }
            if( cmd->curveIndex == -1 ) {
                ufraw_message(UFRAW_ERROR,
                        _("'%s' is not a valid curve name."), curveName);
                return -1;
            }
        }
    }
    cmd->interpolation = -1;
    if (interpolationName!=NULL) {
	/* Keep compatebility with old numbers from ufraw-0.5 */
        if (!strcmp(interpolationName, "full"))
            cmd->interpolation = vng_interpolation;
        else if (!strcmp(interpolationName, "quick"))
            cmd->interpolation = bilinear_interpolation;
	else cmd->interpolation = conf_find_name(interpolationName,
		    interpolationNames);
	if (cmd->interpolation<0 || cmd->interpolation==half_interpolation) {
            ufraw_message(UFRAW_ERROR,
                _("'%s' is not a valid interpolation option."),
                interpolationName);
            return -1;
        }
    }
    if (cmd->shrink!=NULLF && cmd->size!=NULLF) {
        ufraw_message(UFRAW_ERROR,
                        _("you can not specify both --shrink and --size"));
        return -1;
    }
    cmd->type = -1;
    if (outTypeName!=NULL && !cmd->embeddedImage) {
        if (!strcmp(outTypeName, "ppm8"))
            cmd->type = ppm8_type;
        else if (!strcmp(outTypeName, "ppm16"))
            cmd->type = ppm16_type;
        else if (!strcmp(outTypeName, "tiff8"))
#ifdef HAVE_LIBTIFF
        cmd->type = tiff8_type;
#else
        {
            ufraw_message(UFRAW_ERROR,
		    _("ufraw was build without TIFF support."));
            return -1;
        }
#endif
        else if (!strcmp(outTypeName, "tiff16"))
#ifdef HAVE_LIBTIFF
        cmd->type = tiff16_type;
#else
        {
            ufraw_message(UFRAW_ERROR,
		    _("ufraw was build without TIFF support."));
            return -1;
        }
#endif
        else if (!strcmp(outTypeName, "jpeg"))
#ifdef HAVE_LIBJPEG
        cmd->type = jpeg_type;
#else
        {
            ufraw_message(UFRAW_ERROR,
		    _("ufraw was build without JPEG support."));
            return -1;
        }
#endif
        else {
            ufraw_message(UFRAW_ERROR,
		    _("'%s' is not a valid output type."), outTypeName);
            return -1;
        }
    }
    if (cmd->embeddedImage) {
#ifndef HAVE_LIBJPEG
	ufraw_message(UFRAW_ERROR, _("ufraw was build without JPEG support."));
	return -1;
#endif
        if ( outTypeName==NULL || strcmp(outTypeName, "jpeg")==0 )
	    cmd->type = embedded_jpeg_type;
#ifdef HAVE_LIBPNG
        else if ( strcmp(outTypeName, "png")==0 )
	    cmd->type = embedded_png_type;
#endif
	else {
            ufraw_message(UFRAW_ERROR,
		    _("'%s' is not a valid output type for embedded image."),
		    outTypeName);
            return -1;
        }
    }
    cmd->createID = -1;
    if (createIDName!=NULL) {
        if (!strcmp(createIDName, "no"))
            cmd->createID = no_id;
	else if (!strcmp(createIDName, "also"))
            cmd->createID = also_id;
	else if (!strcmp(createIDName, "only"))
            cmd->createID = only_id;
        else {
            ufraw_message(UFRAW_ERROR,
		    _("'%s' is not a valid create-id option."), createIDName);
            return -1;
        }
    }
    g_strlcpy(cmd->outputPath, "", max_path);
    if (outPath!=NULL) {
        if (g_file_test(outPath, G_FILE_TEST_IS_DIR))
            g_strlcpy(cmd->outputPath, outPath, max_path);
        else {
            ufraw_message(UFRAW_ERROR, _("'%s' is not a valid path."), outPath);
            return -1;
        }
    }
    g_strlcpy(cmd->outputFilename, "", max_path);
    if (output!=NULL) {
        if (*argc-optind>1) {
            ufraw_message(UFRAW_ERROR,
		    _("cannot output more than one file to the same output"));
            return -1;
        }
        g_strlcpy(cmd->outputFilename, output, max_path);
    }
    g_strlcpy(cmd->darkframeFile, "", max_path);
    cmd->darkframe = NULL;
    if (darkframeFile!=NULL) {
	char *df = uf_file_set_absolute(darkframeFile);
	cmd->darkframe = ufraw_load_darkframe(df);
        g_strlcpy(cmd->darkframeFile, df, max_path);
	g_free(df);
    }    
    /* cmd->inputFilename is used to store the conf file */
    g_strlcpy(cmd->inputFilename, "", max_path);
    if (conf!=NULL)
        g_strlcpy(cmd->inputFilename, conf, max_path);

    return optind;
}
