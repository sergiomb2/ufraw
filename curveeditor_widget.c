/***************************************************
 * curveeditor_widget.c
 *
 * This is an implementation for a curve editor widget for GTK+
 *
 * It is licensed under the GNU General Public License.
 *
 * Authors: Shawn Freeman, Udi Fuchs
 *
 **************************************************/

//#ifdef HAVE_CONFIG_H
//#  include <config.h>
//#endif

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "nikon_curve.h"

//All the internal data of curve widget is held in this structure
typedef struct {
    CurveData *curve;
    int selectedPoint;
    GdkPixmap *pixmap;
    int width, height;
    void (*callback)();
    gpointer userdata;
    GtkWidget *widget;
    GtkLabel *label;
} CurveEditorWidgetData;

/********************************************************
curveeditor_widget_qsort_compare:
    Comparator for qsort function call. This is a local helper
    function only.
*********************************************************/
int curveeditor_widget_qsort_compare( const void *arg1, const void *arg2 )
{
    /* Compare points. Must define all three return values! */
    if (((CurveAnchorPoint*)arg1)->x > ((CurveAnchorPoint*)arg2)->x)
    {
	return 1;
    }
    else if (((CurveAnchorPoint*)arg1)->x < ((CurveAnchorPoint*)arg2)->x)
    {
	return -1;
    }
    //this should not happen. x-coords must be different for spline to work.
   return 0;
}

/**********************************************
Draws the curve on the drawing area.
***********************************************/
void curveeditor_widget_draw(CurveEditorWidgetData *data)
{
    GdkPoint graphPoints[NIKON_MAX_ANCHORS];
    //get the graphics context
    GdkGC *gc = gdk_gc_new(data->pixmap);
    //Used to alter colors while drawing
    GdkColor color;
    CurveData *curve = data->curve;
    int w = data->width, h = data->height;
    GdkPixmap *graph = data->pixmap;
    double selectedPoint_x = 0;
    
    //fill bg of graph to black
    color.red = 0;
    color.green = 0;
    color.blue = 0;
    gdk_gc_set_rgb_fg_color (gc, &color);
    gdk_draw_rectangle (graph, gc, TRUE, 0, 0, w, h);
  
    //draw the grid lines grey
    color.red = 0x4000;
    color.green = 0x4000;
    color.blue = 0x4000;
    gdk_gc_set_rgb_fg_color (gc, &color);

    //draw grid lines
    gdk_draw_line (graph, gc, 1 * w/4, 0, 1 * w/4, h);
    gdk_draw_line (graph, gc, 2 * w/4, 0, 2 * w/4, h);
    gdk_draw_line (graph, gc, 3 * w/4, 0, 3 * w/4, h);

    gdk_draw_line (graph, gc, 0, 1 * h/4, w, 1 * h/4);
    gdk_draw_line (graph, gc, 0, 2 * h/4, w, 2 * h/4);
    gdk_draw_line (graph, gc, 0, 3 * h/4, w, 3 * h/4);

    //redraw the curve
    CurveSample *sample = CurveSampleInit(w, h-1);

    switch(curve->m_curveType)
    {
    case TONE_CURVE:  //tone curve in white
	color.red = 0xFFFF;
	color.green = 0xFFFF;
	color.blue = 0xFFFF;
	break;

    case RED_CURVE:   //red curve in red
	color.red = 65535;
	color.green = 0;
	color.blue = 0;
	break;
    case GREEN_CURVE: //green curve in green
	color.red = 0;
	color.green = 65535;
	color.blue = 0;
	break;
    case BLUE_CURVE:  //blue curve in blue
	color.red = 0;
	color.green = 0;
	color.blue = 65535;
	break;
    default:
	//??
	break;
    }

    //set the color to draw the curve
    gdk_gc_set_rgb_fg_color (gc, &color);

    //check to see if this curve contains any data
    //We should not ever get here.
    if (curve->m_numAnchors == 0)
    {
	//init this curve to a straight line
	curve->m_min_x = 0;
	curve->m_max_x = 1;
	curve->m_min_y = 0;
	curve->m_max_y = 1;
	curve->m_gamma = 1;
	curve->m_numAnchors = 2;
	curve->m_anchors[0].x = 0;
	curve->m_anchors[0].y = 0;
	curve->m_anchors[1].x = 1;
	curve->m_anchors[1].y = 1;
    }
    int i = 0;

    //get current selected point x value
    if (data->selectedPoint>=0)
	selectedPoint_x = curve->m_anchors[data->selectedPoint].x;

    //make sure the points are in sorted order by x coord
    qsort(curve->m_anchors,curve->m_numAnchors,sizeof(CurveAnchorPoint),
	   curveeditor_widget_qsort_compare);

    //update the selection to match the sorted list
    if (data->selectedPoint>=0)
    {
	for(i = 0; i < curve->m_numAnchors; i++)
	{
	    if (curve->m_anchors[i].x == selectedPoint_x)
	    {
		data->selectedPoint = i;
		break;
	    }
	}
    }

    CurveDataSample(curve, sample);

    for (i=0; i<(int)sample->m_samplingRes; i++)
    {
	gdk_draw_point(graph, gc, i, h-1-sample->m_Samples[i]);
    }
    CurveSampleFree(sample);

    double g = 1 / curve->m_gamma;
    for (i = 0; i < curve->m_numAnchors; i++)
    {
	graphPoints[i].x = (int) (curve->m_anchors[i].x * (w-1));
	graphPoints[i].y = (int) ((1 - pow (curve->m_anchors[i].y, g))*(h-1));
    }

    for (i = 0; i < curve->m_numAnchors; i++)
    {
	gdk_draw_rectangle (graph, gc, TRUE,
		graphPoints[i].x - 1, graphPoints[i].y - 1, 5, 5);
    }

    //highlight selected point
    if (data->selectedPoint >= 0)
    {
	gdk_draw_rectangle (graph, gc, FALSE,
		graphPoints[data->selectedPoint].x - 3,
	        graphPoints[data->selectedPoint].y - 3, 8, 8);

	gdk_draw_rectangle (graph, gc, TRUE,
	        graphPoints[data->selectedPoint].x - 1,
		graphPoints[data->selectedPoint].y - 1, 5, 5);
    }
    gtk_widget_queue_draw(data->widget);
}

