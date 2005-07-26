/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 * by udi Fuchs
 *
 * ufraw.h - Common definitions for UFRaw.
 *
 * based on the gimp plug-in by Pawel T. Jochym jochym at ifj edu pl,
 *
 * based on the gimp plug-in by Dave Coffin
 * http://www.cybercom.net/~dcoffin/
 *
 * UFRaw is licensed under the GNU General Public License.
 * It uses "dcraw" code to do the actual raw decoding.
 */

#ifndef _UFRAW_H
#define _UFRAW_H

#include <gtk/gtk.h>
#include "nikon_curve.h"

#define MAXOUT 255 /* Max output sample */

#define max_curves 20
#define max_anchors 20
#define max_profiles 20
#define max_path 200
#define max_name 80

/* An impossible value for conf float values */
#define NULLF -10000.0

/* Options, like auto-adjust buttons can be in 3 states. Enabled and disabled
 * are obvious. Apply means that the option was selected and some function
 * has to act accourdingly, before changing to one of the first two states */
enum { disabled_state, enabled_state, apply_state };
enum { manual_wb, camera_wb, auto_wb };
/* spot_wb is not part of the above enum since it is not an option in the
 * combo-box, and also because it was added later and we do not want to change
 * the numbering */
#define spot_wb -1
enum { rgb_histogram, r_g_b_histogram, luminosity_histogram, value_histogram,
       saturation_histogram };
enum { linear_histogram, log_histogram };
enum { full_interpolation, four_color_interpolation, quick_interpolation,
       half_interpolation};
enum { no_id, also_id, only_id };
enum { manual_curve, linear_curve, camera_curve };
enum { in_profile, out_profile, profile_types};
enum { raw_expander, exposure_expander, wb_expander, color_expander,
       curve_expander, live_expander, expander_count };
enum { ppm8_type, ppm16_type, tiff8_type, tiff16_type, jpeg_type };

typedef struct {
    char *name;
    double red, blue;
} wb_data;

typedef struct {
    unsigned rgbMax, max, colors, useMatrix;
    int rgbWB[4], colorMatrix[3][4];
    double gamma, linear;
    char profileFile[2][max_path];
    void *profile[2];
    int intent;
    gboolean updateTransform;
    void *colorTransform;
    double saturation;
    guint16 gammaCurve[0x10000];
    CurveData toneCurveData;
    guint16 toneCurve[0x10000];
    guint16 saturationCurve[0x10000];
} developer_data;

typedef guint16 image_type[4];

typedef struct {
    char name[max_name];
    char file[max_path];
    char productName[max_name];
    double gamma, linear;
    gboolean useMatrix;
} profile_data;

/* conf_data holds the configuration data of UFRaw.
 * The data can be split into three groups:
 * IMAGE manipulation, SAVE options and GUI settings.
 * The sources for this information are:
 * DEF: UFRaw's defaults from conf_defaults.
 * RC: users defaults from ~/.ufrawrc. These options are set from the last
 *     interactive session.
 *     If saveConfiguration==disabled_state, IMAGE options are not saved.
 * ID: ufraw ID files used on their original image.
 * CONF: same ID files used as configuration for other raw images.
 * CMD: command line options.
 * UI: interactive input.
 * The options are set in the above order, therefore the last sources will
 * override the first ones with some subtelties:
 * * ID|CONF contains only data which is different from DEF, still it is
 *   assumed that IMAGE and SAVE options are included. Therefore missing
 *   options are set to DEF overwriting RC.
 * * if both CONF and ID are specified, only in/out filenames are taken from ID.
 * * in batch mode SAVE options from RC are ignored.
 * Some fields need special treatment:
 * RC|CONF: auto[Exposure|Black]==enable_state it is switched to apply_state.
 * RC|CONF: if !spot_wb reset chanMul[] to -1.0.
 * CONF|ID: curve/profile are added to the list from RC.
 * CONF: inputFilename, outputFilename are ignored.
 * outputPath can only be specified in CMD or guessed in interactive mode.
 * ID: createID==only_id is switched to also_id.
 * ID: chanMul[] override wb, green, temperature.
 */
typedef struct {
    /* Internal data */
    int confSize, version;

    /* IMAGE manipulation settings */
    int wb;
    double temperature, green;
    double chanMul[4];
    double exposure, saturation, black; /* black is only used in CMD */
    gboolean unclip;
    int autoExposure, autoBlack;
    int curveIndex, curveCount;
    CurveData curve[max_curves];
    int profileIndex[profile_types], profileCount[profile_types];
    profile_data profile[profile_types][max_profiles];
    int intent;
    int interpolation;

    /* SAVE options */
    char inputFilename[max_path], outputFilename[max_path],
	 outputPath[max_path];
    int type, compression, createID, embedExif;
    int shrink, size;
    gboolean overwrite, losslessCompress;

    /* GUI settings */
    int saveConfiguration;
    int histogram, liveHistogramScale, liveHistogramHeight;
    int rawHistogramScale, rawHistogramHeight;
    int expander[expander_count];
    gboolean overExp, underExp;
    char curvePath[max_path];
    char profilePath[max_path];
} conf_data;

typedef struct {
    image_type *image;
    int height, width, trim;
} image_data;

typedef struct {
    char filename[max_path];
    int predictateHeight, predictateWidth, rgbMax, colors, use_coeff, useMatrix;
    float coeff[3][4];
    image_data image;
    void *raw;
    developer_data *developer;
    conf_data *conf;
    void *widget;
    guchar *exifBuf;
    guint exifBufLen;
    int gimpImage;
} ufraw_data;

