//
// BeOS OpenGL functions
//

#include <stdio.h>
#include <be/kernel/image.h>
#include "opengl.h"

static image_id gl_image;

// =============================================================================
// Function pointers

// =============================================================================
// Global functions

void* Sys_GLGetProc (const char *symbol)
{
  void *func;
  
  if (get_image_symbol (gl_image, symbol, B_SYMBOL_TYPE_TEXT, &func) == B_OK)
  	return func;

  printf ("Error loading OpenGL function %s.\n", symbol);
  return NULL;
}

void* Sys_GLGetExtension (const char *symbol)
{
/*
  if (pfnglXGetProcAddressARB == NULL)
    return NULL;
  else
    return pfnglXGetProcAddressARB ((GLubyte*)symbol);
    */
    return NULL;
}

bool Sys_GLOpenLibrary (const char* libname)
{
  if (libname)
  {
    gl_image = load_add_on (libname);
    if (gl_image <= 0)
	  printf ("Error loading OpenGL library %s\n", libname);
  }

  if (gl_image <= 0)
  {
  	libname = "/beos/beos/system/lib/libGL.so";
    gl_image = load_add_on (libname);
    if (gl_image <= 0)
	  printf ("Error loading OpenGL library %s\n", libname);
  }

  if (gl_image <= 0)
    return false;

//  pfnglXChooseVisual = (PFNGLXCHOOSEVISUAL) Sys_GLGetProc ("glXChooseVisual");

  return true;
}

void Sys_GLCloseLibrary ()
{
  if (gl_image <= 0)
  {
  	unload_add_on (gl_image);
    gl_image = (image_id)NULL;
  }

 // pfnglXChooseVisual = NULL;
}
