/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 * by Udi Fuchs,
 *
 * ufraw_routines.c - general routines
 *
 * UFRaw is licensed under the GNU General Public License.
 * It uses DCRaw code to do the actual raw decoding.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
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
	file = g_strconcat(filename, type, NULL);
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
    if (g_path_is_absolute(filename)) {
	return g_strdup(filename);
    } else {
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
	char *cd = g_get_current_dir();
	char *fn = g_build_filename(cd, filename, NULL);
	g_free(cd);
	return fn;
#endif
    }
}

char *uf_markup_buf(char *buffer, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    char *line = g_markup_vprintf_escaped(format, ap);
    va_end(ap);
    if (buffer==NULL) {
	return line;
    } else {
	char *buf;
	buf = g_strconcat(buffer, line, NULL);
	g_free(line);
	g_free(buffer);
	return buf;
    }
}

/* The first column is the "make" of the camera, the
   second is the "model".  Use the make and model as provided
   by dcraw */
const wb_data wb_preset[] = { 

  { "", "", "Manual WB",       { 0, 0, 0, 0} },
  { "", "", "Camera WB",       { 0, 0, 0, 0} },
  { "", "", "Auto WB",         { 0, 0, 0, 0} },

  { "NIKON", "D50", "Incandescent", { 1.328125, 1, 2.500000, 0 } },
  { "NIKON", "D50", "Fluorescent",  { 1.945312, 1, 2.191406, 0 } },
  { "NIKON", "D50", "Sunny",        { 2.140625, 1, 1.398438, 0 } },
  { "NIKON", "D50", "Flash",        { 2.398438, 1, 1.339844, 0 } },
  { "NIKON", "D50", "Shade",        { 2.746094, 1, 1.156250, 0 } },

  { "NIKON", "D70", "Incandescent", { 1.34375, 1, 2.816406, 0 } }, /* 3000K */
  { "NIKON", "D70", "Fluorescent",  { 1.964844, 1, 2.476563, 0 } }, /* 4200K */
  { "NIKON", "D70", "Direct sunlight", { 2.0625, 1, 1.597656, 0 } }, /* 5200K*/
  { "NIKON", "D70", "Flash",	    { 2.441406, 1, 1.5, 0 } },	  /* 5400K */
  { "NIKON", "D70", "Cloudy",	    { 2.257813, 1, 1.457031, 0 } }, /* 6000K */
  { "NIKON", "D70", "Shade",	    { 2.613281, 1, 1.277344, 0 } }, /* 8000K */

  /*
   * D2X with firmware A 1.01 and B 1.01
   * D2X basic WB presets
   */
  { "NIKON", "D2X", "Incandescent", { 0.92086, 1, 2.89928, 0 } }, /* 3000K */
  { "NIKON", "D2X", "Fluorescent",  { 1.42969, 1, 2.62891, 0 } }, /* ~~4200K */
  { "NIKON", "D2X", "Direct sunlight", { 1.52734, 1, 1.69531, 0 } }, /* ~5200K */
  { "NIKON", "D2X", "Flash",	    { 1.66016, 1, 1.53125, 0 } }, /* ~5400K */
  { "NIKON", "D2X", "Cloudy",	    { 1.69141, 1, 1.54688, 0 } }, /* ~6000K */
  { "NIKON", "D2X", "Shade",	    { 1.98047, 1, 1.32812, 0 } }, /* ~8000K */

  /* D2X fine tune presets (0 is basic, above) */
  { "NIKON", "D2X", "Incandescent -3", { 0.98462, 1, 2.61154, 0 } }, /* ~3300K */
  { "NIKON", "D2X", "Incandescent -2", { 0.95880, 1, 2.71536, 0 } }, /* ~3200K */
  { "NIKON", "D2X", "Incandescent -1", { 0.94465, 1, 2.77122, 0 } }, /* 3100K */
  { "NIKON", "D2X", "Incandescent +1", { 0.89510, 1, 3.03846, 0 } }, /* ~2900K */
  { "NIKON", "D2X", "Incandescent +2", { 0.86486, 1, 3.17905, 0 } }, /* 2800K */
  { "NIKON", "D2X", "Incandescent +3", { 0.83388, 1, 3.34528, 0 } }, /* 2700K */
  { "NIKON", "D2X", "Fluorescent -3", { 2.01562, 1, 1.72266, 0 } }, /* ~~7200K */
  { "NIKON", "D2X", "Fluorescent -2", { 1.67969, 1, 1.42578, 0 } }, /* ~~6500K */
  { "NIKON", "D2X", "Fluorescent -1", { 1.42969, 1, 1.80078, 0 } }, /* ~~5000K */
  { "NIKON", "D2X", "Fluorescent +1", { 1.13672, 1, 3.02734, 0 } }, /* ~~3700K */
  { "NIKON", "D2X", "Fluorescent +2", { 0.94118, 1, 2.68498, 0 } }, /* ~~3000K */
  { "NIKON", "D2X", "Fluorescent +3", { 0.83388, 1, 3.51140, 0 } }, /* ~~2700K */
  { "NIKON", "D2X", "Direct sunlight -3", { 1.61328, 1, 1.61328, 0 } }, /* 5600K */
  { "NIKON", "D2X", "Direct sunlight -2", { 1.57031, 1, 1.65234, 0 } }, /* ~5400K */
  { "NIKON", "D2X", "Direct sunlight -1", { 1.55078, 1, 1.67578, 0 } }, /* 5300K */
  { "NIKON", "D2X", "Direct sunlight +1", { 1.48438, 1, 1.74609, 0 } }, /* 5000K */
  { "NIKON", "D2X", "Direct sunlight +2", { 1.45312, 1, 1.76953, 0 } }, /* ~4900K */
  { "NIKON", "D2X", "Direct sunlight +3", { 1.42578, 1, 1.78906, 0 } }, /* 4800K */
  { "NIKON", "D2X", "Flash -3",	    { 1.71484, 1, 1.48438, 0 } }, /* ~~6000K */
  { "NIKON", "D2X", "Flash -2",	    { 1.67578, 1, 1.48438, 0 } }, /* ~~5800K */
  { "NIKON", "D2X", "Flash -1",	    { 1.66797, 1, 1.50781, 0 } }, /* ~~5600K */
  { "NIKON", "D2X", "Flash +1",	    { 1.64453, 1, 1.54297, 0 } }, /* ~~5200K */
  { "NIKON", "D2X", "Flash +2",	    { 1.62891, 1, 1.54297, 0 } }, /* ~~5000K */
  { "NIKON", "D2X", "Flash +3",	    { 1.57031, 1, 1.56641, 0 } }, /* ~~4800K */
  { "NIKON", "D2X", "Cloudy -3",    { 1.79297, 1, 1.46875, 0 } }, /* ~6600K */
  { "NIKON", "D2X", "Cloudy -2",    { 1.76172, 1, 1.49219, 0 } }, /* ~6400K */
  { "NIKON", "D2X", "Cloudy -1",    { 1.72656, 1, 1.51953, 0 } }, /* ~6200K */
  { "NIKON", "D2X", "Cloudy +1",    { 1.65234, 1, 1.57812, 0 } }, /* ~5800K */
  { "NIKON", "D2X", "Cloudy +2",    { 1.61328, 1, 1.61328, 0 } }, /* 5600K */
  { "NIKON", "D2X", "Cloudy +3",    { 1.57031, 1, 1.65234, 0 } }, /* ~5400K */
  { "NIKON", "D2X", "Shade -3",	    { 2.10938, 1, 1.23828, 0 } }, /* ~9200K */
  { "NIKON", "D2X", "Shade -2",	    { 2.07031, 1, 1.26562, 0 } }, /* ~8800K */
  { "NIKON", "D2X", "Shade -1",	    { 2.02734, 1, 1.29688, 0 } }, /* ~8400K */
  { "NIKON", "D2X", "Shade +1",	    { 1.92188, 1, 1.37109, 0 } }, /* ~7500K */
  { "NIKON", "D2X", "Shade +2",	    { 1.86719, 1, 1.41406, 0 } }, /* 7100K */
  { "NIKON", "D2X", "Shade +3",	    { 1.80859, 1, 1.45703, 0 } }, /* 6700K */

  /* D2X Kelvin presets */
  { "NIKON", "D2X", "2500K",	    { 0.74203, 1, 3.67536, 0 } },
  { "NIKON", "D2X", "2550K",	    { 0.76877, 1, 3.58859, 0 } },
  { "NIKON", "D2X", "2650K",	    { 0.81529, 1, 3.42675, 0 } },
  { "NIKON", "D2X", "2700K",	    { 0.83388, 1, 3.34528, 0 } },
  { "NIKON", "D2X", "2800K",	    { 0.86486, 1, 3.17905, 0 } },
  { "NIKON", "D2X", "2850K",	    { 0.87973, 1, 3.10309, 0 } },
  { "NIKON", "D2X", "2950K",	    { 0.90780, 1, 2.96454, 0 } },
  { "NIKON", "D2X", "3000K",	    { 0.92086, 1, 2.89928, 0 } },
  { "NIKON", "D2X", "3100K",	    { 0.94465, 1, 2.77122, 0 } },
  { "NIKON", "D2X", "3200K",	    { 0.96970, 1, 2.65530, 0 } },
  { "NIKON", "D2X", "3300K",	    { 0.99611, 1, 2.55642, 0 } },
  { "NIKON", "D2X", "3400K",	    { 1.01953, 1, 2.46484, 0 } },
  { "NIKON", "D2X", "3600K",	    { 1.07422, 1, 2.34375, 0 } },
  { "NIKON", "D2X", "3700K",	    { 1.09766, 1, 2.26172, 0 } },
  { "NIKON", "D2X", "3800K",	    { 1.12500, 1, 2.18750, 0 } },
  { "NIKON", "D2X", "4000K",	    { 1.17969, 1, 2.06250, 0 } },
  { "NIKON", "D2X", "4200K",	    { 1.24219, 1, 1.96094, 0 } },
  { "NIKON", "D2X", "4300K",	    { 1.27344, 1, 1.91797, 0 } },
  { "NIKON", "D2X", "4500K",	    { 1.33594, 1, 1.83984, 0 } },
  { "NIKON", "D2X", "4800K",	    { 1.42578, 1, 1.78906, 0 } },
  { "NIKON", "D2X", "5000K",	    { 1.48438, 1, 1.74609, 0 } },
  { "NIKON", "D2X", "5300K",	    { 1.55078, 1, 1.67578, 0 } },
  { "NIKON", "D2X", "5600K",	    { 1.61328, 1, 1.61328, 0 } },
  { "NIKON", "D2X", "5900K",	    { 1.67188, 1, 1.56250, 0 } },
  { "NIKON", "D2X", "6300K",	    { 1.74219, 1, 1.50391, 0 } },
  { "NIKON", "D2X", "6700K",	    { 1.80859, 1, 1.45703, 0 } },
  { "NIKON", "D2X", "7100K",	    { 1.86719, 1, 1.41406, 0 } },
  { "NIKON", "D2X", "7700K",	    { 1.94531, 1, 1.35547, 0 } },
  { "NIKON", "D2X", "8300K",	    { 2.01562, 1, 1.30469, 0 } },
  { "NIKON", "D2X", "9100K",	    { 2.09766, 1, 1.24609, 0 } },
  { "NIKON", "D2X", "10000K",	    { 2.17578, 1, 1.18359, 0 } },

  { "Minolta", "DiMAGE 7Hi", "Daylight",    { 1.609375, 1, 1.328125, 0 } },  /* 5500K */
  { "Minolta", "DiMAGE 7Hi", "Tungsten",    { 0.878906, 1, 2.433594, 0 } },  /* 2800K */
  { "Minolta", "DiMAGE 7Hi", "Fluorescent 1", { 1.664062, 1, 2.105469, 0 } },  /* 4060K*/
  { "Minolta", "DiMAGE 7Hi", "Fluorescent 2", { 1.796875, 1, 1.734375, 0 } },  /* 4938K */
  { "Minolta", "DiMAGE 7Hi", "Cloudy",      { 1.730469, 1, 1.269531, 0 } },  /* 5823K */

  { "Canon", "EOS DIGITAL REBEL XT", "Tungsten",    { 1.554250, 1, 2.377034, 0 } }, 
  { "Canon", "EOS DIGITAL REBEL XT", "Daylight",    { 2.392927, 1, 1.487230, 0 } }, 
  { "Canon", "EOS DIGITAL REBEL XT", "Fluorescent", { 1.999040, 1, 1.995202, 0 } }, 
  { "Canon", "EOS DIGITAL REBEL XT", "Shade",	    { 2.827112, 1, 1.235756, 0 } },
  { "Canon", "EOS DIGITAL REBEL XT", "Flash",	    { 2.715128, 1, 1.295678, 0 } }, 
  { "Canon", "EOS DIGITAL REBEL XT", "Cloudy",	    { 2.611984, 1, 1.343811, 0 } },

  { "Canon", "EOS 350D DIGITAL", "Tungsten",    { 1.554250, 1, 2.377034, 0 } }, 
  { "Canon", "EOS 350D DIGITAL", "Daylight",    { 2.392927, 1, 1.487230, 0 } }, 
  { "Canon", "EOS 350D DIGITAL", "Fluorescent", { 1.999040, 1, 1.995202, 0 } }, 
  { "Canon", "EOS 350D DIGITAL", "Shade",      { 2.827112, 1, 1.235756, 0 } },
  { "Canon", "EOS 350D DIGITAL", "Flash",      { 2.715128, 1, 1.295678, 0 } }, 
  { "Canon", "EOS 350D DIGITAL", "Cloudy",     { 2.611984, 1, 1.343811, 0 } },

  { "Canon", "EOS 10D", "Sunlight", { 2.159856, 1, 1.218750, 0 } }, 
  { "Canon", "EOS 10D", "Shadow",   { 2.533654, 1, 1.036058, 0 } }, 
  { "Canon", "EOS 10D", "Cloudy",   { 2.348558, 1, 1.116587, 0 } }, 
  { "Canon", "EOS 10D", "Indescandent", { 1.431544, 1, 1.851040, 0 } }, 
  { "Canon", "EOS 10D", "Flourescent", { 1.891509, 1, 1.647406, 0 } }, 
  { "Canon", "EOS 10D", "Flash",    { 2.385817, 1, 1.115385, 0 } },

  { "FUJIFILM", "FinePix S5000", "Incandescent", { 1.212081, 1, 2.672364, 0 } },
  { "FUJIFILM", "FinePix S5000", "Fluorescent",  { 1.772316, 1, 2.349902, 0 } },
  { "FUJIFILM", "FinePix S5000", "Direct sunlight", { 1.860403, 1, 1.515946, 0 } },
  { "FUJIFILM", "FinePix S5000", "Flash",   { 2.202181, 1, 1.423284, 0 } },
  { "FUJIFILM", "FinePix S5000", "Cloudy",  { 2.036578, 1, 1.382513, 0 } },
  { "FUJIFILM", "FinePix S5000", "Shade",   { 2.357215, 1, 1.212016, 0 } }
};
const int wb_preset_count = sizeof(wb_preset) / sizeof(wb_data);

