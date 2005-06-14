/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 * by Udi Fuchs
 *
 * ufraw.c - The standalone interface to UFRaw.
 *
 * UFRaw is licensed under the GNU General Public License.
 * It uses DCRaw code to do the actual raw decoding.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <errno.h>     /* for errno */
#include <string.h>
#include <math.h>      /* for isnan */
#include <getopt.h>
#ifdef UFRAW_BATCH
#include <glib.h>
#else
#include <gtk/gtk.h>
#endif
#include <locale.h>
#include "ufraw.h"
#include "ufraw_icon.h"

#define NULLF -10000

char helpText[]=
"UFRaw " VERSION " - Unidentified Flying Raw converter for digital camera images.\n"
"\n"
"Usage: ufraw [ options ... ] [ raw-image-files ... ]\n"
"       ufraw-batch [ options ... ] [ raw-image-files ... ]\n"
"       ufraw [ options ... ] [ default-directory ]\n"
"\n"
"By default 'ufraw' displays a preview window for each raw image allowing\n"
"the user to tweak the image parameters before saving. If no raw images\n"
"are given at the command line, UFRaw will display a file chooser dialog.\n"
"To process the images with no questions asked (and no preview) use\n"
"'ufraw-batch' or the --batch option.\n"
"The rest of the options are separated into two groups.\n"
"The options which are related to the image manipulation are:\n"
"\n"
"--wb=camera|auto      White balance setting.\n"
"--temperature=TEMP    Color temperature in Kelvins (2000 - 7000).\n"
"--green=GREEN         Green color normalization.\n"
"--curve=linear|camera\n"
"                      Type of tone curve to use (default camera if such\n"
"                      exsists, linear otherwise).\n"
"--[no]unclip          Unclip [clip] highlights.\n"
"--gamma=GAMMA         Gamma adjustment of the base curve (default 0.45).\n"
"--linearity=LINEARITY Linearity of the base curve (default 0.10).\n"
"--saturation=SAT      Saturation adjustment (default 1.0, 0 for B&W output).\n"
"--exposure=auto|EXPOSURE\n"
"                      Auto exposure or exposure correction in EV (default 0).\n"
"--black-point=BLACK   Set black point (default 0).\n"
"\n"
"The options which are related to the final output are:\n"
"\n"
"--interpolation=full|four-color|quick|half\n"
"                      Interpolation algorithm to use (default full).\n"
"--shrink=FACTOR       Shrink the image by FACTOR (default 1).\n"
"--size=SIZE           Downsize max(height,width) to SIZE.\n"
"--out-type=ppm8|ppm16|tiff8|tiff16|jpeg\n"
"                      Output file formati (default ppm8).\n"
"--create-id=no|also|only\n"
"                      Create no|also|only ID file (default no).\n"
"--compression=VALUE   JPEG compression (0-100, default 85).\n"
"--[no]exif            Embed exif in output JPEG (default embed exif).\n"
"--[no]zip             Enable [disable] TIFF zip compression (default nozip).\n"
"--out-path=PATH       PATH for output file (default use input file's path).\n"
"--output=FILE         Output file name, use '-' to output to stdout.\n"
"--overwrite           Overwrite existing files without asking (default no).\n"
"\n"
"UFRaw first reads the setting from the configuration file $HOME/.ufrawrc\n"
"and then sets the options from the command line. In batch mode, the second\n"
"group of options is NOT read from the configuration file, and therefore,\n"
"must be specified explicitly if non-default values are desired.\n"
"\n"
"Last, but not least, --help displays this help message and exits.\n";

