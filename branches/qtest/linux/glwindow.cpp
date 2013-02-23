#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include "lc_global.h"
#include "lc_application.h"
#include "opengl.h"
#include "glwindow.h"
#include "defines.h"
#include "main.h"
#include "project.h"
#include "system.h"

struct GLWindowPrivate
{
	GtkWidget *widget;
	LC_CURSOR_TYPE Cursor;
};

static Display* WindowDisplay = NULL;
static GdkVisual* WindowGdkVisual = NULL;
static XVisualInfo* WindowXVisualInfo = NULL;
static bool WindowMultisample = false;
static GLXContext WindowContext;
static int WindowContextCount;
static bool dragging;

// =============================================================================
// static functions

static gint realize_event(GtkWidget *widget, gpointer data)
{
	GLWindow *wnd = (GLWindow*)data;

	wnd->MakeCurrent();
	wnd->OnInitialUpdate();

	return TRUE;
}

static gint expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	GLWindow *wnd = (GLWindow*)data;
	GLWindowPrivate *prv = (GLWindowPrivate*)wnd->GetData();

	if (event->count > 0)
		return TRUE;

	wnd->MakeCurrent();
	wnd->OnDraw();

	if (WindowContext)
		pfnglXSwapBuffers(GDK_WINDOW_XDISPLAY(prv->widget->window), GDK_WINDOW_XWINDOW(prv->widget->window));

	return TRUE;
}

static gint button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	GLWindow *wnd = (GLWindow*)data;
	int x, y;

	x = (int)event->x;
	y = widget->allocation.height - (int)event->y - 1;

	if (event->type == GDK_BUTTON_PRESS)
	{
		if (event->button == 1)
			wnd->OnLeftButtonDown(x, y, (event->state & GDK_CONTROL_MASK) != 0, (event->state & GDK_SHIFT_MASK) != 0);
		else if (event->button == 2)
			wnd->OnMiddleButtonDown(x, y, (event->state & GDK_CONTROL_MASK) != 0, (event->state & GDK_SHIFT_MASK) != 0);
		else if (event->button == 3)
			wnd->OnRightButtonDown(x, y, (event->state & GDK_CONTROL_MASK) != 0, (event->state & GDK_SHIFT_MASK) != 0);
	}
	else if (event->type == GDK_2BUTTON_PRESS)
	{
		wnd->OnLeftButtonDoubleClick(x, y, (event->state & GDK_CONTROL_MASK) != 0, (event->state & GDK_SHIFT_MASK) != 0);
	}

	gtk_window_set_focus(GTK_WINDOW(gtk_widget_get_toplevel(widget)), widget);
	gdk_pointer_grab(widget->window, FALSE, (GdkEventMask)(GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK),
	                 NULL, NULL, GDK_CURRENT_TIME);

	return TRUE;
}

static gint button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	GLWindow *wnd = (GLWindow*)data;
	int x, y;

	x = (int)event->x;
	y = widget->allocation.height - (int)event->y - 1;

	gdk_pointer_ungrab(GDK_CURRENT_TIME);

	if (event->button == 1)
		wnd->OnLeftButtonUp(x, y, (event->state & GDK_CONTROL_MASK) != 0, (event->state & GDK_SHIFT_MASK) != 0);
	else if (event->button == 2)
		wnd->OnMiddleButtonUp(x, y, (event->state & GDK_CONTROL_MASK) != 0, (event->state & GDK_SHIFT_MASK) != 0);
	else if (event->button == 3)
		wnd->OnRightButtonUp(x, y, (event->state & GDK_CONTROL_MASK) != 0, (event->state & GDK_SHIFT_MASK) != 0);

	return TRUE;
}

static gint pointer_motion_event(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
	GLWindow *wnd = (GLWindow*)data;
	GdkModifierType state;
	int x, y;

	if (event->is_hint)
	{
		gdk_window_get_pointer(event->window, &x, &y, &state);
		state = (GdkModifierType)0;
	}
	else
	{
		x = (int)event->x;
		y = (int)event->y;
		state = (GdkModifierType)event->state;
	}

	y = widget->allocation.height - y - 1;

	wnd->OnMouseMove(x, y, (event->state & GDK_CONTROL_MASK) != 0, (event->state & GDK_SHIFT_MASK) != 0);

	return TRUE;
}

static gboolean drag_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gpointer data)
{
	if (!dragged_piece)
	{
		return FALSE;
	}

	GLWindow *wnd = (GLWindow*)data;
	y = widget->allocation.height - y - 1;
	lcGetActiveProject()->BeginPieceDrop(dragged_piece);
	wnd->OnLeftButtonUp(x, y, FALSE, FALSE);
	gtk_drag_finish(context, TRUE, FALSE, time);
	return TRUE;
}