/********************************************************
curveeditor_widget_on_focus_event:
*********************************************************/
gboolean curveeditor_widget_on_focus_event(GtkWidget *widget,
	GdkEventFocus *event, CurveEditorWidgetData *data)
{
    widget = widget;

    if (event->in)
	data->selectedPoint = 0;
    else
	data->selectedPoint = -1;
 
    curveeditor_widget_draw(data);

    return FALSE;
}

/********************************************************
curveeditor_widget_on_button_press_event:
*********************************************************/
gboolean curveeditor_widget_on_button_press_event (GtkWidget * widget,
	GdkEventButton * event, CurveEditorWidgetData *data)
{
    int i;

    gtk_widget_grab_focus(widget);

    if (event->button!=1) return FALSE;

    CurveData *curve = data->curve;

    int w = data->width;
    int h = data->height;
    double g = 1 / curve->m_gamma;

    data->selectedPoint = -1;

    for (i = 0; i < curve->m_numAnchors; i++)
    {
	if ( abs(event->x - curve->m_anchors[i].x*(w-1)) < 7 &&
	     abs(event->y - (1- pow(curve->m_anchors[i].y, g))*(h-1)) < 7 )
	{
	  data->selectedPoint = i;
	  break;
	}
    }

    if (data->selectedPoint == -1)
    {
	//add point
	int num = curve->m_numAnchors;
      
	//nikon curve files don't allow more than 20 anchors
	if (curve->m_numAnchors >= NIKON_MAX_ANCHORS) return TRUE;

	int x = (int) event->x;

	//add it to the end
	curve->m_anchors[num].x = (double) x / (w-1);
	curve->m_anchors[num].y = (double) (h-1-event->y) / (h-1);
	data->selectedPoint = num;

	curve->m_numAnchors = num + 1;
          
	//Make sure the selected point draws at the mouse location
	//This is done by taking the point and raising it to the gamma.
	//When the gamma is calculated later, the 1/gamma cancels this out
	//leaving the point where it has been placed.
	curve->m_anchors[data->selectedPoint].y =
		pow (curve->m_anchors[data->selectedPoint].y, curve->m_gamma);
    }
    curveeditor_widget_draw(data);
    if (data->callback!=NULL)
	(*data->callback)(data->widget, data->userdata);
    return TRUE;
}