void process_args(int *argc, char ***argv, cfg_data *cmd, gboolean *batch)
{
    int index=0, c;
    char *wbName=NULL;
    char *curveName=NULL, *outTypeName=NULL, *createIDName=NULL,
	 *outPath=NULL, *output=NULL, *interpolationName=NULL;
    static struct option options[] = {
        {"wb", 1, 0, 'w'},
        {"temperature", 1, 0, 't'},
        {"green", 1, 0, 'g'},
        {"curve", 1, 0, 'c'},
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
/* Binary flags that don't have a value are here at the end */
        {"batch", 0, 0, 'B'},
        {"unclip", 0, 0, 'u'},
        {"nounclip", 0, 0, 'U'},
        {"zip", 0, 0, 'z'},
        {"nozip", 0, 0, 'Z'},
        {"overwrite", 0, 0, 'O'},
	{"exif", 0, 0, 'E'},
	{"noexif", 0, 0, 'F'},
        {"help", 0, 0, 'h'},
        {0, 0, 0, 0}
    };
    void *optPointer[] = { &wbName, &cmd->temperature, &cmd->green, &curveName,
        &cmd->profile[0][0].gamma, &cmd->profile[0][0].linear,
	&cmd->saturation,
        &cmd->exposure, &cmd->black, &interpolationName,
	&cmd->shrink, &cmd->size, &cmd->compression,
	&outTypeName, &createIDName, &outPath, &output };
    cmd->autoExposure = FALSE;
    cmd->unclip=-1;
    cmd->losslessCompress=-1;
    cmd->overwrite=-1;
    cmd->embedExif=-1;
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
		cmd->autoExposure = TRUE;
		break;
	    }
        case 't':
        case 'g':
        case 'C':
        case 'G':
        case 'L':
        case 's':
        case 'S':
        case 'd':
        case 'k':
            if (sscanf(optarg, "%lf", (double *)optPointer[index])==0) {
                ufraw_message(UFRAW_ERROR,
                    "ufraw: '%s' is not a valid value for the --%s option.\n",
                    optarg, options[index].name);
                exit(1);
            }
            break;
        case 'x':
        case 'X':
        case 'j':
            if (sscanf(optarg, "%d", (int *)optPointer[index])==0) {
                ufraw_message(UFRAW_ERROR,
                    "ufraw: '%s' is not a valid value for the --%s option.\n",
                    optarg, options[index].name);
                exit(1);
            }
            break;
        case 'w':
        case 'c':
        case 'i':
        case 'T':
        case 'I':
        case 'p':
        case 'o':
            *(char **)optPointer[index] = optarg;
            break;
        case 'B':
#ifdef WIN32
            ufraw_message(UFRAW_ERROR,
                "ufraw: 'ufraw --batch' not supported on Windows.\n"
		"ufraw: use 'ufraw-batch' instead.\n");
            exit(1);
#else
	    *batch = TRUE; break;
#endif
        case 'u': cmd->unclip = TRUE; break;
        case 'U': cmd->unclip = FALSE; break;
        case 'O': cmd->overwrite = TRUE; break;
        case 'z':
#ifdef HAVE_LIBZ
            cmd->losslessCompress = TRUE; break;
#else
            ufraw_message(UFRAW_ERROR,
                "ufraw: ufraw was build without ZIP support.\n");
            exit(1);
#endif
        case 'Z': cmd->losslessCompress = FALSE; break;
        case 'E': cmd->embedExif = TRUE; break;
        case 'F': cmd->embedExif = FALSE; break;
        case 'h':
            ufraw_message(UFRAW_WARNING, helpText);
            exit(0);
        case '?': /* invalid option. Warning printed by getopt() */
            exit(1);
        default:
            ufraw_message(UFRAW_ERROR, "getopt returned "
                       "character code 0%o ??\n", c);
            exit(1);
        }
    }
    cmd->wb = -1;
    if (wbName!=NULL) {
        if (!strcmp(wbName, "camera")) cmd->wb = camera_wb;
        else if (!strcmp(wbName, "auto")) cmd->wb = auto_wb;
        else {
            ufraw_message(UFRAW_ERROR,
                "'%s' is not a valid white balance option.\n", wbName);
            exit(1);
        }
    }
    cmd->curveIndex = -1;
    if (curveName!=NULL) {
        if (!strcmp(curveName, "linear")) cmd->curveIndex=linear_curve;
        else if (!strcmp(curveName, "camera")) cmd->curveIndex=camera_curve;
        else {
            ufraw_message(UFRAW_ERROR,
                "'%s' is not a valid curve name.\n", curveName);
            exit(1);
        }
    }
    cmd->interpolation = -1;
    if (interpolationName!=NULL) {
        if (!strcmp(interpolationName, "full"))
            cmd->interpolation = full_interpolation;
        else if (!strcmp(interpolationName, "four-color"))
            cmd->interpolation = four_color_interpolation;
        else if (!strcmp(interpolationName, "quick"))
            cmd->interpolation = quick_interpolation;
        else if (!strcmp(interpolationName, "half"))
            cmd->interpolation = half_interpolation;
        else {
            ufraw_message(UFRAW_ERROR,
                "'%s' is not a valid interpolation option.\n",
                interpolationName);
            exit(1);
        }
    }
    if (cmd->shrink!=NULLF && cmd->size!=NULLF) {
        ufraw_message(UFRAW_ERROR,
                        "you can not specify both --shrink and --size");
        exit(1);
    }
    cmd->type = -1;
    if (outTypeName!=NULL) {
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
                "ufraw was build without TIFF support.\n");
            exit(1);
        }
#endif
        else if (!strcmp(outTypeName, "tiff16"))
#ifdef HAVE_LIBTIFF
        cmd->type = tiff16_type;
