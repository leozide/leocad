//
// OpenGL window
//

#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include "opengl.h"
#include "glwindow.h"

typedef struct
{
  GtkWidget  *widget;
  Display*   xdisplay;
  GLXContext context;
} GLWindowPrivate;

// =============================================================================
// static functions

static gint realize_event (GtkWidget *widget, gpointer data)
{
  GLWindow *wnd = (GLWindow*)data;

  wnd->OnInitialUpdate ();
  
  return TRUE;
}

static gint expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  GLWindow *wnd = (GLWindow*)data;

  if (event->count > 0)
    return TRUE;

  wnd->OnDraw ();

  return TRUE;
}

static gint button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  GLWindow *wnd = (GLWindow*)data;
  int x, y;

  x = (int)event->x;
  y = widget->allocation.height - (int)event->y - 1;

  if (event->type == GDK_BUTTON_PRESS)
  {
    if (event->button == 1)
      wnd->OnLeftButtonDown (x, y, (event->state & GDK_CONTROL_MASK) != 0, 
                             (event->state & GDK_SHIFT_MASK) != 0);
    else if (event->button == 3)
      wnd->OnRightButtonDown (x, y, (event->state & GDK_CONTROL_MASK) != 0, 
                              (event->state & GDK_SHIFT_MASK) != 0);
  }
  else if (event->type == GDK_2BUTTON_PRESS)
  {
    wnd->OnLeftButtonDoubleClick (x, y, (event->state & GDK_CONTROL_MASK) != 0, 
                                  (event->state & GDK_SHIFT_MASK) != 0);
  }

  gtk_window_set_focus (GTK_WINDOW (gtk_widget_get_toplevel (widget)), widget);
  gdk_pointer_grab (widget->window, FALSE, (GdkEventMask)(GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|
                    GDK_POINTER_MOTION_MASK|GDK_POINTER_MOTION_HINT_MASK),
                    NULL, NULL, GDK_CURRENT_TIME);

  return TRUE;
}

static gint button_release_event (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  GLWindow *wnd = (GLWindow*)data;
  int x, y;

  x = (int)event->x;
  y = widget->allocation.height - (int)event->y - 1;

  gdk_pointer_ungrab (GDK_CURRENT_TIME);

  if (event->button == 1)
    wnd->OnLeftButtonUp (x, y, (event->state & GDK_CONTROL_MASK) != 0, 
                         (event->state & GDK_SHIFT_MASK) != 0);
  else if (event->button == 3)
    wnd->OnRightButtonUp (x, y, (event->state & GDK_CONTROL_MASK) != 0, 
                          (event->state & GDK_SHIFT_MASK) != 0);

  return TRUE;
}

static gint pointer_motion_event (GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
  GLWindow *wnd = (GLWindow*)data;
  GdkModifierType state;
  int x, y;

  if (event->is_hint)
  {
    gdk_window_get_pointer (event->window, &x, &y, &state);
    state = (GdkModifierType)0;
  }
  else
  {
    x = (int)event->x;
    y = (int)event->y;
    state = (GdkModifierType)event->state;
  }

  y = widget->allocation.height - y - 1;

  wnd->OnMouseMove (x, y, (event->state & GDK_CONTROL_MASK) != 0, (event->state & GDK_SHIFT_MASK) != 0);

  return TRUE;
}

static gint size_allocate_event (GtkWidget *widget, GtkAllocation *allocation, gpointer data)
{
  GLWindow *wnd = (GLWindow*)data;

  wnd->OnSize (allocation->width, allocation->height);

  return TRUE;
}
/*
static void destroy_event (GtkWidget *widget, gpointer data)
{
  GLWindow *wnd = (GLWindow*)data;

  wnd->DestroyContext ();
}
*/
// =============================================================================
// GLWindow class

GLWindow::GLWindow (GLWindow *share)
{
  m_pShare = share;
  m_pData = g_malloc (sizeof (GLWindowPrivate));
}

GLWindow::~GLWindow ()
{
  DestroyContext ();
  g_free (m_pData);
}