/********************************************************
curveeditor_widget_on_show_event:
*********************************************************/
void curveeditor_widget_on_show_event(GtkWidget *widget,
	CurveEditorWidgetData *data)
{
    widget = widget;
    curveeditor_widget_draw(data);
}

/********************************************************
curveeditor_widget_on_motion_notify_event:
*********************************************************/
gboolean curveeditor_widget_on_motion_notify_event(GtkWidget *widget,
	GdkEventButton *event, CurveEditorWidgetData *data)
{
    widget = widget;
    if ((event->state&GDK_BUTTON1_MASK)==0) return TRUE;
    int w = data->width;
    int h = data->height;
    int update = 1;

    CurveData *curve = data->curve;
    
    if (data->selectedPoint < 0) return TRUE;

    double x = (double)event->x / (double)(w-1);
    double y = pow( (double)(h - event->y) / (double)(h-1), curve->m_gamma);

    if (event->x < 0) x = 0;
    if (event->x > w) x = 1;

    if (event->y < 0) y = 1;
    if (event->y > h) y = 0;
	
    //don't allow a draging point to exceed or preceed neighbors or
    //else the spline algorithm will explode.
    int i;
    for(i = 0; i < curve->m_numAnchors; i++)
    {
	if ( i!=data->selectedPoint &&
		fabs(x-curve->m_anchors[i].x)< 1.0/256.0)
	{
	    update = 0;
	}
    }
	      
    //update the points
    if (update)
    {
	curve->m_anchors[data->selectedPoint].x = x;
	curve->m_anchors[data->selectedPoint].y = y;

	curveeditor_widget_draw(data);
	if (data->callback!=NULL)
	    (*data->callback)(data->widget, data->userdata);
    }
    return TRUE;
}

