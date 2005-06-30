/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 * by udi Fuchs,
 *
 * ufraw_routines.c - general routines
 *
 * UFRaw is licensed under the GNU General Public License.
 * It uses "dcraw" code to do the actual raw decoding.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(HAVE_CANONICALIZE_FILE_NAME) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE /* needed for canonicalize_file_name() */
#endif
#include <stdio.h>
#include <stdlib.h> /* needed for getenv() */
#include <string.h>
#include <ctype.h> /* needed for isspace() */
#include <errno.h>
#include <math.h>
#include <glib.h>
#include "ufraw.h"
#include "blackbody.h"

/* we start by some general purpose functions that mostly take care of
 * making the rest of the code platform independ */

const char *uf_get_home_dir()
{
    const char *hd = g_get_home_dir();
    if (hd==NULL)
#ifdef WIN32
        hd = "C:\\";
#else
        hd = "/";
#endif
    return hd;
}

char *uf_file_set_type(const char *filename, const char *type)
{
    char *file, *cp;
    if ( (cp=strrchr(filename, '.'))==NULL)
	file = g_strconcat(filename, type);
    else {
	file = g_new(char, cp - filename + strlen(type) + 1);
	g_strlcpy(file, filename, cp - filename + 1);
	g_strlcpy(file + (cp - filename), type, strlen(type) + 1);
    }
    return file;
}

/* Make sure filename has asolute path */
char *uf_file_set_absolute(const char *filename)
{
#ifdef HAVE_CANONICALIZE_FILE_NAME
    char *path = g_path_get_dirname(filename);
    char *base = g_path_get_basename(filename);
    char *canon = canonicalize_file_name(path);
    char *abs = g_build_filename(canon, base, NULL);
    g_free(path);
    g_free(base);
    g_free(canon);
    return abs;
#else
    // We could use realpath(filename, NULL)
    // if we add a check that it is not buggy
    // This code does not remove '/./' or '/../'
    if (g_path_is_absolute(filename)) {
	return g_strdup(filename);
    } else {
	char *cd = g_get_current_dir();
	char *fn = g_build_filename(cd, filename, NULL);
	g_free(cd);
	return fn;
    }
#endif
}

double uf_nan()
{
#ifdef HAVE_NAN
    return nan("");
#else
    return strtod("NaN", NULL);
#endif
}

const cfg_data cfg_default = {
    sizeof(cfg_data), 5, "", "", "",
    load_default, load_preserve, load_preserve, ppm8_type, 85, no_id,
    TRUE, /* Embed exif data */
    camera_wb, rgb_histogram, linear_histogram, 128, linear_histogram, 128,
    { TRUE, TRUE, TRUE, TRUE, TRUE, TRUE }, full_interpolation, 1, 0,
    4750, 1.2, /* temperature, green */
    0.0, 1.0, 0.0, /* exposure, saturation, black */
    FALSE /* Auto exposure */,
    FALSE /* Auto black */,
    FALSE /* Auto curve */,
    FALSE /* Unclip highlights */,
    FALSE /* Overexposure indicator */,
    FALSE /* Underexposure indicator */,
    FALSE /* Overwrite exsisting files without asking */,
    FALSE /* Lossless compression */,
    /* Curves defaults */
    camera_curve, camera_curve+1,
    { { "Manual curve", TONE_CURVE, 0.0, 1.0, 0.0, 1.0, 1.0,
	  2 , { { 0.0, 0.0 }, { 1.0, 1.0 } } },
      { "Linear curve", TONE_CURVE, 0.0, 1.0, 0.0, 1.0, 1.0,
	  2 , { { 0.0, 0.0 }, { 1.0, 1.0 } } },
      { "Camera curve", TONE_CURVE, 0.0, 1.0, 0.0, 1.0, 1.0,
	  0 , { { 0.0, 0.0 } } },
    },
    "",
    /* Profiles defaults */
    { 0, 0 } , { 1, 1 },
    { { { "sRGB", "", "", 0.45, 0.1, TRUE },
        { "Some ICC Profile", "", "", 0.45, 0.0, FALSE } },
      { { "sRGB", "", "", 0.0, 0.0, FALSE },
        { "Some ICC Profile", "", "", 0.0, 0.0, FALSE } } },
    0, /* intent */
    ""
};

#define D70_RED 2.648505
#define D70_BLUE 1.355692

