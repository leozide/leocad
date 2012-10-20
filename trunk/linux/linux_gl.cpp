//
// Linux OpenGL functions
//

#include <dlfcn.h>
#include <stdio.h>
#include "lc_global.h"
#include "opengl.h"

static void* gl_module;

// =============================================================================
// Function pointers

PFNGLXCHOOSEVISUAL pfnglXChooseVisual;
PFNGLXCREATECONTEXT pfnglXCreateContext;
PFNGLXDESTROYCONTEXT pfnglXDestroyContext;
PFNGLXMAKECURRENT pfnglXMakeCurrent;
PFNGLXCOPYCONTEXT pfnglXCopyContext;
PFNGLXSWAPBUFFERS pfnglXSwapBuffers;
PFNGLXCREATEGLXPIXMAP pfnglXCreateGLXPixmap;
PFNGLXDESTROYGLXPIXMAP pfnglXDestroyGLXPixmap;
PFNGLXQUERYEXTENSION pfnglXQueryExtension;
PFNGLXQUERYVERSION pfnglXQueryVersion;
PFNGLXISDIRECT pfnglXIsDirect;
PFNGLXGETCONFIG pfnglXGetConfig;
PFNGLXGETCURRENTCONTEXT pfnglXGetCurrentContext;
PFNGLXGETCURRENTDRAWABLE pfnglXGetCurrentDrawable;
PFNGLXWAITGL pfnglXWaitGL;
PFNGLXWAITX pfnglXWaitX;
PFNGLXUSEXFONT pfnglXUseXFont;
PFNGLXQUERYEXTENSIONSSTRING pfnglXQueryExtensionsString;
PFNGLXQUERYSERVERSTRING pfnglXQueryServerString;
PFNGLXGETCLIENTSTRING pfnglXGetClientString;
//PFNGLXCREATEGLXPIXMAPMESA pfnglXCreateGLXPixmapMESA;
//PFNGLXRELEASEBUFFERSMESA pfnglXReleaseBuffersMESA;
//PFNGLXCOPYSUBBUFFERMESA pfnglXCopySubBufferMESA;
//PFNGLXSET3DFXMODEMESA pfnglXSet3DfxModeMESA;
//PFNGLXGETVIDEOSYNCSGI pfnglXGetVideoSyncSGI;
//PFNGLXWAITVIDEOSYNCSGI pfnglXWaitVideoSyncSGI;
PFNGLXGETPROCADDRESSARB pfnglXGetProcAddressARB;

// =============================================================================
// Global functions

void* Sys_GLGetProc (const char *symbol)
{
  void* func =  dlsym (gl_module, symbol);
  const char* error = dlerror ();
  if (error)
    printf ("Error loading OpenGL library.\n%s\n", error);
  return func;
}

void* Sys_GLGetExtension (const char *symbol)
{
  if (pfnglXGetProcAddressARB == NULL)
    return NULL;
  else
    return pfnglXGetProcAddressARB ((GLubyte*)symbol);
}

