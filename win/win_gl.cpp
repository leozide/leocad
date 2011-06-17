//
// Windows OpenGL functions
//

#include "stdafx.h"
#include "opengl.h"

static HMODULE gl_module;
static bool gl_ignore_GDI = false;

// ============================================================================
// Function pointers

static WGLCHOOSEPIXELFORMAT pfnwglChoosePixelFormat;
static WGLDESCRIBEPIXELFORMAT pfnwglDescribePixelFormat;
static WGLGETPIXELFORMAT pfnwglGetPixelFormat;
static WGLSETPIXELFORMAT pfnwglSetPixelFormat;
static WGLSWAPBUFFERS pfnwglSwapBuffers;
WGLCOPYCONTEXT pfnwglCopyContext;
WGLCREATECONTEXT pfnwglCreateContext;
WGLCREATELAYERCONTEXT pfnwglCreateLayerContext;
WGLDELETECONTEXT pfnwglDeleteContext;
WGLGETCURRENTCONTEXT pfnwglGetCurrentContext;
WGLGETCURRENTDC pfnwglGetCurrentDC;
WGLGETPROCADDRESS pfnwglGetProcAddress;
WGLMAKECURRENT pfnwglMakeCurrent;
WGLSHARELISTS pfnwglShareLists;
WGLUSEFONTBITMAPS pfnwglUseFontBitmaps;
WGLUSEFONTOUTLINES pfnwglUseFontOutlines;
WGLDESCRIBELAYERPLANE pfnwglDescribeLayerPlane;
WGLSETLAYERPALETTEENTRIES pfnwglSetLayerPaletteEntries;
WGLGETLAYERPALETTEENTRIES pfnwglGetLayerPaletteEntries;
WGLREALIZELAYERPALETTE pfnwglRealizeLayerPalette;
WGLSWAPLAYERBUFFERS pfnwglSwapLayerBuffers;
WGLSWAPINTERVALEXT pfnwglSwapIntervalEXT;
WGLGETDEVICEGAMMARAMPEXT pfnwglGetDeviceGammaRampEXT;
WGLSETDEVICEGAMMARAMPEXT pfnwglSetDeviceGammaRampEXT;

// ============================================================================
// Global functions

BOOL OpenGLSwapBuffers (HDC hdc)
{
  if (!gl_ignore_GDI)
    return SwapBuffers (hdc);
  else
    return pfnwglSwapBuffers (hdc);
}

int OpenGLChoosePixelFormat(HDC hdc, CONST PIXELFORMATDESCRIPTOR * ppfd)
{
  if (!gl_ignore_GDI)
    return ChoosePixelFormat (hdc, ppfd);
  else
    return pfnwglChoosePixelFormat (hdc, ppfd); 
}

int OpenGLDescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd)
{
  if (!gl_ignore_GDI)
    return DescribePixelFormat (hdc, iPixelFormat, nBytes, ppfd);
  else
    return pfnwglDescribePixelFormat (hdc, iPixelFormat, nBytes, ppfd);
}

BOOL OpenGLSetPixelFormat(HDC hdc, int iPixelFormat, CONST PIXELFORMATDESCRIPTOR * ppfd)
{
  if (!gl_ignore_GDI)
    return SetPixelFormat (hdc, iPixelFormat, ppfd);
  else
    return pfnwglSetPixelFormat (hdc, iPixelFormat, ppfd);
}

int OpenGLGetPixelFormat(HDC hdc)
{
  if (!gl_ignore_GDI)
    return GetPixelFormat (hdc);
  else
    return pfnwglGetPixelFormat (hdc);
}

void* Sys_GLGetProc (const char *symbol)
{
	return GetProcAddress (gl_module, symbol);
}

void* Sys_GLGetExtension (const char *symbol)
{
	return pfnwglGetProcAddress (symbol);
}