bool GLWindow::Create (void *data)
{
  int attrlist[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 16, 0 };
  GLWindowPrivate *prv = (GLWindowPrivate*)m_pData;
  Display *dpy = GDK_DISPLAY();
  GdkVisual *visual;
  XVisualInfo *vi;

  // choose visual
  visual = gdk_visual_get_system ();
  if (visual->depth < 16)
    printf ("OpenGL fatal error: LeoCAD needs a display with at least 16 bit colors.\n");

  if (dpy == NULL)
  {
    printf ("OpenGL fatal error: Cannot get display.\n");
    return false;
  }
  prv->xdisplay = dpy;

  vi = pfnglXChooseVisual (dpy, DefaultScreen (dpy), attrlist);
  if (vi == NULL)
  {
    printf ("OpenGL fatal error: glXChooseVisual failed.\n");
    return false;
  }

  visual = gdkx_visual_get (vi->visualid);
  if (visual == NULL)
  {
    printf ("OpenGL fatal error: Cannot get visual.\n");
    return false;
  }

  gtk_widget_push_colormap (gdk_colormap_new (visual, TRUE));
  gtk_widget_push_visual (visual);

  prv->widget = gtk_drawing_area_new ();
  gtk_widget_set_double_buffered(GTK_WIDGET(prv->widget), FALSE);

  if (m_pShare == NULL)
    prv->context = pfnglXCreateContext (dpy, vi, NULL, True);
  else
  {
    GLWindowPrivate *share = (GLWindowPrivate*)m_pShare->m_pData;

    prv->context = pfnglXCreateContext (dpy, vi, share->context, True);
  }

  gtk_widget_pop_visual ();
  gtk_widget_pop_colormap ();

  XFree (vi);

  GTK_WIDGET_SET_FLAGS (prv->widget, GTK_CAN_FOCUS);

  gtk_widget_set_events (GTK_WIDGET (prv->widget), GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);

  // Connect signal handlers
  gtk_signal_connect (GTK_OBJECT (prv->widget), "expose_event", 
                      GTK_SIGNAL_FUNC (expose_event), this);
  //  gtk_signal_connect (GTK_OBJECT (prv->widget), "destroy",
  //                      GTK_SIGNAL_FUNC (destroy_event), this);
  gtk_signal_connect (GTK_OBJECT (prv->widget), "size_allocate",
                      GTK_SIGNAL_FUNC (size_allocate_event), this);
  gtk_signal_connect (GTK_OBJECT (prv->widget), "motion_notify_event",
                      GTK_SIGNAL_FUNC (pointer_motion_event), this);
  gtk_signal_connect (GTK_OBJECT (prv->widget), "button_press_event",
                      GTK_SIGNAL_FUNC (button_press_event), this);
  gtk_signal_connect (GTK_OBJECT (prv->widget), "button_release_event",
                      GTK_SIGNAL_FUNC (button_release_event), this);
  gtk_signal_connect (GTK_OBJECT (prv->widget), "realize",
                      GTK_SIGNAL_FUNC (realize_event), this);

  *((GtkWidget**)data) = prv->widget;

  return true;
}

void GLWindow::DestroyContext ()
{
  GLWindowPrivate *prv = (GLWindowPrivate*)m_pData;

  if (prv->context == pfnglXGetCurrentContext ())
    pfnglXMakeCurrent (prv->xdisplay, None, NULL);

  if (prv->context)
    pfnglXDestroyContext (prv->xdisplay, prv->context);

  prv->context = NULL;
}

void GLWindow::OnInitialUpdate ()
{
  MakeCurrent ();
  GL_InitializeExtensions ();
}

bool GLWindow::MakeCurrent ()
{
  GLWindowPrivate *prv = (GLWindowPrivate*)m_pData;
  gboolean ret = false;

  if (prv->context)
    ret = pfnglXMakeCurrent (prv->xdisplay, GDK_WINDOW_XWINDOW (prv->widget->window), prv->context);

  return ret;
}

void GLWindow::SwapBuffers ()
{
  GLWindowPrivate *prv = (GLWindowPrivate*)m_pData;

  if (prv->context)
    pfnglXSwapBuffers (GDK_WINDOW_XDISPLAY (prv->widget->window), GDK_WINDOW_XWINDOW (prv->widget->window));
}

void GLWindow::Redraw ()
{
  GLWindowPrivate *prv = (GLWindowPrivate*)m_pData;

  gtk_widget_draw (prv->widget, (GdkRectangle*)NULL);
}

void GLWindow::CaptureMouse()
{
}

void GLWindow::ReleaseMouse()
{
}
