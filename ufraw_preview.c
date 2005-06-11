/*
   UFRaw - Unidentified Flying Raw
   Raw photo loader plugin for The GIMP
   by udi Fuchs,

   based on the gimp plug-in by Pawel T. Jochym jochym at ifj edu pl,

   based on the gimp plug-in by Dave Coffin
   http://www.cybercom.net/~dcoffin/

   UFRaw is licensed under the GNU General Public License.
   It uses "dcraw" code to do the actual raw decoding.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <gtk/gtk.h>
#include "ufraw.h"
#include "curveeditor_widget.h"
#include "ufraw_icon.h"

/* Set to huge number so that the preview size is set by the screen size */
const int def_preview_width = 9000;
const int def_preview_height = 9000;
#define raw_his_size 320
#define live_his_size 256

enum { pixel_format, percent_format };

char *expanderText[] = { "Raw histogram", "Exposure", "White balance",
    "Color management", "Curve corrections", "Live histogram", NULL };

typedef struct {
    GtkLabel *labels[4];
    int num;
    int format;
} colorLabels;

/* Global variables for holding information on the preview image */
image_data *previewImage;
#define Developer previewImage->developer
/* Some actions update the progress bar while working, but meanwhile we want
 * to freeze all other actions. After we thaw the dialog we must call
 * update_scales() which was also frozen. */
gboolean freezeDialog;
int spotX1, spotY1, spotX2, spotY2;

/* Remember the Gtk Widgets that we need to access later */
GtkWidget *previewWidget, *previewVBox, *rawHisto, *liveHisto, *curveWidget;
GtkProgressBar *progressBar = NULL;
GtkAdjustment *adjTemperature, *adjGreen, *adjExposure,
	*adjGamma, *adjLinear, *adjContrast, *adjSaturation, *adjCurveBlack;
GtkComboBox *wbCombo, *curveCombo, *profileCombo[profile_types];
GtkToggleButton *expAAButton;
GtkTooltips *toolTips;
GtkLabel *spotPatch;
colorLabels *spotLabels, *avrLabels, *devLabels, *overLabels, *underLabels;

/* cfg holds infomation that is kept for the next image load */
cfg_data *cfg;
cfg_data save_cfg;

/* Curve points to a permanent curve for the GUI to update.
 * We need to make sure that the real curves in cfg always get updated. */
CurveData *Curve;

#define CFG_cameraCurve (cfg->curve[camera_curve].m_numAnchors>=0)

void ufraw_focus(void *window, gboolean focus)
{
    if (focus) {
        GtkWindow *parentWindow = (void *)ufraw_message(UFRAW_SET_PARENT, (char *)window);
	g_object_set_data(G_OBJECT(window), "WindowParent", parentWindow);
	if (parentWindow!=NULL)
	    gtk_window_set_accept_focus(GTK_WINDOW(parentWindow), FALSE);
    } else {
	GtkWindow *parentWindow = g_object_get_data(G_OBJECT(window),
		"WindowParent");
	ufraw_message(UFRAW_SET_PARENT, (char *)parentWindow);
	if (parentWindow!=NULL)
	    gtk_window_set_accept_focus(GTK_WINDOW(parentWindow), TRUE);
    }
}

void ufraw_messenger(char *message,  void *parentWindow)
{
    GtkDialog *dialog;

    dialog = GTK_DIALOG(gtk_message_dialog_new(GTK_WINDOW(parentWindow),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, message));
    gtk_dialog_run(dialog);
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

void window_response(GtkWidget *widget, gpointer user_data)
{
    GtkWindow *window = GTK_WINDOW(gtk_widget_get_toplevel(widget));

    if (freezeDialog) return;
    g_object_set_data(G_OBJECT(window), "WindowResponse", user_data);
    gtk_main_quit();
}

void load_curve(GtkWidget *widget, gpointer user_data)
{
    GtkFileChooser *fileChooser;
    GtkFileFilter *filter;
    GSList *list, *saveList;
    CurveData c;
    char *cp;

    user_data = user_data;
    if (freezeDialog) return;
    if (cfg->curveCount==max_curves) {
        ufraw_message(UFRAW_ERROR, "No more room for new curves.");
        return;
    }
    fileChooser = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new("Load curve",
            GTK_WINDOW(gtk_widget_get_toplevel(widget)),
            GTK_FILE_CHOOSER_ACTION_OPEN,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
            GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL));
    ufraw_focus(fileChooser, TRUE);
    gtk_file_chooser_set_select_multiple(fileChooser, TRUE);
#ifdef HAVE_GTK_2_6
    gtk_file_chooser_set_show_hidden(fileChooser, TRUE);
#endif
    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, "All curve formats");
    gtk_file_filter_add_pattern(filter, "*.ntc");
    gtk_file_filter_add_pattern(filter, "*.NTC");
    gtk_file_filter_add_pattern(filter, "*.ncv");
    gtk_file_filter_add_pattern(filter, "*.NCV");
    gtk_file_filter_add_pattern(filter, "*.curve");
    gtk_file_filter_add_pattern(filter, "*.CURVE");
    gtk_file_chooser_add_filter(fileChooser, filter);

    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, "UFRaw curve format");
    gtk_file_filter_add_pattern(filter, "*.curve");
    gtk_file_filter_add_pattern(filter, "*.CURVE");
    gtk_file_chooser_add_filter(fileChooser, filter);

    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, "Nikon curve format");
    gtk_file_filter_add_pattern(filter, "*.ntc");
    gtk_file_filter_add_pattern(filter, "*.NTC");
    gtk_file_filter_add_pattern(filter, "*.ncv");
    gtk_file_filter_add_pattern(filter, "*.NCV");
    gtk_file_chooser_add_filter(fileChooser, filter);

    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, "All files");
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(fileChooser, filter);

    if (strlen(cfg->curvePath)>0)
        gtk_file_chooser_set_current_folder(fileChooser, cfg->curvePath);
    if (gtk_dialog_run(GTK_DIALOG(fileChooser))==GTK_RESPONSE_ACCEPT) {
        for (list=saveList=gtk_file_chooser_get_filenames(fileChooser);
            list!=NULL && cfg->curveCount<max_curves;
            list=g_slist_next(list)) {
            c = cfg_default.curve[0];
            if (curve_load(&c, list->data)!=UFRAW_SUCCESS) continue;
            gtk_combo_box_append_text(curveCombo, c.name);
            cfg->curve[cfg->curveCount++] = c;
            cfg->curveIndex = cfg->curveCount-1;
            if (CFG_cameraCurve)
                gtk_combo_box_set_active(curveCombo, cfg->curveIndex);
            else
                gtk_combo_box_set_active(curveCombo, cfg->curveIndex-1);
            cp = g_path_get_dirname(list->data);
            g_strlcpy(cfg->curvePath, cp, max_path);
            g_free(cp);
            g_free(list->data);
        }
        if (list!=NULL)
        ufraw_message(UFRAW_ERROR, "No more room for new curves.");
        g_slist_free(saveList);
    }
    ufraw_focus(fileChooser, FALSE);
    gtk_widget_destroy(GTK_WIDGET(fileChooser));
}

void save_curve(GtkWidget *widget, gpointer user_data)
{
    GtkFileChooser *fileChooser;
    GtkFileFilter *filter;
    char defFilename[max_name];
    char *filename, *cp;

    user_data = user_data;
    if (freezeDialog) return;

    fileChooser = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new("Save curve",
            GTK_WINDOW(gtk_widget_get_toplevel(widget)),
            GTK_FILE_CHOOSER_ACTION_SAVE,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
            GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL));
    ufraw_focus(fileChooser, TRUE);
#ifdef HAVE_GTK_2_6
    gtk_file_chooser_set_show_hidden(fileChooser, TRUE);
#endif
    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, "All curve formats");
    gtk_file_filter_add_pattern(filter, "*.ntc");
    gtk_file_filter_add_pattern(filter, "*.NTC");
    gtk_file_filter_add_pattern(filter, "*.ncv");
    gtk_file_filter_add_pattern(filter, "*.NCV");
    gtk_file_filter_add_pattern(filter, "*.curve");
    gtk_file_filter_add_pattern(filter, "*.CURVE");
    gtk_file_chooser_add_filter(fileChooser, filter);

    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, "UFRaw curve format");
    gtk_file_filter_add_pattern(filter, "*.curve");
    gtk_file_filter_add_pattern(filter, "*.CURVE");
    gtk_file_chooser_add_filter(fileChooser, filter);

    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, "Nikon curve format");
    gtk_file_filter_add_pattern(filter, "*.ntc");
    gtk_file_filter_add_pattern(filter, "*.NTC");
    gtk_file_filter_add_pattern(filter, "*.ncv");
    gtk_file_filter_add_pattern(filter, "*.NCV");
    gtk_file_chooser_add_filter(fileChooser, filter);

    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, "All files");
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(fileChooser, filter);

    if (strlen(cfg->curvePath)>0)
        gtk_file_chooser_set_current_folder(fileChooser, cfg->curvePath);
    snprintf(defFilename, max_name, "%s.curve",
	    cfg->curve[cfg->curveIndex].name);
    gtk_file_chooser_set_current_name(fileChooser, defFilename);
    if (gtk_dialog_run(GTK_DIALOG(fileChooser))==GTK_RESPONSE_ACCEPT) {
        filename = gtk_file_chooser_get_filename(fileChooser);
	curve_save(&cfg->curve[cfg->curveIndex], filename);
        cp = g_path_get_dirname(filename);
        g_strlcpy(cfg->curvePath, cp, max_path);
        g_free(cp);
        g_free(filename);
    }
    ufraw_focus(fileChooser, FALSE);
    gtk_widget_destroy(GTK_WIDGET(fileChooser));
}
void load_profile(GtkWidget *widget, long type)
{
    GtkFileChooser *fileChooser;
    GSList *list, *saveList;
    GtkFileFilter *filter;
    char *filename, *cp;
    profile_data p;

    if (freezeDialog) return;
    if (cfg->profileCount[type]==max_profiles) {
        ufraw_message(UFRAW_ERROR, "No more room for new profiles.");
        return;
    }
    fileChooser = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new(
            "Load color profile",
            GTK_WINDOW(gtk_widget_get_toplevel(widget)),
            GTK_FILE_CHOOSER_ACTION_OPEN,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
            GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL));
    ufraw_focus(fileChooser, TRUE);
    gtk_file_chooser_set_select_multiple(fileChooser, TRUE);
