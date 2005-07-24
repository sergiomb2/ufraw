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
#include <getopt.h>
#ifdef UFRAW_BATCH
#include <glib.h>
#else
#include <gtk/gtk.h>
#endif
#include <locale.h>
#include "ufraw.h"
#include "ufraw_icon.h"

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
"\n"
"The input files can be either raw images or ufraw's ID files. ID file\n"
"contain a raw image filename and the parameters for handling the image.\n"
"One can also use and ID file with the option:\n"
"\n"
"--conf=ID-file        Apply the parameters in ID-file to other raw images.\n"
"\n"
"The rest of the options are separated into two groups.\n"
"The options which are related to the image manipulation are:\n"
"\n"
"--wb=camera|auto      White balance setting.\n"
"--temperature=TEMP    Color temperature in Kelvins (2000 - 7000).\n"
"--green=GREEN         Green color normalization.\n"
"--curve=manual|linear|camera|CURVE\n"
"                      Type of tone curve to use. CURVE can be any curve\n"
"                      that was previously loaded in the GUI.\n"
"                      (default camera if such exsists, linear otherwise).\n"
"--[un]clip            Unclip [clip] highlights (default clip).\n"
"--gamma=GAMMA         Gamma adjustment of the base curve (default 0.45).\n"
"--linearity=LINEARITY Linearity of the base curve (default 0.10).\n"
"--saturation=SAT      Saturation adjustment (default 1.0, 0 for B&W output).\n"
"--exposure=auto|EXPOSURE\n"
"                      Auto exposure or exposure correction in EV (default 0).\n"
"--black-point=auto|BLACK\n"
"                      Auto black-point or black-point value (default 0).\n"
"--interpolation=full|four-color|quick\n"
"                      Interpolation algorithm to use (default full).\n"
"\n"
"The options which are related to the final output are:\n"
"\n"
"--shrink=FACTOR       Shrink the image by FACTOR (default 1).\n"
"--size=SIZE           Downsize max(height,width) to SIZE.\n"
"--out-type=ppm8|ppm16|tiff8|tiff16|jpeg\n"
"                      Output file format (default ppm8).\n"
"--create-id=no|also|only\n"
"                      Create no|also|only ID file (default no).\n"
"--compression=VALUE   JPEG compression (0-100, default 85).\n"
"--[no]exif            Embed exif in output JPEG (default embed exif).\n"
"--[no]zip             Enable [disable] TIFF zip compression (default nozip).\n"
"--out-path=PATH       PATH for output file (default use input file's path).\n"
"--output=FILE         Output file name, use '-' to output to stdout.\n"
"--overwrite           Overwrite existing files without asking (default no).\n"
"\n"
"UFRaw first reads the setting from the resource file $HOME/.ufrawrc.\n"
"Then, if an ID file is specified it setting are read. Next, the setting from\n"
"the --conf option are taken, ignoring input/output filenames in the ID file.\n"
"Lastly, the options from the command line are set. In batch mode, the second\n"
"group of options is NOT read from the resource file.\n"
"\n"
"Last, but not least, --help displays this help message and exits.\n";

void process_args(int *argc, char ***argv, conf_data *cmd, conf_data *rc,
	gboolean *batch)
{
    int index=0, c, i;
    char *wbName=NULL;
    char *curveName=NULL, *outTypeName=NULL, *createIDName=NULL,
	 *outPath=NULL, *output=NULL, *conf=NULL, *interpolationName=NULL;
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
        {"conf", 1, 0, 'C'},
/* Binary flags that don't have a value are here at the end */
        {"batch", 0, 0, 'B'},
        {"unclip", 0, 0, 'u'},
        {"clip", 0, 0, 'U'},
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
	&outTypeName, &createIDName, &outPath, &output, &conf };
    cmd->autoExposure = disabled_state;
    cmd->autoBlack = disabled_state;
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
        case 'S':
        case 'd':
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
        case 'C':
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
        if (!strcmp(curveName, "manual")) cmd->curveIndex=manual_curve;
	else if (!strcmp(curveName, "linear")) cmd->curveIndex=linear_curve;
        else if (!strcmp(curveName, "camera")) cmd->curveIndex=camera_curve;
        else {
	    cmd->curveIndex = -1;
	    for( i = camera_curve + 1; i < rc->curveCount; i++ ) {
		if( !strcmp( curveName, rc->curve[i].name )) {
		    cmd->curveIndex = i;
		    break;
		}
	    }
	    if( cmd->curveIndex == -1 ) {
		ufraw_message(UFRAW_ERROR,
			"'%s' is not a valid curve name.\n", curveName);
		exit(1);
	    }
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
    /* cmd->inputFilename is used to store the conf file */
    g_strlcpy(cmd->inputFilename, "", max_path);
    if (conf!=NULL)
        g_strlcpy(cmd->inputFilename, conf, max_path);
}