bool Sys_GLOpenLibrary (const char* libname)
{
	if (libname)
  {
    gl_module = LoadLibrary (libname);

    if (strcmp(libname, "opengl32.dll"))
      gl_ignore_GDI = true;
  }

	if (gl_module == NULL)
  {
		gl_module = LoadLibrary ("opengl32.dll");
    gl_ignore_GDI = false;
  }

	if (gl_module == NULL)
  {
    gl_module = LoadLibrary ("opengl.dll");
    gl_ignore_GDI = true;
  }

	if (gl_module == NULL)
		return false;

	pfnwglChoosePixelFormat = (WGLCHOOSEPIXELFORMAT) Sys_GLGetProc ("wglChoosePixelFormat");
	pfnwglDescribePixelFormat = (WGLDESCRIBEPIXELFORMAT) Sys_GLGetProc ("wglDescribePixelFormat");
	pfnwglGetPixelFormat = (WGLGETPIXELFORMAT) Sys_GLGetProc ("wglGetPixelFormat");
	pfnwglSetPixelFormat = (WGLSETPIXELFORMAT) Sys_GLGetProc ("wglSetPixelFormat");
	pfnwglSwapBuffers = (WGLSWAPBUFFERS) Sys_GLGetProc ("wglSwapBuffers");
	pfnwglCopyContext = (WGLCOPYCONTEXT) Sys_GLGetProc ("wglCopyContext");
	pfnwglCreateContext = (WGLCREATECONTEXT) Sys_GLGetProc ("wglCreateContext");
	pfnwglCreateLayerContext = (WGLCREATELAYERCONTEXT) Sys_GLGetProc ("wglCreateLayerContext");
	pfnwglDeleteContext = (WGLDELETECONTEXT) Sys_GLGetProc ("wglDeleteContext");
	pfnwglGetCurrentContext = (WGLGETCURRENTCONTEXT) Sys_GLGetProc ("wglGetCurrentContext");
	pfnwglGetCurrentDC = (WGLGETCURRENTDC) Sys_GLGetProc ("wglGetCurrentDC");
	pfnwglGetProcAddress = (WGLGETPROCADDRESS) Sys_GLGetProc ("wglGetProcAddress");
	pfnwglMakeCurrent = (WGLMAKECURRENT) Sys_GLGetProc ("wglMakeCurrent");
	pfnwglShareLists = (WGLSHARELISTS) Sys_GLGetProc ("wglShareLists");
	pfnwglUseFontBitmaps = (WGLUSEFONTBITMAPS) Sys_GLGetProc ("wglUseFontBitmaps");
	pfnwglUseFontOutlines = (WGLUSEFONTOUTLINES) Sys_GLGetProc ("wglUseFontOutlines");
	pfnwglDescribeLayerPlane = (WGLDESCRIBELAYERPLANE) Sys_GLGetProc ("wglDescribeLayerPlane");
	pfnwglSetLayerPaletteEntries = (WGLSETLAYERPALETTEENTRIES) Sys_GLGetProc ("wglSetLayerPaletteEntries");
	pfnwglGetLayerPaletteEntries = (WGLGETLAYERPALETTEENTRIES) Sys_GLGetProc ("wglGetLayerPaletteEntries");
	pfnwglRealizeLayerPalette = (WGLREALIZELAYERPALETTE) Sys_GLGetProc ("wglRealizeLayerPalette");
	pfnwglSwapLayerBuffers = (WGLSWAPLAYERBUFFERS) Sys_GLGetProc ("wglSwapLayerBuffers");
	pfnwglSwapIntervalEXT = (WGLSWAPINTERVALEXT) Sys_GLGetProc ("wglSwapIntervalEXT");
	pfnwglGetDeviceGammaRampEXT = (WGLGETDEVICEGAMMARAMPEXT) Sys_GLGetProc ("wglGetDeviceGammaRampEXT");
	pfnwglSetDeviceGammaRampEXT = (WGLSETDEVICEGAMMARAMPEXT) Sys_GLGetProc ("wglSetDeviceGammaRampEXT");

	return true;
}

void Sys_GLCloseLibrary ()
{
	if (gl_module)
	{
		FreeLibrary (gl_module);
		gl_module = NULL;
	}

	pfnwglChoosePixelFormat = NULL;
	pfnwglDescribePixelFormat = NULL;
	pfnwglGetPixelFormat = NULL;
	pfnwglSetPixelFormat = NULL;
	pfnwglSwapBuffers = NULL;
	pfnwglCopyContext = NULL;
	pfnwglCreateContext = NULL;
	pfnwglCreateLayerContext = NULL;
	pfnwglDeleteContext = NULL;
	pfnwglGetCurrentContext = NULL;
	pfnwglGetCurrentDC = NULL;
	pfnwglGetProcAddress = NULL;
	pfnwglMakeCurrent = NULL;
	pfnwglShareLists = NULL;
	pfnwglUseFontBitmaps = NULL;
	pfnwglUseFontOutlines = NULL;
	pfnwglDescribeLayerPlane = NULL;
	pfnwglSetLayerPaletteEntries = NULL;
	pfnwglGetLayerPaletteEntries = NULL;
	pfnwglRealizeLayerPalette = NULL;
	pfnwglSwapLayerBuffers = NULL;
	pfnwglSwapIntervalEXT = NULL;
	pfnwglGetDeviceGammaRampEXT = NULL;
	pfnwglSetDeviceGammaRampEXT = NULL;
}