extern const conf_data conf_default;
extern const wb_data wb_preset[];
extern const int wb_preset_count;
extern const char raw_ext[];
extern const char *file_type[];

/* prototypes for functions in ufraw_ufraw.c */
ufraw_data *ufraw_open(char *filename);
int ufraw_config(ufraw_data *uf, conf_data *rc, conf_data *conf,conf_data *cmd);
int ufraw_load_raw(ufraw_data *uf);
int ufraw_convert_image(ufraw_data *uf);
void ufraw_close(ufraw_data *uf);
int ufraw_set_wb(ufraw_data *uf);
void ufraw_auto_expose(ufraw_data *uf);
void ufraw_auto_black(ufraw_data *uf);
void ufraw_auto_curve(ufraw_data *uf);
void ufraw_batch_messenger(char *message, void *parentWindoW);

/* prototypes for functions in ufraw_preview.c */
int ufraw_preview(ufraw_data *uf, int plugin, long (*write_func)());
void ufraw_focus(void *window, gboolean focus);
void ufraw_messenger(char *message, void *parentWindow);
void preview_progress(void *widget, char *text, double progress);

/* prototypes for functions in ufraw_routines.c */
const char *uf_get_home_dir();
char *uf_file_set_type(const char *filename, const char *type);
char *uf_file_set_absolute(const char *filename);
/* Set locale of LC_NUMERIC to "C" to make sure that printf behaves correctly.*/char *uf_set_locale_C();
void uf_reset_locale(char *locale);
double profile_default_linear(profile_data *p);
double profile_default_gamma(profile_data *p);
int curve_load(CurveData *cp, char *filename);
int curve_save(CurveData *cp, char *filename);
char *curve_buffer(CurveData *cp);

/* prototypes for functions in ufraw_conf.c */
int conf_load(conf_data *c, const char *confFilename);
int conf_save(conf_data *c, char *confFilename, char *buf, int bufSize);
/* Copy the image manipulation options from *src to *dst */
void conf_copy_image(conf_data *dst, const conf_data *src);
/* Copy the 'save options' from *src to *dst */
void conf_copy_save(conf_data *dst, const conf_data *src);
int conf_set_cmd(conf_data *conf, const conf_data *cmd);

/* prototype for functions in ufraw_developer.c */
developer_data *developer_init();
void developer_destroy(developer_data *d);
void developer_profile(developer_data *d, int type, profile_data *p);
void developer_prepare(developer_data *d, int rgbMax, double exposure,
    int unclip, double chanMul[4], float coeff[3][4], int colors, int useMatrix,
    profile_data *in, profile_data *out, int intent,
    double saturation, CurveData *curve);
void develope(void *po, guint16 pix[4], developer_data *d, int mode,
    guint16 *buf, int count);

/* prototype for functions in ufraw_saver.c */
long ufraw_saver(void *widget, gpointer user_data);

/* prototype for functions in ufraw_writer.c */
int ufraw_write_image(ufraw_data *uf);
int ufraw_batch_saver(ufraw_data *uf);

/* prototype for functions in ufraw_chooser.c */
#ifdef HAVE_GTK_2_6
void ufraw_chooser_toggle(GtkToggleButton *button, GtkFileChooser *filechooser);
#endif
void ufraw_chooser(conf_data *conf, char *defPath);

/* prototype for functions in ufraw_exif.c */
int ufraw_exif_from_raw(void *ifd, char *filename, unsigned char **exifBuf,
    unsigned int *exifBufLen);

/* status numbers from dcraw and ufraw */
#define UFRAW_SUCCESS 0
//#define UFRAW_DCRAW_ERROR 1 /* General dcraw unrecoverable error */
//#define UFRAW_DCRAW_UNSUPPORTED 2
//#define UFRAW_DCRAW_NO_CAMERA_WB 3
//#define UFRAW_DCRAW_VERBOSE 4
//#define UFRAW_DCRAW_OPEN_ERROR 5
#define UFRAW_DCRAW_SET_LOG 4 /* DCRAW_VERBOSE */
#define UFRAW_ERROR 100
#define UFRAW_CANCEL 101
#define UFRAW_ABORT_SAVE 102 /* Unrecoverable error during image save */
#define UFRAW_RC_VERSION 103 /* Mismatch in version from .ufrawrc */
#define UFRAW_WARNING 104
#define UFRAW_MESSAGE 105
#define UFRAW_SET_ERROR 200
#define UFRAW_SET_WARNING 201
#define UFRAW_SET_LOG 202
#define UFRAW_GET_ERROR 203 /* Return the warning buffer if an error occured */
#define UFRAW_GET_WARNING 204 /* Return the warning buffer */
#define UFRAW_GET_LOG 205 /* Return the log buffer */
#define UFRAW_BATCH_MESSAGE 206
#define UFRAW_INTERACTIVE_MESSAGE 207
#define UFRAW_REPORT 208 /* Report previews messages */
#define UFRAW_CLEAN 209 /* Clean all buffers */
#define UFRAW_RESET 210 /* Reset warnings and errors */
#define UFRAW_SET_PARENT 211 /* Set parent window for message dialog */
#define UFRAW_LCMS_WARNING 0x1000
#define UFRAW_LCMS_RECOVERABLE 0x2000
#define UFRAW_LCMS_ABORTED 0x3000

char *ufraw_message(int code, const char *format, ...);

#endif /*_UFRAW_H*/