#ifdef HAVE_GTK_2_6
    gtk_file_chooser_set_show_hidden(fileChooser, TRUE);
#endif
    if (strlen(cfg->profilePath)>0)
        gtk_file_chooser_set_current_folder(fileChooser, cfg->profilePath);
    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, "Color Profiles");
    gtk_file_filter_add_pattern(filter, "*.icm");
    gtk_file_filter_add_pattern(filter, "*.ICM");
    gtk_file_filter_add_pattern(filter, "*.icc");
    gtk_file_filter_add_pattern(filter, "*.ICC");
    gtk_file_chooser_add_filter(fileChooser, filter);
    filter = GTK_FILE_FILTER(gtk_file_filter_new());
    gtk_file_filter_set_name(filter, "All files");
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(fileChooser, filter);
    if (gtk_dialog_run(GTK_DIALOG(fileChooser))==GTK_RESPONSE_ACCEPT) {
        for (list=saveList=gtk_file_chooser_get_filenames(fileChooser);
            list!=NULL && cfg->profileCount[type]<max_profiles;
            list=g_slist_next(list))
        {
            filename = list->data;
            p = cfg_default.profile[type][0];
            g_strlcpy(p.file, filename, max_path);
	    /* Make sure we update product name */
	    Developer->updateTransform = TRUE;
            developer_profile(Developer, type, &p);
            if (Developer->profile[type]==NULL) {
                g_free(list->data);
                continue;
            }
            /* Set some defaults to the profile's curve */
	    p.linear = 0.0;
            if ( !strncmp("Nikon D70 for NEF", p.productName, 17) ||
                 !strncmp("Nikon D100 for NEF", p.productName, 18) ||
	         !strncmp("Nikon D1 for NEF", p.productName, 16) )
	        p.gamma = 0.45;
	    else
	        p.gamma = 1.0;

	    char *base = g_path_get_basename(filename);
	    char *name = uf_file_set_type(base, "");
	    g_strlcpy(p.name, name, max_name);
	    g_free(name);
	    g_free(base);
            cfg->profile[type][cfg->profileCount[type]++] = p;
            gtk_combo_box_append_text(profileCombo[type], p.name);
            cfg->profileIndex[type] = cfg->profileCount[type]-1;
            cp = g_path_get_dirname(list->data);
            g_strlcpy(cfg->profilePath, cp, max_path);
            g_free(cp);
            g_free(list->data);
        }
        gtk_combo_box_set_active(profileCombo[type], cfg->profileIndex[type]);
        if (list!=NULL)
            ufraw_message(UFRAW_ERROR, "No more room for new profiles.");
        g_slist_free(saveList);
    }
    ufraw_focus(fileChooser, FALSE);
    gtk_widget_destroy(GTK_WIDGET(fileChooser));
}

colorLabels *color_labels_new(GtkTable *table, int x, int y, char *label,
        int format)
{
    colorLabels *l;
    GtkWidget *lbl;
    int c, i = 0;

    l = g_new(colorLabels, 1);
    l->format = format;
    if (label!=NULL){
        lbl = gtk_label_new(label);
        gtk_misc_set_alignment(GTK_MISC(lbl), 1, 0.5);
        gtk_table_attach(table, lbl, x, x+1, y, y+1,
                GTK_SHRINK|GTK_FILL, GTK_FILL, 0, 0);
        i++;
    }
    for (c=0; c<3; c++, i++) {
        l->labels[c] = GTK_LABEL(gtk_label_new(NULL));
        gtk_table_attach_defaults(table, GTK_WIDGET(l->labels[c]),
                x+i, x+i+1, y, y+1);
    }
    return l;
}

void color_labels_set(colorLabels *l, double data[3])
{
    int c;
    char buf1[max_name], buf2[max_name];
    const char *colorName[] = {"red", "green", "blue"};

    for (c=0; c<3; c++) {
        switch (l->format) {
        case pixel_format: snprintf(buf1, max_name, "%3.f", data[c]); break;
        case percent_format:
                      if (data[c]<10.0)
                          snprintf(buf1, max_name, "%2.1f%%", data[c]);
                      else
                          snprintf(buf1, max_name, "%2.0f%%", data[c]);
                      break;
        default: snprintf(buf1, max_name, "ERR");
        }
        snprintf(buf2, max_name, "<span foreground='%s'>%s</span>",
                colorName[c], buf1);
        gtk_label_set_markup(l->labels[c], buf2);
    }
}

int renderDefault, renderOverexposed, renderUnderexposed;
gpointer render_default=&renderDefault, render_overexposed=&renderOverexposed,
    render_underexposed=&renderUnderexposed;

gboolean render_raw_histogram(gpointer mode);
gboolean render_preview_image(gpointer mode);
void render_spot();
void draw_spot();

void render_preview(GtkWidget *widget, gpointer mode)
{
    widget = widget;
    if (freezeDialog) return;
    g_idle_remove_by_data((gpointer)render_default);
    g_idle_remove_by_data((gpointer)render_underexposed);
    g_idle_remove_by_data((gpointer)render_overexposed);
    g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
            render_raw_histogram, (gpointer)mode, NULL);
}

static int renderRestart = FALSE;
gboolean render_raw_histogram(gpointer mode)
{
    static GdkPixbuf *pixbuf;
    static guint8 *pixies;
    int rowstride;
    guint8 pix[99], *p8 , *p;
    guint16 pixtmp[9999];
    image_type p16;
    int x, c, cl, y, y1;
    int raw_his[raw_his_size][3], raw_his_max;

    cfg->curve[cfg->curveIndex] = *curveeditor_widget_get_curve(curveWidget);
    developer_prepare(Developer, previewImage->rgbMax, pow(2, cfg->exposure),
	    cfg->unclip, cfg->temperature, cfg->green, previewImage->preMul,
            &cfg->profile[0][cfg->profileIndex[0]],
            &cfg->profile[1][cfg->profileIndex[1]], cfg->intent,
            Curve->m_saturation, Curve);
    pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(rawHisto));
    if (gdk_pixbuf_get_height(pixbuf)!=cfg->rawHistogramHeight+2) {
        pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
	        raw_his_size+2, cfg->rawHistogramHeight+2);
	gtk_image_set_from_pixbuf(GTK_IMAGE(rawHisto), pixbuf);
	g_object_unref(pixbuf);
    }
    pixies = gdk_pixbuf_get_pixels(pixbuf);
    rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    memset(pixies, 0, (gdk_pixbuf_get_height(pixbuf)-1)*
            rowstride + gdk_pixbuf_get_width(pixbuf));
    memset(raw_his, 0, sizeof(raw_his));
    for (y=0; y<previewImage->height; y++)
        for (x=0; x<previewImage->width; x++)
            for (c=0; c<3; c++)
                raw_his[MIN( previewImage->image[y*previewImage->width+x][c] *
                             (raw_his_size-1) / previewImage->rgbMax,
                             raw_his_size-1) ][c]++;
    for (x=0, raw_his_max=1; x<raw_his_size; x++)
        for (c=0; c<3; c++) {
	    if (cfg->rawHistogramScale==log_histogram)
		raw_his[x][c] = log(1+raw_his[x][c])*1000;
            raw_his_max = MAX(raw_his_max, raw_his[x][c]);
	}
    p8 = pix;
    p = pixies + cfg->rawHistogramHeight*rowstride + 3;
    /* draw curves on the raw histogram */
    for (x=0; x<raw_his_size; x++)
        for (c=0; c<3; c++, p++) {
	    for (y=0; y<cfg->rawHistogramHeight; y++)
                pixies[(cfg->rawHistogramHeight-y)*rowstride+3*(x+1)+c]=
                    raw_his[x][c]*cfg->rawHistogramHeight/raw_his_max>y ? 255:0;
            /* Value for pixel x of color c in a grey pixel */
            for (cl=0; cl<3; cl++)
                p16[cl] = MIN((guint64)x*Developer->rgbMax *
                          Developer->rgbWB[c] /
                          Developer->rgbWB[cl] / raw_his_size, 0xFFFF);
            develope(p8, p16, Developer, 8, pixtmp, 1);
            y=p8[c] * (cfg->rawHistogramHeight-1)/MAXOUT;
            /* Value for pixel x+1 of color c in a grey pixel */
            for (cl=0; cl<3; cl++)
                p16[cl] = MIN((guint64)(x+1)*Developer->rgbMax *
                         Developer->rgbWB[c] /
                         Developer->rgbWB[cl]/raw_his_size-1, 0xFFFF);
            develope(p8, p16, Developer, 8, pixtmp, 1);
            y1=p8[c] * (cfg->rawHistogramHeight-1)/MAXOUT;
            for (; y<=y1; y++) *(p-y*rowstride) = 255 - *(p-y*rowstride);
            /* Value for pixel x of pure color c */
            p16[0] = p16[1] = p16[2] = 0;
            p16[c] = MIN((guint64)x*Developer->rgbMax/raw_his_size, 0xFFFF);
            develope(p8, p16, Developer, 8, pixtmp, 1);
            y1=p8[c] * (cfg->rawHistogramHeight-1)/MAXOUT;
            for (; y<y1; y++) *(p-y*rowstride) = 160 - *(p-y*rowstride)/4;
            /* Value for pixel x+1 of pure color c */
            p16[c] = MIN((guint64)(x+1)*Developer->rgbMax/raw_his_size - 1,
                     0xFFFF);
            develope(p8, p16, Developer, 8, pixtmp, 1);
            y1=p8[c] * (cfg->rawHistogramHeight-1)/MAXOUT;
            for (; y<=y1; y++) *(p-y*rowstride) = 255 - *(p-y*rowstride);
        }
    gtk_widget_queue_draw(rawHisto);
    renderRestart = TRUE;
    g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
            render_preview_image, (gpointer)mode, NULL);
    return FALSE;
}