/********************************************************
curveeditor_widget_on_key_press_event:
*********************************************************/
gboolean curveeditor_widget_on_key_press_event(GtkWidget *widget,
	GdkEventKey * event, CurveEditorWidgetData *data)
{
    int i;
    widget = widget;
    CurveData *curve = data->curve;
    int w = data->width, h = data->height;

    //There must be a point selected
    if (data->selectedPoint<0) return FALSE;

    //insert adds a point between the current one an the next one
    if (event->keyval == GDK_Insert)
    {
	if (data->selectedPoint >= curve->m_numAnchors-1) return TRUE;
	if (curve->m_numAnchors >= NIKON_MAX_ANCHORS) return TRUE;
	if ( ( curve->m_anchors[data->selectedPoint+1].x -
	      curve->m_anchors[data->selectedPoint].x ) < 2.0/(w-1) )
		  return TRUE;
	
	CurveSample *sample = CurveSampleInit(w, h-1);
	CurveDataSample(curve, sample);

	//Add the point at the end - it will be sorted later anyway
	curve->m_anchors[curve->m_numAnchors].x =
	    ( curve->m_anchors[data->selectedPoint].x +
	      curve->m_anchors[data->selectedPoint+1].x )/2;
	curve->m_anchors[curve->m_numAnchors].y =
	    (double)sample->m_Samples[(int)(curve->m_anchors[
		curve->m_numAnchors].x*(w))]/(h-1);
	CurveSampleFree(sample);

	data->selectedPoint = curve->m_numAnchors++;

	//redraw graph
	curveeditor_widget_draw(data);
	if (data->callback!=NULL)
	    (*data->callback)(data->widget, data->userdata);

	return TRUE;
    }

    //delete removes points
    if (event->keyval == GDK_Delete)
    {
	//a minimum of two points must be available at all times!
	if (curve->m_numAnchors==2) return TRUE;

	for (i = data->selectedPoint; i < curve->m_numAnchors - 1; i++)
        {
	    curve->m_anchors[i].x = curve->m_anchors[i + 1].x;
	    curve->m_anchors[i].y = curve->m_anchors[i + 1].y;
	}
	curve->m_numAnchors--;

	//set selected point to next point, but dont move to last point
	if (data->selectedPoint >= curve->m_numAnchors-1)
	    data->selectedPoint--;

	//redraw graph
	curveeditor_widget_draw(data);
	if (data->callback!=NULL)
	    (*data->callback)(data->widget, data->userdata);

	return TRUE;
    }
    
    //Home jumps to first point
    if (event->keyval == GDK_Home) {
	data->selectedPoint = 0;
	curveeditor_widget_draw(data);

	return TRUE;
    }
    
    //End jumps to last point
    if (event->keyval == GDK_End) {
	data->selectedPoint = curve->m_numAnchors-1;
	curveeditor_widget_draw(data);

	return TRUE;
    }
    
    //Page Up jumps to previous point
    if (event->keyval == GDK_Page_Up) {
	data->selectedPoint--;
	if (data->selectedPoint < 0)
	    data->selectedPoint = 0;
	curveeditor_widget_draw(data);

	return TRUE;
    }
    
    //Page Down jumps to next point
    if (event->keyval == GDK_Page_Down) {
	data->selectedPoint++;
	if (data->selectedPoint >= curve->m_numAnchors)
	    data->selectedPoint = curve->m_numAnchors-1;
	curveeditor_widget_draw(data);

	return TRUE;
    }
    
    //Up/Down/Left/Right moves the point around
    if (event->keyval == GDK_Up || event->keyval == GDK_Down ||
	    event->keyval == GDK_Left || event->keyval == GDK_Right)
    {
	if (event->keyval==GDK_Up) {
	    curve->m_anchors[data->selectedPoint].y += 1.0/(h-1);
	    if (curve->m_anchors[data->selectedPoint].y>1.0)
		curve->m_anchors[data->selectedPoint].y = 1.0;
	}
	if (event->keyval==GDK_Down) {
	    curve->m_anchors[data->selectedPoint].y -= 1.0/(h-1);
	    if (curve->m_anchors[data->selectedPoint].y<0.0)
		curve->m_anchors[data->selectedPoint].y = 0.0;
	}
	if (event->keyval==GDK_Right) {
	    curve->m_anchors[data->selectedPoint].x += 1.0/(w-1);
	    if (curve->m_anchors[data->selectedPoint].x>1.0)
		curve->m_anchors[data->selectedPoint].x = 1.0;
	    if (data->selectedPoint<curve->m_numAnchors-1 &&
		    curve->m_anchors[data->selectedPoint].x >=
		    curve->m_anchors[data->selectedPoint+1].x-0.5/(w-1))
	    curve->m_anchors[data->selectedPoint].x -= 1.0/(w-1);
	}
	if (event->keyval==GDK_Left) {
	    curve->m_anchors[data->selectedPoint].x -= 1.0/(w-1);
	    if (curve->m_anchors[data->selectedPoint].x<0.0)
		curve->m_anchors[data->selectedPoint].x = 0.0;
	    if (data->selectedPoint>0 &&
		    curve->m_anchors[data->selectedPoint].x <=
		    curve->m_anchors[data->selectedPoint-1].x+0.5/(w-1))
	    curve->m_anchors[data->selectedPoint].x += 1.0/(w-1);
	}
	curveeditor_widget_draw(data);
	if (data->callback!=NULL)
	    (*data->callback)(data->widget, data->userdata);

	return TRUE;
    }
    return FALSE;
}