const wb_data wb_preset[] = { { "Manual WB", 0, 0},
    { "Camera WB", 0, 0},
    { "Auto WB", 0, 0},
    { "Incandescent", 1.34375/D70_RED, 2.816406/D70_BLUE }, /* 3000K */
    { "Fluorescent", 1.964844/D70_RED, 2.476563/D70_BLUE }, /* 4200K */
    { "Direct sunlight", 2.0625/D70_RED, 1.597656/D70_BLUE }, /* 5200K*/
    { "Flash", 2.441406/D70_RED, 1.5/D70_BLUE }, /* 5400K */
    { "Cloudy", 2.257813/D70_RED, 1.457031/D70_BLUE }, /* 6000K */
    { "Shade", 2.613281/D70_RED, 1.277344/D70_BLUE } /* 8000K */
};
//    { "7Hi Daylight", 1.609375/1.42, 1.328125/1.25 }, /* 5500K */
//    { "7Hi Tungsten", 0.878906/1.42, 2.433594/1.25 }, /* 2800K */
//    { "7Hi Fluorescent 1", 1.664062/1.42, 2.105469/1.25 }, /* 4060K */
//    { "7Hi Fluorescent 2", 1.796875/1.42, 1.734375/1.25 }, /* 4938K */
//    { "7Hi Cloudy", 1.730469/1.42, 1.269531/1.25 } /* 5823K */ };
const int wb_preset_count = sizeof(wb_preset) / sizeof(wb_data);

const char raw_ext[]= "bay,bmq,cr2,crw,cs1,dc2,dcr,fff,hdr,k25,kdc,mrw,nef,"
        "orf,pef,raf,raw,rdc,srf,x3f,jpg,tif";

const char *file_type[] = { ".ppm", ".ppm", ".tif", ".tif", ".jpg" };

double profile_default_linear(profile_data *p)
{
    if ( !strcmp(p->name, "sRGB") )
	return 0.1;
    else
	return 0.0;
}

double profile_default_gamma(profile_data *p)
{
    if ( !strcmp(p->name, "sRGB") )
	return 0.45;
    else if ( !strncmp(p->productName, "Nikon D70 for NEF", 17)
	    || !strncmp(p->productName, "Nikon D100 for NEF", 18)
	    || !strncmp(p->productName, "Nikon D1 for NEF", 16) )
	return 0.45;
    else
	return 1.0;
}

void curve_parse_start(GMarkupParseContext *context, const gchar *element,
    const gchar **names, const gchar **values, gpointer user, GError **error)
{
    CurveData *c = user;
    int int_value;
    GQuark ufrawQuark = g_quark_from_static_string("UFRaw");

    context = context;
    while (*names!=NULL) {
        sscanf(*values, "%d", &int_value);
        if (!strcmp(element, "Curve") && !strcmp(*names, "Version")) {
            if (int_value!=cfg_default.version)
                g_set_error(error, ufrawQuark, UFRAW_RC_VERSION,
                    "Curve version is not supported");
        }
        names++;
        values++;
    }
    if (!strcmp("Curve", element)) {
	/* m_gamma==-1 marks that we are inside a XML Curve block.
	 * This is ok since we never set m_gamma. */
	c->m_gamma = -1.0;
	/* m_numAnchors==-1 marks that no anchors where read from the XML */
	c->m_numAnchors = -1;
    }
}

void curve_parse_end(GMarkupParseContext *context, const gchar *element,
         gpointer user, GError **error)
{
    CurveData *c = user;
    context = context;
    error = error;
    if (!strcmp("Curve", element)) {
	c->m_gamma = cfg_default.curve[0].m_gamma;
	if (c->m_numAnchors<0)
	    c->m_numAnchors = cfg_default.curve[0].m_numAnchors;
    }
}

void curve_parse_text(GMarkupParseContext *context, const gchar *text,
	gsize len, gpointer user, GError **error)
{
    CurveData *c = user;
    const gchar *element = g_markup_parse_context_get_element(context);
    char temp[max_path];
    error = error;
    for(; len>0 && isspace(*text); len--, text++);
    for(; len>0 && isspace(text[len-1]); len--);
    if (len==0) return;
    if (len>max_path-1) len=max_path-1;
    strncpy(temp, text, len);
    temp[len] = '\0';
    if (!strcmp("Curve", element)) {
        g_strlcpy(c->name, temp, max_name);
    }
    /* A negative gamma marks that we are in a Curve XML block */
    if (c->m_gamma < 0) {
        if (!strcmp("MinXY", element))
            sscanf(temp, "%lf %lf", &c->m_min_x, &c->m_min_y);
        if (!strcmp("MaxXY", element))
            sscanf(temp, "%lf %lf", &c->m_max_x, &c->m_max_y);
        if (!strcmp("AnchorXY", element)) {
	    /* If one anchor is supplied then all anchors should be supplied */
	    if (c->m_numAnchors<0) c->m_numAnchors = 0;
            sscanf(temp, "%lf %lf",
                    &c->m_anchors[c->m_numAnchors].x,
                    &c->m_anchors[c->m_numAnchors].y);
            c->m_numAnchors++;
        }
    }
}