gboolean render_preview_image(gpointer mode)
{
    static GdkPixbuf *pixbuf;
    static guint8 *pixies;
    static int width, height, x, y, c, o, min, max, rowstride, y0;
    static int live_his[live_his_size][4], live_his_max;
    guint8 *p8;
    guint16 pixtmp[9999];
    double rgb[3];
    guint64 sum[3], sqr[3];

    if (renderRestart) {
        renderRestart = FALSE;
        pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(previewWidget));
        width = gdk_pixbuf_get_width(pixbuf);
        height = gdk_pixbuf_get_height(pixbuf);
        rowstride = gdk_pixbuf_get_rowstride(pixbuf);
        pixies = gdk_pixbuf_get_pixels(pixbuf);
#ifdef DEBUG
        fprintf(stderr, "render_preview_image: w=%d, h=%d, r=%d, mode=%d\n",
                width, height, rowstride, mode);
        fflush(stderr);
#endif
        memset(live_his, 0, sizeof(live_his));
        y = y0 = 0;
    }
    for (; y<height; y++) {
        develope(&pixies[y*rowstride], previewImage->image[y*width],
                Developer, 8, pixtmp, width);
        for (x=0, p8=&pixies[y*rowstride]; x<width; x++, p8+=3) {
            for (c=0, max=0, min=0x100; c<3; c++) {
                max = MAX(max, p8[c]);
                min = MIN(min, p8[c]);
                live_his[p8[c]][c]++;
            }
            if (cfg->histogram==luminosity_histogram)
                live_his[(int)(0.3*p8[0]+0.59*p8[1]+0.11*p8[2])][3]++;
            if (cfg->histogram==value_histogram)
                live_his[max][3]++;
            if (cfg->histogram==saturation_histogram) {
                if (max==0) live_his[0][3]++;
                else live_his[255*(max-min)/max][3]++;
            }
            for (c=0; c<3; c++) {
                o = p8[c];
                if (mode==render_default) {
                    if (cfg->overExp && max==MAXOUT) o = 0;
                    if (cfg->underExp && min==0) o = 255;
                } else if (mode==render_overexposed) {
                    if (o!=255) o = 0;
                } else if (mode==render_underexposed) {
                    if (o!=0) o = 255;
                }
                pixies[y*rowstride+3*x+c] = o;
            }
        }
        if (y%32==31) {
            gtk_widget_queue_draw_area(previewWidget, 0, y0, width, y+1-y0);
            y0 = y+1;
            y++;
            return TRUE;
        }
    }
    gtk_widget_queue_draw_area(previewWidget, 0, y0, width, y+1-y0);

    /* draw live histogram */
    pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(liveHisto));
    if (gdk_pixbuf_get_height(pixbuf)!=cfg->liveHistogramHeight+2) {
        pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
	        live_his_size+2, cfg->liveHistogramHeight+2);
	gtk_image_set_from_pixbuf(GTK_IMAGE(liveHisto), pixbuf);
	g_object_unref(pixbuf);
    }
    pixies = gdk_pixbuf_get_pixels(pixbuf);
    rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    memset(pixies, 0, (gdk_pixbuf_get_height(pixbuf)-1)*rowstride +
            gdk_pixbuf_get_width(pixbuf));
    for (c=0; c<3; c++) {
        sum[c] = 0;
        sqr[c] = 0;
        for (x=1; x<live_his_size; x++) {
            sum[c] += x*live_his[x][c];
            sqr[c] += (guint64)x*x*live_his[x][c];
        }
    }
    for (x=1, live_his_max=1; x<live_his_size-1; x++) {
	if (cfg->liveHistogramScale==log_histogram)
	    for (c=0; c<4; c++) live_his[x][c] = log(1+live_his[x][c])*1000;
        if (cfg->histogram==rgb_histogram)
            for (c=0; c<3; c++)
                live_his_max = MAX(live_his_max, live_his[x][c]);
        else if (cfg->histogram==r_g_b_histogram)
            live_his_max = MAX(live_his_max,
                    live_his[x][0]+live_his[x][1]+live_his[x][2]);
        else live_his_max = MAX(live_his_max, live_his[x][3]);
    }
#ifdef DEBUG
    fprintf(stderr, "render_preview: live_his_max=%d\n", live_his_max);
    fflush(stderr);
#endif
    for (x=0; x<live_his_size; x++) for (y=0; y<cfg->liveHistogramHeight; y++)
        if (cfg->histogram==r_g_b_histogram) {
            if (y*live_his_max < live_his[x][0]*cfg->liveHistogramHeight)
                pixies[(cfg->liveHistogramHeight-y)*rowstride+3*(x+1)+0] = 255;
            else if (y*live_his_max <
                    (live_his[x][0]+live_his[x][1])*cfg->liveHistogramHeight)
                pixies[(cfg->liveHistogramHeight-y)*rowstride+3*(x+1)+1] = 255;
            else if (y*live_his_max <
                    (live_his[x][0]+live_his[x][1]+live_his[x][2])
                    *cfg->liveHistogramHeight)
                pixies[(cfg->liveHistogramHeight-y)*rowstride+3*(x+1)+2] = 255;
        } else {
            for (c=0; c<3; c++)
                if (cfg->histogram==rgb_histogram) {
                    if (y*live_his_max<live_his[x][c]*cfg->liveHistogramHeight)
                        pixies[(cfg->liveHistogramHeight-y)*rowstride+3*(x+1)+c]
			    = 255;
                } else {
                    if (y*live_his_max<live_his[x][3]*cfg->liveHistogramHeight)
                        pixies[(cfg->liveHistogramHeight-y)*rowstride+3*(x+1)+c]
			    = 255;
                }
       }

    /* draw vertical line at quarters "behind" the live histogram */
    for (y=-1; y<cfg->liveHistogramHeight+1; y++) for (x=64; x<255; x+=64)
        if (pixies[(cfg->liveHistogramHeight-y)*rowstride+3*(x+1)+0]==0 &&
            pixies[(cfg->liveHistogramHeight-y)*rowstride+3*(x+1)+1]==0 &&
            pixies[(cfg->liveHistogramHeight-y)*rowstride+3*(x+1)+2]==0 )
            for (c=0; c<3; c++)
                pixies[(cfg->liveHistogramHeight-y)*rowstride+3*(x+1)+c]=64;
    gtk_widget_queue_draw(liveHisto);
    for (c=0;c<3;c++) rgb[c] = sum[c]/height/width;
    color_labels_set(avrLabels, rgb);
    for (c=0;c<3;c++) rgb[c] = sqrt(sqr[c]/height/width-rgb[c]*rgb[c]);
    color_labels_set(devLabels, rgb);
    for (c=0;c<3;c++) rgb[c] = 100.0*live_his[live_his_size-1][c]/height/width;
    color_labels_set(overLabels, rgb);
    for (c=0;c<3;c++) rgb[c] = 100.0*live_his[0][c]/height/width;
    color_labels_set(underLabels, rgb);
    render_spot();
    return FALSE;
}

void render_spot()
{
    GdkPixbuf *pixbuf;
    guint8 *pixies;
    int width, height, rowstride;
    int c, y, x;
    int spotStartX, spotStartY, spotSizeX, spotSizeY;
    double rgb[3];
    char tmp[max_name];

    if (spotX1<0) return;
    pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(previewWidget));
    width = gdk_pixbuf_get_width(pixbuf);
    height = gdk_pixbuf_get_height(pixbuf);
    rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    pixies = gdk_pixbuf_get_pixels(pixbuf);
    for (c=0; c<3; c++) rgb[c] = 0;
    spotSizeY = abs(spotY1-spotY2)+1;
    spotStartY = MIN(spotY1, spotY2);
    spotSizeX = abs(spotX1-spotX2)+1;
    spotStartX = MIN(spotX1, spotX2);
    for (y=0; y<spotSizeY; y++) {
        for (x=0; x<spotSizeX; x++)
            for (c=0; c<3; c++)
		rgb[c] += pixies[(spotStartY+y)*rowstride+(spotStartX+x)*3+c];
    }
    for (c=0; c<3; c++) rgb[c] /= spotSizeX * spotSizeY;
    color_labels_set(spotLabels, rgb);
    snprintf(tmp, max_name, "<span background='#%02X%02X%02X'>"
	    "                    </span>",
	    (int)rgb[0], (int)rgb[1], (int)rgb[2]);
    gtk_label_set_markup(spotPatch, tmp);
    draw_spot();
    gtk_widget_queue_draw_area(previewWidget, spotStartX-1, spotStartY-1,
	    spotStartX+spotSizeX+1, spotStartY+spotSizeY+1);
}

void draw_spot()
{
    GdkPixbuf *pixbuf;
    guint8 *pixies;
    int width, height, x, y, rowstride;
    int spotStartX, spotStartY, spotSizeX, spotSizeY;

    if (spotX1<0) return;
    pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(previewWidget));
    width = gdk_pixbuf_get_width(pixbuf);
    height = gdk_pixbuf_get_height(pixbuf);
    rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    pixies = gdk_pixbuf_get_pixels(pixbuf);
    spotSizeY = abs(spotY1-spotY2)+1;
    spotStartY = MIN(spotY1, spotY2);
    spotSizeX = abs(spotX1-spotX2)+1;
    spotStartX = MIN(spotX1, spotX2);
    for (x=0; x<=spotSizeX; x++) {
        pixbuf_reverse(spotStartX+x, spotStartY-1,
                pixies, width, height, rowstride);
        pixbuf_reverse(spotStartX+x, spotStartY+spotSizeY+1,
                pixies, width, height, rowstride);
    }
    for (y=-1; y<=spotSizeY+1; y++) {
        pixbuf_reverse(spotStartX-1, spotStartY+y,
                pixies, width, height, rowstride);
        pixbuf_reverse(spotStartX+spotSizeX+1, spotStartY+y,
	        pixies, width, height, rowstride);
    }
    gtk_widget_queue_draw_area(previewWidget,
	    spotStartX-1, spotStartY-1,
	    spotSizeX+3, spotSizeY+3);
}