bool Sys_GLOpenLibrary (const char* libname)
{
  const char *error;

  if (libname)
  {
    gl_module = dlopen (libname, RTLD_LAZY|RTLD_GLOBAL);
    error = dlerror ();
    if (error)
      printf ("Error loading OpenGL library.\n%s\n", error);
  }

  if (gl_module == NULL)
  {
    gl_module = dlopen ("libGL.so.1", RTLD_LAZY|RTLD_GLOBAL);
    error = dlerror ();
    if (error)
      printf ("Error loading OpenGL library.\n%s\n", error);
  }

  if (gl_module == NULL)
  {
    gl_module = dlopen ("libMesaGL.so.1", RTLD_LAZY|RTLD_GLOBAL);
    error = dlerror ();
    if (error)
      printf ("Error loading OpenGL library.\n%s\n", error);
  }

  if (gl_module == NULL)
    return false;

  pfnglXChooseVisual = (PFNGLXCHOOSEVISUAL) Sys_GLGetProc ("glXChooseVisual");
  pfnglXCreateContext = (PFNGLXCREATECONTEXT) Sys_GLGetProc ("glXCreateContext");
  pfnglXDestroyContext = (PFNGLXDESTROYCONTEXT) Sys_GLGetProc ("glXDestroyContext");
  pfnglXMakeCurrent = (PFNGLXMAKECURRENT) Sys_GLGetProc ("glXMakeCurrent");
  pfnglXCopyContext = (PFNGLXCOPYCONTEXT) Sys_GLGetProc ("glXCopyContext");
  pfnglXSwapBuffers = (PFNGLXSWAPBUFFERS) Sys_GLGetProc ("glXSwapBuffers");
  pfnglXCreateGLXPixmap = (PFNGLXCREATEGLXPIXMAP) Sys_GLGetProc ("glXCreateGLXPixmap");
  pfnglXDestroyGLXPixmap = (PFNGLXDESTROYGLXPIXMAP) Sys_GLGetProc ("glXDestroyGLXPixmap");
  pfnglXQueryExtension = (PFNGLXQUERYEXTENSION) Sys_GLGetProc ("glXQueryExtension");
  pfnglXQueryVersion = (PFNGLXQUERYVERSION) Sys_GLGetProc ("glXQueryVersion");
  pfnglXIsDirect = (PFNGLXISDIRECT) Sys_GLGetProc ("glXIsDirect");
  pfnglXGetConfig = (PFNGLXGETCONFIG) Sys_GLGetProc ("glXGetConfig");
  pfnglXGetCurrentContext = (PFNGLXGETCURRENTCONTEXT) Sys_GLGetProc ("glXGetCurrentContext");
  pfnglXGetCurrentDrawable = (PFNGLXGETCURRENTDRAWABLE) Sys_GLGetProc ("glXGetCurrentDrawable");
  pfnglXWaitGL = (PFNGLXWAITGL) Sys_GLGetProc ("glXWaitGL");
  pfnglXWaitX = (PFNGLXWAITX) Sys_GLGetProc ("glXWaitX");
  pfnglXUseXFont = (PFNGLXUSEXFONT) Sys_GLGetProc ("glXUseXFont");
  pfnglXQueryExtensionsString = (PFNGLXQUERYEXTENSIONSSTRING) Sys_GLGetProc ("glXQueryExtensionsString");
  pfnglXQueryServerString = (PFNGLXQUERYSERVERSTRING) Sys_GLGetProc ("glXQueryServerString");
  pfnglXGetClientString = (PFNGLXGETCLIENTSTRING) Sys_GLGetProc ("glXGetClientString");
  //  pfnglXCreateGLXPixmapMESA = (PFNGLXCREATEGLXPIXMAPMESA) Sys_GLGetProc ("glXCreateGLXPixmapMESA");
  //  pfnglXReleaseBuffersMESA = (PFNGLXRELEASEBUFFERSMESA) Sys_GLGetProc ("glXReleaseBuffersMESA");
  //  pfnglXCopySubBufferMESA = (PFNGLXCOPYSUBBUFFERMESA) Sys_GLGetProc ("glXCopySubBufferMESA");
  //  pfnglXSet3DfxModeMESA = (PFNGLXSET3DFXMODEMESA) Sys_GLGetProc ("glXSet3DfxModeMESA");
  //  pfnglXGetVideoSyncSGI = (PFNGLXGETVIDEOSYNCSGI) Sys_GLGetProc ("glXGetVideoSyncSGI");
  //  pfnglXWaitVideoSyncSGI = (PFNGLXWAITVIDEOSYNCSGI) Sys_GLGetProc ("glXWaitVideoSyncSGI");
  pfnglXGetProcAddressARB = (PFNGLXGETPROCADDRESSARB) Sys_GLGetProc ("glXGetProcAddressARB");

  return true;
}

void Sys_GLCloseLibrary ()
{
  if (gl_module)
  {
    dlclose (gl_module);
    gl_module = NULL;
  }

  pfnglXChooseVisual = NULL;
  pfnglXCreateContext = NULL;
  pfnglXDestroyContext = NULL;
  pfnglXMakeCurrent = NULL;
  pfnglXCopyContext = NULL;
  pfnglXSwapBuffers = NULL;
  pfnglXCreateGLXPixmap = NULL;
  pfnglXDestroyGLXPixmap = NULL;
  pfnglXQueryExtension = NULL;
  pfnglXQueryVersion = NULL;
  pfnglXIsDirect = NULL;
  pfnglXGetConfig = NULL;
  pfnglXGetCurrentContext = NULL;
  pfnglXGetCurrentDrawable = NULL;
  pfnglXWaitGL = NULL;
  pfnglXWaitX = NULL;
  pfnglXUseXFont = NULL;
  pfnglXQueryExtensionsString = NULL;
  pfnglXQueryServerString = NULL;
  pfnglXGetClientString = NULL;
  //  pfnglXCreateGLXPixmapMESA = NULL;
  //  pfnglXReleaseBuffersMESA = NULL;
  //  pfnglXCopySubBufferMESA = NULL;
  //  pfnglXSet3DfxModeMESA = NULL;
  //  pfnglXGetVideoSyncSGI = NULL;
  //  pfnglXWaitVideoSyncSGI = NULL;
  pfnglXGetProcAddressARB = NULL;
}