int curve_load(CurveData *cp, char *filename)
{
    NikonData data;

    if ( !strcasecmp(filename + strlen(filename) - 4, ".ntc") ||
	 !strcasecmp(filename + strlen(filename) - 4, ".ncv") ) {
        /* Try loading ntc/ncv files */
        if (LoadNikonData(filename, &data)!=UFRAW_SUCCESS) {
            ufraw_message(UFRAW_ERROR, "Invalid Nikon curve file '%s'",
                        filename);
            return UFRAW_ERROR;
        }
        *cp = data.curves[TONE_CURVE];
    } else {
	/* Load UFRaw's curve file format */
        char line[max_path];
        FILE *in;
        GMarkupParser parser={&curve_parse_start, &curve_parse_end,
                &curve_parse_text, NULL, NULL};
        GMarkupParseContext *context;
        GError *err = NULL;
        GQuark ufrawQuark = g_quark_from_static_string("UFRaw");

        *cp = cfg_default.curve[0];
        if ( (in=fopen(filename, "r"))==NULL ) {
            ufraw_message(UFRAW_ERROR, "Error opening Curve file '%s': %s",
			    filename, strerror(errno));
	    return UFRAW_ERROR;
	}
        context = g_markup_parse_context_new(&parser, 0, cp, NULL);
        line[max_path-1] = '\0';
        fgets(line, max_path-1, in);
        while (!feof(in)) {
            if (!g_markup_parse_context_parse(context, line,
                    strlen(line), &err)) {
                ufraw_message(UFRAW_ERROR, "Error parsing %s\n%s",
                        filename, err->message);
                if (g_error_matches(err, ufrawQuark, UFRAW_RC_VERSION))
                    return UFRAW_RC_VERSION;
                else return UFRAW_ERROR;
            }
            fgets(line, max_path, in);
        }
        g_markup_parse_context_end_parse(context, NULL);
        g_markup_parse_context_free(context);
        fclose(in);
    }
    char *base = g_path_get_basename(filename);
    char *name = uf_file_set_type(base, "");
    g_strlcpy(cp->name, name, max_name);
    g_free(name);
    g_free(base);
    return UFRAW_SUCCESS;
}

int curve_save(CurveData *cp, char *filename)
{
    int nikon_file_type = -1;

    /* Try saving ntc/ncv format */
    if ( !strcasecmp(filename+strlen(filename)-4, ".ntc") )
        nikon_file_type = NTC_FILE;
    else if (!strcasecmp(filename+strlen(filename)-4, ".ncv") )
        nikon_file_type = NCV_FILE;

    //if it's ntc or ncv
    if (nikon_file_type != -1)  {
        NikonData data;

        //clear it out
        memset(&data,0,sizeof(NikonData));

        //check to see if we can save
        if (cp->m_numAnchors < 2) {
            ufraw_message(UFRAW_ERROR, "Error saving Nikon curve file."
                        "Curve data is not from an NEF, NTC, or NCV file.");
            return UFRAW_ERROR;
        }
	data.curves[TONE_CURVE] = *cp;

        if (SaveNikonDataFile(&data, filename, nikon_file_type,
                                NIKON_VERSION_4_1)!=UFRAW_SUCCESS) {
            ufraw_message(UFRAW_ERROR, "Invalid Nikon curve file '%s'",
                            filename);
            return UFRAW_ERROR;
        }
//        FreeNikonData(&data);
    } else {
        /* Save UFRaw's curve format */
        FILE *out;

        if ( (out=fopen(filename, "w"))==NULL ) {
            ufraw_message(UFRAW_ERROR, "Error opening file '%s': %s",
                        filename, g_strerror(errno));
            return UFRAW_ERROR;
        }
        char *base = g_path_get_basename(filename);
	char *name = uf_file_set_type(base, "");
        g_free(base);
        fprintf(out, "<Curve Version='%d'>%s\n", cfg_default.version, name);
	g_free(name);
        char *buf = curve_buffer(cp);
        if (buf!=NULL) fprintf(out, buf);
        g_free(buf);
        fprintf(out, "</Curve>\n");
        fclose(out);
    }
    return UFRAW_SUCCESS;
}

void RGB_to_temperature(double *rgb, double *temperature, double *green)
{
    int l, r, m;
    double rbRatio;

    rbRatio = rgb[0]/rgb[2];

    for (l=0, r=sizeof(bbWB)/(sizeof(float)*3), m=(l+r)/2; r-l>1 ; m=(l+r)/2) {
        if (bbWB[m][0]/bbWB[m][2] > rbRatio) l = m;
        else r = m;
    }
    *temperature = m*10+2000;
    *green = (bbWB[m][1]/bbWB[m][0])/(rgb[1]/rgb[0]);
}

