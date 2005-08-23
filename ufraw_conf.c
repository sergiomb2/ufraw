/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 * by Udi Fuchs,
 *
 * ufraw_conf.c - handle configuration issues
 *
 * UFRaw is licensed under the GNU General Public License.
 * It uses "dcraw" code to do the actual raw decoding.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <ctype.h> /* needed for isspace() */
#include <errno.h>
#include <math.h>
#include <glib.h>
#include "ufraw.h"

const conf_data conf_default = {
    /* Internal data */
    sizeof(conf_data), 5, /* confSize, version */

    /* Image manipulation settings */
    camera_wb, /* wb */
    4750, 1.2, /* temperature, green */
    { -1.0, -1.0, -1.0, -1.0 }, /* chanMul[] */
    0.0, 1.0, 0.0, /* exposure, saturation, black */
    FALSE /* Unclip highlights */,
    disabled_state, /* autoExposure */
    disabled_state, /* autoBlack */
    camera_curve, camera_curve+1, /* curveIndex, curveCount */
    /* Curve data defaults */
    { { "Manual curve", TONE_CURVE, 0.0, 1.0, 0.0, 1.0, 1.0,
	  2 , { { 0.0, 0.0 }, { 1.0, 1.0 } } },
      { "Linear curve", TONE_CURVE, 0.0, 1.0, 0.0, 1.0, 1.0,
	  2 , { { 0.0, 0.0 }, { 1.0, 1.0 } } },
      { "Camera curve", TONE_CURVE, 0.0, 1.0, 0.0, 1.0, 1.0,
	  0 , { { 0.0, 0.0 } } },
    },
    { 0, 0 } , { 1, 1 }, /* profileIndex[], profileCount[] */
    /* Profile data defaults */
    { { { "sRGB", "", "", 0.45, 0.1, TRUE },
        { "Some ICC Profile", "", "", 0.45, 0.0, FALSE } },
      { { "sRGB", "", "", 0.0, 0.0, FALSE },
        { "Some ICC Profile", "", "", 0.0, 0.0, FALSE } } },
    0, /* intent */
    full_interpolation, /* interpolation */

    /* Save options */
    "", "", "", /* inputFilename, outputFilename, outputPath */
    ppm8_type, 85, no_id, /* type, compression, createID */
    TRUE, /* embedExif */
    1, 0, /* shrink, size */
    FALSE, /* overwrite exsisting files without asking */
    FALSE, /* losslessCompress */

    /* GUI settings */
    enabled_state, /* saveConfiguration */
    rgb_histogram, /* histogram */
    linear_histogram, 128, /* liveHistogramScale, liveHistogramHeight */
    linear_histogram, 128, /* rawHistogramScale, rawHistogramHeight */
    { TRUE, TRUE, TRUE, TRUE, TRUE, TRUE }, /* expander[] */
    FALSE, /* overExp indicator */
    FALSE, /* underExp indicator */
    "", "" /* curvePath, profilePath */
};

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
     * m_numAnchors==0 marks that no anchors where read from the XML. */
    if (!strcmp("ManualCurve", element)) {
	c->curveCount = - manual_curve;
	c->curve[-c->curveCount].m_numAnchors = 0;
    }
    if (!strcmp("LinearCurve", element)) {
	c->curveCount = - linear_curve;
	c->curve[-c->curveCount].m_numAnchors = 0;
    }
    if (!strcmp("CameraCurve", element)) {
	c->curveCount = - camera_curve;
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
    if ( c->curveCount<=0 &&
	 ( !strcmp("ManualCurve", element) ||
	   !strcmp("LinearCurve", element) ||
	   !strcmp("CameraCurve", element) ) ) {
	if (c->curve[-c->curveCount].m_numAnchors==0)
	    c->curve[-c->curveCount].m_numAnchors = 2;
	c->curveCount = camera_curve+1;
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
    GQuark ufrawQuark = g_quark_from_static_string("UFRaw");
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
                    "Can't open ID file %s for reading\n%s\n",
                    IDFilename, strerror(errno) );
            return UFRAW_ERROR;
        }
	confFilename = g_strdup(IDFilename);
    }
    locale = uf_set_locale_C();
    context = g_markup_parse_context_new(&parser, 0, c, NULL);
    line[max_path-1] = '\0';
    fgets(line, max_path-1, in);
    while (!feof(in)) {
        if (!g_markup_parse_context_parse(context, line, strlen(line), &err)) {
            ufraw_message(UFRAW_SET_ERROR, "Error parsing %s\n%s",
                    confFilename, err->message);
	    g_markup_parse_context_end_parse(context, NULL);
	    uf_reset_locale(locale);
	    g_free(confFilename);
            if (g_error_matches(err, ufrawQuark, UFRAW_RC_VERSION))
                return UFRAW_RC_VERSION;
            else return UFRAW_ERROR;
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
    /* a few consistency settings */
    if (c->curveIndex>=c->curveCount) c->curveIndex = conf_default.curveIndex;
    return UFRAW_SUCCESS;
}

#define conf_printf(format, ...) {\
    gchar *conf_print_temp = g_markup_printf_escaped(format, ## __VA_ARGS__);\
    if (buf==NULL) {\
	fputs(conf_print_temp, out);\
    } else {\
	bufIndex += g_strlcpy(buf+bufIndex, conf_print_temp, bufSize-bufIndex);\
    }\
    g_free(conf_print_temp);\
}

int conf_save(conf_data *c, char *IDFilename, char *buf, int bufSize)
{
    char *confFilename, *locale;
    const char *hd;
    FILE *out=NULL;
    char *type, *current;
    int bufIndex=0, i, j;

    if (buf==NULL) {
	if (IDFilename==NULL) {
            hd = uf_get_home_dir();
            confFilename = g_build_filename(hd, ".ufrawrc", NULL);
	} else
	    confFilename = g_strdup(IDFilename);
        if ( (out=fopen(confFilename, "w"))==NULL ) {
            ufraw_message(UFRAW_ERROR,
                    "Can't open file %s for writing\n%s\n",
                    confFilename, strerror(errno) );
            return UFRAW_ERROR;
        }
        g_free(confFilename);
    }
    locale = uf_set_locale_C();
    conf_printf("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
    conf_printf("<UFRaw Version='%d'>\n", c->version);
    if (strlen(c->inputFilename)>0 && IDFilename!=NULL) {
	char *utf8 = g_filename_to_utf8(c->inputFilename, -1, NULL, NULL, NULL);
	if (utf8==NULL) utf8 = g_strdup("Unknown file name");
        conf_printf("<InputFilename>%s</InputFilename>\n", utf8);
	g_free(utf8);
    }
    if (strlen(c->outputFilename)>0 && IDFilename!=NULL) {
	char *utf8=g_filename_to_utf8(c->outputFilename, -1, NULL, NULL, NULL);
	if (utf8==NULL) utf8 = g_strdup("Unknown file name");
        conf_printf("<OutputFilename>%s</OutputFilename>\n", utf8);
	g_free(utf8);
    }
    if (strlen(c->outputPath)>0) {
	char *utf8=g_filename_to_utf8(c->outputPath, -1, NULL, NULL, NULL);
	if (utf8!=NULL) conf_printf("<OutputPath>%s</OutputPath>\n", utf8);
	g_free(utf8);
    } else {
	/* Guess outputPath */
        char *inPath = g_path_get_dirname(c->inputFilename);
	char *outPath = g_path_get_dirname(c->outputFilename);
	if ( strcmp(outPath,".") && strcmp(inPath, outPath) ) {
	    char *utf8=g_filename_to_utf8(outPath, -1, NULL, NULL, NULL);
	    if (utf8!=NULL) conf_printf("<OutputPath>%s</OutputPath>\n", utf8);
	    g_free(utf8);
	}
	g_free(outPath);
	g_free(inPath);
    }
					    
    /* GUI settings are only saved to .ufrawrc */
    if (IDFilename==NULL) {
        if (c->saveConfiguration!=conf_default.saveConfiguration)
            conf_printf("<SaveConfiguration>%d</SaveConfiguration>\n",
		    c->saveConfiguration);
        if (c->expander[raw_expander]!=conf_default.expander[raw_expander])
            conf_printf("<RawExpander>%d</RawExpander>\n",
                    c->expander[raw_expander]);
        if (c->expander[exposure_expander]!=
		conf_default.expander[exposure_expander])
            conf_printf("<ExposureExpander>%d</ExposureExpander>\n",
                    c->expander[exposure_expander]);
        if (c->expander[wb_expander]!=conf_default.expander[wb_expander])
            conf_printf("<WBExpander>%d</WBExpander>\n",
		    c->expander[wb_expander]);
        if (c->expander[color_expander]!=conf_default.expander[color_expander])
            conf_printf("<ColorExpander>%d</ColorExpander>\n",
                    c->expander[color_expander]);
        if (c->expander[curve_expander]!=conf_default.expander[curve_expander])
            conf_printf("<CurveExpander>%d</CurveExpander>\n",
                    c->expander[curve_expander]);
        if (c->expander[live_expander]!=conf_default.expander[live_expander])
            conf_printf("<LiveExpander>%d</LiveExpander>\n",
                    c->expander[live_expander]);
        if (c->histogram!=conf_default.histogram)
            conf_printf("<Histogram>%d</Histogram>\n", c->histogram);
        if (c->liveHistogramScale!=conf_default.liveHistogramScale)
            conf_printf("<LiveHistogramScale>%d</LiveHistogramScale>\n",
		    c->liveHistogramScale);
        if (c->liveHistogramHeight!=conf_default.liveHistogramHeight)
            conf_printf("<LiveHistogramHeight>%d</LiveHistogramHeight>\n",
		    c->liveHistogramHeight);
        if (c->rawHistogramScale!=conf_default.rawHistogramScale)
            conf_printf("<RawHistogramScale>%d</RawHistogramScale>\n",
		    c->rawHistogramScale);
        if (c->rawHistogramHeight!=conf_default.rawHistogramHeight)
            conf_printf("<RawHistogramHeight>%d</RawHistogramHeight>\n",
		    c->rawHistogramHeight);
        if (c->overExp!=conf_default.overExp)
            conf_printf("<OverExposure>%d</OverExposure>\n", c->overExp);
        if (c->underExp!=conf_default.underExp)
            conf_printf("<UnderExposure>%d</UnderExposure>\n", c->underExp);
	if (strlen(c->curvePath)>0) {
	    char *utf8 = g_filename_to_utf8(c->curvePath, -1, NULL, NULL, NULL);
	    if (utf8!=NULL)
		conf_printf("<CurvePath>%s</CurvePath>\n", utf8);
	    g_free(utf8);
	}
	if (strlen(c->profilePath)>0) {
	    char *utf8 = g_filename_to_utf8(c->profilePath, -1,NULL,NULL,NULL);
	    if (utf8!=NULL)
		conf_printf("<ProfilePath>%s</ProfilePath>\n", utf8);
	    g_free(utf8);
	}
    }
    if (c->interpolation!=conf_default.interpolation)
        conf_printf("<Interpolation>%d</Interpolation>\n", c->interpolation);
    if (c->wb!=conf_default.wb)
        conf_printf("<WB>%d</WB>\n", c->wb);
    conf_printf("<Temperature>%d</Temperature>\n", (int)floor(c->temperature));
    conf_printf("<Green>%lf</Green>\n", c->green);
    if (c->chanMul[3]==0.0) {
	conf_printf("<ChannelMultipliers>%f %f %f</ChannelMultipliers>\n",
		c->chanMul[0], c->chanMul[1], c->chanMul[2]);
    } else {
	conf_printf("<ChannelMultipliers>%f %f %f %f</ChannelMultipliers>\n",
		c->chanMul[0], c->chanMul[1], c->chanMul[2], c->chanMul[3]);
    }
    if (c->exposure!=conf_default.exposure)
        conf_printf("<Exposure>%lf</Exposure>\n", c->exposure);
    if (c->unclip!=conf_default.unclip)
        conf_printf("<Unclip>%d</Unclip>\n", c->unclip);
    if (c->autoExposure!=conf_default.autoExposure)
        conf_printf("<AutoExposure>%d</AutoExposure>\n", c->autoExposure);
    if (c->autoBlack!=conf_default.autoBlack)
        conf_printf("<AutoBlack>%d</AutoBlack>\n", c->autoBlack);
    if (c->saturation!=conf_default.saturation)
        conf_printf("<Saturation>%lf</Saturation>\n", c->saturation);
    if (c->size!=conf_default.size)
        conf_printf("<Size>%d</Size>\n", c->size);
    if (c->shrink!=conf_default.shrink)
        conf_printf("<Shrink>%d</Shrink>\n", c->shrink);
    if (c->type!=conf_default.type)
        conf_printf("<OutputType>%d</OutputType>\n", c->type);
    if (c->createID!=conf_default.createID)
        conf_printf("<CreateID>%d</CreateID>\n", c->createID);
    if (c->embedExif!=conf_default.embedExif)
        conf_printf("<EmbedExif>%d</EmbedExif>\n", c->embedExif);
    if (c->compression!=conf_default.compression)
        conf_printf("<Compression>%d</Compression>\n", c->compression);
    if (c->overwrite!=conf_default.overwrite)
        conf_printf("<Overwrite>%d</Overwrite>\n", c->overwrite);
    if (c->losslessCompress!=conf_default.losslessCompress)
        conf_printf("<LosslessCompression>%d</LosslessCompression>\n",
                c->losslessCompress);
    for (i=0; i<c->curveCount; i++) {
        char *curveBuf = curve_buffer(&c->curve[i]);
        /* Write curve if it is non-default and we are not writing to .ufraw */
	/* But ALWAYS write the current curve */
        if ( c->curveIndex==i || (curveBuf!=NULL && IDFilename==NULL) ) {
	    if (curveBuf==NULL) curveBuf = g_strdup("");
            current = i==c->curveIndex?"yes":"no";
            switch (i) {
                case manual_curve:
                        conf_printf("<ManualCurve Current='%s'>\n", current);
			conf_printf(curveBuf);
			conf_printf("</ManualCurve>\n");
                        break;
                case linear_curve:
                        conf_printf("<LinearCurve Current='%s'>\n", current);
			conf_printf(curveBuf);
			conf_printf("</LinearCurve>\n");
                        break;
                case camera_curve:
                        conf_printf("<CameraCurve Current='%s'>\n", current);
			conf_printf(curveBuf);
			conf_printf("</CameraCurve>\n");
                        break;
                default:
                        conf_printf("<Curve Current='%s'>%s\n", current,
				c->curve[i].name);
			conf_printf(curveBuf);
			conf_printf("</Curve>\n");
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
            conf_printf("<sRGB%s Current='%s'>\n", type, current);
            if (c->profile[j][0].gamma!=conf_default.profile[j][0].gamma)
                conf_printf("\t<Gamma>%lf</Gamma>\n", c->profile[j][0].gamma);
            if (c->profile[j][0].linear!=conf_default.profile[j][0].linear)
                conf_printf("\t<Linearity>%lf</Linearity>\n",
                        c->profile[j][0].linear);
            if (c->profile[j][0].useMatrix!=
		    conf_default.profile[j][0].useMatrix)
                conf_printf("\t<UseColorMatrix>%d</UseColorMatrix>\n",
                        c->profile[j][0].useMatrix);
            conf_printf("</sRGB%s>\n", type);
        }
	/* While the default ICC profile is conf_default.profile[j][1] */
        for (i=1; i<c->profileCount[j]; i++) {
	    if (IDFilename!=NULL && i!=c->profileIndex[j])
		continue;
            current = i==c->profileIndex[j]?"yes":"no";
            conf_printf("<%s Current='%s'>%s\n",
		    type, current, c->profile[j][i].name);
	    char *utf8 = g_filename_to_utf8(c->profile[j][i].file,
		    -1, NULL, NULL, NULL);
	    if (utf8==NULL) utf8 = g_strdup("Unknown file name");
            conf_printf("\t<File>%s</File>\n", utf8);
	    g_free(utf8);
            conf_printf("\t<ProductName>%s</ProductName>\n",
                    c->profile[j][i].productName);
            if (c->profile[j][i].gamma!=conf_default.profile[j][1].gamma)
                conf_printf("\t<Gamma>%lf</Gamma>\n", c->profile[j][i].gamma);
            if (c->profile[j][i].linear!=conf_default.profile[j][1].linear)
                conf_printf("\t<Linearity>%lf</Linearity>\n",
                        c->profile[j][i].linear);
            if (c->profile[j][i].useMatrix!=
		    conf_default.profile[j][1].useMatrix)
                conf_printf("\t<UseColorMatrix>%d</UseColorMatrix>\n",
                        c->profile[j][i].useMatrix);
            conf_printf("</%s>\n", type);
        }
    }
    if (c->intent!=conf_default.intent)
        conf_printf("<Intent>%d</Intent>\n", c->intent);
    if (IDFilename!=NULL) {
	char *utf8 = g_filename_to_utf8(ufraw_message(UFRAW_GET_LOG, NULL),
		-1, NULL, NULL, NULL);
	if (utf8!=NULL)
	    conf_printf("<Log>\n%s</Log>\n", utf8);
	g_free(utf8);
    }
    conf_printf("</UFRaw>\n");
    uf_reset_locale(locale);
    if (buf==NULL) fclose(out);
    else if (bufIndex >= bufSize) return UFRAW_ERROR;
    return UFRAW_SUCCESS;
}

/* Copy the image manipulation options from *src to *dst */
void conf_copy_image(conf_data *dst, const conf_data *src)
{
    int i, j;

    dst->interpolation = src->interpolation;
    dst->wb = src->wb;
    dst->temperature = src->temperature;
    dst->green = src->green;
    dst->exposure = src->exposure;
    dst->saturation = src->saturation;
    dst->black = src->black;
    dst->autoExposure = src->autoExposure;
    dst->autoBlack = src->autoBlack;
    dst->unclip = src->unclip;
    /* We only copy the current curve */
    if (src->curveIndex<=camera_curve) {
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
    dst->type = src->type;
    dst->compression = src->compression;
    dst->createID = src->createID;
    dst->embedExif = src->embedExif;
    dst->shrink = src->shrink;
    dst->size = src->size;
    dst->overwrite = src->overwrite;
    dst->losslessCompress = src->losslessCompress;
}

int conf_set_cmd(conf_data *conf, const conf_data *cmd)
{
    if (cmd->overwrite!=-1) conf->overwrite = cmd->overwrite;
    if (cmd->unclip!=-1) conf->unclip = cmd->unclip;
    if (cmd->losslessCompress!=-1)
	conf->losslessCompress = cmd->losslessCompress;
    if (cmd->embedExif!=-1) conf->embedExif = cmd->embedExif;
    if (cmd->compression!=NULLF) conf->compression = cmd->compression;
    if (cmd->autoExposure) {
	conf->autoExposure = cmd->autoExposure;
    }
    if (cmd->exposure!=NULLF) {
	conf->exposure = cmd->exposure;
	conf->autoExposure = disabled_state;
    }
    if (cmd->wb>=0) conf->wb = cmd->wb;
    if (cmd->temperature!=NULLF) conf->temperature = cmd->temperature;
    if (cmd->green!=NULLF) conf->green = cmd->green;
    if (cmd->temperature!=NULLF || cmd->green!=NULLF) conf->wb = manual_wb;
    if (cmd->profile[0][0].gamma!=NULLF)
	conf->profile[0][conf->profileIndex[0]].gamma =
		cmd->profile[0][0].gamma;
    if (cmd->profile[0][0].linear!=NULLF)
	conf->profile[0][conf->profileIndex[0]].linear =
		cmd->profile[0][0].linear;
    if (cmd->saturation!=NULLF)
	conf->saturation=cmd->saturation;
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
            conf->interpolation = full_interpolation;
    }
    if (cmd->size!=NULLF) {
        conf->size = cmd->size;
        conf->shrink = 1;
        if (conf->interpolation==half_interpolation)
            conf->interpolation = full_interpolation;
    }
    if (cmd->type>=0) conf->type = cmd->type;
    if (cmd->createID>=0) conf->createID = cmd->createID;
    if (strlen(cmd->outputPath)>0)
	g_strlcpy(conf->outputPath, cmd->outputPath, max_path);
    if (strlen(cmd->outputFilename)>0) {
        if (conf->createID!=no_id && !strcmp(cmd->outputFilename,"-")) {
            ufraw_message(UFRAW_ERROR,
                "cannot --create-id with stdout");
            return UFRAW_ERROR;
        }
	g_strlcpy(conf->outputFilename, cmd->outputFilename, max_path);
    }
    return UFRAW_SUCCESS;
}
