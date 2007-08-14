/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw-batch.c - The standalone interface to UFRaw in batch mode.
 * Copyright 2004-2007 by Udi Fuchs
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
#include <glib.h>
#include <glib/gi18n.h>
#include "ufraw.h"

gboolean silentMessenger;
char *ufraw_binary;

int ufraw_batch_saver(ufraw_data *uf);

int main (int argc, char **argv)
{
    ufraw_data *uf;
    conf_data rc, cmd, conf;
    int status;

    ufraw_binary = g_path_get_basename(argv[0]);
    uf_init_locale(argv[0]);

    /* Load $HOME/.ufrawrc */
    conf_load(&rc, NULL);

    /* Half interpolation is an option only for the GIMP plug-in.
     * For the stand-alone tool it is disabled */
    if (rc.interpolation==half_interpolation)
	rc.interpolation = ahd_interpolation;

    /* In batch the save options are always set to default */
    conf_copy_save(&rc, &conf_default);
    g_strlcpy(rc.outputPath, "", max_path);
    g_strlcpy(rc.inputFilename, "", max_path);
    g_strlcpy(rc.outputFilename, "", max_path);

    /* Put the command-line options in cmd */
    int optInd = ufraw_process_args(&argc, &argv, &cmd, &rc);
    if (optInd<0) exit(1);
    if (optInd==0) exit(0);
    silentMessenger = cmd.silent;

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
	    ufraw_message(UFRAW_WARNING, _("No input file, nothing to do."));
    }
    for (; optInd<argc; optInd++) {
        uf = ufraw_open(argv[optInd]);
        if (uf==NULL) {
            ufraw_message(UFRAW_REPORT, NULL);
            continue;
        }
        status = ufraw_config(uf, &rc, &conf, &cmd);
	if (status==UFRAW_ERROR) {
	    ufraw_close(uf);
	    g_free(uf);
	    exit(1);
	}
        if (ufraw_load_raw(uf)!=UFRAW_SUCCESS) {
	    ufraw_close(uf);
	    g_free(uf);
            continue;
	}
        ufraw_message(UFRAW_MESSAGE, _("loaded %s"), uf->filename);
        if (ufraw_batch_saver(uf)==UFRAW_SUCCESS)
            ufraw_message(UFRAW_MESSAGE, _("saved %s"),
		    uf->conf->outputFilename);
        ufraw_close(uf);
        g_free(uf);
    }
//    ufraw_close(cmd.darkframe);
    exit(0);
}
int ufraw_batch_saver(ufraw_data *uf)
{
    if ( !uf->conf->overwrite && uf->conf->createID!=only_id
       && strcmp(uf->conf->outputFilename, "-")
       && g_file_test(uf->conf->outputFilename, G_FILE_TEST_EXISTS) ) {
        char ans[max_name];
        /* First letter of the word 'yes' for the y/n question */
        gchar *yChar = g_utf8_strdown(_("y"), -1);
        /* First letter of the word 'no' for the y/n question */
        gchar *nChar = g_utf8_strup(_("n"), -1);
        g_printerr(_("%s: overwrite '%s'?"), ufraw_binary,
                uf->conf->outputFilename);
        g_printerr(" [%s/%s] ", yChar, nChar);
        if ( fgets(ans, max_name, stdin)==NULL ) ans[0] = '\0';
        gchar *ans8 = g_utf8_strdown(ans, 1);
        if ( g_utf8_collate(ans8, yChar)!=0 ) {
            g_free(yChar);
            g_free(nChar);
            g_free(ans8);
            return UFRAW_CANCEL;
        }
        g_free(yChar);
        g_free(nChar);
        g_free(ans8);
    }
    if (strcmp(uf->conf->outputFilename, "-")) {
        char *absname = uf_file_set_absolute(uf->conf->outputFilename);
        g_strlcpy(uf->conf->outputFilename, absname, max_path);
        g_free(absname);
    }
    if (uf->conf->embeddedImage) {
        int status = ufraw_convert_embedded(uf);
        if (status!=UFRAW_SUCCESS) return status;
        status = ufraw_write_embedded(uf);
        return status;
    } else {
        int status = ufraw_write_image(uf);
	if ( status!=UFRAW_SUCCESS )
	    ufraw_message(status, ufraw_get_message(uf));
	return status;
    }
}

void ufraw_messenger(char *message, void *parentWindow)
{
    parentWindow = parentWindow;
    if (!silentMessenger) ufraw_batch_messenger(message);
}

void preview_progress(void *widget, char *text, double progress)
{
    widget = widget;
    text = text;
    progress = progress;
}