static void drag_leave(GtkWidget *widget, GdkDragContext *context, guint time, gpointer data)
{
	if (dragging)
	{
		dragging = false;
		lcGetActiveProject()->HandleNotify(LC_CAPTURE_LOST, 0);
	}
}

static gboolean drag_motion(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gpointer data)
{
	if (!dragged_piece)
	{
		return FALSE;
	}

	GLWindow *wnd = (GLWindow*)data;
	y = widget->allocation.height - y - 1;

	if (!dragging)
	{
		dragging = true;
		lcGetActiveProject()->BeginPieceDrop(dragged_piece);
	}

	wnd->OnMouseMove(x, y, FALSE, FALSE);

	gdk_drag_status(context, GDK_ACTION_COPY, time);
	return TRUE;
}

static gboolean scroll_event(GtkWidget *widget, GdkEventScroll *event, gpointer data)
{
	GLWindow *wnd = (GLWindow*)data;
	float Direction;
	int x, y;

	switch (event->direction)
	{
		case GDK_SCROLL_UP:
			Direction = 1.0f;
			break;
		case GDK_SCROLL_DOWN:
			Direction = -1.0f;
			break;
		default:
			return FALSE;
	}

	x = (int)event->x;
	y = widget->allocation.height - (int)event->y - 1;

	wnd->OnMouseWheel(x, y, Direction, (event->state & GDK_CONTROL_MASK) != 0, (event->state & GDK_SHIFT_MASK) != 0);

	return TRUE;
}

static gint size_allocate_event(GtkWidget *widget, GtkAllocation *allocation, gpointer data)
{
	GLWindow *wnd = (GLWindow*)data;

	wnd->OnSize(allocation->width, allocation->height);

	return TRUE;
}
/*
static void destroy_event(GtkWidget *widget, gpointer data)
{
	GLWindow *wnd = (GLWindow*)data;

	wnd->DestroyContext();
}
*/

void GL_InitializeExtensions()
{
}

// =============================================================================
// GLWindow class

GLWindow::GLWindow(GLWindow *share)
{
	m_pShare = share;
	m_pData = g_malloc(sizeof(GLWindowPrivate));
	memset(m_pData, 0, sizeof(GLWindowPrivate));
}

GLWindow::~GLWindow()
{
	DestroyContext();
	g_free(m_pData);
}

bool GLWindow::CreateFromWindow(void *data)
{
	GLWindowPrivate *prv = (GLWindowPrivate*)m_pData;

	if (WindowContext)
		WindowContextCount++;
	else
	{
		int attrlist[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 16, 0 };

		WindowDisplay = GDK_DISPLAY();

		if (!WindowDisplay)
		{
			printf("OpenGL fatal error: Cannot get display.\n");
			return false;
		}

		WindowGdkVisual = gdk_visual_get_system();
		if (WindowGdkVisual->depth < 16)
			printf("OpenGL fatal error: LeoCAD needs a display with at least 16 bit colors.\n");

		int AASamples = Sys_ProfileLoadInt("Default", "AASamples", 1);

		if (AASamples > 1)
		{
			int attrlistAA[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 16, GLX_SAMPLE_BUFFERS_ARB, 1, GLX_SAMPLES_ARB, AASamples, 0 };
			WindowXVisualInfo = pfnglXChooseVisual(WindowDisplay, DefaultScreen(WindowDisplay), attrlistAA);

			if (WindowXVisualInfo)
				WindowMultisample = true;
			else
				printf("OpenGL error: Could not find multisample visual.\n");
		}

		if (!WindowXVisualInfo)
		{
			WindowXVisualInfo = pfnglXChooseVisual(WindowDisplay, DefaultScreen(WindowDisplay), attrlist);
			if (!WindowXVisualInfo)
			{
				printf("OpenGL fatal error: glXChooseVisual failed.\n");
				return false;
			}
		}

		WindowGdkVisual = gdkx_visual_get(WindowXVisualInfo->visualid);
		if (WindowGdkVisual == NULL)
		{
			printf("OpenGL fatal error: Cannot get visual.\n");
			return false;
		}

		WindowContext = pfnglXCreateContext(WindowDisplay, WindowXVisualInfo, NULL, True);

		if (!WindowContext)
		{
			printf("OpenGL fatal error: Cannot create context.\n");
			return false;
		}

		WindowContextCount = 1;
	}

	gtk_widget_push_colormap(gdk_colormap_new(WindowGdkVisual, TRUE));
	gtk_widget_push_visual(WindowGdkVisual);

	prv->widget = gtk_drawing_area_new();
	gtk_widget_set_double_buffered(GTK_WIDGET(prv->widget), FALSE);

	gtk_widget_pop_visual();
	gtk_widget_pop_colormap();

	GTK_WIDGET_SET_FLAGS(prv->widget, GTK_CAN_FOCUS);

	gtk_widget_set_events(GTK_WIDGET(prv->widget), GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);

	gtk_drag_dest_set(prv->widget, GTK_DEST_DEFAULT_MOTION, drag_target_list, 1, GDK_ACTION_COPY);

	// Connect signal handlers
	gtk_signal_connect(GTK_OBJECT(prv->widget), "expose_event", GTK_SIGNAL_FUNC(expose_event), this);
//	gtk_signal_connect(GTK_OBJECT(prv->widget), "destroy", GTK_SIGNAL_FUNC(destroy_event), this);
	gtk_signal_connect(GTK_OBJECT(prv->widget), "size_allocate", GTK_SIGNAL_FUNC(size_allocate_event), this);
	gtk_signal_connect(GTK_OBJECT(prv->widget), "motion_notify_event", GTK_SIGNAL_FUNC(pointer_motion_event), this);
	gtk_signal_connect(GTK_OBJECT(prv->widget), "button_press_event", GTK_SIGNAL_FUNC(button_press_event), this);
	gtk_signal_connect(GTK_OBJECT(prv->widget), "button_release_event", GTK_SIGNAL_FUNC(button_release_event), this);
	gtk_signal_connect(GTK_OBJECT(prv->widget), "drag_drop", GTK_SIGNAL_FUNC(drag_drop), this);
	gtk_signal_connect(GTK_OBJECT(prv->widget), "drag_leave", GTK_SIGNAL_FUNC(drag_leave), this);
	gtk_signal_connect(GTK_OBJECT(prv->widget), "drag_motion", GTK_SIGNAL_FUNC(drag_motion), this);
	gtk_signal_connect(GTK_OBJECT(prv->widget), "scroll_event", GTK_SIGNAL_FUNC(scroll_event), this);
	gtk_signal_connect(GTK_OBJECT(prv->widget), "realize", GTK_SIGNAL_FUNC(realize_event), this);

	*((GtkWidget**)data) = prv->widget;

	return true;
}