#else
        {
            ufraw_message(UFRAW_ERROR,
                "ufraw was build without TIFF support.\n");
            exit(1);
        }
#endif
        else if (!strcmp(outTypeName, "jpeg"))
#ifdef HAVE_LIBJPEG
        cmd->type = jpeg_type;
#else
        {
            ufraw_message(UFRAW_ERROR,
                "ufraw was build without JPEG support.\n");
            exit(1);
        }
#endif
        else {
            ufraw_message(UFRAW_ERROR,
                "'%s' is not a valid output type.\n", outTypeName);
            exit(1);
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
                "'%s' is not a valid create-id option.\n", createIDName);
            exit(1);
        }
    }
    g_strlcpy(cmd->outputPath, "", max_path);
    if (outPath!=NULL) {
        if (g_file_test(outPath, G_FILE_TEST_IS_DIR))
            g_strlcpy(cmd->outputPath, outPath, max_path);
        else {
            ufraw_message(UFRAW_ERROR, "'%s' is not a valid path.\n", outPath);
            exit(1);
        }
    }
    g_strlcpy(cmd->outputFilename, "", max_path);
    if (output!=NULL) {
        if (!*batch) {
            ufraw_message(UFRAW_ERROR,
                "--output option is valid only in batch mode");
            exit(1);
        }
        if (*argc-optind>1) {
            ufraw_message(UFRAW_ERROR, "cannot output more than "
                                "one file to the same output");
            exit(1);
        }
        g_strlcpy(cmd->outputFilename, output, max_path);
    }
}