/* update the UI entries that could have changed automatically */
void update_scales()
{
    if (freezeDialog) return;
    freezeDialog = TRUE;
    if (cfg->curveIndex>camera_curve && !CFG_cameraCurve)
        gtk_combo_box_set_active(curveCombo, cfg->curveIndex-1);
    else
        gtk_combo_box_set_active(curveCombo, cfg->curveIndex);
    gtk_combo_box_set_active(wbCombo, cfg->wb);
    gtk_adjustment_set_value(adjTemperature, cfg->temperature);
    gtk_adjustment_set_value(adjGreen, cfg->green);
    gtk_adjustment_set_value(adjExposure, cfg->exposure);
    gtk_adjustment_set_value(adjGamma,
            cfg->profile[0][cfg->profileIndex[0]].gamma);
    gtk_adjustment_set_value(adjLinear,
            cfg->profile[0][cfg->profileIndex[0]].linear);
    gtk_adjustment_set_value(adjContrast, Curve->m_contrast);
    gtk_adjustment_set_value(adjSaturation, Curve->m_saturation);
    gtk_adjustment_set_value(adjCurveBlack, Curve->m_black);
    freezeDialog = FALSE;
    render_preview(NULL, render_default);
};

void spot_wb(GtkWidget *widget, gpointer user_data)
{
    int spotStartX, spotStartY, spotSizeX, spotSizeY;
    int x, y, c;
    double rgb[3];

    widget = widget;
    user_data = user_data;

    if (freezeDialog) return;
    if (spotX1<=0) return;
    spotSizeY = abs(spotY1-spotY2)+1;
    spotStartY = MIN(spotY1, spotY2);
    spotSizeX = abs(spotX1-spotX2)+1;
    spotStartX = MIN(spotX1, spotX2);

    for (c=0; c<3; c++) rgb[c] = 0;
    for (y=spotStartY; y<spotStartY+spotSizeY; y++)
        for (x=spotStartX; x<spotStartX+spotSizeX; x++)
            for (c=0; c<3; c++)
                rgb[c] += previewImage->image[y*previewImage->width+x][c];
    for (c=0; c<3; c++)
        rgb[c] /= spotSizeX * spotSizeY;
    ufraw_message(UFRAW_SET_LOG, "spot_wb: r=%f, g=%f, b=%f\n",
            rgb[0], rgb[1], rgb[2]);
    for (c=0; c<3; c++) rgb[c] *= previewImage->preMul[c];
    RGB_to_temperature(rgb, &cfg->temperature, &cfg->green);
    cfg->wb = 0;
    cfg->autoExposure = FALSE;
    gtk_toggle_button_set_active(expAAButton, FALSE);
    update_scales();
}

void spot_press(GtkWidget *event_box, GdkEventButton *event, gpointer data)
{
    event_box = event_box;
    data = data;
    if (freezeDialog) return;
    if (event->button!=1) return;
    draw_spot();
    spotX1 = event->x;
    spotY1 = event->y;
    spotX2 = event->x;
    spotY2 = event->y;
    render_spot();
}

gboolean spot_motion(GtkWidget *event_box, GdkEventMotion *event, gpointer data)
{
    event_box = event_box;
    data = data;
    if ((event->state&GDK_BUTTON1_MASK)==0) return FALSE;
    draw_spot();
    spotX2 = MAX(MIN(event->x,previewImage->width),0);
    spotY2 = MAX(MIN(event->y,previewImage->height),0);
    render_spot();
    return FALSE;
}

GtkWidget *table_with_frame(GtkWidget *box, char *label, gboolean expand)
{
    GtkWidget *frame, *expander;
    GtkWidget *table;

    if (GTK_IS_NOTEBOOK(box)) {
	table = gtk_vbox_new(FALSE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(box),
		GTK_WIDGET(table), gtk_label_new(label));
    } else if (label!=NULL) {
	table = gtk_table_new(10, 10, FALSE);
        expander = gtk_expander_new(label);
        gtk_expander_set_expanded(GTK_EXPANDER(expander), expand);
        gtk_box_pack_start(GTK_BOX(box), expander, FALSE, FALSE, 0);
        frame = gtk_frame_new(NULL);
        gtk_container_add(GTK_CONTAINER(expander), GTK_WIDGET(frame));
        gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(table));
    } else {
	table = gtk_table_new(10, 10, FALSE);
        frame = gtk_frame_new(NULL);
        gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(table));
    }
    return table;
}

void button_update(GtkButton *button, long type)
{
    if (type==1) {
	cfg->exposure = cfg_default.exposure;
	cfg->autoExposure = FALSE;
	gtk_toggle_button_set_active(expAAButton, FALSE);
    }
    if (type==2) {
	cfg->autoExposure = 
	    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
	if (cfg->autoExposure) ufraw_auto_expose(previewImage);
    }
    if (type==3) cfg->profile[0][cfg->profileIndex[0]].gamma =
			cfg_default.profile[0][0].gamma;
    if (type==4) cfg->profile[0][cfg->profileIndex[0]].linear =
			cfg_default.profile[0][0].linear;
    if (type==10) Curve->m_contrast = cfg_default.curve[0].m_contrast;
    if (type==20) Curve->m_saturation = cfg_default.curve[0].m_saturation;
    if (type==101) Curve->m_black = cfg_default.curve[0].m_black;
    if (type==102) {
	ufraw_auto_black(previewImage);
	Curve->m_black = cfg->curve[cfg->curveIndex].m_black;
    }
    update_scales();
    return;
}

void toggle_button_update(GtkToggleButton *button, gboolean *valuep)
{
    *valuep = gtk_toggle_button_get_active(button);
    if (valuep==&cfg->unclip) {
	cfg->autoExposure = FALSE;
	gtk_toggle_button_set_active(expAAButton, FALSE);
	/* ugly flip needed because cfg->unclip=!clip_button_active */
	cfg->unclip = !cfg->unclip;
	char text[max_name];
        snprintf(text, max_name, "Highlight clipping\n"
		"Current state: %s", cfg->unclip ? "unclip" : "clip");
	gtk_tooltips_set_tip(toolTips, GTK_WIDGET(button), text, NULL);
    }
    render_preview(NULL, render_default);
}

void toggle_button(GtkTable *table, int x, int y, char *label, gboolean *valuep)
{
    GtkWidget *widget, *align;

    widget = gtk_check_button_new_with_label(label);
    if (label==NULL) {
        align = gtk_alignment_new(1, 0, 0, 1);
        gtk_container_add(GTK_CONTAINER(align), widget);
    } else align = widget;
    gtk_table_attach(table, align, x, x+1, y, y+1, 0, 0, 0, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), *valuep);
    g_signal_connect(G_OBJECT(widget), "toggled",
            G_CALLBACK(toggle_button_update), valuep);
}

void adjustment_update(GtkAdjustment *adj, double *valuep)
{
    if (freezeDialog) return;
    if (valuep==&cfg->profile[0][0].gamma)
        valuep = (void *)&cfg->profile[0][cfg->profileIndex[0]].gamma;
    if (valuep==&cfg->profile[0][0].linear)
        valuep = (void *)&cfg->profile[0][cfg->profileIndex[0]].linear;
    *valuep = gtk_adjustment_get_value(adj);
    if (valuep==&cfg->temperature || valuep==&cfg->green) {
        cfg->wb = 0;
        gtk_combo_box_set_active(wbCombo, cfg->wb);
	cfg->autoExposure = FALSE;
	gtk_toggle_button_set_active(expAAButton, FALSE);
    }
    if (valuep==&cfg->exposure) {
	cfg->autoExposure = FALSE;
	gtk_toggle_button_set_active(expAAButton, FALSE);
    }
    render_preview(NULL, render_default);
}

GtkAdjustment *adjustment_scale(GtkTable *table,
    int x, int y, char *label, double value, double *valuep,
    double min, double max, double step, double jump, int accuracy, char *tip)
{
    GtkAdjustment *adj;
    GtkWidget *w, *l;

    w = gtk_event_box_new();
    l = gtk_label_new(label);
    gtk_misc_set_alignment(GTK_MISC(l), 1, 0.5);
    gtk_container_add(GTK_CONTAINER(w),l);
    gtk_tooltips_set_tip(toolTips, w, tip, NULL);
    gtk_table_attach(table, w, x, x+1, y, y+1, GTK_SHRINK|GTK_FILL, 0, 0, 0);
    adj = GTK_ADJUSTMENT(gtk_adjustment_new(value, min, max, step, jump, 0));
    g_signal_connect(G_OBJECT(adj), "value-changed",
            G_CALLBACK(adjustment_update), valuep);
    w = gtk_hscale_new(adj);
    gtk_scale_set_draw_value(GTK_SCALE(w), FALSE);
    gtk_tooltips_set_tip(toolTips, w, tip, NULL);
    gtk_table_attach(table, w, x+1, x+5, y, y+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    w = gtk_spin_button_new(adj, step, accuracy);
    gtk_tooltips_set_tip(toolTips, w, tip, NULL);
    gtk_table_attach(table, w, x+5, x+7, y, y+1, GTK_SHRINK|GTK_FILL, 0, 0, 0);
    return adj;
}

void combo_update(GtkWidget *combo, gint *valuep)
{
    if (freezeDialog) return;
    *valuep = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    if (valuep==&cfg->curveIndex) {
        if (!CFG_cameraCurve && cfg->curveIndex>=camera_curve)
            cfg->curveIndex++;
	*Curve = cfg->curve[cfg->curveIndex];
	curveeditor_widget_update(curveWidget);
        update_scales();
    } else if (valuep==&cfg->profileIndex[in_profile]) {
        update_scales();
    } else if (valuep==&cfg->wb) {
        ufraw_set_wb(previewImage);
	cfg->autoExposure = FALSE;
	gtk_toggle_button_set_active(expAAButton, FALSE);
        update_scales();
    } else if (valuep!=&cfg->interpolation)
        render_preview(NULL, render_default);
}

void radio_menu_update(GtkRadioMenuItem *item, gint *valuep)
{
    int type = (int)g_object_get_data(G_OBJECT(item), "Radio-Value");
    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item))) {
	*valuep = type;
        render_preview(NULL, render_default);
    }
}

