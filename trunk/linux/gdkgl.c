/* 
 * Copyright (C) 1998 Janne Lof <jlof@mail.student.oulu.fi>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include "gdkgl.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include <gdk/gdkx.h>
#ifdef __cplusplus
}
#endif /* __cplusplus */

#include <string.h>
#include "opengl.h"

static XVisualInfo *get_xvisualinfo(GdkVisual *visual)
{
  Display *dpy;
  XVisualInfo vinfo_template;
  XVisualInfo *vi;
  int nitems_return;

  dpy = GDK_DISPLAY();

  /* TODO: is this right way to get VisualInfo from Visual ?? */
  /* AFAIK VisualID and depth should be enough to uniquely identify visual */
  vinfo_template.visual   = GDK_VISUAL_XVISUAL(visual);
  vinfo_template.visualid = XVisualIDFromVisual(vinfo_template.visual);
  vinfo_template.depth    = visual->depth;
  vi = XGetVisualInfo(dpy, VisualIDMask|VisualDepthMask, &vinfo_template, &nitems_return);

  g_assert( vi!=0  && nitems_return==1 ); /* visualinfo needs to be unique */

  /* remember to XFree returned XVisualInfo !!! */
  return vi;
}


struct _GdkGLContextPrivate {
  Display    *xdisplay;
  GLXContext glxcontext;
  guint ref_count;
};

typedef struct _GdkGLContextPrivate GdkGLContextPrivate;

gint gdk_gl_query(void)
{
  return (pfnglXQueryExtension(GDK_DISPLAY(),NULL,NULL) == True) ? TRUE : FALSE;
}

GdkVisual *gdk_gl_choose_visual(int *attrList)
{
  Display *dpy;
  XVisualInfo *vi;
  GdkVisual  *visual;

  g_return_val_if_fail(attrList != NULL, NULL);

  dpy = GDK_DISPLAY();
  /* TODO:  translate GDK_GL_ to GLX_ */
  if ((vi = pfnglXChooseVisual(dpy,DefaultScreen(dpy), attrList)) == NULL) {
    return NULL;
  }
  visual = gdkx_visual_get(vi->visualid);
  XFree(vi);
  return visual;
}


int gdk_gl_get_config(GdkVisual *visual, int attrib)
{
  Display *dpy;
  XVisualInfo *vi;
  int value;
  
  g_return_val_if_fail(visual != NULL, -1);

  dpy = GDK_DISPLAY();
 
  vi = get_xvisualinfo(visual);

  /* TODO:  translate GDK_GL_ to GLX_ */
  if (pfnglXGetConfig(dpy, vi, attrib, &value) == 0) {
    XFree(vi);
    return value;
  }
  XFree(vi);
  return -1;
}


GdkGLContext *gdk_gl_context_new(GdkVisual *visual)
{
  return gdk_gl_context_share_new(visual, NULL, FALSE);
}

GdkGLContext *gdk_gl_context_share_new(GdkVisual *visual, GdkGLContext *sharelist, gint direct)
{
  Display *dpy;
  XVisualInfo *vi;
  GLXContext glxcontext;
  GdkGLContextPrivate *contextprivate;

  g_return_val_if_fail(visual != NULL, NULL);

  dpy = GDK_DISPLAY();

  vi = get_xvisualinfo(visual);

  if (sharelist) {
    glxcontext = pfnglXCreateContext(dpy, vi, ((GdkGLContextPrivate*)sharelist)->glxcontext, direct ? True : False);
  } else {
    glxcontext = pfnglXCreateContext(dpy, vi, 0, direct ? True : False);
  }
  XFree(vi);
  if (glxcontext == NULL) {
    return NULL;
  }

  contextprivate = g_new(GdkGLContextPrivate, 1);
  contextprivate->xdisplay = dpy;
  contextprivate->glxcontext = glxcontext;
  contextprivate->ref_count = 1;

  return (GdkGLContext*)contextprivate;
}

GdkGLContext *gdk_gl_context_ref(GdkGLContext *context)
{
  GdkGLContextPrivate *contextprivate = (GdkGLContextPrivate*)context;

  g_return_val_if_fail(context != NULL, NULL);
  contextprivate->ref_count += 1;

  return context;
}

void gdk_gl_context_unref(GdkGLContext *context)
{
  GdkGLContextPrivate *contextprivate = (GdkGLContextPrivate*)context;

  g_return_if_fail(context != NULL);

  if (contextprivate->ref_count > 1) {
    contextprivate->ref_count -= 1;
  } else {
    if (contextprivate->glxcontext == pfnglXGetCurrentContext())
      pfnglXMakeCurrent(contextprivate->xdisplay, None, NULL);
    pfnglXDestroyContext(contextprivate->xdisplay, contextprivate->glxcontext);
    memset(context, 0, sizeof(GdkGLContextPrivate));
    g_free(context);
  }
}

