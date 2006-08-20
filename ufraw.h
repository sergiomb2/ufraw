/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw.h - Common definitions for UFRaw.
 * Copyright 2004-2006 by Udi Fuchs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation. You should have received
 * a copy of the license along with this program.
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

/* An impossible value for conf float values */
#define NULLF -10000.0

/* Options, like auto-adjust buttons can be in 3 states. Enabled and disabled
 * are obvious. Apply means that the option was selected and some function
 * has to act accourdingly, before changing to one of the first two states */
enum { disabled_state, enabled_state, apply_state };

#define spot_wb "Spot WB"
#define manual_wb "Manual WB"
#define camera_wb "Camera WB"
#define auto_wb "Auto WB"

enum { rgb_histogram, r_g_b_histogram, luminosity_histogram, value_histogram,
       saturation_histogram };
enum { linear_histogram, log_histogram };
/* The following enum should match the dcraw_interpolation enum
 * in dcraw_api.h. */
enum { ahd_interpolation, vng_interpolation, four_color_interpolation,
       bilinear_interpolation, half_interpolation };
extern const char *interpolationNames[];
enum { no_id, also_id, only_id };
enum { manual_curve, linear_curve, custom_curve, camera_curve };
enum { in_profile, out_profile, profile_types};
enum { raw_expander, exposure_expander, wb_expander, color_expander,
       curve_expander, live_expander, expander_count };
enum { ppm8_type, ppm16_type, tiff8_type, tiff16_type, jpeg_type,
       embedded_jpeg_type };

typedef struct {
    char *make;
    char *model;
    char *name;
    int tuning;
    double channel[4];
} wb_data;

typedef struct {
    unsigned rgbMax, max, exposure, colors, useMatrix;
    gboolean unclip;
    int rgbWB[4], rgbNorm[4], colorMatrix[3][4];
    double gamma, linear;
    char profileFile[2][max_path];
    void *profile[2];
    int intent;
    gboolean updateTransform;
    void *colorTransform;
    double saturation;
    CurveData baseCurveData, luminosityCurveData;
    guint16 gammaCurve[0x10000];
    void *luminosityProfile;
    void *TransferFunction[3];
    void *saturationProfile;
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
 * ID: UFRaw ID files used on their original image.
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
//    int wb;
    char wb[max_name];
    double WBTuning;
    double temperature, green;
    double chanMul[4];
    double exposure, saturation, black; /* black is only used in CMD */
    gboolean unclip;
    int autoExposure, autoBlack;
    int BaseCurveIndex, BaseCurveCount;
    CurveData BaseCurve[max_curves];
    int curveIndex, curveCount;
    CurveData curve[max_curves];
    int profileIndex[profile_types], profileCount[profile_types];
    profile_data profile[profile_types][max_profiles];
    int intent;
    int interpolation;
    char darkframeFile[max_path];
    struct ufraw_struct *darkframe;

    /* SAVE options */
    char inputFilename[max_path], outputFilename[max_path],
	 outputPath[max_path];
    int type, compression, createID, embedExif;
    int shrink, size;
    gboolean overwrite, losslessCompress, embeddedImage;

    /* GUI settings */
    double Zoom;
    int Scale;
    int saveConfiguration;
    int histogram, liveHistogramScale, liveHistogramHeight;
    int rawHistogramScale, rawHistogramHeight;
    int expander[expander_count];
    gboolean overExp, underExp;
    char curvePath[max_path];
    char profilePath[max_path];

    /* EXIF data */
    int flip;
    float iso_speed, shutter, aperture, focal_len;
    char exifSource[max_name], isoText[max_name], shutterText[max_name],
	 apertureText[max_name], focalLenText[max_name], lensText[max_name];
    char timestamp[max_name], make[max_name], model[max_name];
} conf_data;

typedef struct {
    image_type *image;
    guint8 *buffer;
    int height, width;
} image_data;

typedef struct ufraw_struct {
    char filename[max_path];
    int predictedHeight, predictedWidth, rgbMax, colors, raw_color, useMatrix;
    float rgb_cam[3][4];
    image_data image;
    image_data thumb;
    void *raw;
    developer_data *developer;
    conf_data *conf;
    void *widget;
    guchar *exifBuf;
    guint exifBufLen;
    int gimpImage;
    int *RawLumHistogram;
    int RawLumChanMul[4];
    int RawLumCount;
} ufraw_data;

extern const conf_data conf_default;
extern const wb_data wb_preset[];
extern const int wb_preset_count;
extern const char raw_ext[];
extern const char *file_type[];