void container_remove(GtkWidget *widget, gpointer user_data)
{
    GtkContainer *container = GTK_CONTAINER(user_data);
    gtk_container_remove(container, widget);
}

void delete_from_list(GtkWidget *widget, gpointer user_data)
{
    GtkDialog *dialog;
    long type, index;

    dialog = GTK_DIALOG(gtk_widget_get_ancestor(widget, GTK_TYPE_DIALOG));
    type = (long)g_object_get_data(G_OBJECT(widget), "Type");
    index = (long)user_data;
    if (type<2) {
        gtk_combo_box_remove_text(profileCombo[type], index);
        cfg->profileCount[type]--;
        if (cfg->profileIndex[type]==cfg->profileCount[type])
            cfg->profileIndex[type]--;
        for (; index<cfg->profileCount[type]; index++)
            cfg->profile[type][index] = cfg->profile[type][index+1];
    } else {
        if (CFG_cameraCurve)
            gtk_combo_box_remove_text(curveCombo, index);
        else
            gtk_combo_box_remove_text(curveCombo, index-1);
        cfg->curveCount--;
        if (cfg->curveIndex==cfg->curveCount)
            cfg->curveIndex--;
        if (cfg->curveIndex==camera_curve && !CFG_cameraCurve)
            cfg->curveIndex = linear_curve;
        for (; index<cfg->curveCount; index++)
            cfg->curve[index] = cfg->curve[index+1];
    }
    gtk_dialog_response(dialog, GTK_RESPONSE_APPLY);
}

void configuration_save(GtkWidget *widget, gpointer user_data)
{
    widget = widget;
    user_data = user_data;
    save_configuration(cfg, Developer, NULL, NULL, 0);
    save_cfg = *previewImage->cfg;
}

void options_combo_update(GtkWidget *combo, gint *valuep)
{
    GtkDialog *dialog;

    *valuep = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    dialog = GTK_DIALOG(gtk_widget_get_ancestor(combo, GTK_TYPE_DIALOG));
    gtk_dialog_response(dialog, GTK_RESPONSE_APPLY);
}

void options_dialog(GtkWidget *widget, gpointer user_data)
{
    GtkWidget *optionsDialog, *profileTable[2];
    GtkWidget *notebook, *label, *page, *button, *text, *box, *image;
    GtkComboBox *combo;
    GtkTable *curveTable, *table;
    GtkTextBuffer *cfgBuffer, *buffer;
    char txt[max_name], buf[10000];
    int i, j;

    user_data = user_data;
    if (freezeDialog) return;
    optionsDialog = gtk_dialog_new_with_buttons("UFRaw options",
            GTK_WINDOW(gtk_widget_get_toplevel(widget)),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
    ufraw_focus(optionsDialog, TRUE);
    gtk_dialog_set_default_response(GTK_DIALOG(optionsDialog),
            GTK_RESPONSE_OK);
    notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(optionsDialog)->vbox),
            notebook);

    label = gtk_label_new("Settings");
    box = gtk_vbox_new(FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box, label);
    table = GTK_TABLE(table_with_frame(box, "Initial settings", TRUE));
    combo = GTK_COMBO_BOX(gtk_combo_box_new_text());
    gtk_combo_box_append_text(combo, "Preserve WB");
    gtk_combo_box_append_text(combo, "Camera WB");
    gtk_combo_box_append_text(combo, "Auto WB");
    gtk_combo_box_set_active(combo, cfg->wbLoad);
    g_signal_connect(G_OBJECT(combo), "changed",
            G_CALLBACK(options_combo_update), &cfg->wbLoad);
    gtk_table_attach_defaults(table, GTK_WIDGET(combo), 0, 1, 0, 1);
    combo = GTK_COMBO_BOX(gtk_combo_box_new_text());
    gtk_combo_box_append_text(combo, "Preserve curve corrections");
    gtk_combo_box_append_text(combo, "Default curve corrections");
    gtk_combo_box_set_active(combo, cfg->curveLoad);
    g_signal_connect(G_OBJECT(combo), "changed",
            G_CALLBACK(options_combo_update), &cfg->curveLoad);
    gtk_table_attach_defaults(table, GTK_WIDGET(combo), 1, 2, 0, 1);
    combo = GTK_COMBO_BOX(gtk_combo_box_new_text());
    gtk_combo_box_append_text(combo, "Preserve exposure");
    gtk_combo_box_append_text(combo, "Default exposure");
    gtk_combo_box_append_text(combo, "Auto exposure");
    gtk_combo_box_set_active(combo, cfg->exposureLoad);
    g_signal_connect(G_OBJECT(combo), "changed",
            G_CALLBACK(options_combo_update), &cfg->exposureLoad);
    gtk_table_attach_defaults(table, GTK_WIDGET(combo), 2, 3, 0, 1);

    profileTable[0] = table_with_frame(box, "Input color profiles", TRUE);
    profileTable[1] = table_with_frame(box, "Output color profiles", TRUE);
    curveTable = GTK_TABLE(table_with_frame(box, "Curves", TRUE));

    label = gtk_label_new("Configuration");
    box = gtk_vbox_new(FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box, label);
    page = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(box), page, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(page),
            GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    text = gtk_text_view_new();
    gtk_container_add(GTK_CONTAINER(page), text);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
    cfgBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
    table = GTK_TABLE(gtk_table_new(10, 10, FALSE));
    gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(table), FALSE, FALSE, 0);
    button = gtk_button_new_from_stock(GTK_STOCK_SAVE);
    gtk_tooltips_set_tip(toolTips, button, "Save configuration file", NULL);
    g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(configuration_save), NULL);
    gtk_table_attach(table, button, 0, 1, 0, 1, 0, 0, 0, 0);

    label = gtk_label_new("Log");
    page = gtk_scrolled_window_new(NULL, NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, label);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(page),
            GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    text = gtk_text_view_new();
    gtk_container_add(GTK_CONTAINER(page), text);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
    gtk_text_buffer_set_text(buffer, ufraw_message(UFRAW_GET_LOG, NULL), -1);

    label = gtk_label_new("About");
    box = gtk_vbox_new(FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box, label);
    image = gtk_image_new_from_pixbuf(
            gdk_pixbuf_new_from_inline(-1, ufraw_icon, FALSE, NULL));
    gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 0);
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label),
            "<span size='xx-large'><b>UFRaw " VERSION "</b></span>");
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "\n"
            "The <b>U</b>nidentified <b>F</b>lying <b>Raw</b> "
            "(<b>UFRaw</b>) is a utility to read\n"
            "and manipulate raw images from digital cameras.\n"
	    "UFRaw relies on <b>D</b>igital <b>C</b>amera "
	    "<b>Raw</b> (<b>DCRaw</b>)\n"
	    "for the actual encoding of the raw images.\n\n"
            "Author: Udi Fuchs\n"
            "Homepage: http://ufraw.sourceforge.net/\n\n");
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

    while (1) {
        for (j=0; j<2; j++) {
            gtk_container_foreach(GTK_CONTAINER(profileTable[j]),
                    (GtkCallback)(container_remove), profileTable[j]);
            table = GTK_TABLE(profileTable[j]);
            for (i=1; i<cfg->profileCount[j]; i++) {
                snprintf(txt, max_name, "%s (%s)",
                        cfg->profile[j][i].name,
                        cfg->profile[j][i].productName);
                label = gtk_label_new(txt);
                gtk_table_attach_defaults(table, label, 0, 1, i, i+1);
                button = gtk_button_new_from_stock(GTK_STOCK_DELETE);
                g_object_set_data(G_OBJECT(button), "Type", (void*)j);
                g_signal_connect(G_OBJECT(button), "clicked",
                        G_CALLBACK(delete_from_list), (gpointer)i);
                gtk_table_attach(table, button, 1, 2, i, i+1, 0, 0, 0, 0);
            }
        }
        gtk_container_foreach(GTK_CONTAINER(curveTable),
                (GtkCallback)(container_remove), curveTable);
        table = curveTable;
        for (i=camera_curve+1; i<cfg->curveCount; i++) {
            label = gtk_label_new(cfg->curve[i].name);
            gtk_table_attach_defaults(table, label, 0, 1, i, i+1);
            button = gtk_button_new_from_stock(GTK_STOCK_DELETE);
            g_object_set_data(G_OBJECT(button), "Type", (void*)2);
            g_signal_connect(G_OBJECT(button), "clicked",
                    G_CALLBACK(delete_from_list), (gpointer)i);
            gtk_table_attach(table, button, 1, 2, i, i+1, 0, 0, 0, 0);
        }
        save_configuration(cfg, Developer, NULL, buf, 10000);
        gtk_text_buffer_set_text(cfgBuffer, buf, -1);
        gtk_widget_show_all(optionsDialog);
        if (gtk_dialog_run(GTK_DIALOG(optionsDialog))!=
                    GTK_RESPONSE_APPLY) {
	    ufraw_focus(optionsDialog, FALSE);
            gtk_widget_destroy(optionsDialog);
            render_preview(NULL, render_default);
            return;
        }
    }
}

gboolean window_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    data = data;
    event = event;
    if (freezeDialog) return TRUE;
    g_object_set_data(G_OBJECT(widget), "WindowResponse",
            (gpointer)GTK_RESPONSE_CANCEL);
    gtk_main_quit();
    return TRUE;
}