int main (int argc, char **argv)
{
    ufraw_data *uf;
    conf_data rc, cmd, conf;
    gboolean batch=FALSE;
    int status;
#ifndef UFRAW_BATCH
    GtkWidget *dummyWindow=NULL;
#endif
    const char *locale;

    /* Disable the Hebrew and Arabic locale, since the right-to-left setting
     * does not go well with the preview window. */
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
    /* Load $HOME/.ufrawrc */
    conf_load(&rc, NULL);

    /* Half interpolation is an option only for the gimp plug-in.
     * For the stand-alone tool it is disabled */
    if (rc.interpolation==half_interpolation)
	rc.interpolation = full_interpolation;

    if (batch) {
	/* In batch the save options are always set to default */
	conf_copy_save(&rc, &conf_default);
	g_strlcpy(rc.outputPath, "", max_path);
    }
    g_strlcpy(rc.inputFilename, "", max_path);
    g_strlcpy(rc.outputFilename, "", max_path);

    /* Put the command-line options in cmd */
    process_args(&argc, &argv, &cmd, &rc, &batch);

    /* Create a dummyWindow for the GUI error messenger */
#ifndef UFRAW_BATCH
    if (!batch && dummyWindow==NULL) {
        dummyWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_icon(GTK_WINDOW(dummyWindow),
                gdk_pixbuf_new_from_inline(-1, ufraw_icon, FALSE, NULL));
        ufraw_message(UFRAW_SET_PARENT, (char *)dummyWindow);
    }
#endif
    /* Load the --conf file. version==0 means ignore conf. */
    conf.version = 0;
    if (strlen(cmd.inputFilename)>0) {
	status = conf_load(&conf, cmd.inputFilename);
	if (status==UFRAW_SUCCESS) {
	    strcpy(conf.inputFilename, "");
	    strcpy(conf.outputFilename, "");
	    strcpy(conf.outputPath, "");
	} else {
            ufraw_message(UFRAW_REPORT, NULL);
	    conf.version = 0;
	}
    }
    if (optind==argc) {
        if (batch) {
	    ufraw_message(UFRAW_WARNING, "no input file, nothing to do.\n");
	} else {
	    status = ufraw_config(NULL, &rc, &conf, &cmd);
	    if (status==UFRAW_ERROR) exit(1);
	    ufraw_chooser(&rc, NULL);
	    exit(0);
	}
    }
    /* If there only one argument and it is a directory, use it as the
     * default directory for the file-chooser */
    if (!batch
	&& optind==argc-1 && g_file_test(argv[optind],G_FILE_TEST_IS_DIR)) {
	status = ufraw_config(NULL, &rc, &conf, &cmd);
	if (status==UFRAW_ERROR) exit(1);
        ufraw_chooser(&rc, argv[optind]);
        exit(0);
    }
	
    for (; optind<argc; optind++) {
        uf = ufraw_open(argv[optind]);
        if (uf==NULL) {
            ufraw_message(UFRAW_REPORT, NULL);
            continue;
        }
        status = ufraw_config(uf, &rc, &conf, &cmd);
	if (status==UFRAW_ERROR) exit(1);
        if (batch) {
            if (ufraw_load_raw(uf)!=UFRAW_SUCCESS)
                continue;
            ufraw_message(UFRAW_MESSAGE, "loaded %s\n",
                uf->filename);
            if (ufraw_batch_saver(uf)==UFRAW_SUCCESS)
                ufraw_message(UFRAW_MESSAGE, "saved %s\n",
			uf->conf->outputFilename);
            ufraw_close(uf);
            g_free(uf);
        }
#ifndef UFRAW_BATCH
        if (!batch) {
            ufraw_preview(uf, FALSE, ufraw_saver);
	    rc = *uf->conf;
	}
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

void ufraw_chooser(conf_data *conf, char *defPath)
{
    conf = conf;
    defPath = defPath;
}

void preview_progress(void *widget, char *text, double progress)
{
    widget = widget;
    text = text;
    progress = progress;
}
#endif