/* ufraw_binary contains the name of the binary file for error messages.
 * It should be set in every UFRaw main() */
extern char *ufraw_binary;

/* prototypes for functions in ufraw_ufraw.c */
ufraw_data *ufraw_open(char *filename);
int ufraw_config(ufraw_data *uf, conf_data *rc, conf_data *conf,conf_data *cmd);
int ufraw_load_raw(ufraw_data *uf);
ufraw_data *ufraw_load_darkframe(char *darkframeFilename);
int ufraw_convert_image(ufraw_data *uf);
void ufraw_close(ufraw_data *uf);
int ufraw_set_wb(ufraw_data *uf);
void ufraw_auto_expose(ufraw_data *uf);
void ufraw_auto_black(ufraw_data *uf);
void ufraw_auto_curve(ufraw_data *uf);
void ufraw_batch_messenger(char *message);

/* prototypes for functions in ufraw_preview.c */
int ufraw_preview(ufraw_data *uf, int plugin, long (*save_func)());
void ufraw_focus(void *window, gboolean focus);
void ufraw_messenger(char *message, void *parentWindow);
void preview_progress(void *widget, char *text, double progress);

/* prototypes for functions in ufraw_routines.c */
const char *uf_get_home_dir();
char *uf_file_set_type(const char *filename, const char *type);
char *uf_file_set_absolute(const char *filename);
/* Set locale of LC_NUMERIC to "C" to make sure that printf behaves correctly.*/char *uf_set_locale_C();
void uf_reset_locale(char *locale);
char *uf_markup_buf(char *buffer, const char *format, ...);
double profile_default_linear(profile_data *p);
double profile_default_gamma(profile_data *p);
void Temperature_to_RGB(double T, double RGB[3]);
void RGB_to_Temperature(double RGB[3], double *T, double *Green);
int curve_load(CurveData *cp, char *filename);
int curve_save(CurveData *cp, char *filename);
char *curve_buffer(CurveData *cp);

/* prototypes for functions in ufraw_conf.c */
int conf_load(conf_data *c, const char *confFilename);
int conf_save(conf_data *c, char *confFilename, char **confBuffer);
/* Copy the image manipulation options from *src to *dst */
void conf_copy_image(conf_data *dst, const conf_data *src);
/* Copy the 'save options' from *src to *dst */
void conf_copy_save(conf_data *dst, const conf_data *src);
int conf_set_cmd(conf_data *conf, const conf_data *cmd);
int ufraw_process_args(int *argc, char ***argv, conf_data *cmd, conf_data *rc);

/* prototype for functions in ufraw_developer.c */
developer_data *developer_init();
void developer_destroy(developer_data *d);
void developer_profile(developer_data *d, int type, profile_data *p);
void developer_prepare(developer_data *d, int rgbMax, double exposure,
    int unclip, double chanMul[4], float rgb_cam[3][4], int colors,
    int useMatrix, profile_data *in, profile_data *out, int intent,
    double saturation, CurveData *baseCurve, CurveData *curve);
void develope(void *po, guint16 pix[4], developer_data *d, int mode,
    guint16 *buf, int count);

/* prototype for functions in ufraw_saver.c */
long ufraw_saver(void *widget, gpointer user_data);

/* prototype for functions in ufraw_writer.c */
int ufraw_write_image(ufraw_data *uf);
int ufraw_batch_saver(ufraw_data *uf);

/* prototype for functions in ufraw_embedded.c */
int ufraw_read_embedded(ufraw_data *uf);
int ufraw_convert_embedded(ufraw_data *uf);
int ufraw_write_embedded(ufraw_data *uf);

/* prototype for functions in ufraw_chooser.c */
void ufraw_chooser(conf_data *conf, char *defPath);

/* prototype for functions in ufraw_exif.c */
int ufraw_exif_from_raw(ufraw_data *uf);

/* status numbers from DCRaw and UFRaw */
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
#define UFRAW_REPORT 208 /* Report previous messages */
#define UFRAW_CLEAN 209 /* Clean all buffers */
#define UFRAW_RESET 210 /* Reset warnings and errors */
#define UFRAW_SET_PARENT 211 /* Set parent window for message dialog */
#define UFRAW_LCMS_WARNING 0x1000
#define UFRAW_LCMS_RECOVERABLE 0x2000
#define UFRAW_LCMS_ABORTED 0x3000

char *ufraw_message(int code, const char *format, ...);

#endif /*_UFRAW_H*/