void expander_state(GtkWidget *widget, gpointer user_data)
{
    const char *text;
    GtkWidget *label;
    int i;

    user_data = user_data;
    if (!GTK_IS_EXPANDER(widget)) return;
    label = gtk_expander_get_label_widget(GTK_EXPANDER(widget));
    text = gtk_label_get_text(GTK_LABEL(label));
    for (i=0; expanderText[i]!=NULL; i++)
        if (!strcmp(text, expanderText[i]))
            cfg->expander[i] = gtk_expander_get_expanded(GTK_EXPANDER(widget));
}

gboolean live_histo_menu(GtkMenu *menu, GdkEventButton *event)
{
    if (event->button!=3) return FALSE;
    gtk_menu_popup(menu, NULL, NULL, NULL, NULL, event->button, event->time);
    return TRUE;
}

gboolean preview_cursor(GtkWidget *widget, GdkEventCrossing *event,
	GdkCursor *cursor)
{
    widget = widget;
    gdk_window_set_cursor(event->window, cursor);
    return TRUE;
}

void preview_progress(char *text, double progress)
{
    if (progressBar==NULL) return;
    gtk_progress_bar_set_text(progressBar, text);
    gtk_progress_bar_set_fraction(progressBar, progress);
    while (gtk_events_pending()) gtk_main_iteration();
}