const char raw_ext[]= "bay,bmq,cr2,crw,cs1,dc2,dcr,dng,erf,fff,hdr,jpg,k25,"
	"kdc,mdc,mos,mrw,nef,orf,pef,pxn,raf,raw,rdc,sr2,srf,sti,tif,ufraw,x3f";

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

/* Convert between Temperature and RGB.
 * Base on information from http://www.brucelindbloom.com/
 * The generalization to 2000K < T < 4000K is my own and
 * should be taken with a grain of salt.
 */
const double XYZ_to_RGB[3][3] = {
    { 3.24071,	-0.969258,  0.0556352 },
    {-1.53726,	1.87599,    -0.203996 },
    {-0.498571,	0.0415557,  1.05707 } };

void Temperature_to_RGB(double T, double RGB[3])
{
    int c;
    double xD, yD, X, Y, Z, max;
    if (T<= 4000) {
        xD = 0.27475e9/(T*T*T) - 0.98598e6/(T*T) + 1.17444e3/T + 0.145986;
    } else if (T<= 7000) {
	xD = -4.6070e9/(T*T*T) + 2.9678e6/(T*T) + 0.09911e3/T + 0.244063;
    } else {
	xD = -2.0064e9/(T*T*T) + 1.9018e6/(T*T) + 0.24748e3/T + 0.237040;
    }
    yD = -3*xD*xD + 2.87*xD - 0.275;
    X = xD/yD;
    Y = 1;
    Z = (1-xD-yD)/yD;
    max = 0;
    for (c=0; c<3; c++) {
	RGB[c] = X*XYZ_to_RGB[0][c] + Y*XYZ_to_RGB[1][c] + Z*XYZ_to_RGB[2][c];
	if (RGB[c]>max) max = RGB[c];
    }
    for (c=0; c<3; c++) RGB[c] = RGB[c]/max;
}

