/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 * by Udi Fuchs,
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
#include <stdlib.h> /* needed for canonicalize_file_name() */
#include <string.h>
#include <ctype.h> /* needed for isspace() */
#include <errno.h>
#include <locale.h>
#include <glib.h>
#include "ufraw.h"

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

const char raw_ext[]= "bay,bmq,cr2,crw,cs1,dc2,dcr,dng,erf,fff,hdr,jpg,k25,"
	"kdc,mdc,mos,mrw,nef,orf,pef,pxn,raf,raw,rdc,srf,tif,ufraw,x3f";

const char *file_type[] = { ".ppm", ".ppm", ".tif", ".tif", ".jpg" };

/* Set locale of LC_NUMERIC to "C" to make sure that printf behaves correctly.*/
char *uf_set_locale_C()
{
    char test[max_name], *locale;
    snprintf(test, max_name, "%.1f", 1234.5);
    if (strcmp(test, "1234.5")) {
	locale = setlocale(LC_NUMERIC, "C");
	snprintf(test, max_name, "%.1f", 1234.5);
	if (!strcmp(test, "1234.5")) {
	    return locale;
	} else {
	    setlocale(LC_NUMERIC, locale);
	    ufraw_message(UFRAW_ERROR, "Fatal error setting C locale");
	    return NULL;
	}
    } else {
	return NULL;
    }
}

void uf_reset_locale(char *locale)
{
    setlocale(LC_NUMERIC, locale);
}

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
	    || !strncmp(p->productName, "Nikon DBase for NEF", 19) /*For D100*/
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
            if (int_value!=conf_default.version)
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
	/* m_numAnchors==0 marks that no anchors where read from the XML */
	c->m_numAnchors = 0;
    }
}

void curve_parse_end(GMarkupParseContext *context, const gchar *element,
         gpointer user, GError **error)
{
    CurveData *c = user;
    context = context;
    error = error;
    if (!strcmp("Curve", element)) {
	c->m_gamma = conf_default.curve[0].m_gamma;
	if (c->m_numAnchors==0)
	    c->m_numAnchors = conf_default.curve[0].m_numAnchors;
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
        char line[max_path], *locale;
        FILE *in;
        GMarkupParser parser={&curve_parse_start, &curve_parse_end,
                &curve_parse_text, NULL, NULL};
        GMarkupParseContext *context;
        GError *err = NULL;
        GQuark ufrawQuark = g_quark_from_static_string("UFRaw");

        *cp = conf_default.curve[0];
        if ( (in=fopen(filename, "r"))==NULL ) {
            ufraw_message(UFRAW_ERROR, "Error opening Curve file '%s': %s",
			    filename, strerror(errno));
	    return UFRAW_ERROR;
	}
	locale = uf_set_locale_C();
        context = g_markup_parse_context_new(&parser, 0, cp, NULL);
        line[max_path-1] = '\0';
        fgets(line, max_path-1, in);
        while (!feof(in)) {
            if (!g_markup_parse_context_parse(context, line,
                    strlen(line), &err)) {
                ufraw_message(UFRAW_ERROR, "Error parsing %s\n%s",
                        filename, err->message);
		g_markup_parse_context_free(context);
		uf_reset_locale(locale);
                if (g_error_matches(err, ufrawQuark, UFRAW_RC_VERSION))
                    return UFRAW_RC_VERSION;
                else return UFRAW_ERROR;
            }
            fgets(line, max_path, in);
        }
        g_markup_parse_context_end_parse(context, NULL);
        g_markup_parse_context_free(context);
	uf_reset_locale(locale);
        fclose(in);
    }
    char *base = g_path_get_basename(filename);
    char *name = uf_file_set_type(base, "");
    char *utf8 = g_filename_to_utf8(name, -1, NULL, NULL, NULL);
    if (utf8==NULL) utf8 = g_strdup("Unknown file name");
    g_strlcpy(cp->name, utf8, max_name);
    g_free(utf8);
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
	char *locale = uf_set_locale_C();
	fprintf(out, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
        char *base = g_path_get_basename(filename);
	char *name = uf_file_set_type(base, "");
	char *utf8 = g_filename_to_utf8(name, -1, NULL, NULL, NULL);
	if (utf8==NULL) utf8 = strdup("Unknown file name");
        fprintf(out, "<Curve Version='%d'>%s\n", conf_default.version, utf8);
	g_free(utf8);
	g_free(name);
        g_free(base);
        char *buf = curve_buffer(cp);
        if (buf!=NULL) fprintf(out, buf);
        g_free(buf);
        fprintf(out, "</Curve>\n");
	uf_reset_locale(locale);
        fclose(out);
    }
    return UFRAW_SUCCESS;
}

char *curve_buffer(CurveData *c)
{
    const int bufSize = 1000;
    char *buf = g_new(char, bufSize);
    int bufIndex = 0, i;
    if ( c->m_min_x!=conf_default.curve[0].m_min_x ||
	 c->m_min_y!=conf_default.curve[0].m_min_y ||
	 c->m_max_x!=conf_default.curve[0].m_max_x ||
	 c->m_max_y!=conf_default.curve[0].m_max_y ) {
        bufIndex += snprintf(buf+bufIndex, bufSize-bufIndex,
                "\t<MinXY>%lf %lf</MinXY>\n", c->m_min_x, c->m_min_y);
        bufIndex += snprintf(buf+bufIndex, bufSize-bufIndex,
                "\t<MaxXY>%lf %lf</MaxXY>\n", c->m_max_x, c->m_max_y);
    }
    if ( c->m_numAnchors!=conf_default.curve[0].m_numAnchors ||
	 c->m_anchors[0].x!=conf_default.curve[0].m_anchors[0].x ||
	 c->m_anchors[0].y!=conf_default.curve[0].m_anchors[0].y ||
	 c->m_anchors[1].x!=conf_default.curve[0].m_anchors[1].x ||
	 c->m_anchors[1].y!=conf_default.curve[0].m_anchors[1].y ) {
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