void GLWindow::DestroyContext()
{
	if (!WindowContext)
		return;

	WindowContextCount--;

	if (WindowContextCount)
		return;

	if (WindowContext == pfnglXGetCurrentContext())
		pfnglXMakeCurrent(WindowDisplay, None, NULL);

	pfnglXDestroyContext(WindowDisplay, WindowContext);
	WindowContext = NULL;

	XFree(WindowXVisualInfo);
	WindowXVisualInfo = NULL;
}

void GLWindow::OnInitialUpdate()
{
	MakeCurrent();

	GL_InitializeSharedExtensions();
//	GL_InitializeExtensions();

	if (WindowMultisample)
		glEnable(GL_MULTISAMPLE_ARB);
}

bool GLWindow::MakeCurrent()
{
	GLWindowPrivate *prv = (GLWindowPrivate*)m_pData;

	if (!WindowContext)
		return false;

	return pfnglXMakeCurrent(WindowDisplay, GDK_WINDOW_XWINDOW(prv->widget->window), WindowContext);
}

void GLWindow::Redraw(bool ForceRedraw)
{
	GLWindowPrivate *prv = (GLWindowPrivate*)m_pData;

	gtk_widget_draw(prv->widget, (GdkRectangle*)NULL);
}

void GLWindow::CaptureMouse()
{
}

void GLWindow::ReleaseMouse()
{
}

static void create_bitmap_and_mask_from_xpm(GdkBitmap **bitmap, GdkBitmap **mask, const char **xpm)
{
	int height, width, colors;
	char pixmap_buffer [(32 * 32)/8];
	char mask_buffer [(32 * 32)/8];
	int x, y, pix;
	int transparent_color, black_color;

	sscanf(xpm [0], "%d %d %d %d", &height, &width, &colors, &pix);

	g_assert(height == 32);
	g_assert(width == 32);
	g_assert(colors == 3);

	transparent_color = ' ';
	black_color = '.';

	for (y = 0; y < 32; y++)
	{
		for (x = 0; x < 32;)
		{
			char value = 0, maskv = 0;

			for (pix = 0; pix < 8; pix++, x++)
			if (xpm [4+y][x] != transparent_color)
			{
				maskv |= 1 << pix;

				if (xpm [4+y][x] != black_color)
					value |= 1 << pix;
			}

			pixmap_buffer [(y * 4 + x/8)-1] = value;
			mask_buffer [(y * 4 + x/8)-1] = maskv;
		}
	}

	*bitmap = gdk_bitmap_create_from_data(NULL, pixmap_buffer, 32, 32);
	*mask = gdk_bitmap_create_from_data(NULL, mask_buffer, 32, 32);
}

