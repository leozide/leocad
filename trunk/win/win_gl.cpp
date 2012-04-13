#include "lc_global.h"

void* Sys_GLGetExtension(const char* Symbol)
{
	return wglGetProcAddress(Symbol);
}
