/***************************************************
 * curveeditor_widget.h - a curve editor widget for GTK+
 * Copyright 2004-2008 by Shawn Freeman, Udi Fuchs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 **************************************************/

GtkWidget *curveeditor_widget_new(int height, int width,
	void (*callback)(), gpointer userdata);

void curveeditor_widget_update(GtkWidget *widget);

void curveeditor_widget_set_curve(GtkWidget *widget, CurveData *curve);

CurveData *curveeditor_widget_get_curve(GtkWidget *widget);

gboolean curveeditor_widget_get_coordinates(GtkWidget *widget,
	double *x, double *y);