void RGB_to_Temperature(double RGB[3], double *T, double *Green)
{
    double Tmax, Tmin, testRGB[3];
    Tmin = 2000;
    Tmax = 12000;
    for (*T=(Tmax+Tmin)/2; Tmax-Tmin>10; *T=(Tmax+Tmin)/2) {
	Temperature_to_RGB(*T, testRGB);
	if (testRGB[2]/testRGB[0] > RGB[2]/RGB[0])
	    Tmax = *T;
	else
	    Tmin = *T;
    }
    *Green = (testRGB[1]/testRGB[0]) / (RGB[1]/RGB[0]);
}

/* Following code might be useful if we want to convert temperature from
 * pre 0.7 calculation to the current temperature calculation */
/*
#include <blackbody.h>
void BB_Temperature_to_RGB(double T, double RGB[3])
{
    int i = T/10-200;
    int c;
    for (c=0; c<3; c++) RGB[c] = bbWB[c];
}
*/

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
    char *buf = NULL;
    int i;
    if ( c->m_min_x!=conf_default.curve[0].m_min_x ||
	 c->m_min_y!=conf_default.curve[0].m_min_y ||
	 c->m_max_x!=conf_default.curve[0].m_max_x ||
	 c->m_max_y!=conf_default.curve[0].m_max_y ) {
	buf = uf_markup_buf(buf,
		"\t<MinXY>%lf %lf</MinXY>\n", c->m_min_x, c->m_min_y);
	buf = uf_markup_buf(buf,
                "\t<MaxXY>%lf %lf</MaxXY>\n", c->m_max_x, c->m_max_y);
    }
    if ( c->m_numAnchors!=conf_default.curve[0].m_numAnchors ||
	 c->m_anchors[0].x!=conf_default.curve[0].m_anchors[0].x ||
	 c->m_anchors[0].y!=conf_default.curve[0].m_anchors[0].y ||
	 c->m_anchors[1].x!=conf_default.curve[0].m_anchors[1].x ||
	 c->m_anchors[1].y!=conf_default.curve[0].m_anchors[1].y ) {
        for (i=0; i<c->m_numAnchors; i++)
	    buf = uf_markup_buf(buf,
                    "\t<AnchorXY>%lf %lf</AnchorXY>\n",
                    c->m_anchors[i].x, c->m_anchors[i].y);
    }
    return buf;
}
