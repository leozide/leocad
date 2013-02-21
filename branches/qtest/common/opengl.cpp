#include "lc_global.h"
#include <string.h>
#include <stdio.h>
#include "opengl.h"

void* Sys_GLGetExtension(const char* Symbol, void* Data);

GLBINDBUFFERARBPROC glBindBufferARB;
GLDELETEBUFFERSARBPROC glDeleteBuffersARB;
GLGENBUFFERSARBPROC glGenBuffersARB;
GLISBUFFERARBPROC glIsBufferARB;
GLBUFFERDATAARBPROC glBufferDataARB;
GLBUFFERSUBDATAARBPROC glBufferSubDataARB;
GLGETBUFFERSUBDATAARBPROC glGetBufferSubDataARB;
GLMAPBUFFERARBPROC glMapBufferARB;
GLUNMAPBUFFERARBPROC glUnmapBufferARB;
GLGETBUFFERPARAMETERIVARBPROC glGetBufferParameterivARB;
GLGETBUFFERPOINTERVARBPROC glGetBufferPointervARB;

bool GL_SupportsVertexBufferObject = false;
bool GL_UseVertexBufferObject = false;

bool GL_ExtensionSupported(const GLubyte* Extensions, const char* Name)
{
	const GLubyte *start;
	GLubyte *where, *terminator;

	where = (GLubyte*)strchr(Name, ' ');
	if (where || *Name == '\0')
		return false;

	if (!Extensions)
		return false;

	for (start = Extensions; ;)
	{
		where = (GLubyte*)strstr((const char*)start, Name);
		if (!where)
			break;

		terminator = where + strlen(Name);
		if (where == start || *(where - 1) == ' ')
			if (*terminator == ' ' || *terminator == '\0')
				return true;

		start = terminator;
	}

	return false;
}

void GL_InitializeSharedExtensions(void* Data)
{
	const GLubyte* Extensions = glGetString(GL_EXTENSIONS);

	if (GL_ExtensionSupported(Extensions, "GL_ARB_vertex_buffer_object"))
	{
		glBindBufferARB = (GLBINDBUFFERARBPROC)Sys_GLGetExtension("glBindBufferARB", Data);
		glDeleteBuffersARB = (GLDELETEBUFFERSARBPROC)Sys_GLGetExtension("glDeleteBuffersARB", Data);
		glGenBuffersARB = (GLGENBUFFERSARBPROC)Sys_GLGetExtension("glGenBuffersARB", Data);
		glIsBufferARB = (GLISBUFFERARBPROC)Sys_GLGetExtension("glIsBufferARB", Data);
		glBufferDataARB = (GLBUFFERDATAARBPROC)Sys_GLGetExtension("glBufferDataARB", Data);
		glBufferSubDataARB = (GLBUFFERSUBDATAARBPROC)Sys_GLGetExtension("glBufferSubDataARB", Data);
		glGetBufferSubDataARB = (GLGETBUFFERSUBDATAARBPROC)Sys_GLGetExtension("glGetBufferSubDataARB", Data);
		glMapBufferARB = (GLMAPBUFFERARBPROC)Sys_GLGetExtension("glMapBufferARB", Data);
		glUnmapBufferARB = (GLUNMAPBUFFERARBPROC)Sys_GLGetExtension("glUnmapBufferARB", Data);
		glGetBufferParameterivARB = (GLGETBUFFERPARAMETERIVARBPROC)Sys_GLGetExtension("glGetBufferParameterivARB", Data);
		glGetBufferPointervARB = (GLGETBUFFERPOINTERVARBPROC)Sys_GLGetExtension("glGetBufferPointervARB", Data);

		GL_UseVertexBufferObject = true;
		GL_SupportsVertexBufferObject = true;
	}
}