gint gdk_gl_make_current(GdkDrawable *drawable, GdkGLContext *context)
{
  GdkGLContextPrivate *contextprivate = (GdkGLContextPrivate*)context;

  g_return_val_if_fail(drawable != NULL, FALSE);
  g_return_val_if_fail(context  != NULL, FALSE);

  return (pfnglXMakeCurrent(contextprivate->xdisplay, GDK_WINDOW_XWINDOW(drawable), contextprivate->glxcontext) == True) ? TRUE : FALSE;
}

void gdk_gl_swap_buffers(GdkDrawable *drawable)
{
  g_return_if_fail(drawable != NULL);

  pfnglXSwapBuffers(GDK_WINDOW_XDISPLAY(drawable), GDK_WINDOW_XWINDOW(drawable));
}

void gdk_gl_wait_gdk(void)
{
  pfnglXWaitX();
}

void gdk_gl_wait_gl (void)
{
  pfnglXWaitGL();
}


/* glpixmap stuff */

struct _GdkGLPixmapPrivate {
  Display   *xdisplay;
  GLXPixmap glxpixmap;
  GdkPixmap *front_left;
  guint     ref_count;
};

typedef struct _GdkGLPixmapPrivate GdkGLPixmapPrivate;


GdkGLPixmap *gdk_gl_pixmap_new(GdkVisual *visual, GdkPixmap *pixmap)
{
  Display *dpy;
  XVisualInfo *vi;
  Pixmap xpixmap;
  GdkGLPixmapPrivate *contextprivate;
  GLXPixmap glxpixmap;
  gint depth;

  g_return_val_if_fail(pixmap != NULL, NULL);
  g_return_val_if_fail(visual != NULL, NULL);
  g_return_val_if_fail(gdk_window_get_type(pixmap) == GDK_WINDOW_PIXMAP, NULL);

  gdk_window_get_geometry(pixmap, 0,0,0,0, &depth);
  g_return_val_if_fail(gdk_gl_get_config(visual, GDK_GL_BUFFER_SIZE) == depth, NULL);

  dpy = GDK_DISPLAY();

  vi = get_xvisualinfo(visual);
  xpixmap = ((GdkPixmapPrivate*)pixmap)->xwindow;
  glxpixmap = pfnglXCreateGLXPixmap(dpy, vi, xpixmap);
  XFree(vi);

  g_return_val_if_fail(glxpixmap != None, NULL);

  contextprivate = g_new(GdkGLPixmapPrivate, 1);
  contextprivate->xdisplay  = dpy;
  contextprivate->glxpixmap = glxpixmap;
  contextprivate->front_left = gdk_pixmap_ref(pixmap);
  contextprivate->ref_count = 1;

  return (GdkGLPixmap*)contextprivate;
}


GdkGLPixmap *gdk_gl_pixmap_ref(GdkGLPixmap *glpixmap)
{
  GdkGLPixmapPrivate *contextprivate = (GdkGLPixmapPrivate*)glpixmap;

  g_return_val_if_fail(glpixmap != NULL, NULL);
  contextprivate->ref_count += 1;

  return glpixmap;
}

void gdk_gl_pixmap_unref(GdkGLPixmap *glpixmap)
{
  GdkGLPixmapPrivate *contextprivate = (GdkGLPixmapPrivate*)glpixmap;

  g_return_if_fail(glpixmap != NULL);

  if (contextprivate->ref_count > 1) {
    contextprivate->ref_count -= 1;
  } else {
    pfnglXDestroyGLXPixmap(contextprivate->xdisplay, contextprivate->glxpixmap);
    pfnglXWaitGL();
    gdk_pixmap_unref(contextprivate->front_left);
    pfnglXWaitX();
    memset(glpixmap, 0, sizeof(GdkGLPixmapPrivate));
    g_free(glpixmap);
  }
}

gint gdk_gl_pixmap_make_current(GdkGLPixmap *glpixmap, GdkGLContext *context)
{
  Display  *dpy;
  GLXPixmap glxpixmap;
  GLXContext glxcontext;

  g_return_val_if_fail(glpixmap != NULL, FALSE);
  g_return_val_if_fail(context  != NULL, FALSE);

  dpy        = ((GdkGLContextPrivate*)context)->xdisplay;
  glxpixmap  = ((GdkGLPixmapPrivate*)glpixmap)->glxpixmap;
  glxcontext = ((GdkGLContextPrivate*)context)->glxcontext;

  return (pfnglXMakeCurrent(dpy, glxpixmap, glxcontext) == True) ? TRUE : FALSE;
}

/* fonts */
void gdk_gl_use_gdk_font(GdkFont *font, int first, int count, int list_base)
{
  g_return_if_fail(font != NULL);
  pfnglXUseXFont(gdk_font_id(font), first, count, list_base);
}