int ufraw_preview(image_data *image, int plugin, long (*save_func)())
{
    GtkWidget *previewWindow;
    GtkTable *table;
    GtkBox *previewHBox, *box, *hbox;
    GtkComboBox *combo;
    GtkWidget *button, *saveButton, *saveAsButton=NULL, *event_box, *align,
	    *label, *vBox, *noteBox, *page, *menu, *menu_item;
    GSList *group;
    GdkPixbuf *pixbuf;
    GdkRectangle screen;
    char previewHeader[max_path];
    int max_preview_width, max_preview_height;
    int preview_width, preview_height, scale, shrinkSave, sizeSave, i;
    long j;
    int status, rowstride;
    guint8 *pixies;
    image_data preview_image;
    char progressText[max_name], text[max_name];

    save_cfg = *image->cfg;
    cfg = image->cfg;
    spotX1 = -1;
    freezeDialog = TRUE;

    toolTips = gtk_tooltips_new();
    previewWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    snprintf(previewHeader, max_path, "%s - UFRaw Photo Loader",
            image->filename);
    gtk_window_set_title(GTK_WINDOW(previewWindow), previewHeader);
    gtk_window_set_icon(GTK_WINDOW(previewWindow),
            gdk_pixbuf_new_from_inline(-1, ufraw_icon, FALSE, NULL));
    gtk_window_set_resizable(GTK_WINDOW(previewWindow), FALSE);
    g_signal_connect(G_OBJECT(previewWindow), "delete-event",
            G_CALLBACK(window_delete_event), NULL);
    ufraw_focus(previewWindow, TRUE);

    gdk_screen_get_monitor_geometry(gdk_screen_get_default(), 0, &screen);
    max_preview_width = MIN(def_preview_width, screen.width-400);
    max_preview_height = MIN(def_preview_height, screen.height-120);
    scale = MAX((image->width-1)/max_preview_width,
            (image->height-1)/max_preview_height)+1;
    scale = MAX(2, scale);
    preview_width = image->width / scale;
    preview_height = image->height / scale;
    snprintf(progressText, max_name, "size %dx%d, scale 1/%d",
            image->height, image->width, scale);

    if (screen.height<800) {
	if (cfg->liveHistogramHeight==cfg_default.liveHistogramHeight)
	    cfg->liveHistogramHeight = 96;
	if (cfg->rawHistogramHeight==cfg_default.rawHistogramHeight)
	    cfg->rawHistogramHeight = 96;
    }
    previewHBox = GTK_BOX(gtk_hbox_new(FALSE, 0));
    gtk_container_add(GTK_CONTAINER(previewWindow), GTK_WIDGET(previewHBox));
    previewVBox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(previewHBox, previewVBox, TRUE, TRUE, 2);

    table = GTK_TABLE(table_with_frame(previewVBox, expanderText[raw_expander],
            cfg->expander[raw_expander]));
    event_box = gtk_event_box_new();
    gtk_table_attach(table, event_box, 0, 1, 1, 2, 0, 0, 0, 0);
    pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
            raw_his_size+2, cfg->rawHistogramHeight+2);
    rawHisto = gtk_image_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);
    gtk_container_add(GTK_CONTAINER(event_box), rawHisto);
    pixies = gdk_pixbuf_get_pixels(pixbuf);
    rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    memset(pixies, 0, (gdk_pixbuf_get_height(pixbuf)-1)* rowstride +
            gdk_pixbuf_get_width(pixbuf));
    menu = gtk_menu_new();
    g_signal_connect_swapped(G_OBJECT(event_box), "button_press_event",
            G_CALLBACK(live_histo_menu), menu);
    group = NULL;
    menu_item = gtk_radio_menu_item_new_with_label(group, "Linear");
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 6, 7);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    cfg->rawHistogramScale==linear_histogram);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)linear_histogram);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
            G_CALLBACK(radio_menu_update), &cfg->rawHistogramScale);
    menu_item = gtk_radio_menu_item_new_with_label(group, "Logarithmic");
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 7, 8);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    cfg->rawHistogramScale==log_histogram);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)log_histogram);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
            G_CALLBACK(radio_menu_update), &cfg->rawHistogramScale);

    menu_item = gtk_separator_menu_item_new();
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 8, 9);

    group = NULL;
    menu_item = gtk_radio_menu_item_new_with_label(group, "96 Pixels");
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 9, 10);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    cfg->rawHistogramHeight==96);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)96);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
            G_CALLBACK(radio_menu_update), &cfg->rawHistogramHeight);
    menu_item = gtk_radio_menu_item_new_with_label(group, "128 Pixels");
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 10, 11);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    cfg->rawHistogramHeight==128);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)128);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
            G_CALLBACK(radio_menu_update), &cfg->rawHistogramHeight);
    menu_item = gtk_radio_menu_item_new_with_label(group, "192 Pixels");
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 11, 12);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    cfg->rawHistogramHeight==192);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)192);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
            G_CALLBACK(radio_menu_update), &cfg->rawHistogramHeight);
    menu_item = gtk_radio_menu_item_new_with_label(group, "256 Pixels");
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 12, 13);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    cfg->rawHistogramHeight==256);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)256);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
            G_CALLBACK(radio_menu_update), &cfg->rawHistogramHeight);
    gtk_widget_show_all(menu);

    table = GTK_TABLE(table_with_frame(previewVBox, NULL, FALSE));
    spotLabels = color_labels_new(table, 0, 1, "Spot values:", pixel_format);
    spotPatch = GTK_LABEL(gtk_label_new(NULL));
    gtk_table_attach_defaults(table, GTK_WIDGET(spotPatch), 6, 7, 1, 2);

    noteBox = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(previewVBox), noteBox, FALSE, FALSE, 0);

    page = table_with_frame(noteBox, "Base", TRUE);

    table = GTK_TABLE(table_with_frame(page, NULL, TRUE));
    label = gtk_label_new("White balance");
    gtk_table_attach(table, label, 0, 1, 0 , 1, 0, 0, 0, 0);
    wbCombo = GTK_COMBO_BOX(gtk_combo_box_new_text());
    for (i=0; i<wb_preset_count; i++)
        gtk_combo_box_append_text(wbCombo, wb_preset[i].name);
    gtk_combo_box_set_active(wbCombo, cfg->wb);
    g_signal_connect(G_OBJECT(wbCombo), "changed",
            G_CALLBACK(combo_update), &cfg->wb);
    gtk_table_attach(table, GTK_WIDGET(wbCombo), 1, 6, 0, 1, GTK_FILL, 0, 0, 0);

    adjTemperature = adjustment_scale(table, 0, 1,
            "Temperature [K]", cfg->temperature, &cfg->temperature,
            2000, 7000, 50, 200, 0,
            "White balance color temperature (K)");
    adjGreen = adjustment_scale(table, 0, 2, "Green component",
            cfg->green, &cfg->green, 0.2, 2.5, 0.01, 0.05, 2,
            "Green component");
    button = gtk_button_new_with_label("Spot\nWB");
    gtk_tooltips_set_tip(toolTips, button,
	    "Select a spot on the preview image to apply spot white balance",
	    NULL);
    gtk_table_attach(table, button, 7, 8, 1, 3, GTK_FILL, 0, 0, 0);
    g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(spot_wb), NULL);

    table = GTK_TABLE(table_with_frame(page, expanderText[color_expander],
            cfg->expander[color_expander]));
    for (j=0; j<profile_types; j++) {
        label = gtk_label_new(j==in_profile ? "Input profile" :
                j==out_profile ? "Output profile" : "Error");
        gtk_table_attach(table, label, 0, 1, 3*j+1, 3*j+2, 0, 0, 0, 0);
        profileCombo[j] = GTK_COMBO_BOX(gtk_combo_box_new_text());
        for (i=0; i<cfg->profileCount[j]; i++)
            gtk_combo_box_append_text(profileCombo[j],
                    cfg->profile[j][i].name);
        gtk_combo_box_set_active(profileCombo[j], cfg->profileIndex[j]);
        g_signal_connect(G_OBJECT(profileCombo[j]), "changed",
                G_CALLBACK(combo_update), &cfg->profileIndex[j]);
        gtk_table_attach(table, GTK_WIDGET(profileCombo[j]),
                1, 7, 3*j+1, 3*j+2, GTK_FILL, GTK_FILL, 0, 0);
        button = gtk_button_new();
        gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_stock(
                GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON));
        gtk_table_attach(table, button, 7, 8, 3*j+1, 3*j+2,
                GTK_SHRINK, GTK_FILL, 0, 0);
        g_signal_connect(G_OBJECT(button), "clicked",
                G_CALLBACK(load_profile), (void *)j);
    }
    adjGamma = adjustment_scale(table, 0, 2, "Gamma",
            cfg->profile[0][cfg->profileIndex[0]].gamma,
            &cfg->profile[0][0].gamma, 0.1, 1.0, 0.01, 0.05, 2,
            "Gamma correction for the input profile");
    button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_stock(
            GTK_STOCK_REFRESH, GTK_ICON_SIZE_BUTTON));
    gtk_tooltips_set_tip(toolTips, button, "Reset gamma to default", NULL);
    gtk_table_attach(table, button, 7, 8, 2, 3, 0, 0, 0, 0);
    g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(button_update), (gpointer)3);

    adjLinear = adjustment_scale(table, 0, 3, "Linearity",
            cfg->profile[0][cfg->profileIndex[0]].linear,
            &cfg->profile[0][0].linear, 0.0, 1.0, 0.01, 0.05, 2,
            "Linear part of the gamma correction");
    button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_stock(
            GTK_STOCK_REFRESH, GTK_ICON_SIZE_BUTTON));
    gtk_tooltips_set_tip(toolTips, button, "Reset linearity to default", NULL);
    gtk_table_attach(table, button, 7, 8, 3, 4, 0, 0, 0, 0);
    g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(button_update), (gpointer)4);

    label = gtk_label_new("Intent");
    gtk_table_attach(table, label, 0, 1, 5, 6, 0, 0, 0, 0);
    combo = GTK_COMBO_BOX(gtk_combo_box_new_text());
    gtk_combo_box_append_text(combo, "Perceptual");
    gtk_combo_box_append_text(combo, "Relative colorimetric");
    gtk_combo_box_append_text(combo, "Saturation");
    gtk_combo_box_append_text(combo, "Absolute colorimetric");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), cfg->intent);
    g_signal_connect(G_OBJECT(combo), "changed",
            G_CALLBACK(combo_update), &cfg->intent);
    gtk_table_attach(table, GTK_WIDGET(combo), 1, 7, 5, 6, GTK_FILL, 0, 0, 0);

    page = table_with_frame(noteBox, "Corrections", TRUE);

    table = GTK_TABLE(table_with_frame(page, NULL, TRUE));
    adjExposure = adjustment_scale(table, 0, 0, "Exposure",
            isnan(cfg->exposure) ? 0 : cfg->exposure, &cfg->exposure,
            -3, 3, 0.01, 1.0/6, 2, "EV compensation");

    button = gtk_toggle_button_new();
    gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_stock(
            GTK_STOCK_CUT, GTK_ICON_SIZE_BUTTON));
    snprintf(text, max_name, "Highlight clipping\n"
	    "Current state: %s", cfg->unclip ? "unclip" : "clip");
    gtk_tooltips_set_tip(toolTips, button, text, NULL);
    gtk_table_attach(table, button, 7, 8, 0, 1, 0, 0, 0, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), !cfg->unclip);
    g_signal_connect(G_OBJECT(button), "toggled",
            G_CALLBACK(toggle_button_update), &cfg->unclip);

    button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_stock(
            GTK_STOCK_REFRESH, GTK_ICON_SIZE_BUTTON));
    gtk_tooltips_set_tip(toolTips, button, "Reset exposure to default", NULL);
    gtk_table_attach(table, button, 8, 9, 0, 1, 0, 0, 0, 0);
    g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(button_update), (gpointer)1);

    expAAButton = GTK_TOGGLE_BUTTON(gtk_toggle_button_new());
    gtk_container_add(GTK_CONTAINER(expAAButton), gtk_image_new_from_stock(
            GTK_STOCK_EXECUTE, GTK_ICON_SIZE_BUTTON));
    gtk_tooltips_set_tip(toolTips, GTK_WIDGET(expAAButton),
	    "Auto adjust exposure", NULL);
    gtk_table_attach(table, GTK_WIDGET(expAAButton), 9, 10, 0, 1, 0, 0, 0, 0);
    gtk_toggle_button_set_active(expAAButton, cfg->autoExposure);
    g_signal_connect(G_OBJECT(expAAButton), "clicked",
            G_CALLBACK(button_update), (gpointer)2);

    table = GTK_TABLE(table_with_frame(page, expanderText[curve_expander],
            cfg->expander[curve_expander]));
    curveCombo = GTK_COMBO_BOX(gtk_combo_box_new_text());
    /* Fill in the curve names, skipping "camera curve" if there is
     * no cameraCurve. This will make some mess later with the counting */
    for (i=0; i<cfg->curveCount; i++)
        if ( i!=camera_curve || CFG_cameraCurve )
            gtk_combo_box_append_text(curveCombo, cfg->curve[i].name);
    /* This is part of the mess with the combo_box counting */
    if (cfg->curveIndex>camera_curve && !CFG_cameraCurve)
        gtk_combo_box_set_active(curveCombo, cfg->curveIndex-1);
    else
        gtk_combo_box_set_active(curveCombo, cfg->curveIndex);
    g_signal_connect(G_OBJECT(curveCombo), "changed",
            G_CALLBACK(combo_update), &cfg->curveIndex);
    gtk_table_attach(table, GTK_WIDGET(curveCombo), 0, 7, 0, 1,
            GTK_FILL, GTK_FILL, 0, 0);
    button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_stock(
            GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON));
    gtk_table_attach(table, button, 7, 8, 0, 1, GTK_SHRINK, GTK_FILL, 0, 0);
    gtk_tooltips_set_tip(toolTips, button, "Load curve", NULL);
    g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(load_curve), NULL);
    button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_stock(
            GTK_STOCK_SAVE_AS, GTK_ICON_SIZE_BUTTON));
    gtk_tooltips_set_tip(toolTips, button, "Save curve", NULL);
    gtk_table_attach(table, button, 8, 9, 0, 1, GTK_SHRINK, GTK_FILL, 0, 0);
    g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(save_curve), NULL);

    curveWidget = curveeditor_widget_new(256, 256,
	    render_preview, render_default);
    curveeditor_widget_set_curve(curveWidget, &cfg->curve[cfg->curveIndex]);
    Curve = curveeditor_widget_get_curve(curveWidget);
    gtk_table_attach(table, curveWidget, 0, 9, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

    i = 2;
    adjContrast = adjustment_scale(table, 0, i++,
            "Contrast", Curve->m_contrast, &Curve->m_contrast,
            -0.99, 2, 0.01, 0.10, 2, "Contrast");
    button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_stock(
            GTK_STOCK_REFRESH, GTK_ICON_SIZE_BUTTON));
    gtk_tooltips_set_tip(toolTips, button, "Reset contrast to default", NULL);
    gtk_table_attach(table, button, 7, 8, i-1, i, 0, 0, 0, 0);
    g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(button_update), (gpointer)10);

    adjSaturation = adjustment_scale(table, 0, i++, "Saturation",
            Curve->m_saturation, &Curve->m_saturation,
            0.0, 3.0, 0.01, 0.1, 2, "Saturation");
    button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_stock(
            GTK_STOCK_REFRESH, GTK_ICON_SIZE_BUTTON));
    gtk_tooltips_set_tip(toolTips, button, "Reset saturation to default", NULL);
    gtk_table_attach(table, button, 7, 8, i-1, i, 0, 0, 0, 0);
    g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(button_update), (gpointer)20);

    gtk_table_attach(table, gtk_hseparator_new(), 0, 7, i, i+1,
            GTK_FILL, 0, 0, 0);
    i++;
    adjCurveBlack = adjustment_scale(table, 0, i++, "Black point",
            Curve->m_black, &Curve->m_black, 0, 0.50, 0.01, 0.05, 2,
            "Black point");
    button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_stock(
            GTK_STOCK_REFRESH, GTK_ICON_SIZE_BUTTON));
    gtk_tooltips_set_tip(toolTips, button, "Reset black-point to default",NULL);
    gtk_table_attach(table, button, 7, 8, i-1, i, 0, 0, 0, 0);
    g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(button_update), (gpointer)101);

    button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_stock(
            GTK_STOCK_EXECUTE, GTK_ICON_SIZE_BUTTON));
    gtk_tooltips_set_tip(toolTips, button, "Auto adjust black-point", NULL);
    gtk_table_attach(table, button, 8, 9, i-1, i, 0, 0, 0, 0);
    g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(button_update), (gpointer)102);

    table = GTK_TABLE(table_with_frame(previewVBox, expanderText[live_expander],
            cfg->expander[live_expander]));
    event_box = gtk_event_box_new();
    gtk_table_attach(table, event_box, 0, 7, 1, 2, 0, 0, 0, 0);
    pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
            live_his_size+2, cfg->liveHistogramHeight+2);
    liveHisto = gtk_image_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);
    gtk_container_add(GTK_CONTAINER(event_box), liveHisto);
    pixies = gdk_pixbuf_get_pixels(pixbuf);
    rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    memset(pixies, 0, (gdk_pixbuf_get_height(pixbuf)-1)* rowstride +
            gdk_pixbuf_get_width(pixbuf));
    menu = gtk_menu_new();
    g_signal_connect_swapped(G_OBJECT(event_box), "button_press_event",
            G_CALLBACK(live_histo_menu), menu);
    group = NULL;
    menu_item = gtk_radio_menu_item_new_with_label(group, "RGB histogram");
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 0, 1);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    cfg->histogram==rgb_histogram);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)rgb_histogram);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
            G_CALLBACK(radio_menu_update), &cfg->histogram);
    menu_item = gtk_radio_menu_item_new_with_label(group, "R+G+B histogram");
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 1, 2);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    cfg->histogram==r_g_b_histogram);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)r_g_b_histogram);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
            G_CALLBACK(radio_menu_update), &cfg->histogram);
    menu_item = gtk_radio_menu_item_new_with_label(group,
	    "Luminosity histogram");
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 2, 3);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    cfg->histogram==luminosity_histogram);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)luminosity_histogram);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
            G_CALLBACK(radio_menu_update), &cfg->histogram);
    menu_item = gtk_radio_menu_item_new_with_label(group,
	    "Value (maximum) histogram");
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 3, 4);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    cfg->histogram==value_histogram);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)value_histogram);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
            G_CALLBACK(radio_menu_update), &cfg->histogram);
    menu_item = gtk_radio_menu_item_new_with_label(group,
	    "Saturation histogram");
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 4, 5);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    cfg->histogram==saturation_histogram);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)saturation_histogram);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
            G_CALLBACK(radio_menu_update), &cfg->histogram);

    menu_item = gtk_separator_menu_item_new();
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 5, 6);

    group = NULL;
    menu_item = gtk_radio_menu_item_new_with_label(group, "Linear");
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 6, 7);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    cfg->liveHistogramScale==linear_histogram);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)linear_histogram);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
            G_CALLBACK(radio_menu_update), &cfg->liveHistogramScale);
    menu_item = gtk_radio_menu_item_new_with_label(group, "Logarithmic");
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 7, 8);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    cfg->liveHistogramScale==log_histogram);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)log_histogram);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
            G_CALLBACK(radio_menu_update), &cfg->liveHistogramScale);

    menu_item = gtk_separator_menu_item_new();
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 8, 9);

    group = NULL;
    menu_item = gtk_radio_menu_item_new_with_label(group, "96 Pixels");
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 9, 10);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    cfg->liveHistogramHeight==96);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)96);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
            G_CALLBACK(radio_menu_update), &cfg->liveHistogramHeight);
    menu_item = gtk_radio_menu_item_new_with_label(group, "128 Pixels");
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 10, 11);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    cfg->liveHistogramHeight==128);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)128);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
            G_CALLBACK(radio_menu_update), &cfg->liveHistogramHeight);
    menu_item = gtk_radio_menu_item_new_with_label(group, "192 Pixels");
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 11, 12);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    cfg->liveHistogramHeight==192);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)192);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
            G_CALLBACK(radio_menu_update), &cfg->liveHistogramHeight);
    menu_item = gtk_radio_menu_item_new_with_label(group, "256 Pixels");
    gtk_menu_attach(GTK_MENU(menu), menu_item, 0, 1, 12, 13);
    group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
	    cfg->liveHistogramHeight==256);
    g_object_set_data(G_OBJECT(menu_item), "Radio-Value",
	    (gpointer)256);
    g_signal_connect(G_OBJECT(menu_item), "toggled",
            G_CALLBACK(radio_menu_update), &cfg->liveHistogramHeight);
    gtk_widget_show_all(menu);

    i = 2;
    avrLabels = color_labels_new(table, 0, i++, "Average:", pixel_format);
    devLabels = color_labels_new(table, 0, i++, "Std. deviation:",
            pixel_format);
    overLabels = color_labels_new(table, 0, i, "Overexposed:", percent_format);
    toggle_button(table, 4, i, NULL, &cfg->overExp);
    button = gtk_button_new_with_label("Indicate");
    gtk_table_attach(table, button, 6, 7, i, i+1,
            GTK_SHRINK|GTK_FILL, GTK_FILL, 0, 0);
    g_signal_connect(G_OBJECT(button), "pressed",
            G_CALLBACK(render_preview), (void *)render_overexposed);
    g_signal_connect(G_OBJECT(button), "released",
            G_CALLBACK(render_preview), (void *)render_default);
    i++;
    underLabels = color_labels_new(table, 0, i, "Underexposed:",
            percent_format);
    toggle_button(table, 4, i, NULL, &cfg->underExp);
    button = gtk_button_new_with_label("Indicate");
    gtk_table_attach(table, button, 6, 7, i, i+1,
            GTK_SHRINK|GTK_FILL, GTK_FILL, 0, 0);
    g_signal_connect(G_OBJECT(button), "pressed",
            G_CALLBACK(render_preview), (void *)render_underexposed);
    g_signal_connect(G_OBJECT(button), "released",
            G_CALLBACK(render_preview), (void *)render_default);
    i++;

