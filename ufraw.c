/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw.c - The standalone interface to UFRaw.
 * Copyright 2004-2012 by Udi Fuchs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "ufraw.h"
#include "uf_gtk.h"
#include <stdlib.h>    /* for exit */
#include <errno.h>     /* for errno */
#include <string.h>
#include <glib/gi18n.h>

char *ufraw_binary;

int main(int argc, char **argv)
{
    ufraw_data *uf;
    conf_data rc, cmd, conf;
    int status;
    GtkWidget *dummyWindow = NULL;
    int optInd;
    int plugin = 0;

#if !GLIB_CHECK_VERSION(2,31,0)
    g_thread_init(NULL);
#endif
    gdk_threads_init();
    gdk_threads_enter();
    char *argFile = uf_win32_locale_to_utf8(argv[0]);
    ufraw_binary = g_path_get_basename(argFile);
    uf_init_locale(argFile);
    uf_win32_locale_free(argFile);
    char *gtkrcfile = g_build_filename(uf_get_home_dir(),
                                       ".ufraw-gtkrc", NULL);
    gtk_rc_add_default_file(gtkrcfile);
    g_free(gtkrcfile);
    gtk_init(&argc, &argv);
    ufraw_icons_init();
#ifdef WIN32
    dummyWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_icon_name(GTK_WINDOW(dummyWindow), "ufraw");
    ufraw_message(UFRAW_SET_PARENT, (char *)dummyWindow);
#endif
    /* Load $HOME/.ufrawrc */
    conf_load(&rc, NULL);

    /* Half interpolation is an option only for the GIMP plug-in.
     * For the stand-alone tool it is disabled */
    if (rc.interpolation == half_interpolation)
        rc.interpolation = ahd_interpolation;

    if (!rc.RememberOutputPath)
        g_strlcpy(rc.outputPath, "", max_path);
    g_strlcpy(rc.inputFilename, "", max_path);
    g_strlcpy(rc.outputFilename, "", max_path);

    /* Put the command-line options in cmd */
    optInd = ufraw_process_args(&argc, &argv, &cmd, &rc);
    if (strlen(cmd.outputFilename) != 0) {
        plugin = 3;
    }
    if (cmd.silent) {
        ufraw_message(UFRAW_ERROR,
                      _("--silent option is valid only in batch mode"));
        optInd = -1;
    }
    if (cmd.embeddedImage) {
        ufraw_message(UFRAW_ERROR, _("Extracting embedded image is not supported in interactive mode"));
        optInd = -1;
    }
    if (optInd < 0) {
        gdk_threads_leave();
        exit(1);
    }
    if (optInd == 0) {
        gdk_threads_leave();
        exit(0);
    }
    /* Create a dummyWindow for the GUI error messenger */
    if (dummyWindow == NULL) {
        dummyWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_icon_name(GTK_WINDOW(dummyWindow), "ufraw");
        ufraw_message(UFRAW_SET_PARENT, (char *)dummyWindow);
    }
    conf_file_load(&conf, cmd.inputFilename);

    if (optInd == argc) {
        ufraw_chooser(&rc, &conf, &cmd, NULL);
//	ufraw_close(cmd.darkframe);
        gdk_threads_leave();
        exit(0);
    }
    /* If there only one argument and it is a directory, use it as the
     * default directory for the file-chooser */
    argFile = uf_win32_locale_to_utf8(argv[optInd]);
    if (optInd == argc - 1 && g_file_test(argFile, G_FILE_TEST_IS_DIR)) {
        ufraw_chooser(&rc, &conf, &cmd, argFile);
        uf_win32_locale_free(argFile);
//	ufraw_close(cmd.darkframe);
        gdk_threads_leave();
        exit(0);
    }
    uf_win32_locale_free(argFile);

    int exitCode = 0;
    for (; optInd < argc; optInd++) {
        argFile = uf_win32_locale_to_utf8(argv[optInd]);
        uf = ufraw_open(argFile);
        uf_win32_locale_free(argFile);
        if (uf == NULL) {
            exitCode = 1;
            ufraw_message(UFRAW_REPORT, NULL);
            continue;
        }
        status = ufraw_config(uf, &rc, &conf, &cmd);
        if (status == UFRAW_ERROR) {
            ufraw_close_darkframe(uf->conf);
            ufraw_close(uf);
            g_free(uf);
            gdk_threads_leave();
            exit(1);
        }
        ufraw_preview(uf, &rc, plugin, NULL);
        g_free(uf);
    }
    if (dummyWindow != NULL) gtk_widget_destroy(dummyWindow);

//    ufraw_close(cmd.darkframe);
    ufobject_delete(cmd.ufobject);
    ufobject_delete(rc.ufobject);
    gdk_threads_leave();
    exit(exitCode);
}
