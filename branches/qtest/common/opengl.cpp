#include "lc_global.h"
#include <string.h>
#include <stdio.h>
#include "opengl.h"
#include "mainwnd.h"
#include "preview.h"

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

GLISRENDERBUFFERPROC glIsRenderbufferARB;
GLBINDRENDERBUFFERPROC glBindRenderbufferARB;
GLDELETERENDERBUFFERSPROC glDeleteRenderbuffersARB;
GLGENRENDERBUFFERSPROC glGenRenderbuffersARB;
GLRENDERBUFFERSTORAGEPROC glRenderbufferStorageARB;
GLGETRENDERBUFFERPARAMETERIVPROC glGetRenderbufferParameterivARB;
GLISFRAMEBUFFERPROC glIsFramebufferARB;
GLBINDFRAMEBUFFERPROC glBindFramebufferARB;
GLDELETEFRAMEBUFFERSPROC glDeleteFramebuffersARB;
GLGENFRAMEBUFFERSPROC glGenFramebuffersARB;
GLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatusARB;
GLFRAMEBUFFERTEXTURE1DPROC glFramebufferTexture1DARB;
GLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2DARB;
GLFRAMEBUFFERTEXTURE3DPROC glFramebufferTexture3DARB;
GLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbufferARB;
GLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glGetFramebufferAttachmentParameterivARB;
GLGENERATEMIPMAPPROC glGenerateMipmapARB;
GLBLITFRAMEBUFFERPROC glBlitFramebufferARB;
GLRENDERBUFFERSTORAGEMULTISAMPLEPROC glRenderbufferStorageMultisampleARB;
GLFRAMEBUFFERTEXTURELAYERPROC glFramebufferTextureLayerARB;

bool GL_SupportsVertexBufferObject;
bool GL_UseVertexBufferObject;
bool GL_SupportsFramebufferObject;

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

	if (GL_ExtensionSupported(Extensions, "GL_ARB_framebuffer_object"))
	{
		glIsRenderbufferARB = (GLISRENDERBUFFERPROC)Sys_GLGetExtension("glIsRenderbuffer", Data);
		glBindRenderbufferARB = (GLBINDRENDERBUFFERPROC)Sys_GLGetExtension("glBindRenderbuffer", Data);
		glDeleteRenderbuffersARB = (GLDELETERENDERBUFFERSPROC)Sys_GLGetExtension("glDeleteRenderbuffers", Data);
		glGenRenderbuffersARB = (GLGENRENDERBUFFERSPROC)Sys_GLGetExtension("glGenRenderbuffers", Data);
		glRenderbufferStorageARB = (GLRENDERBUFFERSTORAGEPROC)Sys_GLGetExtension("glRenderbufferStorage", Data);
		glGetRenderbufferParameterivARB = (GLGETRENDERBUFFERPARAMETERIVPROC)Sys_GLGetExtension("glGetRenderbufferParameteriv", Data);
		glIsFramebufferARB = (GLISFRAMEBUFFERPROC)Sys_GLGetExtension("glIsFramebuffer", Data);
		glBindFramebufferARB = (GLBINDFRAMEBUFFERPROC)Sys_GLGetExtension("glBindFramebuffer", Data);
		glDeleteFramebuffersARB = (GLDELETEFRAMEBUFFERSPROC)Sys_GLGetExtension("glDeleteFramebuffers", Data);
		glGenFramebuffersARB = (GLGENFRAMEBUFFERSPROC)Sys_GLGetExtension("glGenFramebuffers", Data);
		glCheckFramebufferStatusARB = (GLCHECKFRAMEBUFFERSTATUSPROC)Sys_GLGetExtension("glCheckFramebufferStatus", Data);
		glFramebufferTexture1DARB = (GLFRAMEBUFFERTEXTURE1DPROC)Sys_GLGetExtension("glFramebufferTexture1D", Data);
		glFramebufferTexture2DARB = (GLFRAMEBUFFERTEXTURE2DPROC)Sys_GLGetExtension("glFramebufferTexture2D", Data);
		glFramebufferTexture3DARB = (GLFRAMEBUFFERTEXTURE3DPROC)Sys_GLGetExtension("glFramebufferTexture3D", Data);
		glFramebufferRenderbufferARB = (GLFRAMEBUFFERRENDERBUFFERPROC)Sys_GLGetExtension("glFramebufferRenderbuffer", Data);
		glGetFramebufferAttachmentParameterivARB = (GLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)Sys_GLGetExtension("glGetFramebufferAttachmentParameteriv", Data);
		glGenerateMipmapARB = (GLGENERATEMIPMAPPROC)Sys_GLGetExtension("glGenerateMipmap", Data);
		glBlitFramebufferARB = (GLBLITFRAMEBUFFERPROC)Sys_GLGetExtension("glBlitFramebuffer", Data);
		glRenderbufferStorageMultisampleARB = (GLRENDERBUFFERSTORAGEMULTISAMPLEPROC)Sys_GLGetExtension("glRenderbufferStorageMultisample", Data);
		glFramebufferTextureLayerARB = (GLFRAMEBUFFERTEXTURELAYERPROC)Sys_GLGetExtension("glFramebufferTextureLayer", Data);

		GL_SupportsFramebufferObject = true;
	}
}

static GLuint gFramebufferObject;
static GLuint gFramebufferTexture;
static GLuint gDepthRenderbufferObject;

bool GL_BeginRenderToTexture(int Width, int Height)
{
	if (!GL_SupportsFramebufferObject)
		return false;

	gMainWindow->mPreviewWidget->MakeCurrent();

	glGenFramebuffersARB(1, &gFramebufferObject);
	glGenTextures(1, &gFramebufferTexture);
	glGenRenderbuffersARB(1, &gDepthRenderbufferObject);

	glBindFramebufferARB(GL_DRAW_FRAMEBUFFER, gFramebufferObject);

	glBindTexture(GL_TEXTURE_2D, gFramebufferTexture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glFramebufferTexture2DARB(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gFramebufferTexture, 0);

	glBindRenderbufferARB(GL_RENDERBUFFER, gDepthRenderbufferObject);
	glRenderbufferStorageARB(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, Width, Height);
	glFramebufferRenderbufferARB(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gDepthRenderbufferObject);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebufferARB(GL_FRAMEBUFFER, gFramebufferObject);

	if (glCheckFramebufferStatusARB(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		GL_EndRenderToTexture();
		return false;
	}

	return true;
}

void GL_EndRenderToTexture()
{
	glDeleteFramebuffersARB(1, &gFramebufferObject);
	gFramebufferObject = 0;
	glDeleteTextures(1, &gFramebufferTexture);
	gFramebufferTexture = 0;
	glDeleteRenderbuffersARB(1, &gDepthRenderbufferObject);
	gDepthRenderbufferObject = 0;
}
