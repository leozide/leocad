#ifndef _LINUX_GL_H_
#define _LINUX_GL_H_

// =============================================================================
// GLX functions typedefs

typedef XVisualInfo* (*PFNGLXCHOOSEVISUAL) (Display *dpy, int screen, int *attribList);
typedef GLXContext (*PFNGLXCREATECONTEXT) (Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct);
typedef void (*PFNGLXDESTROYCONTEXT) (Display *dpy, GLXContext ctx);
typedef Bool (*PFNGLXMAKECURRENT) (Display *dpy, GLXDrawable drawable, GLXContext ctx);
typedef void (*PFNGLXCOPYCONTEXT) (Display *dpy, GLXContext src, GLXContext dst, GLuint mask);
typedef void (*PFNGLXSWAPBUFFERS) (Display *dpy, GLXDrawable drawable);
typedef GLXPixmap (*PFNGLXCREATEGLXPIXMAP) (Display *dpy, XVisualInfo *visual, Pixmap pixmap);
typedef void (*PFNGLXDESTROYGLXPIXMAP) (Display *dpy, GLXPixmap pixmap);
typedef Bool (*PFNGLXQUERYEXTENSION) (Display *dpy, int *errorb, int *event);
typedef Bool (*PFNGLXQUERYVERSION) (Display *dpy, int *maj, int *min);
typedef Bool (*PFNGLXISDIRECT) (Display *dpy, GLXContext ctx);
typedef int (*PFNGLXGETCONFIG) (Display *dpy, XVisualInfo *visual, int attrib, int *value);
typedef GLXContext (*PFNGLXGETCURRENTCONTEXT) (void);
typedef GLXDrawable (*PFNGLXGETCURRENTDRAWABLE) (void);
typedef void (*PFNGLXWAITGL) (void);
typedef void (*PFNGLXWAITX) (void);
typedef void (*PFNGLXUSEXFONT) (Font font, int first, int count, int list);

// GLX 1.1 and later
typedef const char* (*PFNGLXQUERYEXTENSIONSSTRING) (Display *dpy, int screen); 
typedef const char* (*PFNGLXQUERYSERVERSTRING) (Display *dpy, int screen, int name);
typedef const char* (*PFNGLXGETCLIENTSTRING) (Display *dpy, int name);

// GLX_MESA_pixmap_colormap
typedef GLXPixmap (*PFNGLXCREATEGLXPIXMAPMESA) (Display *dpy, XVisualInfo *visual, Pixmap pixmap, Colormap cmap);

// GLX_MESA_release_buffers
typedef Bool (*PFNGLXRELEASEBUFFERSMESA) (Display *dpy, GLXDrawable d);

// GLX_MESA_copy_sub_buffer
typedef void (*PFNGLXCOPYSUBBUFFERMESA) (Display *dpy, GLXDrawable drawable, int x, int y, int width, int height);

// GLX_MESA_set_3dfx_mode
typedef GLboolean (*PFNGLXSET3DFXMODEMESA) (GLint mode);

// GLX_SGI_video_sync
typedef int (*PFNGLXGETVIDEOSYNCSGI) (unsigned int *count);
typedef int (*PFNGLXWAITVIDEOSYNCSGI) (int divisor, int remainder, unsigned int *count);

// GLX_ARB_get_proc_address
typedef void* (*PFNGLXGETPROCADDRESSARB) (const GLubyte *procName);


// =============================================================================
// GLX extern declarations

extern PFNGLXCHOOSEVISUAL pfnglXChooseVisual;
extern PFNGLXCREATECONTEXT pfnglXCreateContext;
extern PFNGLXDESTROYCONTEXT pfnglXDestroyContext;
extern PFNGLXMAKECURRENT pfnglXMakeCurrent;
extern PFNGLXCOPYCONTEXT pfnglXCopyContext;
extern PFNGLXSWAPBUFFERS pfnglXSwapBuffers;
extern PFNGLXCREATEGLXPIXMAP pfnglXCreateGLXPixmap;
extern PFNGLXDESTROYGLXPIXMAP pfnglXDestroyGLXPixmap;
extern PFNGLXQUERYEXTENSION pfnglXQueryExtension;
extern PFNGLXQUERYVERSION pfnglXQueryVersion;
extern PFNGLXISDIRECT pfnglXIsDirect;
extern PFNGLXGETCONFIG pfnglXGetConfig;
extern PFNGLXGETCURRENTCONTEXT pfnglXGetCurrentContext;
extern PFNGLXGETCURRENTDRAWABLE pfnglXGetCurrentDrawable;
extern PFNGLXWAITGL pfnglXWaitGL;
extern PFNGLXWAITX pfnglXWaitX;
extern PFNGLXUSEXFONT pfnglXUseXFont;
extern PFNGLXQUERYEXTENSIONSSTRING pfnglXQueryExtensionsString;
extern PFNGLXQUERYSERVERSTRING pfnglXQueryServerString;
extern PFNGLXGETCLIENTSTRING pfnglXGetClientString;
//extern PFNGLXCREATEGLXPIXMAPMESA pfnglXCreateGLXPixmapMESA;
//extern PFNGLXRELEASEBUFFERSMESA pfnglXReleaseBuffersMESA;
//extern PFNGLXCOPYSUBBUFFERMESA pfnglXCopySubBufferMESA;
//extern PFNGLXSET3DFXMODEMESA pfnglXSet3DfxModeMESA;
//extern PFNGLXGETVIDEOSYNCSGI pfnglXGetVideoSyncSGI;
//extern PFNGLXWAITVIDEOSYNCSGI pfnglXWaitVideoSyncSGI;
extern PFNGLXGETPROCADDRESSARB pfnglXGetProcAddressARB;


// =============================================================================
// Replace GLX functions

#define glXChooseVisual pfnglXChooseVisual;
#define glXCreateContext pfnglXCreateContext;
#define glXDestroyContext pfnglXDestroyContext;
#define glXMakeCurrent pfnglXMakeCurrent;
#define glXCopyContext pfnglXCopyContext;
#define glXSwapBuffers pfnglXSwapBuffers;
#define glXCreateGLXPixmap pfnglXCreateGLXPixmap;
#define glXDestroyGLXPixmap pfnglXDestroyGLXPixmap;
#define glXQueryExtension pfnglXQueryExtension;
#define glXQueryVersion pfnglXQueryVersion;
#define glXIsDirect pfnglXIsDirect;
#define glXGetConfig pfnglXGetConfig;
#define glXGetCurrentContext pfnglXGetCurrentContext;
#define glXGetCurrentDrawable pfnglXGetCurrentDrawable;
#define glXWaitGL pfnglXWaitGL;
#define glXWaitX pfnglXWaitX;
#define glXUseXFont pfnglXUseXFont;
#define glXQueryExtensionsString pfnglXQueryExtensionsString;
#define glXQueryServerString pfnglXQueryServerString;
#define glXGetClientString pfnglXGetClientString;
//#define glXCreateGLXPixmapMESA pfnglXCreateGLXPixmapMESA;
//#define glXReleaseBuffersMESA pfnglXReleaseBuffersMESA;
//#define glXCopySubBufferMESA pfnglXCopySubBufferMESA;
//#define glXSet3DfxModeMESA pfnglXSet3DfxModeMESA;
//#define glXGetVideoSyncSGI pfnglXGetVideoSyncSGI;
//#define glXWaitVideoSyncSGI pfnglXWaitVideoSyncSGI;
#define glXGetProcAddressARB pfnglXGetProcAddressARB;


#endif // _LINUX_GL_H_
