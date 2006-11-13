/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw.c - The standalone interface to UFRaw.
 * Copyright 2004-2006 by Udi Fuchs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation. You should have received
 * a copy of the license along with this program.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <errno.h>     /* for errno */
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "ufraw.h"

char *ufraw_binary;

int main (int argc, char **argv)
{
    ufraw_data *uf;
    conf_data rc, cmd, conf;
    int status;
    GtkWidget *dummyWindow=NULL;
    int optInd;

    ufraw_binary = g_path_get_basename(argv[0]);
    uf_init_locale(argv[0]);
    gtk_init(&argc, &argv);
    ufraw_icons_init();
#ifdef WIN32
    dummyWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
#ifdef HAVE_GTK_2_6
    gtk_window_set_icon_name(GTK_WINDOW(dummyWindow), "ufraw-ufraw");
#else
    gtk_window_set_icon(GTK_WINDOW(dummyWindow),
	    gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
		    "ufraw-ufraw", 48, GTK_ICON_LOOKUP_USE_BUILTIN, NULL));
#endif
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
    if ( strlen(cmd.outputFilename)!=0 ) {
	ufraw_message(UFRAW_ERROR,
		"--output option is valid only in batch mode");
	optInd = -1;
    }
    if ( cmd.silent ) {
	ufraw_message(UFRAW_ERROR,
		"--silent is valid only in batch mode");
	optInd = -1;
    }
    if ( cmd.embeddedImage ) {
        ufraw_message(UFRAW_ERROR, _("Extracting embedded image is not supported in interactive mode"));
	optInd = -1;
    }
    if (optInd<0) exit(1);
    if (optInd==0) exit(0);

    /* Create a dummyWindow for the GUI error messenger */
    if (dummyWindow==NULL) {
	dummyWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
#ifdef HAVE_GTK_2_6
	gtk_window_set_icon_name(GTK_WINDOW(dummyWindow), "ufraw-ufraw");
#else
	gtk_window_set_icon(GTK_WINDOW(dummyWindow),
		gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
			"ufraw-ufraw", 48, GTK_ICON_LOOKUP_USE_BUILTIN, NULL));
#endif
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
//	ufraw_close(cmd.darkframe);
	exit(0);
    }
    /* If there only one argument and it is a directory, use it as the
     * default directory for the file-chooser */
    if (optInd==argc-1 && g_file_test(argv[optInd],G_FILE_TEST_IS_DIR)) {
	status = ufraw_config(NULL, &rc, &conf, &cmd);
	if (status==UFRAW_ERROR) exit(1);
        ufraw_chooser(&rc, argv[optInd]);
//	ufraw_close(cmd.darkframe);
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

//    ufraw_close(cmd.darkframe);
    exit(0);
}