void cfg_parse_start(GMarkupParseContext *context, const gchar *element,
    const gchar **names, const gchar **values, gpointer user, GError **error)
{
    cfg_data *c = user;
    int int_value;
    GQuark ufrawQuark = g_quark_from_static_string("UFRaw");

    context = context;
    while (*names!=NULL) {
        if (!strcasecmp(*values, "yes"))
            int_value = 1;
        else
            sscanf(*values, "%d", &int_value);
        if (!strcmp(element, "UFRaw") && !strcmp(*names, "Version")) {
            if (int_value==3) {
                ufraw_message(UFRAW_WARNING,
                    "Trying to convert .ufrawrc from UFRaw-0.4 or earlier");
                c->version = int_value;
            }
            if (int_value!=c->version)
                g_set_error(error, ufrawQuark, UFRAW_RC_VERSION,
                    "UFRaw version in .ufrawrc is not supported");
        }
        if (!strcmp(*names,"Current") && int_value!=0) {
            if (!strcmp("ManualCurve", element))
                c->curveIndex = manual_curve;
            if (!strcmp("LinearCurve", element))
                c->curveIndex = linear_curve;
            if (!strcmp("CameraCurve", element))
                c->curveIndex = camera_curve;
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
     * m_numAnchors==-1 marks that no anchors where read from the XML. */
    if (!strcmp("ManualCurve", element)) {
	c->curveCount = - manual_curve;
	c->curve[-c->curveCount].m_numAnchors = -1;
    }
    if (!strcmp("LinearCurve", element)) {
	c->curveCount = - linear_curve;
	c->curve[-c->curveCount].m_numAnchors = -1;
    }
    if (!strcmp("CameraCurve", element)) {
	c->curveCount = - camera_curve;
	c->curve[-c->curveCount].m_numAnchors = -1;
    }
    if (!strcmp("sRGBInputProfile", element)) c->profileCount[0] = - 0;
    if (!strcmp("sRGBOutputProfile", element)) c->profileCount[1] = - 0;
}

void cfg_parse_end(GMarkupParseContext *context, const gchar *element,
         gpointer user, GError **error)
{
    cfg_data *c = user;
    context = context;
    error = error;
    if ( c->curveCount<=0 &&
	 ( !strcmp("ManualCurve", element) ||
	   !strcmp("LinearCurve", element) ||
	   !strcmp("CameraCurve", element) ) ) {
	if (c->curve[-c->curveCount].m_numAnchors<0)
	    c->curve[-c->curveCount].m_numAnchors = 2;
	c->curveCount = camera_curve+1;
    }
    if (c->curveCount<=0 && !strcmp("Curve", element)) {
	if (c->curve[-c->curveCount].m_numAnchors<0)
	    c->curve[-c->curveCount].m_numAnchors = 2;
        c->curveCount = - c->curveCount + 1;
    }
    if (!strcmp("sRGBInputProfile", element)) c->profileCount[0] = 2;
    if (!strcmp("sRGBOutputProfile", element)) c->profileCount[1] = 1;
    if (c->profileCount[0]<=0 && !strcmp("InputProfile", element))
        c->profileCount[0] = - c->profileCount[0] + 1;
    if (c->profileCount[1]<=0 && !strcmp("OutputProfile", element))
        c->profileCount[1] = - c->profileCount[1] + 1;
}

void cfg_parse_text(GMarkupParseContext *context, const gchar *text, gsize len,
        gpointer user, GError **error)
{
    cfg_data *c = user;
    const gchar *element = g_markup_parse_context_get_element(context);
    char temp[max_path];
    int i;
    error = error;
    for(; len>0 && isspace(*text); len--, text++);
    for(; len>0 && isspace(text[len-1]); len--);
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
	    /* If one anchor is supplied then all anchors should be supplied */
	    if (c->curve[i].m_numAnchors<0) c->curve[i].m_numAnchors = 0;
            sscanf(temp, "%lf %lf",
                    &c->curve[i].m_anchors[c->curve[i].m_numAnchors].x,
                    &c->curve[i].m_anchors[c->curve[i].m_numAnchors].y);
            c->curve[i].m_numAnchors++;
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
        if (!strcmp("File", element))
            g_strlcpy(c->profile[1][i].file, temp, max_path);
        if (!strcmp("ProductName", element))
            g_strlcpy(c->profile[0][i].productName, temp, max_path);
        return;
    }
    if (!strcmp("Curve", element)) {
        c->curveCount = - c->curveCount;
        i = - c->curveCount;
        c->curve[i] = cfg_default.curve[0];
        g_strlcpy(c->curve[i].name, temp, max_name);
	/* m_numAnchors==-1 marks that no anchors where read from the XML */
	c->curve[-c->curveCount].m_numAnchors = -1;
    }
    if (!strcmp("InputProfile", element)) {
        c->profileCount[0] = - c->profileCount[0];
        i = - c->profileCount[0];
        c->profile[0][i] = cfg_default.profile[0][1];
        g_strlcpy(c->profile[0][i].name, temp, max_name);
    }
    if (!strcmp("OutputProfile", element)) {
        c->profileCount[1] = - c->profileCount[1];
        i = - c->profileCount[1];
        c->profile[1][i] = cfg_default.profile[1][0];
        g_strlcpy(c->profile[1][i].name, temp, max_name);
    }
    if (!strcmp("InputFilename", element))
            g_strlcpy(c->inputFilename, temp, max_path);
    if (!strcmp("OutputFilename", element))
            g_strlcpy(c->outputFilename, temp, max_path);
    if (!strcmp("LoadWB", element)) sscanf(temp, "%d", &c->wbLoad);
    if (!strcmp("LoadCurve", element)) sscanf(temp, "%d", &c->curveLoad);
    if (!strcmp("LoadExposure", element)) sscanf(temp, "%d", &c->exposureLoad);
    if (!strcmp("Interpolation", element))
            sscanf(temp, "%d", &c->interpolation);
    if (!strcmp("RawExpander", element))
            sscanf(temp, "%d", &c->expander[raw_expander]);
    if (!strcmp("ExposureExpander", element))
            sscanf(temp, "%d", &c->expander[exposure_expander]);
    if (!strcmp("WBExpander", element))
            sscanf(temp, "%d", &c->expander[wb_expander]);
    if (!strcmp("ColorExpander", element))
            sscanf(temp, "%d", &c->expander[color_expander]);
    if (!strcmp("CurveExpander", element))
            sscanf(temp, "%d", &c->expander[curve_expander]);
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
    if (!strcmp("WB", element)) sscanf(temp, "%d", &c->wb);
    if (!strcmp("Temperature", element)) sscanf(temp, "%lf", &c->temperature);
    if (!strcmp("Green", element)) sscanf(temp, "%lf", &c->green);
    if (!strcmp("Exposure", element)) sscanf(temp, "%lf", &c->exposure);
    if (!strcmp("Saturation", element)) sscanf(temp, "%lf", &c->saturation);
    if (!strcmp("Unclip", element)) sscanf(temp, "%d", &c->unclip);
    if (!strcmp("AutoExposure", element)) sscanf(temp, "%d", &c->autoExposure);
    if (!strcmp("AutoBlack", element)) sscanf(temp, "%d", &c->autoBlack);
    if (!strcmp("AutoCurve", element)) sscanf(temp, "%d", &c->autoCurve);
    if (!strcmp("CurvePath", element)) g_strlcpy(c->curvePath, temp, max_path);
    if (!strcmp("Intent", element)) sscanf(temp, "%d", &c->intent);
    if (!strcmp("ProfilePath", element))
            g_strlcpy(c->profilePath, temp, max_path);
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

int load_configuration(cfg_data *c)
{
    char *cfgFilename, line[max_path];
    const char *hd;
    FILE *in;
    GMarkupParser parser={&cfg_parse_start, &cfg_parse_end,
            &cfg_parse_text, NULL, NULL};
    GMarkupParseContext *context;
    GError *err = NULL;
    GQuark ufrawQuark = g_quark_from_static_string("UFRaw");
    int i;

    *c = cfg_default;
    hd = uf_get_home_dir();
    cfgFilename = g_build_filename(hd, ".ufrawrc", NULL);
    if ( (in=fopen(cfgFilename, "r"))!=NULL ) {
        context = g_markup_parse_context_new(&parser, 0, c, NULL);
        line[max_path-1] = '\0';
        fgets(line, max_path-1, in);
        while (!feof(in)) {
            if (!g_markup_parse_context_parse(context, line,
                    strlen(line), &err)) {
                ufraw_message(UFRAW_ERROR, "Error parsing %s\n%s",
                        cfgFilename, err->message);
                if (g_error_matches(err, ufrawQuark, UFRAW_RC_VERSION))
                    return UFRAW_RC_VERSION;
                else return UFRAW_ERROR;
            }
            fgets(line, max_path, in);
        }
        g_markup_parse_context_end_parse(context, NULL);
        g_markup_parse_context_free(context);
        fclose(in);
    }
    if (c->version==3) {
        c->version = cfg_default.version;
        /* Don't add linear part to existing profile curves (except sRGB) */
        for (i=2; i<c->profileCount[0]; i++)
            c->profile[0][i].linear = 0.0;
    }
    /* a few consistency settings */
    if (c->curveIndex>=c->curveCount) c->curveIndex = cfg_default.curveIndex;
    g_free(cfgFilename);
    return UFRAW_SUCCESS;
}

#define cfg_printf(format, ...) (\
    bufIndex += buf==NULL ? fprintf(out, format, ## __VA_ARGS__) :\
        snprintf(buf+bufIndex, bufSize-bufIndex, format, ## __VA_ARGS__)\
)

char *curve_buffer(CurveData *c)
{
    const int bufSize = 1000;
    char *buf = g_new(char, bufSize);
    int bufIndex = 0, i;
    if ( c->m_min_x!=cfg_default.curve[0].m_min_x ||
	 c->m_min_y!=cfg_default.curve[0].m_min_y ||
	 c->m_max_x!=cfg_default.curve[0].m_max_x ||
	 c->m_max_y!=cfg_default.curve[0].m_max_y ) {
        bufIndex += snprintf(buf+bufIndex, bufSize-bufIndex,
                "\t<MinXY>%lf %lf</MinXY>\n", c->m_min_x, c->m_min_y);
        bufIndex += snprintf(buf+bufIndex, bufSize-bufIndex,
                "\t<MaxXY>%lf %lf</MaxXY>\n", c->m_max_x, c->m_max_y);
    }
    if ( c->m_numAnchors!=cfg_default.curve[0].m_numAnchors ||
	 c->m_anchors[0].x!=cfg_default.curve[0].m_anchors[0].x ||
	 c->m_anchors[0].y!=cfg_default.curve[0].m_anchors[0].y ||
	 c->m_anchors[1].x!=cfg_default.curve[0].m_anchors[1].x ||
	 c->m_anchors[1].y!=cfg_default.curve[0].m_anchors[1].y ) {
        for (i=0; i<c->m_numAnchors; i++)
            bufIndex += snprintf(buf+bufIndex, bufSize-bufIndex,
                    "\t<AnchorXY>%lf %lf</AnchorXY>\n",
                    c->m_anchors[i].x, c->m_anchors[i].y);
    }
    if (bufIndex==0) {
        g_free(buf);
        buf = NULL;
    }
    return buf;
}

int save_configuration(cfg_data *c, developer_data *d,
	char *confFilename, char *buf, int bufSize)
{
    char *cfgFilename;
    const char *hd;
    FILE *out=NULL;
    char *type, *current;
    int bufIndex=0, i, j;

    if (buf==NULL) {
	if (confFilename==NULL) {
            hd = uf_get_home_dir();
            cfgFilename = g_build_filename(hd, ".ufrawrc", NULL);
	} else
	    cfgFilename = g_strdup(confFilename);
        if ( (out=fopen(cfgFilename, "w"))==NULL ) {
            ufraw_message(UFRAW_ERROR,
                    "Can't open file %s for writing\n%s\n",
                    cfgFilename, strerror(errno) );
            return UFRAW_ERROR;
        }
        g_free(cfgFilename);
    }
    cfg_printf("<UFRaw Version='%d'>\n", c->version);
    if (strlen(c->inputFilename)>0)
        cfg_printf("<InputFilename>%s</InputFilename>\n", c->inputFilename);
    if (strlen(c->outputFilename)>0)
        cfg_printf("<OutputFilename>%s</OutputFilename>\n", c->outputFilename);
    if (confFilename==NULL) {
        if (c->wbLoad!=cfg_default.wbLoad)
            cfg_printf("<LoadWB>%d</LoadWB>\n", c->wbLoad);
        if (c->curveLoad!=cfg_default.curveLoad)
            cfg_printf("<LoadCurve>%d</LoadCurve>\n", c->curveLoad);
        if (c->exposureLoad!=cfg_default.exposureLoad)
            cfg_printf("<LoadExposure>%d</LoadExposure>\n", c->exposureLoad);
        if (c->expander[raw_expander]!=cfg_default.expander[raw_expander])
            cfg_printf("<RawExpander>%d</RawExpander>\n",
                    c->expander[raw_expander]);
        if (c->expander[exposure_expander]!=
		cfg_default.expander[exposure_expander])
            cfg_printf("<ExposureExpander>%d</ExposureExpander>\n",
                    c->expander[exposure_expander]);
        if (c->expander[wb_expander]!=cfg_default.expander[wb_expander])
            cfg_printf("<WBExpander>%d</WBExpander>\n",
		    c->expander[wb_expander]);
        if (c->expander[color_expander]!=cfg_default.expander[color_expander])
            cfg_printf("<ColorExpander>%d</ColorExpander>\n",
                    c->expander[color_expander]);
        if (c->expander[curve_expander]!=cfg_default.expander[curve_expander])
            cfg_printf("<CurveExpander>%d</CurveExpander>\n",
                    c->expander[curve_expander]);
        if (c->expander[live_expander]!=cfg_default.expander[live_expander])
            cfg_printf("<LiveExpander>%d</LiveExpander>\n",
                    c->expander[live_expander]);
        if (c->histogram!=cfg_default.histogram)
            cfg_printf("<Histogram>%d</Histogram>\n", c->histogram);
        if (c->liveHistogramScale!=cfg_default.liveHistogramScale)
            cfg_printf("<LiveHistogramScale>%d</LiveHistogramScale>\n",
		    c->liveHistogramScale);
        if (c->liveHistogramHeight!=cfg_default.liveHistogramHeight)
            cfg_printf("<LiveHistogramHeight>%d</LiveHistogramHeight>\n",
		    c->liveHistogramHeight);
        if (c->rawHistogramScale!=cfg_default.rawHistogramScale)
            cfg_printf("<RawHistogramScale>%d</RawHistogramScale>\n",
		    c->rawHistogramScale);
        if (c->rawHistogramHeight!=cfg_default.rawHistogramHeight)
            cfg_printf("<RawHistogramHeight>%d</RawHistogramHeight>\n",
		    c->rawHistogramHeight);
        if (c->overExp!=cfg_default.overExp)
            cfg_printf("<OverExposure>%d</OverExposure>\n", c->overExp);
        if (c->underExp!=cfg_default.underExp)
            cfg_printf("<UnderExposure>%d</UnderExposure>\n", c->underExp);
    }
    if (c->interpolation!=cfg_default.interpolation)
        cfg_printf("<Interpolation>%d</Interpolation>\n", c->interpolation);
    if (c->wb!=cfg_default.wb)
        cfg_printf("<WB>%d</WB>\n", c->wb);
    cfg_printf("<Temperature>%d</Temperature>\n", (int)floor(c->temperature));
    cfg_printf("<Green>%lf</Green>\n", c->green);
    if (d!=NULL) {
	if (d->rgbWB[3]==0) {
	    cfg_printf("<ChannelMultipliers>%d %d %d</ChannelMultipliers>\n",
		d->rgbWB[0], d->rgbWB[1], d->rgbWB[2]);
	} else {
	    cfg_printf("<ChannelMultipliers>%d %d %d %d</ChannelMultipliers>\n",
		d->rgbWB[0], d->rgbWB[1], d->rgbWB[2], d->rgbWB[3]);
	}
    }
    if (c->exposure!=cfg_default.exposure)
        cfg_printf("<Exposure>%lf</Exposure>\n", c->exposure);
    if (c->unclip!=cfg_default.unclip)
        cfg_printf("<Unclip>%d</Unclip>\n", c->unclip);
    if (c->autoExposure!=cfg_default.autoExposure)
        cfg_printf("<AutoExposure>%d</AutoExposure>\n", c->autoExposure);
    if (c->autoBlack!=cfg_default.autoBlack)
        cfg_printf("<AutoBlack>%d</AutoBlack>\n", c->autoBlack);
    if (c->autoCurve!=cfg_default.autoCurve)
        cfg_printf("<AutoCurve>%d</AutoCurve>\n", c->autoCurve);
    if (c->saturation!=cfg_default.saturation)
        cfg_printf("<Saturation>%lf</Saturation>\n", c->saturation);
    if (c->size!=cfg_default.size)
        cfg_printf("<Size>%d</Size>\n", c->size);
    if (c->shrink!=cfg_default.shrink)
        cfg_printf("<Shrink>%d</Shrink>\n", c->shrink);
    if (c->type!=cfg_default.type)
        cfg_printf("<OutputType>%d</OutputType>\n", c->type);
    if (c->createID!=cfg_default.createID)
        cfg_printf("<CreateID>%d</CreateID>\n", c->createID);
    if (c->embedExif!=cfg_default.embedExif)
        cfg_printf("<EmbedExif>%d</EmbedExif>\n", c->embedExif);
    if (c->compression!=cfg_default.compression)
        cfg_printf("<Compression>%d</Compression>\n", c->compression);
    if (c->overwrite!=cfg_default.overwrite)
        cfg_printf("<Overwrite>%d</Overwrite>\n", c->overwrite);
    if (c->losslessCompress!=cfg_default.losslessCompress)
        cfg_printf("<LosslessCompression>%d</LosslessCompression>\n",
                c->losslessCompress);
    for (i=0; i<c->curveCount; i++) {
        char *curveBuf = curve_buffer(&c->curve[i]);
        /* Write curve if it is non-default and we are not writing to .ufraw */
	/* But ALWAYS write the current curve */
        if ( c->curveIndex==i || (curveBuf!=NULL && confFilename==NULL) ) {
	    if (curveBuf==NULL) curveBuf = g_strdup("");
            current = i==c->curveIndex?" Current='yes'":"";
            switch (i) {
                case manual_curve:
                        cfg_printf("<ManualCurve%s>\n%s</ManualCurve>\n",
                                        current, curveBuf);
                        break;
                case linear_curve:
                        cfg_printf("<LinearCurve%s>\n%s</LinearCurve>\n",
                                        current, curveBuf);
                        break;
                case camera_curve:
                        cfg_printf("<CameraCurve%s>\n%s</CameraCurve>\n",
                                        current, curveBuf);
                        break;
                default:
                        cfg_printf("<Curve%s>%s\n%s</Curve>\n",
                                        current, c->curve[i].name, curveBuf);
            }
        }
        g_free(curveBuf);
    }
    if (strlen(c->curvePath)>0 && confFilename==NULL)
        cfg_printf("<CurvePath>%s</CurvePath>\n", c->curvePath);
    for (j=0; j<profile_types; j++) {
        type = j==in_profile ? "InputProfile" :
                j==out_profile ? "OutputProfile" : "Error";
	/* The default sRGB profile is cfg_default.profile[j][0] */
        if ( c->profileIndex[j]==0 || ( confFilename==NULL &&
             ( c->profile[j][0].gamma!=cfg_default.profile[0][0].gamma ||
               c->profile[j][0].linear!=cfg_default.profile[0][0].linear ||
               c->profile[j][0].useMatrix!=
		   cfg_default.profile[0][0].useMatrix ) ) ) {
            current = c->profileIndex[j]==0 ? " Current='yes'" : "";
            cfg_printf("<sRGB%s%s>\n", type, current);
            if (c->profile[j][0].gamma!=cfg_default.profile[j][0].gamma)
                cfg_printf("\t<Gamma>%lf</Gamma>\n", c->profile[j][0].gamma);
            if (c->profile[j][0].linear!=cfg_default.profile[j][0].linear)
                cfg_printf("\t<Linearity>%lf</Linearity>\n",
                        c->profile[j][0].linear);
            if (c->profile[j][0].useMatrix!=cfg_default.profile[j][0].useMatrix)
                cfg_printf("\t<UseColorMatrix>%d</UseColorMatrix>\n",
                        c->profile[j][0].useMatrix);
            cfg_printf("</sRGB%s>\n", type);
        }
	/* While the default ICC profile is cfg_default.profile[j][1] */
        for (i=1; i<c->profileCount[j]; i++) {
	    if (confFilename!=NULL && i!=c->profileIndex[j])
		continue;
            current = i==c->profileIndex[j]?" Current='yes'":"";
            cfg_printf("<%s%s>%s\n", type, current, c->profile[j][i].name);
            cfg_printf("\t<File>%s</File>\n", c->profile[j][i].file);
            cfg_printf("\t<ProductName>%s</ProductName>\n",
                    c->profile[j][i].productName);
            if (c->profile[j][i].gamma!=cfg_default.profile[j][1].gamma)
                cfg_printf("\t<Gamma>%lf</Gamma>\n", c->profile[j][i].gamma);
            if (c->profile[j][i].linear!=cfg_default.profile[j][1].linear)
                cfg_printf("\t<Linearity>%lf</Linearity>\n",
                        c->profile[j][i].linear);
            if (c->profile[j][i].useMatrix!=cfg_default.profile[j][1].useMatrix)
                cfg_printf("\t<UseColorMatrix>%d</UseColorMatrix>\n",
                        c->profile[j][i].useMatrix);
            cfg_printf("</%s>\n", type);
        }
    }
    if (c->intent!=cfg_default.intent)
        cfg_printf("<Intent>%d</Intent>\n", c->intent);
    if (strlen(c->profilePath)>0 && confFilename==NULL)
        cfg_printf("<ProfilePath>%s</ProfilePath>\n", c->profilePath);
    if (confFilename!=NULL)
	cfg_printf("<Log>\n%s</Log>\n", ufraw_message(UFRAW_GET_LOG, NULL));
    cfg_printf("</UFRaw>\n");
    if (buf==NULL) fclose(out);
    else if (bufIndex >= bufSize) return UFRAW_ERROR;
    return UFRAW_SUCCESS;
}