/*    right side of the preview window */
    vBox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(previewHBox, vBox, FALSE, FALSE, 2);
    align = gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_box_pack_start(GTK_BOX(vBox), align, TRUE, TRUE, 0);
    box = GTK_BOX(gtk_vbox_new(FALSE, 0));
    gtk_container_add(GTK_CONTAINER(align), GTK_WIDGET(box));
    event_box = gtk_event_box_new();
    gtk_box_pack_start(box, event_box, FALSE, FALSE, 0);
    pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
            preview_width, preview_height);
    previewWidget = gtk_image_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);
    gtk_container_add(GTK_CONTAINER(event_box), previewWidget);
    g_signal_connect(G_OBJECT(event_box), "button_press_event",
            G_CALLBACK(spot_press), NULL);
    g_signal_connect(G_OBJECT(event_box), "motion-notify-event",
            G_CALLBACK(spot_motion), NULL);
    g_signal_connect(G_OBJECT(event_box), "enter-notify-event",
	    G_CALLBACK(preview_cursor), gdk_cursor_new(GDK_HAND2));
    g_signal_connect(G_OBJECT(event_box), "leave-notify-event",
	    G_CALLBACK(preview_cursor), NULL);

    progressBar = GTK_PROGRESS_BAR(gtk_progress_bar_new());
    gtk_box_pack_start(box, GTK_WIDGET(progressBar), FALSE, FALSE, 0);

    box = GTK_BOX(gtk_hbox_new(FALSE, 6));
    gtk_box_pack_start(GTK_BOX(vBox), GTK_WIDGET(box), FALSE, FALSE, 6);
    if (plugin) {
        combo = GTK_COMBO_BOX(gtk_combo_box_new_text());
        gtk_combo_box_append_text(combo, "Full interpolation");
        gtk_combo_box_append_text(combo, "Four color interpolation");
        gtk_combo_box_append_text(combo, "Quick interpolation");
        gtk_combo_box_append_text(combo, "half-size interpolation");
        gtk_combo_box_set_active(combo, cfg->interpolation);
        g_signal_connect(G_OBJECT(combo), "changed",
                G_CALLBACK(combo_update), &cfg->interpolation);
        gtk_box_pack_start(box, GTK_WIDGET(combo), FALSE, FALSE, 0);
    }
    align = gtk_alignment_new(0.99, 0.5, 0, 1);
    gtk_box_pack_start(box, align, TRUE, TRUE, 6);
    box = GTK_BOX(gtk_hbox_new(TRUE, 6));
    gtk_container_add(GTK_CONTAINER(align), GTK_WIDGET(box));
    button = gtk_button_new();
    hbox = GTK_BOX(gtk_hbox_new(FALSE, 6));
    gtk_container_add(GTK_CONTAINER(button), GTK_WIDGET(hbox));
    gtk_box_pack_start(hbox, gtk_image_new_from_stock(
            GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_BUTTON), FALSE, FALSE, 0);
    gtk_box_pack_start(hbox, gtk_label_new("Options"),
            FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(options_dialog), previewWindow);
    gtk_box_pack_start(box, button, TRUE, TRUE, 0);
    button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(window_response), (gpointer)GTK_RESPONSE_CANCEL);
    gtk_box_pack_start(box, button, TRUE, TRUE, 0);
    if (plugin) {
	saveButton = gtk_button_new_from_stock(GTK_STOCK_OK);
        gtk_box_pack_start(box, saveButton, TRUE, TRUE, 0);
        gtk_widget_grab_focus(saveButton);
    } else {
	saveButton = gtk_button_new_from_stock(GTK_STOCK_SAVE);
        gtk_box_pack_start(box, saveButton, TRUE, TRUE, 0);
	char *text = (char *)ufraw_saver(NULL, image);
	gtk_tooltips_set_tip(toolTips, saveButton, text, NULL);
	g_free(text);
	saveAsButton = gtk_button_new_from_stock(GTK_STOCK_SAVE_AS);
        gtk_box_pack_start(box, saveAsButton, TRUE, TRUE, 0);
        gtk_widget_grab_focus(saveAsButton);
    }
    gtk_widget_show_all(previewWindow);

    preview_progress("Loading preview", 0.2);
    ufraw_load_raw(image, TRUE);

    shrinkSave = image->cfg->shrink;
    sizeSave = image->cfg->size;
    image->cfg->shrink = scale;
    image->cfg->size = 0;
    previewImage = &preview_image;
    ufraw_convert_image(previewImage, image);
    image->cfg->shrink = shrinkSave;
    image->cfg->size = sizeSave;
    previewImage->cfg = image->cfg;
    if (preview_width!=previewImage->width ||
        preview_height!=previewImage->height) {
        pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
            previewImage->width, previewImage->height);
        gtk_image_set_from_pixbuf(GTK_IMAGE(previewWidget), pixbuf);
        g_object_unref(pixbuf);
    }
    ufraw_message(UFRAW_SET_LOG, "ufraw_preview: rgbMax=%d\n",
            previewImage->rgbMax);
    preview_progress(progressText, 0);
    freezeDialog = FALSE;
    update_scales();
    /* The SAVE/OK button is connect only here so it won't be pressed
     * earlier */
    g_signal_connect(G_OBJECT(saveButton), "clicked",
            G_CALLBACK(save_func), image);
    if (saveAsButton!=NULL) g_signal_connect(G_OBJECT(saveAsButton), "clicked",
            G_CALLBACK(save_func), image);
    gtk_main();
    status = (long)g_object_get_data(G_OBJECT(previewWindow),
            "WindowResponse");
    gtk_container_foreach(GTK_CONTAINER(previewVBox),
            (GtkCallback)(expander_state), NULL);
    ufraw_focus(previewWindow, FALSE);
    gtk_widget_destroy(previewWindow);

    progressBar = NULL;
    previewImage->developer = NULL;
    ufraw_close(previewImage);
    ufraw_close(image);
    /* In interactive mode outputPath is taken into account only once */
    strcpy(image->cfg->outputPath, "");
    if (status==GTK_RESPONSE_OK)
	save_configuration(cfg, Developer, NULL, NULL, 0);
    else *image->cfg = save_cfg;
    if (status!=GTK_RESPONSE_OK) return UFRAW_CANCEL;
    return UFRAW_SUCCESS;
}
