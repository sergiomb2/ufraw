/***************************************************
 * curveeditor_widget.h
 *
 * This is an implementation for a curve editor widget for GTK+
 *
 * It is licensed under the GNU General Public License.
 * 
 * Authors: Shawn Freeman, Udi Fuchs
 *
 **************************************************/

GtkWidget *curveeditor_widget_new(int height, int width,
	void (*callback)(), gpointer userdata);

void curveeditor_widget_update(GtkWidget *widget);

void curveeditor_widget_set_curve(GtkWidget *widget, CurveData *curve);

CurveData *curveeditor_widget_get_curve(GtkWidget *widget);

gboolean curveeditor_widget_get_coordinates(GtkWidget *widget,
	double *x, double *y);

