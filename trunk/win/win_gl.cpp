#include "lc_global.h"
#include "opengl.h"

BOOL OpenGLSwapBuffers(HDC hdc)
{
	return SwapBuffers(hdc);
}

int OpenGLChoosePixelFormat(HDC hdc, CONST PIXELFORMATDESCRIPTOR * ppfd)
{
	return ChoosePixelFormat(hdc, ppfd);
}

int OpenGLDescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd)
{
	return DescribePixelFormat(hdc, iPixelFormat, nBytes, ppfd);
}

BOOL OpenGLSetPixelFormat(HDC hdc, int iPixelFormat, CONST PIXELFORMATDESCRIPTOR * ppfd)
{
	return SetPixelFormat(hdc, iPixelFormat, ppfd);
}

int OpenGLGetPixelFormat(HDC hdc)
{
	return GetPixelFormat(hdc);
}

void* Sys_GLGetExtension(const char* Symbol)
{
	return wglGetProcAddress(Symbol);
}

bool Sys_GLOpenLibrary(const char* libname)
{
	return true;
}

void Sys_GLCloseLibrary()
{
}
