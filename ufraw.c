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
#include "config.h"
#endif

#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <errno.h>     /* for errno */
#include <string.h>
#include <gtk/gtk.h>
#include <locale.h>
#include "ufraw.h"
#include "ufraw_icon.h"

char *ufraw_binary;

int main (int argc, char **argv)
{
    ufraw_data *uf;
    conf_data rc, cmd, conf;
    int status;
    GtkWidget *dummyWindow=NULL;
    const char *locale;
    int optInd;

    ufraw_binary = g_path_get_basename(argv[0]);

    /* Disable the Hebrew and Arabic locale, since the right-to-left setting
     * does not go well with the preview window. */
    locale = setlocale(LC_ALL, "");
    if ( locale!=NULL &&
        (!strncmp(locale, "he", 2) || !strncmp(locale, "iw", 2) ||
        !strncmp(locale, "ar", 2) ||
        !strncmp(locale, "Hebrew", 6) || !strncmp(locale, "Arabic", 6) ) ) {
        /* I'm not sure why the following doesn't work (on Windows at least) */
	/* locale = setlocale(LC_ALL, "C");
         * gtk_disable_setlocale(); */
        /* so I'm using setenv */
        g_setenv("LC_ALL", "C", TRUE);
    }
    gtk_init(&argc, &argv);
#ifdef WIN32
    dummyWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_icon(GTK_WINDOW(dummyWindow),
            gdk_pixbuf_new_from_inline(-1, ufraw_icon, FALSE, NULL));
    ufraw_message(UFRAW_SET_PARENT, (char *)dummyWindow);
#endif
    /* Load $HOME/.ufrawrc */
    conf_load(&rc, NULL);

    /* Half interpolation is an option only for the GIMP plug-in.
     * For the stand-alone tool it is disabled */
    if (rc.interpolation==half_interpolation)
	rc.interpolation = ahd_interpolation;

    g_strlcpy(rc.inputFilename, "", max_path);
    g_strlcpy(rc.outputFilename, "", max_path);

    /* Put the command-line options in cmd */
    optInd = ufraw_process_args(&argc, &argv, &cmd, &rc);
    if (strlen(cmd.outputFilename)!=0) {
	    ufraw_message(UFRAW_ERROR,
		    "--output option is valid only in batch mode");
	    optInd = -1;
    }
    if (optInd<0) exit(1);
    if (optInd==0) exit(0);

    /* Create a dummyWindow for the GUI error messenger */
    if (dummyWindow==NULL) {
        dummyWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_icon(GTK_WINDOW(dummyWindow),
                gdk_pixbuf_new_from_inline(-1, ufraw_icon, FALSE, NULL));
        ufraw_message(UFRAW_SET_PARENT, (char *)dummyWindow);
    }
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
    if (optInd==argc) {
	status = ufraw_config(NULL, &rc, &conf, &cmd);
	if (status==UFRAW_ERROR) exit(1);
	ufraw_chooser(&rc, NULL);
	exit(0);
    }
    /* If there only one argument and it is a directory, use it as the
     * default directory for the file-chooser */
    if (optInd==argc-1 && g_file_test(argv[optInd],G_FILE_TEST_IS_DIR)) {
	status = ufraw_config(NULL, &rc, &conf, &cmd);
	if (status==UFRAW_ERROR) exit(1);
        ufraw_chooser(&rc, argv[optInd]);
        exit(0);
    }
	
    for (; optInd<argc; optInd++) {
        uf = ufraw_open(argv[optInd]);
        if (uf==NULL) {
            ufraw_message(UFRAW_REPORT, NULL);
            continue;
        }
        status = ufraw_config(uf, &rc, &conf, &cmd);
	if (status==UFRAW_ERROR) exit(1);
        ufraw_preview(uf, FALSE, ufraw_saver);
	rc = *uf->conf;
    }
    if (dummyWindow!=NULL) gtk_widget_destroy(dummyWindow);

    exit(0);
}
