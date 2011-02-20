/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw_saver.c - The GUI file saver.
 * Copyright 2004-2011 by Udi Fuchs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "ufraw.h"
#include "uf_gtk.h"
#include <errno.h>
#include <string.h>
#include <glib/gi18n.h>

static int ufraw_overwrite_dialog(char *filename, GtkWidget *widget)
{
    char message[max_path];
    int response;
    GtkWidget *dialog = gtk_dialog_new_with_buttons(_("File exists"),
                        GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                        GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                        GTK_STOCK_NO, GTK_RESPONSE_NO,
                        GTK_STOCK_YES, GTK_RESPONSE_YES, NULL);
    char *utf8 = g_filename_to_utf8(filename, -1, NULL, NULL, NULL);
    if (utf8 == NULL) utf8 = g_strdup("Unknown file name");
    snprintf(message, max_path,
             _("File '%s' already exists.\nOverwrite?"), utf8);
    g_free(utf8);
    GtkWidget *label = gtk_label_new(message);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), label);
    gtk_widget_show_all(dialog);
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return (response == GTK_RESPONSE_YES);
}

long ufraw_save_now(ufraw_data *uf, void *widget)
{
    if (!uf->conf->overwrite && uf->conf->createID != only_id
            && g_file_test(uf->conf->outputFilename, G_FILE_TEST_EXISTS)) {
        if (!ufraw_overwrite_dialog(uf->conf->outputFilename, widget))
            return UFRAW_ERROR;
    }
    int status = ufraw_write_image(uf);
    if (status == UFRAW_ERROR) {
        ufraw_message(status, ufraw_get_message(uf));
        return UFRAW_ERROR;
    }
    if (status == UFRAW_WARNING) {
        ufraw_message(status, ufraw_get_message(uf));
    }
    if (ufraw_get_message(uf) != NULL)
        ufraw_message(UFRAW_SET_LOG, ufraw_get_message(uf));
    return UFRAW_SUCCESS;
}

long ufraw_send_to_gimp(ufraw_data *uf)
{
    char *basename = g_path_get_basename(uf->conf->inputFilename);
    char *template = g_strconcat(basename, "_XXXXXX", NULL);
    g_free(basename);
    char *confFilename = NULL;
    GError *err = NULL;
    int fd = g_file_open_tmp(template, &confFilename, &err);
    g_free(template);
    if (fd == -1) {
        g_free(confFilename);
        ufraw_message(UFRAW_ERROR, "%s\n%s",
                      _("Error creating temporary file."), err->message);
        g_error_free(err);
        return UFRAW_ERROR;
    }
    FILE *out = fdopen(fd, "w");
    if (out == NULL) {
        g_free(confFilename);
        ufraw_message(UFRAW_ERROR, "%s\n%s",
                      _("Error creating temporary file."), g_strerror(errno));
        return UFRAW_ERROR;
    }
    char *buffer;
    int saveCreateID = uf->conf->createID;
    uf->conf->createID = send_id;
    conf_save(uf->conf, confFilename, &buffer);
    uf->conf->createID = saveCreateID;
    if (fwrite(buffer, strlen(buffer), 1, out) != 1) {
        g_free(buffer);
        g_free(confFilename);
        ufraw_message(UFRAW_ERROR, "%s\n%s",
                      _("Error creating temporary file."), g_strerror(errno));
        return UFRAW_ERROR;
    }
    g_free(buffer);
    if (fclose(out) != 0) {
        g_free(confFilename);
        ufraw_message(UFRAW_ERROR, "%s\n%s",
                      _("Error creating temporary file."), g_strerror(errno));
        return UFRAW_ERROR;
    }
    char *fullConfFilename = g_strconcat(confFilename, ".ufraw", NULL);
    if (g_rename(confFilename, fullConfFilename) == -1) {
        g_free(confFilename);
        g_free(fullConfFilename);
        ufraw_message(UFRAW_ERROR, "%s\n%s",
                      _("Error creating temporary file."), g_strerror(errno));
        return UFRAW_ERROR;
    }
    g_free(confFilename);
    char *commandLine = g_strdup_printf("%s \"%s\"",
                                        uf->conf->remoteGimpCommand, fullConfFilename);
    /* gimp-remote starts the gimp in a fork().
     * Therefore we must call it asynchronously. */
    if (!g_spawn_command_line_async(commandLine, &err)) {
        g_free(commandLine);
#ifdef WIN32
        if (strcmp(uf->conf->remoteGimpCommand,
                   conf_default.remoteGimpCommand) == 0) {
            /* If the user didn't play with the remoteGimpCommand,
             * try to run Gimp-2.6 instead of Gimp-2.4 */
            g_strlcpy(uf->conf->remoteGimpCommand, "gimp-2.6.exe", max_path);
            commandLine = g_strdup_printf("%s \"%s\"",
                                          uf->conf->remoteGimpCommand, fullConfFilename);
            g_error_free(err);
            err = NULL;
            /* gimp-remote starts the gimp in a fork().
             * Therefore we must call it asynchronously. */
            if (!g_spawn_command_line_async(commandLine, &err)) {
                g_free(commandLine);
                g_free(fullConfFilename);
                ufraw_message(UFRAW_ERROR, "%s\n%s",
                              _("Error activating Gimp."), err->message);
                g_error_free(err);
                return UFRAW_ERROR;
            }
        } else
#endif
        {
            g_free(fullConfFilename);
            ufraw_message(UFRAW_ERROR, "%s\n%s",
                          _("Error activating Gimp."), err->message);
            g_error_free(err);
            return UFRAW_ERROR;
        }
    }
    g_free(fullConfFilename);
    g_free(commandLine);
    // Sleep for 0.2 seconds, giving time for gimp-remote to do the X query.
    g_usleep(200 * 1000);
    return UFRAW_SUCCESS;
}