int main (int argc, char **argv)
{
    image_data *image;
    cfg_data cfg, cmd;
    gboolean batch=FALSE;
#ifndef UFRAW_BATCH
    GtkWidget *dummyWindow=NULL;
#endif
    const char *locale;

    locale = setlocale(LC_ALL, "");
    if ( locale!=NULL &&
        (!strncmp(locale, "he", 2) || !strncmp(locale, "iw", 2) ||
        !strncmp(locale, "ar", 2) ||
        !strncmp(locale, "Hebrew", 6) || !strncmp(locale, "Arabic", 6) ) ) {
        /* I'm not sure why the following doesn't work (on Windows at least) */
	/* locale = setlocale(LC_ALL, "en_US");
         * gtk_disable_setlocale(); */
        /* so I'm using setenv */
        g_setenv("LC_ALL", "en_US", TRUE);
    }
#ifdef UFRAW_BATCH
    batch=TRUE;
#else
    gtk_init(&argc, &argv);
#ifdef WIN32
    dummyWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_icon(GTK_WINDOW(dummyWindow),
            gdk_pixbuf_new_from_inline(-1, ufraw_icon, FALSE, NULL));
    ufraw_message(UFRAW_SET_PARENT, (char *)dummyWindow);
#endif
#endif

    process_args(&argc, &argv, &cmd, &batch);

#ifndef UFRAW_BATCH
    if (!batch && dummyWindow==NULL) {
        dummyWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_icon(GTK_WINDOW(dummyWindow),
                gdk_pixbuf_new_from_inline(-1, ufraw_icon, FALSE, NULL));
        ufraw_message(UFRAW_SET_PARENT, (char *)dummyWindow);
    }
#endif
    ufraw_config(NULL, &cfg);
    if (batch) {
        /* The save options are always set to default */
        if (cfg.interpolation!=four_color_interpolation)
            cfg.interpolation = full_interpolation;
        cfg.shrink = cfg_default.shrink;
        cfg.size = cfg_default.size;
        cfg.type = cfg_default.type;
	cfg.createID = cfg_default.createID;
	cfg.embedExif = cfg_default.embedExif;
        cfg.compression = cfg_default.compression;
        cfg.losslessCompress = cfg_default.losslessCompress;
        cfg.overwrite = FALSE;
        /* Disable some of the resetting done in ufraw_config() */
        cfg.wbLoad = load_preserve;
        cfg.curveLoad = load_preserve;
        if (cfg.exposureLoad!=load_auto ||
                cmd.exposure!=NULLF || cmd.black!=NULLF)
            cfg.exposureLoad = load_preserve;
        if (isnan(cfg.exposure)
		&& cmd.exposure==NULLF && cmd.black!=NULLF)
            cmd.exposure = cfg_default.exposure;
    }
    if (cmd.overwrite!=-1) cfg.overwrite = cmd.overwrite;
    if (cmd.unclip!=-1) cfg.unclip = cmd.unclip;
    if (cmd.losslessCompress!=-1) cfg.losslessCompress = cmd.losslessCompress;
    if (cmd.embedExif!=-1) cfg.embedExif = cmd.embedExif;
    if (cmd.compression!=NULLF) cfg.compression = cmd.compression;
    if (cmd.autoExposure) {
	cfg.exposure = uf_nan();
	cfg.autoExposure = TRUE;
    }
    if (cmd.exposure!=NULLF) {
	cfg.exposure = cmd.exposure;
	cfg.autoExposure = FALSE;
    }
    if (cmd.wb>=0) cfg.wb = cmd.wb;
    if (cmd.temperature!=NULLF) cfg.temperature = cmd.temperature;
    if (cmd.green!=NULLF) cfg.green = cmd.green;
    if (cmd.temperature!=NULLF || cmd.green!=NULLF) cfg.wb = preserve_wb;
    if (cmd.profile[0][0].gamma!=NULLF)
	cfg.profile[0][cfg.profileIndex[0]].gamma = cmd.profile[0][0].gamma;
    if (cmd.profile[0][0].linear!=NULLF)
	cfg.profile[0][cfg.profileIndex[0]].linear = cmd.profile[0][0].linear;
    if (cmd.saturation!=NULLF)
	cfg.saturation=cmd.saturation;
    if (cmd.curveIndex>=0) cfg.curveIndex = cmd.curveIndex;
    if (cmd.black!=NULLF)
	CurveDataSetPoint(&cfg.curve[cfg.curveIndex],
		0, cmd.black, 0);
    if (cmd.interpolation>=0) cfg.interpolation = cmd.interpolation;
    if (cmd.shrink!=NULLF) {
        cfg.shrink = cmd.shrink;
        cfg.size = 0;
        if (cfg.interpolation==half_interpolation)
            cfg.interpolation = full_interpolation;
    }
    if (cmd.size!=NULLF) {
        cfg.size = cmd.size;
        cfg.shrink = 1;
        if (cfg.interpolation==half_interpolation)
            cfg.interpolation = full_interpolation;
    }
    if (cmd.type>=0) cfg.type = cmd.type;
    if (cmd.createID>=0) cfg.createID = cmd.createID;
    if (strlen(cmd.outputPath)>0)
	g_strlcpy(cfg.outputPath, cmd.outputPath, max_path);
    if (strlen(cmd.outputFilename)>0) {
        if (cfg.createID!=no_id && !strcmp(cmd.outputFilename,"-")) {
            ufraw_message(UFRAW_ERROR,
                "cannot --create-id with stdout");
            exit(1);
        }
	g_strlcpy(cfg.outputFilename, cmd.outputFilename, max_path);
    } else {
	/* outputFilename can be used to guess output path,
	 * but not in batch mode */
	// BUG - this should be set for every file
	if (batch) g_strlcpy(cfg.outputFilename, "", max_path);
    }
    if (optind==argc) {
        if (batch) ufraw_message(UFRAW_WARNING,
                "no input file, nothing to do.\n");
	else {
#ifndef UFRAW_BATCH
	    ufraw_chooser(&cfg, NULL);
#endif
	    exit(0);
	}
    }
    /* If there only one argument and it is a directory, use it as the
     * default directory for the file-chooser */
    if (!batch
	&& optind==argc-1 && g_file_test(argv[optind],G_FILE_TEST_IS_DIR)) {
#ifndef UFRAW_BATCH
        ufraw_chooser(&cfg, argv[optind]);
#endif
        exit(0);
    }
	
    for (; optind<argc; optind++) {
        image = ufraw_open(argv[optind]);
        if (image==NULL) {
            ufraw_message(UFRAW_REPORT, NULL);
            continue;
        }
        ufraw_config(image, &cfg);
        if (batch) {
            if (ufraw_load_raw(image, FALSE)!=UFRAW_SUCCESS)
                continue;
            ufraw_message(UFRAW_MESSAGE, "loaded %s\n",
                image->filename);
            if (ufraw_batch_saver(image)==UFRAW_SUCCESS)
                ufraw_message(UFRAW_MESSAGE, "saved %s\n", cfg.outputFilename);
            ufraw_close(image);
            g_free(image);
        }
#ifndef UFRAW_BATCH
        if (!batch)
            ufraw_preview(image, FALSE, ufraw_saver);
#endif
    }
#ifndef UFRAW_BATCH
    if (dummyWindow!=NULL) gtk_widget_destroy(dummyWindow);
#endif
    exit(0);
}

#ifdef UFRAW_BATCH
void ufraw_messenger(char *message, void *parentWindow)
{
    message = message;
    parentWindow = parentWindow;
}

void preview_progress(char *text, double progress)
{
    text = text;
    progress = progress;
}
#endif