/********************************************************
curveeditor_widget_on_destroy:
*********************************************************/
void curveeditor_widget_on_destroy(GtkWidget *widget,
	CurveEditorWidgetData *data)
{
    widget = widget;

    g_free(data->curve);
    g_free(data);
}

GtkWidget *curveeditor_widget_new(int height, int width,
	GCallback callback, gpointer userdata)
{
    CurveEditorWidgetData *data = g_new0(CurveEditorWidgetData,1);
    CurveData *curve = g_new0(CurveData,1);
    GtkWidget *curveImage, *curveEventBox, *curveAlign;

    data->pixmap = gdk_pixmap_new(NULL, height, width, 24);
    curveImage = gtk_image_new_from_pixmap(data->pixmap, NULL);
    g_object_unref(data->pixmap);
    curveEventBox = gtk_event_box_new();
    GTK_WIDGET_SET_FLAGS (curveEventBox, GTK_CAN_FOCUS);
    gtk_container_add(GTK_CONTAINER(curveEventBox), curveImage);
    curveAlign = gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_container_add(GTK_CONTAINER(curveAlign), curveEventBox);

    data->curve = curve;
    data->widget = curveAlign;
    data->height = height;
    data->width = width;
    data->callback = callback;
    data->userdata = userdata;
    data->selectedPoint = 0;
    data->label = NULL;

    g_signal_connect_after((gpointer)curveEventBox, "button-press-event",
	G_CALLBACK (curveeditor_widget_on_button_press_event), data);
    g_signal_connect_after((gpointer)curveEventBox, "show",
	G_CALLBACK(curveeditor_widget_on_show_event), data);
    g_signal_connect_after((gpointer)curveEventBox, "motion-notify-event",
	G_CALLBACK(curveeditor_widget_on_motion_notify_event), data);
    g_signal_connect_after((gpointer)curveEventBox, "key-press-event",
	G_CALLBACK(curveeditor_widget_on_key_press_event), data);
    g_signal_connect_after((gpointer)curveEventBox, "focus-in-event",
	G_CALLBACK(curveeditor_widget_on_focus_event), data);
    g_signal_connect_after((gpointer)curveEventBox, "focus-out-event",
	G_CALLBACK(curveeditor_widget_on_focus_event), data);
    g_signal_connect_after((gpointer)curveEventBox, "destroy",
	G_CALLBACK(curveeditor_widget_on_destroy), data);

    g_object_set_data(G_OBJECT(curveAlign), "curve-widget-data",data);

    return curveAlign;
}

void curveeditor_widget_update(GtkWidget *widget)
{
    CurveEditorWidgetData *data = g_object_get_data(G_OBJECT(widget),
	    "curve-widget-data");
    curveeditor_widget_draw(data);
}

void curveeditor_widget_set_curve(GtkWidget *widget, CurveData *curve)
{
    CurveEditorWidgetData *data = g_object_get_data(G_OBJECT(widget),
	    "curve-widget-data");
    *data->curve = *curve;
    curveeditor_widget_draw(data);
}

CurveData *curveeditor_widget_get_curve(GtkWidget *widget)
{
    CurveEditorWidgetData *data = g_object_get_data(G_OBJECT(widget),
	    "curve-widget-data");
    return data->curve;
}

/**********************************************
curveeditor_widget_get_coordinates:
***********************************************/
gboolean curveeditor_widget_get_coordinates(GtkWidget *widget,
	double *x, double *y)
{
    CurveEditorWidgetData *data = g_object_get_data(G_OBJECT(widget),
	    "curve-widget-data");
    //get the current x and y labels
    if (data->selectedPoint != -1)
    {
	*x = data->curve->m_anchors[data->selectedPoint].x;
	*y = data->curve->m_anchors[data->selectedPoint].y;
	return TRUE;
    } else {
	return FALSE;
    }
}

