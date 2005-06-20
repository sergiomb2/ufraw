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

#include "nikon_curve.h"

#define MAXOUT 255 /* Max output sample */

#define max_curves 20
#define max_anchors 20
#define max_profiles 20
#define max_path 200
#define max_name 80

enum { manual_wb, camera_wb, auto_wb };
enum { rgb_histogram, r_g_b_histogram, luminosity_histogram, value_histogram,
       saturation_histogram };
enum { linear_histogram, log_histogram };
enum { full_interpolation, four_color_interpolation, quick_interpolation,
       half_interpolation};
enum { no_id, also_id, only_id };
enum { load_preserve, load_default, load_auto };
enum { linear_curve, camera_curve };
enum { in_profile, out_profile, profile_types};
enum { raw_expander, exposure_expander, wb_expander, color_expander,
       curve_expander, live_expander, expander_count };
enum { ppm8_type, ppm16_type, tiff8_type, tiff16_type, jpeg_type };

typedef struct {
    char *name;
    double red, blue;
} wb_data;

typedef struct {
    int rgbMax, max;
    int rgbWB[4];
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
} profile_data;

/* cfg holds infomation that is kept for the next image load */
typedef struct {
    int cfgSize, version;
    char inputFilename[max_path], outputFilename[max_path],
	 outputPath[max_path];
    int wbLoad, curveLoad, exposureLoad, type, compression, createID, embedExif;
    int wb, histogram, liveHistogramScale, liveHistogramHeight;
    int rawHistogramScale, rawHistogramHeight;
    int expander[expander_count], interpolation, shrink, size;
    double temperature, green, exposure, saturation, black;
    gboolean autoExposure, autoBlack, autoCurve;
    gboolean unclip, overExp, underExp, overwrite, losslessCompress;
    int curveIndex, curveCount;
    CurveData curve[max_curves];
    char curvePath[max_path];
    int profileIndex[profile_types], profileCount[profile_types];
    profile_data profile[profile_types][max_profiles];
    int intent;
    char profilePath[max_path];
} cfg_data;

typedef struct {
    char filename[max_path];
    int height, width, trim, rgbMax;
    float preMul[4];
    image_type *image;
    void *raw;
    developer_data *developer;
    cfg_data *cfg;
    void *widget;
    guchar *exifBuf;
    guint exifBufLen;
    int gimpImage;
} image_data;

extern const cfg_data cfg_default;
extern const wb_data wb_preset[];
extern const int wb_preset_count;
extern const char raw_ext[];
extern const char *file_type[];

/* prototypes for functions in ufraw_ufraw.c */
image_data *ufraw_open(char *filename);
int ufraw_config(image_data *image, cfg_data *cfg);
int ufraw_load_raw(image_data *image, int interactive);
int ufraw_convert_image(image_data *image, image_data *rawImage);
void ufraw_close(image_data *image);
int ufraw_set_wb(image_data *image);
void ufraw_auto_expose(image_data *image);
void ufraw_auto_black(image_data *image);
void ufraw_auto_curve(image_data *image);
void ufraw_batch_messenger(char *message, void *parentWindoW);

/* prototypes for functions in ufraw_preview.c */
int ufraw_preview(image_data *image, int plugin, long (*write_func)());
void ufraw_focus(void *window, gboolean focus);
void ufraw_messenger(char *message, void *parentWindow);
void preview_progress(void *widget, char *text, double progress);

/* prototypes for functions in ufraw_routines.c */
const char *uf_get_home_dir();
char *uf_file_set_type(const char *filename, const char *type);
char *uf_file_set_absolute(const char *filename);
double uf_nan();
double profile_default_linear(profile_data *p);
double profile_default_gamma(profile_data *p);
int curve_load(CurveData *cp, char *filename);
int curve_save(CurveData *cp, char *filename);
char *curve_buffer(CurveData *cp, gboolean withPoints);
developer_data *developer_init();
void developer_destroy(developer_data *d);
void developer_profile(developer_data *d, int type, profile_data *p);
void developer_prepare(developer_data *d, int rgbMax, double exposure,
    int unclip, double temperature, double green, float pre_mul[4],
    profile_data *in, profile_data *out, int intent,
    double saturation, CurveData *curve);
inline void develope(void *po, guint16 pix[4], developer_data *d, int mode,
    guint16 *buf, int count);
void RGB_to_temperature(double *rgb, double *temperature, double *green);
int load_configuration(cfg_data *c);
int save_configuration(cfg_data *c, developer_data *d,
	char *confFilename, char *buf, int bufSize);

/* prototype for functions in ufraw_saver.c */
long ufraw_saver(void *widget, gpointer user_data);

/* prototype for functions in ufraw_writer.c */
int ufraw_write_image(image_data *image);
int ufraw_batch_saver(image_data *image);

/* prototype for functions in ufraw_chooser.c */
void ufraw_chooser(cfg_data *cfg, char *defPath);

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
