#include "lc_global.h"
#include <string.h>
#include <stdio.h>
#include "opengl.h"
#include "lc_glwidget.h"
#include "mainwnd.h"
#include "preview.h"

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

void GL_InitializeSharedExtensions(lcGLWidget* Window)
{
	const GLubyte* Extensions = glGetString(GL_EXTENSIONS);

	if (GL_ExtensionSupported(Extensions, "GL_ARB_vertex_buffer_object"))
	{
		glBindBufferARB = (GLBINDBUFFERARBPROC)Window->GetExtensionAddress("glBindBufferARB");
		glDeleteBuffersARB = (GLDELETEBUFFERSARBPROC)Window->GetExtensionAddress("glDeleteBuffersARB");
		glGenBuffersARB = (GLGENBUFFERSARBPROC)Window->GetExtensionAddress("glGenBuffersARB");
		glIsBufferARB = (GLISBUFFERARBPROC)Window->GetExtensionAddress("glIsBufferARB");
		glBufferDataARB = (GLBUFFERDATAARBPROC)Window->GetExtensionAddress("glBufferDataARB");
		glBufferSubDataARB = (GLBUFFERSUBDATAARBPROC)Window->GetExtensionAddress("glBufferSubDataARB");
		glGetBufferSubDataARB = (GLGETBUFFERSUBDATAARBPROC)Window->GetExtensionAddress("glGetBufferSubDataARB");
		glMapBufferARB = (GLMAPBUFFERARBPROC)Window->GetExtensionAddress("glMapBufferARB");
		glUnmapBufferARB = (GLUNMAPBUFFERARBPROC)Window->GetExtensionAddress("glUnmapBufferARB");
		glGetBufferParameterivARB = (GLGETBUFFERPARAMETERIVARBPROC)Window->GetExtensionAddress("glGetBufferParameterivARB");
		glGetBufferPointervARB = (GLGETBUFFERPOINTERVARBPROC)Window->GetExtensionAddress("glGetBufferPointervARB");

		GL_UseVertexBufferObject = true;
		GL_SupportsVertexBufferObject = true;
	}

	if (GL_ExtensionSupported(Extensions, "GL_ARB_framebuffer_object"))
	{
		glIsRenderbufferARB = (GLISRENDERBUFFERPROC)Window->GetExtensionAddress("glIsRenderbuffer");
		glBindRenderbufferARB = (GLBINDRENDERBUFFERPROC)Window->GetExtensionAddress("glBindRenderbuffer");
		glDeleteRenderbuffersARB = (GLDELETERENDERBUFFERSPROC)Window->GetExtensionAddress("glDeleteRenderbuffers");
		glGenRenderbuffersARB = (GLGENRENDERBUFFERSPROC)Window->GetExtensionAddress("glGenRenderbuffers");
		glRenderbufferStorageARB = (GLRENDERBUFFERSTORAGEPROC)Window->GetExtensionAddress("glRenderbufferStorage");
		glGetRenderbufferParameterivARB = (GLGETRENDERBUFFERPARAMETERIVPROC)Window->GetExtensionAddress("glGetRenderbufferParameteriv");
		glIsFramebufferARB = (GLISFRAMEBUFFERPROC)Window->GetExtensionAddress("glIsFramebuffer");
		glBindFramebufferARB = (GLBINDFRAMEBUFFERPROC)Window->GetExtensionAddress("glBindFramebuffer");
		glDeleteFramebuffersARB = (GLDELETEFRAMEBUFFERSPROC)Window->GetExtensionAddress("glDeleteFramebuffers");
		glGenFramebuffersARB = (GLGENFRAMEBUFFERSPROC)Window->GetExtensionAddress("glGenFramebuffers");
		glCheckFramebufferStatusARB = (GLCHECKFRAMEBUFFERSTATUSPROC)Window->GetExtensionAddress("glCheckFramebufferStatus");
		glFramebufferTexture1DARB = (GLFRAMEBUFFERTEXTURE1DPROC)Window->GetExtensionAddress("glFramebufferTexture1D");
		glFramebufferTexture2DARB = (GLFRAMEBUFFERTEXTURE2DPROC)Window->GetExtensionAddress("glFramebufferTexture2D");
		glFramebufferTexture3DARB = (GLFRAMEBUFFERTEXTURE3DPROC)Window->GetExtensionAddress("glFramebufferTexture3D");
		glFramebufferRenderbufferARB = (GLFRAMEBUFFERRENDERBUFFERPROC)Window->GetExtensionAddress("glFramebufferRenderbuffer");
		glGetFramebufferAttachmentParameterivARB = (GLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)Window->GetExtensionAddress("glGetFramebufferAttachmentParameteriv");
		glGenerateMipmapARB = (GLGENERATEMIPMAPPROC)Window->GetExtensionAddress("glGenerateMipmap");
		glBlitFramebufferARB = (GLBLITFRAMEBUFFERPROC)Window->GetExtensionAddress("glBlitFramebuffer");
		glRenderbufferStorageMultisampleARB = (GLRENDERBUFFERSTORAGEMULTISAMPLEPROC)Window->GetExtensionAddress("glRenderbufferStorageMultisample");
		glFramebufferTextureLayerARB = (GLFRAMEBUFFERTEXTURELAYERPROC)Window->GetExtensionAddress("glFramebufferTextureLayer");

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