void GLWindow::SetCursor(LC_CURSOR_TYPE Type)
{
#include "pixmaps/cr_brick.xpm"
#include "pixmaps/cr_light.xpm"
#include "pixmaps/cr_spot.xpm"
#include "pixmaps/cr_cam.xpm"
#include "pixmaps/cr_sel.xpm"
#include "pixmaps/cr_selm.xpm"
#include "pixmaps/cr_move.xpm"
#include "pixmaps/cr_rot.xpm"
#include "pixmaps/cr_paint.xpm"
#include "pixmaps/cr_erase.xpm"
#include "pixmaps/cr_pan.xpm"
#include "pixmaps/cr_rotv.xpm"
#include "pixmaps/cr_roll.xpm"
#include "pixmaps/cr_zoom.xpm"
#include "pixmaps/cr_zoomr.xpm"

	// TODO: Missing LC_CURSOR_ROTATEX and LC_CURSOR_ROTATEY.
	// TODO: Auto-generate xpms from bmps in the Makefile.
	const char** Cursors[LC_CURSOR_COUNT] =
	{
		NULL,     // LC_CURSOR_DEFAULT
		cr_brick, // LC_CURSOR_BRICK
		cr_light, // LC_CURSOR_LIGHT
		cr_spot,  // LC_CURSOR_SPOTLIGHT
		cr_cam,   // LC_CURSOR_CAMERA
		cr_sel,   // LC_CURSOR_SELECT
		cr_selm,  // LC_CURSOR_SELECT_GROUP
		cr_move,  // LC_CURSOR_MOVE
		cr_rot,   // LC_CURSOR_ROTATE
		cr_rot,   // LC_CURSOR_ROTATEX
		cr_rot,   // LC_CURSOR_ROTATEY
		cr_erase, // LC_CURSOR_DELETE
		cr_paint, // LC_CURSOR_PAINT
		cr_zoom,  // LC_CURSOR_ZOOM
		cr_zoomr, // LC_CURSOR_ZOOM_REGION
		cr_pan,   // LC_CURSOR_PAN
		cr_roll,  // LC_CURSOR_ROLL
		cr_rotv,  // LC_CURSOR_ROTATE_VIEW
	};

	int Offsets[LC_CURSOR_COUNT][2] =
	{
		{  0,  0 }, // LC_CURSOR_DEFAULT
		{  8,  3 }, // LC_CURSOR_BRICK
		{ 15, 15 }, // LC_CURSOR_LIGHT
		{  7, 10 }, // LC_CURSOR_SPOTLIGHT
		{ 15,  9 }, // LC_CURSOR_CAMERA
		{  0,  2 }, // LC_CURSOR_SELECT
		{  0,  2 }, // LC_CURSOR_SELECT_GROUP
		{ 15, 15 }, // LC_CURSOR_MOVE
		{ 15, 15 }, // LC_CURSOR_ROTATE
		{ 15, 15 }, // LC_CURSOR_ROTATEX
		{ 15, 15 }, // LC_CURSOR_ROTATEY
		{  0, 10 }, // LC_CURSOR_DELETE
		{ 14, 14 }, // LC_CURSOR_PAINT
		{ 15, 15 }, // LC_CURSOR_ZOOM
		{  9,  9 }, // LC_CURSOR_ZOOM_REGION
		{ 15, 15 }, // LC_CURSOR_PAN
		{ 15, 15 }, // LC_CURSOR_ROLL
		{ 15, 15 }, // LC_CURSOR_ROTATE_VIEW
	};

	GLWindowPrivate *prv = (GLWindowPrivate*)m_pData;

	if (prv->Cursor == Type)
		return;

	const char** xpm = Cursors[Type];
	int x = Offsets[Type][0];
	int y = Offsets[Type][1];

	GdkBitmap *bitmap;
	GdkBitmap *mask;
	GdkCursor *cursor;
	GdkColor white = {0, 0xffff, 0xffff, 0xffff};
	GdkColor black = {0, 0x0000, 0x0000, 0x0000};

	if (xpm != NULL)
	{
		create_bitmap_and_mask_from_xpm(&bitmap, &mask, xpm);
		cursor = gdk_cursor_new_from_pixmap(bitmap, mask, &white, &black, x, y);
		gdk_window_set_cursor(prv->widget->window, cursor);
	}
	else
	{
		cursor = gdk_cursor_new(GDK_LEFT_PTR);
		gdk_window_set_cursor(prv->widget->window, cursor);
		gdk_cursor_destroy(cursor);
	}

	prv->Cursor = Type;
}

