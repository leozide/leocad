#ifndef _WIN_GL_H_
#define _WIN_GL_H_

// ============================================================================
// WGL functions typedefs

typedef int   (WINAPI* WGLCHOOSEPIXELFORMAT) (HDC, CONST PIXELFORMATDESCRIPTOR *);
typedef int   (WINAPI* WGLDESCRIBEPIXELFORMAT) (HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
typedef int   (WINAPI* WGLGETPIXELFORMAT)(HDC);
typedef BOOL  (WINAPI* WGLSETPIXELFORMAT)(HDC, int, CONST PIXELFORMATDESCRIPTOR *);
typedef BOOL  (WINAPI* WGLSWAPBUFFERS)(HDC);
typedef BOOL  (WINAPI* WGLCOPYCONTEXT)(HGLRC, HGLRC, UINT);
typedef HGLRC (WINAPI* WGLCREATECONTEXT)(HDC);
typedef HGLRC (WINAPI* WGLCREATELAYERCONTEXT)(HDC, int);
typedef BOOL  (WINAPI* WGLDELETECONTEXT)(HGLRC);
typedef HGLRC (WINAPI* WGLGETCURRENTCONTEXT)(VOID);
typedef HDC   (WINAPI* WGLGETCURRENTDC)(VOID);
typedef PROC  (WINAPI* WGLGETPROCADDRESS)(LPCSTR);
typedef BOOL  (WINAPI* WGLMAKECURRENT)(HDC, HGLRC);
typedef BOOL  (WINAPI* WGLSHARELISTS)(HGLRC, HGLRC);
typedef BOOL  (WINAPI* WGLUSEFONTBITMAPS)(HDC, DWORD, DWORD, DWORD);
typedef BOOL  (WINAPI* WGLUSEFONTOUTLINES)(HDC, DWORD, DWORD, DWORD, FLOAT, FLOAT, int, LPGLYPHMETRICSFLOAT);
typedef BOOL  (WINAPI* WGLDESCRIBELAYERPLANE)(HDC, int, int, UINT, LPLAYERPLANEDESCRIPTOR);
typedef int   (WINAPI* WGLSETLAYERPALETTEENTRIES)(HDC, int, int, int, CONST COLORREF *);
typedef int   (WINAPI* WGLGETLAYERPALETTEENTRIES)(HDC, int, int, int, COLORREF *);
typedef BOOL  (WINAPI* WGLREALIZELAYERPALETTE)(HDC, int, BOOL);
typedef BOOL  (WINAPI* WGLSWAPLAYERBUFFERS)(HDC, UINT);
typedef BOOL  (WINAPI* WGLSWAPINTERVALEXT)(int interval);
typedef BOOL  (WINAPI* WGLGETDEVICEGAMMARAMPEXT) (unsigned char *pRed, unsigned char *pGreen, unsigned char *pBlue);
typedef BOOL  (WINAPI* WGLSETDEVICEGAMMARAMPEXT) (const unsigned char *pRed, const unsigned char *pGreen, const unsigned char *pBlue);


// ============================================================================
// WGL extern declarations

extern WGLCHOOSEPIXELFORMAT pfnwglChoosePixelFormat;
extern WGLDESCRIBEPIXELFORMAT pfnwglDescribePixelFormat;
extern WGLGETPIXELFORMAT pfnwglGetPixelFormat;
extern WGLSETPIXELFORMAT pfnwglSetPixelFormat;
extern WGLSWAPBUFFERS pfnwglSwapBuffers;
extern WGLCOPYCONTEXT pfnwglCopyContext;
extern WGLCREATECONTEXT pfnwglCreateContext;
extern WGLCREATELAYERCONTEXT pfnwglCreateLayerContext;
extern WGLDELETECONTEXT pfnwglDeleteContext;
extern WGLGETCURRENTCONTEXT pfnwglGetCurrentContext;
extern WGLGETCURRENTDC pfnwglGetCurrentDC;
extern WGLGETPROCADDRESS pfnwglGetProcAddress;
extern WGLMAKECURRENT pfnwglMakeCurrent;
extern WGLSHARELISTS pfnwglShareLists;
extern WGLUSEFONTBITMAPS pfnwglUseFontBitmaps;
extern WGLUSEFONTOUTLINES pfnwglUseFontOutlines;
extern WGLDESCRIBELAYERPLANE pfnwglDescribeLayerPlane;
extern WGLSETLAYERPALETTEENTRIES pfnwglSetLayerPaletteEntries;
extern WGLGETLAYERPALETTEENTRIES pfnwglGetLayerPaletteEntries;
extern WGLREALIZELAYERPALETTE pfnwglRealizeLayerPalette;
extern WGLSWAPLAYERBUFFERS pfnwglSwapLayerBuffers;
extern WGLSWAPINTERVALEXT pfnwglSwapIntervalEXT;
extern WGLGETDEVICEGAMMARAMPEXT pfnwglGetDeviceGammaRampEXT;
extern WGLSETDEVICEGAMMARAMPEXT pfnwglSetDeviceGammaRampEXT;


// ============================================================================
// Replace WGL functions
/*
#undef wglUseFontBitmaps
#undef wglUseFontOutlines

#define wglChoosePixelFormat pfnwglChoosePixelFormat;
#define wglDescribePixelFormat pfnwglDescribePixelFormat;
#define wglGetPixelFormat pfnwglGetPixelFormat;
#define wglSetPixelFormat pfnwglSetPixelFormat;
#define wglSwapBuffers pfnwglSwapBuffers;
#define wglCopyContext pfnwglCopyContext;
#define wglCreateContext pfnwglCreateContext;
#define wglCreateLayerContext pfnwglCreateLayerContext;
#define wglDeleteContext pfnwglDeleteContext;
#define wglGetCurrentContext pfnwglGetCurrentContext;
#define wglGetCurrentDC pfnwglGetCurrentDC;
#define wglGetProcAddress pfnwglGetProcAddress;
#define wglMakeCurrent pfnwglMakeCurrent;
#define wglShareLists pfnwglShareLists;
#define wglUseFontBitmaps pfnwglUseFontBitmaps;
#define wglUseFontOutlines pfnwglUseFontOutlines;
#define wglDescribeLayerPlane pfnwglDescribeLayerPlane;
#define wglSetLayerPaletteEntries pfnwglSetLayerPaletteEntries;
#define wglGetLayerPaletteEntries pfnwglGetLayerPaletteEntries;
#define wglRealizeLayerPalette pfnwglRealizeLayerPalette;
#define wglSwapLayerBuffers pfnwglSwapLayerBuffers;
#define wglSwapIntervalEXT pfnwglSwapIntervalEXT;
#define wglGetDeviceGammaRampEXT pfnwglGetDeviceGammaRampEXT;
#define wglSetDeviceGammaRampEXT pfnwglSetDeviceGammaRampEXT;
*/

#endif // _WIN_GL_H_
