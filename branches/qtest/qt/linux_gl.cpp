#include "lc_global.h"
#include <QtOpenGL>

void* Sys_GLGetExtension(const char* Symbol, void* Data)
{
	QGLWidget* Widget = (QGLWidget*)Data;

	return Widget->context()->getProcAddress(Symbol);
}
